#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <QProgressBar>
#include <QApplication>
#include <QElapsedTimer>
#include <QMessageBox>
#include <QThread>
#include "update_increase.h"
#include "uart.h"
#include "crc.h"
#include "common.h"
#include "lz4.h"
#include "multi_threads.h"
#include "page_update.h"
#include "mainwindow.h"
#include "../QLogger/QLogger.h"
#include <QDateTime>

CmdCbPkt g_updateCmdCbPkt[THREAD_SUBTYPE_UPDATE_CMD_LAST] = {
    {THREAD_SUBTYPE_UPDATE_CMD_FIRST, nullptr},
    {THREAD_SUBTYPE_UPDATE_CMD_UPDATE_FW, CmdUpdate}
};

int GetUpdateFileSize(const char* filePath)
{
    // open file
    FILE* file = fopen(filePath, "rb"); // Open the file in binary read mode
    if (file == NULL) {
        QLog_Error("CamManager", QString("[%1] [%2:%3] Unable to open file %4").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz"))
                       .arg(__FILE__).arg(__LINE__).arg(filePath));
        return -1;
    }

    // Move the file pointer to the end of the file
    if (fseek(file, 0, SEEK_END) != 0) {
        QLog_Error("CamManager", QString("[%1] [%2:%3] Unable to seek to the end of the file").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz"))
                       .arg(__FILE__).arg(__LINE__));
        return -2;
    }

    // Get the current file location, that is, the file size
    int fileSize = ftell(file);
    if (fileSize == -1L) {
        QLog_Error("CamManager", QString("[%1] [%2:%3] Unable to get the file size").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz"))
                       .arg(__FILE__).arg(__LINE__));
        fclose(file);
        return -3;
    }

    // close file
    fclose(file);

    return fileSize;
}

int GetFile(const char* filePath, uint8_t* buf, int len)
{
    // open file
    FILE* file = fopen(filePath, "rb"); // Open the file in binary read mode
    if (file == NULL) {
        QLog_Error("CamManager", QString("[%1] [%2:%3] Unable to open file: %4").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz"))
                       .arg(__FILE__).arg(__LINE__).arg(filePath));
        return -1;
    }

    // Read data from the file to the buffer
    size_t bytesRead = fread(buf, sizeof(char), len, file);
    if (bytesRead == 0 && !feof(file)) {
        QLog_Error("CamManager", QString("[%1] [%2:%3] Read operation failed: %4").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz"))
                       .arg(__FILE__).arg(__LINE__).arg(filePath));
        fclose(file);
        return -2;
    }

    // close file
    fclose(file);

    return bytesRead;
}

int SetMbUartBaudrate(QSerialPort *dev, uint32_t baudrate)
{
    int len = 0;
    UartPacket packet;
    uint8_t buf[UART_PACKET_LEN_MAX] = {0};
    uint32_t br = baudrate;

    if (dev == NULL)
        return -1;

    memset(buf, 0x55, sizeof(buf));
    /* send handshake */
    packet.head = hton16(UART_PACKET_HEAD);
    packet.type = CMD;
    packet.compressType = COMPRESS_TYPE_NONE;
    packet.reserved[0] = 0x00;
    packet.reserved[1] = 0x00;
    packet.seqNum = 0;
    packet.dataLen = 5;
    packet.data[0] = CMD_SET_BAUDRATE;
    memcpy(packet.data + 1, &br, sizeof(br));
    len = (uint8_t *)packet.data - (uint8_t *)&packet.head;

    memcpy(buf, &packet, len + 5);
    packet.crc = calc_crc16(buf, len + 5);
    memcpy(buf + len + 5, &packet.crc, sizeof(packet.crc));

    /* send handshake to mcu */
    UartWriteBuf(dev, (const char *)buf, len + 5 + sizeof(packet.crc));

    /* do not need to parse the response data from mcu,
     * because the baudrate of mcu and upper machine have not been matched */

    return 0;
}

int ParseResponseDataHandshake(QSerialPort *dev)
{
    int i = 0;
    uint8_t *buf = NULL;
    uint8_t read_buf[UART_PACKET_LEN_MAX] = {0};
    uint16_t crc = 0, calc_crc = 0;
    int error_code = -1, read_len = 0;
    uint16_t data_len = 0;
    QElapsedTimer timer;

    if (dev == NULL)
        return error_code;
    error_code --;

    /* receive handshake with timeout of 5s */
    memset(read_buf, 0x00, sizeof(read_buf));
    timer.start();
    do {
        read_len = UartReadBuf(dev, (char *)read_buf);
        DelayMs(10);
    } while (read_len == 0 && timer.elapsed() < 5000);

    if (read_len > 0) {
        PrintBuf((const char *)read_buf, (uint32_t)read_len, "Handshake:");
    } else {
        QLog_Error("CamManager", QString("[%1] [%2:%3] get uart read data failed")
                       .arg(QDateTime::currentDateTime()
                                .toString("yyyy-MM-dd HH:mm:ss.zzz"))
                       .arg(__FILE__).arg(__LINE__));
        return error_code;
    }
    error_code --;

    buf = read_buf;
    /* look for packet head */
    for (i = 0; i < read_len; i ++) {
        if (buf[i] == ((UART_PACKET_HEAD >> 8) & 0xFF) && buf[i + 1] == (UART_PACKET_HEAD & 0xFF)) {
            QLog_Debug("CamManager", QString("[%1] [%2:%3] find head OK")
                           .arg(QDateTime::currentDateTime()
                                    .toString("yyyy-MM-dd HH:mm:ss.zzz"))
                           .arg(__FILE__).arg(__LINE__));
            buf += i;
            break;
        }
    }

    /* check crc16 */
    data_len = *(uint16_t *)(buf + 8); // /* 2B head + 1B type + 1B compress type + 2B reserved + 2B serial num */
    crc = *(buf + 10 + data_len) + (*(buf + 11 + data_len) << 8);
    calc_crc = calc_crc16(buf, 10 + data_len);

    QLog_Debug("CamManager", QString("[%1] [%2:%3] data_len: %4")
                                 .arg(QDateTime::currentDateTime()
                                          .toString("yyyy-MM-dd HH:mm:ss.zzz"))
                                 .arg(__FILE__).arg(__LINE__).arg(data_len));

    if (crc != calc_crc) {
        QLog_Debug("CamManager", QString("[%1] [%2:%3] data_len: %4, crc: 0x%5, calc_crc: 0x%6")
                       .arg(QDateTime::currentDateTime()
                                .toString("yyyy-MM-dd HH:mm:ss.zzz"))
                       .arg(__FILE__).arg(__LINE__).arg(data_len)
                       .arg(crc, 0, 16).arg(calc_crc, 0, 16));
        QLog_Error("CamManager", QString("[%1] [%2:%3] check crc failed")
                                     .arg(QDateTime::currentDateTime()
                                              .toString("yyyy-MM-dd HH:mm:ss.zzz"))
                                     .arg(__FILE__).arg(__LINE__));
        return error_code;
    }
    error_code --;

    /* check type */
    if (buf[2] == RESPONSE_HANDSHAKE) {
        return 0;
    } else {
        QLog_Error("CamManager", QString("[%1] [%2:%3] check handshake type failed")
                       .arg(QDateTime::currentDateTime()
                                .toString("yyyy-MM-dd HH:mm:ss.zzz"))
                       .arg(__FILE__).arg(__LINE__));
        return error_code;
    }

    return 0;
}

int Handshake(QSerialPort *dev)
{
    int ret = 0, len = 0;
    UartPacket packet;
    uint8_t buf[UART_PACKET_LEN_MAX] = {0};

    if (dev == NULL)
        return -1;

    memset(buf, 0x55, sizeof(buf));
    /* send handshake */
    packet.head = hton16(UART_PACKET_HEAD);
    packet.type = HANDSHAKE;
    packet.compressType = COMPRESS_TYPE_NONE;
    packet.reserved[0] = 0x00;
    packet.reserved[1] = 0x00;
    packet.seqNum = 0;
    packet.dataLen = 1;
    packet.data[0] = HANDSHAKE_PC_REQUEST_LINK;
    len = (uint8_t *)packet.data - (uint8_t *)&packet.head;

    memcpy(buf, &packet, len + 1); /* 1 means packet.data[0] */
    packet.crc = calc_crc16(buf, len + 1);
    memcpy(buf + len + 1, &packet.crc, sizeof(packet.crc));

    qDebug() << "handshake";

    /* send handshake to mcu */
    UartWriteBuf(dev, (const char *)buf, len + 1 + sizeof(packet.crc));

    DelayMs(500);
    /* parse the response data from mcu */
    QLog_Debug("CamManager", QString("[%1] [%2:%3] ParseResponseDataHandshake")
                                 .arg(QDateTime::currentDateTime()
                                          .toString("yyyy-MM-dd HH:mm:ss.zzz"))
                                 .arg(__FILE__).arg(__LINE__));
    if ((ret = ParseResponseDataHandshake(dev)) < 0) {
        QLog_Error("CamManager", QString("[%1] [%2:%3] parse uart handshake data failed, ret: %4")
                       .arg(QDateTime::currentDateTime()
                                .toString("yyyy-MM-dd HH:mm:ss.zzz"))
                       .arg(__FILE__).arg(__LINE__).arg(ret));
        // qDebug() << "parse uart handshake data failed, ret:" << ret;
        return -2;
    } else {
        QLog_Debug("CamManager", QString("[%1] [%2:%3] parse uart handshake data OK")
                       .arg(QDateTime::currentDateTime()
                                .toString("yyyy-MM-dd HH:mm:ss.zzz"))
                       .arg(__FILE__).arg(__LINE__));
    }
    return 0;
}

int ParseResponseDataUpdateFileInfo(QSerialPort *dev)
{
    uint8_t readBuf[UART_PACKET_LEN_MAX] = {0};
    int readLen = 0;
    QElapsedTimer timer;

    if (dev == NULL)
        return -1;

    /* read data from uart */
    memset(readBuf, 0x00, sizeof(readBuf));
    timer.start();
    do {
        readLen = UartReadBuf(dev, (char *)readBuf);
        DelayMs(10);
    } while (readLen == 0 && timer.elapsed() < 1000);

    if (readLen <= 0) {
        QLog_Error("CamManager", QString("[%1] [%2:%3] get uart read data failed")
                       .arg(QDateTime::currentDateTime()
                                .toString("yyyy-MM-dd HH:mm:ss.zzz"))
                       .arg(__FILE__).arg(__LINE__));
        return -2;
    }

    QLog_Debug("CamManager", QString("[%1] [%2:%3] ----- data update file info response -----")
                                 .arg(QDateTime::currentDateTime()
                                          .toString("yyyy-MM-dd HH:mm:ss.zzz"))
                                 .arg(__FILE__).arg(__LINE__));
    QLog_Debug("CamManager", QString("[%1] [%2:%3] read len: %4")
                                 .arg(QDateTime::currentDateTime()
                                          .toString("yyyy-MM-dd HH:mm:ss.zzz"))
                                 .arg(__FILE__).arg(__LINE__).arg(readLen));

    /* TODO: parse read data */
    uint8_t *p = NULL;
    int i = 0;
    p = readBuf;
    for (i = 0; i < readLen - 1; i ++) {
        if (readBuf[i] == ((UART_PACKET_HEAD >> 8) & 0xFF) && readBuf[i + 1] == (UART_PACKET_HEAD & 0xFF)) {
            p = readBuf + i;
            break;
        }
    }

    // packet len is not enough
    if (i + 11 > readLen) {
        qDebug() << "len is short:" << readLen << "head offset:" << i;
        return -3;
    }

    if (*(p + 2) == 0x80 && *(p + 10) == 0x01) {
        qDebug() << "update info OK";
        return 0;
    }

    if (*(p + 2) == 0x80 && *(p + 10) == 0x02) {
        qDebug() << "update info ERR";
        return -4;
    }

    return 0;
}

int SendFileInfoPacket(QSerialPort *dev, int uartPktNum, int uartFilesize, uint8_t updateType, uint8_t updateFile)
{
    /* the xml file upgrade method is currently not feasible */

    updateFile = ONLINE_FILE;

    /* use the online file upgrade as the default            */

    UartPacket packet;
    uint16_t dataLen = 0;
    uint16_t pktNumBe = 0;
    int uartFilesizeBe = 0, uartInfoLen = 0, bufLen = 0, ret = 0;
    uint8_t buf[32] = {0x00};

    memset(buf, 0x00, sizeof(buf));
    memset(&packet, 0x00, PACKET_DATA_SIZE + 12);

    QLog_Debug("CamManager", QString("[%1] [%2:%3] send file info ...")
                                 .arg(QDateTime::currentDateTime()
                                          .toString("yyyy-MM-dd HH:mm:ss.zzz"))
                                 .arg(__FILE__).arg(__LINE__));

    packet.head = hton16(UART_PACKET_HEAD);
    packet.type = FIRMWARE_UPDATE;
    packet.compressType = COMPRESS_TYPE_NONE;
    packet.reserved[0] = 0x00;
    packet.reserved[1] = 0x00;
    packet.seqNum = 0;
    dataLen = 8; // 2B pkt num + 4B file size + 1B update type + 1B update file
    packet.dataLen = dataLen;
    pktNumBe = uartPktNum;
    uartFilesizeBe = uartFilesize;

    memcpy(packet.data, &pktNumBe, sizeof(pktNumBe));
    memcpy(packet.data + sizeof(pktNumBe), &uartFilesizeBe, sizeof(uartFilesizeBe));
    *(packet.data + sizeof(pktNumBe) + sizeof(uartFilesizeBe)) = updateType;
    *(packet.data + sizeof(pktNumBe) + sizeof(uartFilesizeBe) + 1) = updateFile;

    uartInfoLen = 10; /* 2B head + 1B type + 1B compress type + 2B reserved + 2B serial num + 2B data len */
    bufLen = uartInfoLen + dataLen;
    memcpy(buf, &packet, bufLen);
    packet.crc = calc_crc16(buf, bufLen);
    memcpy(buf + bufLen, &packet.crc, sizeof(packet.crc));

    QLog_Debug("CamManager", QString("[%1] [%2:%3] ----------- update info packet ---------------")
                                 .arg(QDateTime::currentDateTime()
                                          .toString("yyyy-MM-dd HH:mm:ss.zzz"))
                                 .arg(__FILE__).arg(__LINE__));
    UartWriteBuf(dev, (const char *)buf, bufLen + sizeof(packet.crc));

    DelayMs(1000); /* wait for 1s so that microblaze responsed data can be get */
    /* wait for response from mcu */
    if ((ret = ParseResponseDataUpdateFileInfo(dev)) < 0) {
        QLog_Error("CamManager", QString("[%1] [%2:%3] parse responce data update file info failed, error code: %4")
                       .arg(QDateTime::currentDateTime()
                                .toString("yyyy-MM-dd HH:mm:ss.zzz"))
                       .arg(__FILE__).arg(__LINE__).arg(ret));
        return -1;
    }

    return 0;
}

int CheckAckBuf(uint8_t *buf, int len, uint16_t seqNum)
{
    uint8_t *p = NULL;
    int i = 0;
    uint16_t seqNumAck = 0;

    p = buf;

    for (i = 0; i < len; i ++) {
        if (buf[i] == ((UART_PACKET_HEAD >> 8) & 0xFF) && buf[i + 1] == (UART_PACKET_HEAD & 0xFF)) {
            p += i;
            break;
        }
    }

    if (i + 10 > len) {
        return -1;
    }

    seqNumAck = *(uint16_t *)(p + 6);
    QLog_Debug("CamManager", QString("[%1] [%2:%3] sn: %4, sna: %5")
                                 .arg(QDateTime::currentDateTime()
                                          .toString("yyyy-MM-dd HH:mm:ss.zzz"))
                                 .arg(__FILE__).arg(__LINE__).arg(seqNum).arg(seqNumAck));
    if (seqNum != seqNumAck) {
        return -4;
    }

    if (*(p + 2) == 0x80 && *(p + 10) == 0x01) {
        return 0;
    } else if (*(p + 2) == 0x80 && *(p + 10) == 0x02) {
        return -2;
    } else {
        return -3;
    }

}

int UartSendIncreasePacket(QSerialPort *hSerial, const char *uartBuf, int uartLen, int uartPktNum, int uartFilesize, uint8_t updateType, uint8_t updateFile)
{
    /* the xml file upgrade method is currently not feasible */

    updateFile = ONLINE_FILE;

    /* use the online file upgrade as the default            */

    int len = 0, ret = 0, uartReadLen = 0;
    uint16_t pktLen = 0, uartInfoLen = 0;
    UartPacket *uartPkt = NULL;
    uint8_t readBuf[32] = {0};

    if (hSerial == NULL || uartBuf == NULL || uartLen <= 0) {
        return -1;
    }

    uartInfoLen = 10; /* 10 = len of head/type/compress_file/reserved/serial_num/dataLen */
    memset(readBuf, 0x00, sizeof(readBuf));

    // handshake
    if ((ret = Handshake(hSerial)) < 0) {
        QLog_Error("CamManager", QString("[%1] [%2:%3] handshake failed")
                       .arg(QDateTime::currentDateTime()
                                .toString("yyyy-MM-dd HH:mm:ss.zzz"))
                       .arg(__FILE__).arg(__LINE__));
        return -2;
    }

    // send file infomation packet
    if ((ret = SendFileInfoPacket(hSerial, uartPktNum, uartFilesize, updateType, updateFile)) < 0) {
        QLog_Error("CamManager", QString("[%1] [%2:%3] send file information packet failed")
                       .arg(QDateTime::currentDateTime()
                                .toString("yyyy-MM-dd HH:mm:ss.zzz"))
                       .arg(__FILE__).arg(__LINE__));
        return -3;
    }

    // send data
    int cnt = 0;
    while (len < uartLen) {
        cnt++;
        QLog_Debug("CamManager", QString("[%1] [%2:%3] send: cnt: %4, %5 / %6")
                                     .arg(QDateTime::currentDateTime()
                                              .toString("yyyy-MM-dd HH:mm:ss.zzz"))
                                     .arg(__FILE__).arg(__LINE__).arg(cnt).arg(len).arg(uartLen));

        uartPkt = (UartPacket *)(uartBuf + len);
        if (uartPkt->head != hton16(UART_PACKET_HEAD)) {
            QLog_Error("CamManager", QString("[%1] [%2:%3] uart packet head error")
                           .arg(QDateTime::currentDateTime()
                                    .toString("yyyy-MM-dd HH:mm:ss.zzz"))
                           .arg(__FILE__).arg(__LINE__));
            return -4;
        }

        pktLen = uartPkt->dataLen + uartInfoLen + sizeof(uint16_t);

        UartWriteBuf(hSerial, (const char *)uartPkt, pktLen);

        DelayMs(50); // 50ms
        int cnt = 0;
        do {
            uartReadLen = UartReadBuf(hSerial, (const char *)readBuf);
            cnt ++;
            DelayMs(10);
        } while (uartReadLen == 0 && cnt < 3);

        if (cnt >= 3) {
            QLog_Error("CamManager", QString("[%1] [%2:%3] read acknowledge failed, cnt: %4")
                           .arg(QDateTime::currentDateTime()
                                    .toString("yyyy-MM-dd HH:mm:ss.zzz"))
                           .arg(__FILE__).arg(__LINE__).arg(cnt));
            return -6;
        }

        // parse ack packet
        ret = CheckAckBuf(readBuf, uartReadLen, uartPkt->seqNum);
        if (ret == 0) {
            len += pktLen;
        } else {
            QLog_Error("CamManager", QString("[%1] [%2:%3] check acknowledge data failed, resend this packet")
                           .arg(QDateTime::currentDateTime()
                                    .toString("yyyy-MM-dd HH:mm:ss.zzz"))
                           .arg(__FILE__).arg(__LINE__));
        }
    }

    QLog_Debug("CamManager", QString("[%1] [%2:%3] Increase update finish")
                                 .arg(QDateTime::currentDateTime()
                                          .toString("yyyy-MM-dd HH:mm:ss.zzz"))
                                 .arg(__FILE__).arg(__LINE__));

    return 0;
}

int ReadSendUpdateDataAck(QSerialPort *dev, uint16_t serial_num)
{
    if (dev == NULL)
        return -1;

    uint8_t read_buf[UART_PACKET_LEN_MAX] = { 0 };
    int ret = 0, readBytes = 0;
    QElapsedTimer timer;
    int totalBytes = 0;

    timer.start();
    int timeout = 1000; // 1 second
    do {
        readBytes = UartReadBuf(dev, (const char*)(read_buf + totalBytes));
        totalBytes += readBytes;

        if (totalBytes <= 0) {
            DelayMs(5);
        }
        if (timer.elapsed() > timeout) {
            qDebug() << "ReadSendUpdateDataAck: timeout, no response from device, serial_num:" << serial_num;
            return -1;
        }
    } while (totalBytes <= 15);

    totalBytes = totalBytes > 2 ? totalBytes : 0; /* because uart of microblaze will send 2 more bytes before 0xEB90 */
    if (totalBytes > 0) {
        if ((ret = CheckAckBuf(read_buf, totalBytes, serial_num)) == 0) {
            return ret;
        } else {
            qDebug() << "check ack buf failed, ret:" << ret << "readBytes: " << readBytes;
            return -2;
        }
    } else {
        return -3;
    }
    return 0;
}

void SyncProgressBar(int value)
{
    ThreadManager *thread = NULL;
    thread = g_threadList->Get(THREAD_ID_UPDATE_UI); // send data to queue of UI thread
    QByteArray queueData(sizeof(ThreadDataPkt), 0);
    QByteArray progressValue(1, 0);

    progressValue[0] = value;
    MakeThreadDataPkt(THREAD_TYPE_ACK_UPDATE_FW, THREAD_SUBTYPE_UPDATE_ACK_PROGRESS_BAR, progressValue, queueData);
    thread->EnqueueData(queueData);

    EmitUpdateUiSignal();
}

void SyncUpdateRet(const QString &str)
{
    qDebug() << "SyncUpdateRet called, str:" << str;
    ThreadManager *thread = NULL;
    QByteArray queueData(sizeof(ThreadDataPkt), 0);
    QByteArray srcArray = str.toUtf8();

    thread = g_threadList->Get(THREAD_ID_UPDATE_FW); // send data to queue of UPDATE_FW thread where UpdateRet is defined
    MakeThreadDataPkt(THREAD_TYPE_ACK_UPDATE_FW, THREAD_SUBTYPE_UPDATE_ACK_RET, srcArray, queueData);
    thread->EnqueueData(queueData);

    EmitUpdateUiSignal();
}

static void SendUpdateData(QSerialPort *dev, uint8_t* file_space, int remain_in, int offset_in, uint16_t serial_num_in)
{
    int ret = 0, buf_len = 0, barValue = 0, lastBarValue = 0;
    int remain = 0, offset = 0;
    uint16_t crc16_ret = 0, serial_num = 0, data_len = 0, compressLen = 0;
    UartPacket packet;
    uint8_t buf[UART_PACKET_LEN_MAX] = { 0 };
    int timeout = 1000;
    QElapsedTimer recvTimer;

    packet.head = hton16(UART_PACKET_HEAD);
    packet.type = FIRMWARE_UPDATE;
    packet.compressType = COMPRESS_TYPE_LZ4;
    packet.reserved[0] = 0x00;
    packet.reserved[1] = 0x00;

    remain = remain_in;
    offset = offset_in;
    serial_num = serial_num_in;
    do {
        packet.compressType = COMPRESS_TYPE_LZ4; /* refresh the compression type of the next packet of data */
        memset(buf, 0x00, sizeof(buf)); /* clear buf */
        data_len = remain >= PACKET_DATA_SIZE ? PACKET_DATA_SIZE : remain;
        packet.seqNum = serial_num;

        compressLen = LZ4_compress_default((const char *)(file_space + offset), (char *)packet.data, data_len, PACKET_DATA_SIZE);
        if(compressLen == 0) { //compress failed
            memcpy(packet.data, file_space + offset, data_len);
            compressLen = data_len;
            packet.compressType = COMPRESS_TYPE_NONE;
        }

        packet.dataLen = compressLen;

        buf_len = 10 + packet.dataLen; /* 10 = len of head/type/compress_file/reserved/serial_num/dataLen */
        memcpy(buf, &packet, buf_len);

        crc16_ret = calc_crc16(buf, buf_len); /* calc crc16 */
        memcpy(buf + buf_len, &crc16_ret, sizeof(crc16_ret));
        buf_len += sizeof(crc16_ret);

        /* send data by uart */
        UartWriteBuf(dev, (const char*)buf, buf_len);

        recvTimer.start();
        /* wait a specified time so that microblaze responsed data can be get */
        do {
            ret = ReadSendUpdateDataAck(dev, serial_num); /* wait for response from mcu */
        } while(ret < 0 && recvTimer.elapsed() < timeout);

        if (ret >= 0) {
            remain -= data_len;
            offset += data_len;
            serial_num++;
            timeout = 150; /* this should be ms, which is used in vTaskDelay(ms), + 30 */

            // set progressBar
            barValue = (int)((remain_in - remain) * 100 / remain_in);
            if (barValue < 3) {
                barValue = 3;
            }

            if (barValue > lastBarValue) {
                SyncProgressBar(barValue); // send data to ui update task
                lastBarValue = barValue;
            }
        } else {
            qDebug() << "parse response data failed, seq: " << serial_num;
            /* if ret <= 0, means that packet crc or length is wrong, so delay 1000ms and resend this packet */
            timeout = 1000;
        }
    } while (remain > 0);
}

int FullUpdate(QSerialPort *dev, const char *filePath, uint8_t updateFile)
{
    /* the xml file upgrade method is currently not feasible */

    updateFile = ONLINE_FILE;

    /* use the online file upgrade as the default            */

    int ret = 0;
    uint8_t* file_space = NULL;
    const char* update_filename = filePath;
    int file_size = 0, remain = 0, offset = 0, FilePktNumBe = 0;
    uint16_t FilePktNum = 0;
    uint16_t serial_num = 0;
    const int initial_len = 4096;
    QElapsedTimer timer;

    /* calc file packet num and total file size */
    file_size = GetUpdateFileSize(update_filename); /* get file size */

    if(file_size % UART_PACKET_LEN_MAX) {
        FilePktNum = file_size / UART_PACKET_LEN_MAX + 1;
    } else {
        FilePktNum = file_size / UART_PACKET_LEN_MAX;
    }
    FilePktNumBe = FilePktNum;

    QString fileStr = QString::fromUtf8(update_filename);

    /* 0. handshake */
    timer.start();
    do {
        ret = Handshake(dev);
        if (ret < 0) {
            qDebug() << "handshake err: " << ret;
        } else {
            SyncProgressBar(1); // send data to ui update task
            qDebug() << "handshake ok";
            break;
        }
    } while (timer.elapsed() < 5000);

    if (ret < 0) {
        return -1;
    }

    /* 1. make the file info packet */
    if ((ret = SendFileInfoPacket(dev, FilePktNumBe, file_size, UPDATE_TYPE_FULL, updateFile)) < 0) {
        qDebug() << "send update info err: " << ret;
        SyncUpdateRet(QString("发送更新信息失败，请检查设备状态"));
        return -2;
    } else {
        SyncProgressBar(2); // send data to ui update task
        qDebug() << "update info ok";
    }

    /* 2. send update data packet */
    /* uart update */
    /* read total file into memory */
    QLog_Debug("CamManager", QString("[%1] [%2:%3] send update data ...")
                                 .arg(QDateTime::currentDateTime()
                                          .toString("yyyy-MM-dd HH:mm:ss.zzz"))
                                 .arg(__FILE__).arg(__LINE__));
    if ((file_space = (uint8_t *)calloc(file_size, sizeof(uint8_t))) == NULL) {
        QLog_Error("CamManager", QString("[%1] [%2:%3] malloc space failed")
                       .arg(QDateTime::currentDateTime()
                                .toString("yyyy-MM-dd HH:mm:ss.zzz"))
                       .arg(__FILE__).arg(__LINE__));
        SyncUpdateRet(QString("内存分配失败"));
        return -3;
    }

    if (GetFile(update_filename, file_space, file_size) < 0) {
        QLog_Error("CamManager", QString("[%1] [%2:%3] read update file data failed")
                       .arg(QDateTime::currentDateTime()
                                .toString("yyyy-MM-dd HH:mm:ss.zzz"))
                       .arg(__FILE__).arg(__LINE__));
        free(file_space);
        SyncUpdateRet(QString("读取升级文件失败"));
        return -4;
    }

    /* make the update data packet */
    /* head and type are the same with upper of file info packet */
    /* Initially, write the file TOP.bin starting from the offset 4096. Once this is complete,
       proceed to write the initial 4096 bytes of data back to the beginning of TOP.bin.
       This step is crucial to prevent the failure that may occur when attempting to write
       TOP.bin to flash memory, as it ensures the ability to jump to the golden zone is not compromised. */

    /* send from 4096 to last byte */
    remain = file_size - initial_len;
    offset = initial_len;
    serial_num = initial_len / PACKET_DATA_SIZE + 1;

    DelayMs(300);
    SendUpdateData(dev, file_space, remain, offset, serial_num);

    /* send from 0 to 4096 */
    remain = initial_len;
    offset = 0;
    serial_num = 1;
    SendUpdateData(dev, file_space, remain, offset, serial_num);

    // set progressBar as 100%
    SyncProgressBar(100);
    QApplication::processEvents();

    /* free memory */
    free(file_space);

    qDebug() << "update ok";
    return 0;
}

// #define INCREASE_UPDATE

#ifdef INCREASE_UPDATE
int IncreaseUpdate(QSerialPort *hSerial, const char *filePath, uint8_t updateFile, QProgressBar *progressBar)
{
    /* the xml file upgrade method is currently not feasible */

    updateFile = ONLINE_FILE;

    /* use the online file upgrade as the default            */

    int fileSize = 0, fileRead = 0;
    // const char *filePath = "./patch.bin";
    const char *patchSpace = NULL, *uartSpace = NULL;
    PatchPacket *pkt = NULL;
    UartPacket uartPkt;
    int patchLen = 0, uartLen = 0, patchInfoLen = 0, uartInfoLen = 0, compressLen = 0;
    int uartPktNum = 0, uartFilesize = 0;
    uint16_t crc = 0, readCrc = 0;

    if (hSerial == NULL)
        return -1;

    patchInfoLen = sizeof(uint16_t) * 4; // head/seqNum/addrOffset/dataLen
    uartInfoLen = 10; /* 10 = len of head/type/compress_file/reserved/serial_num/dataLen */

    // read patch.bin, check crc, get flash offset / data len / compressed data
    if ((fileSize = GetUpdateFileSize(filePath)) < 0) {
        QLog_Error("CamManager", QString("[%1] [%2:%3] get file size of %4 failed")
                       .arg(QDateTime::currentDateTime()
                                .toString("yyyy-MM-dd HH:mm:ss.zzz"))
                       .arg(__FILE__).arg(__LINE__).arg(filePath));
        return -1;
    }

    QLog_Debug("CamManager", QString("[%1] [%2:%3] filesize: %4")
                                 .arg(QDateTime::currentDateTime()
                                          .toString("yyyy-MM-dd HH:mm:ss.zzz"))
                                 .arg(__FILE__).arg(__LINE__).arg(fileSize));

    if ((patchSpace = (const char *)calloc(fileSize, sizeof(char))) == NULL) {
        QLog_Error("CamManager", QString("[%1] [%2:%3] malloc space failed")
                       .arg(QDateTime::currentDateTime()
                                .toString("yyyy-MM-dd HH:mm:ss.zzz"))
                       .arg(__FILE__).arg(__LINE__));
        return -2;
    }

    if ((fileRead = GetFile(filePath, (uint8_t *)patchSpace, fileSize)) != fileSize) {
        free((void *)patchSpace);
        QLog_Error("CamManager", QString("[%1] [%2:%3] get file of %4 failed")
                                     .arg(QDateTime::currentDateTime()
                                              .toString("yyyy-MM-dd HH:mm:ss.zzz"))
                                     .arg(__FILE__).arg(__LINE__).arg(filePath));
        return -3;
    }

    // use double size of patch to uart space
    if ((uartSpace = (const char *)calloc(fileSize * 2, sizeof(char))) == NULL) {
        free((void *)patchSpace);
        QLog_Error("CamManager", QString("[%1] [%2:%3] malloc space failed")
                                     .arg(QDateTime::currentDateTime()
                                              .toString("yyyy-MM-dd HH:mm:ss.zzz"))
                                     .arg(__FILE__).arg(__LINE__));
        return -4;
    }

    QLog_Debug("CamManager", QString("[%1] [%2:%3] patchSpace: %4, uartSpace: %5")
                                 .arg(QDateTime::currentDateTime()
                                          .toString("yyyy-MM-dd HH:mm:ss.zzz"))
                                 .arg(__FILE__).arg(__LINE__).arg((quintptr)patchSpace, 0, 16)
                                 .arg((quintptr)uartSpace, 0, 16));
    // check crc, get flash offset / data len / compressed data
    uartPkt.head = hton16(UART_PACKET_HEAD);
    uartPkt.type = FIRMWARE_UPDATE; // firmware update
    uartPkt.compressType = COMPRESS_TYPE_LZ4;
    uartPkt.reserved[0] = 0x00;
    uartPkt.reserved[1] = 0x00;

    while (patchLen < fileSize) {
        uartPkt.compressType = COMPRESS_TYPE_LZ4; /* refresh the compression type of the next packet of data */

        pkt = (PatchPacket *)(patchSpace + patchLen);

        if (pkt->head != hton16(UART_PACKET_HEAD)) {
            QLog_Error("CamManager", QString("[%1] [%2:%3] packet head error, 0x%4")
                           .arg(QDateTime::currentDateTime()
                                    .toString("yyyy-MM-dd HH:mm:ss.zzz"))
                           .arg(__FILE__).arg(__LINE__).arg(pkt->head, 0, 16));
            free((void *)patchSpace);
            free((void *)uartSpace);
            return -5;
        }

        readCrc = *(uint16_t *)((uint8_t *)pkt + patchInfoLen + pkt->dataLen);
        crc = calc_crc16((const uint8_t *)pkt, patchInfoLen + pkt->dataLen);
        if (crc != readCrc) {
            QLog_Error("CamManager", QString("[%1] [%2:%3] calculate patch crc error, seqNum: %4, calculate crc: 0x%5, packet crc: 0x%6, dataLen: %7")
                           .arg(QDateTime::currentDateTime()
                                    .toString("yyyy-MM-dd HH:mm:ss.zzz"))
                           .arg(__FILE__).arg(__LINE__).arg(pkt->seqNum)
                           .arg(crc, 0, 16).arg(readCrc, 0, 16)
                           .arg(pkt->dataLen + patchInfoLen));
            free((void *)patchSpace);
            free((void *)uartSpace);
            return -6;
        }

        // make uart packet
        uartPkt.seqNum = pkt->addrOffset;

        compressLen = LZ4_compress_default((const char *)pkt->data, (char *)uartPkt.data, pkt->dataLen, PACKET_DATA_SIZE);
        if(compressLen == 0) { //compress failed
            memcpy(uartPkt.data, pkt->data, pkt->dataLen);
            compressLen = pkt->dataLen;
            uartPkt.compressType = COMPRESS_TYPE_NONE;
        }

        uartPkt.dataLen = compressLen;

        memcpy((void *)(uartSpace + uartLen), &uartPkt, uartInfoLen);
        memcpy((void *)(uartSpace + uartLen + uartInfoLen), uartPkt.data, uartPkt.dataLen);
        crc = calc_crc16((const uint8_t *)(uartSpace + uartLen), uartInfoLen + uartPkt.dataLen); // crc: uart info and data
        memcpy((void *)(uartSpace + uartLen + uartInfoLen + uartPkt.dataLen), &crc, sizeof(crc));

        patchLen += patchInfoLen + pkt->dataLen + sizeof(uint16_t);
        uartLen += uartInfoLen + uartPkt.dataLen + sizeof(crc);

        uartPktNum ++;
        uartFilesize += uartPkt.dataLen;
    }

    // send uart data
    UartSendIncreasePacket(hSerial, uartSpace, uartLen, uartPktNum, uartFilesize, UPDATE_TYPE_INCREASE, updateFile);

    free((void *)patchSpace);
    free((void *)uartSpace);

    return 0;
}
#endif

int FirmwareUpdate(QSerialPort *dev, const char *filePath)
{
    /* the xml file upgrade method is currently not feasible */

    uint8_t updateFile = ONLINE_FILE;

    /* use the online file upgrade as the default            */

    int ret = 0;

    // set FPGA microblaze uart baudrate as 230400
    SetMbUartBaudrate(dev, BAUD_115200_VALUE);
    DelayMs(500);

    // set locale uart baudrate as 230400
    dev->setBaudRate(115200);
    DelayMs(1000);
    qDebug() << "setBaudRate ok";

    // full update
    ret = FullUpdate(dev, filePath, updateFile);

    // set FPGA microblaze uart baudrate as 115200
    SetMbUartBaudrate(dev, BAUD_115200_VALUE);
    DelayMs(300);

    // set sd3403 uart baudrate as 115200
    dev->setBaudRate(115200);
    DelayMs(200);

    return ret;
}
void CmdUpdate(void *param)
{
    if (param == nullptr) {
        return;
    }

    int ret = 0;
    uint16_t dataLen = 0;
    static QMutex updateMutex;
    QElapsedTimer timer;
    ThreadDataPkt *buf = (ThreadDataPkt *)param;
    uint8_t filepath[FILE_PATH_LEN_MAX];

    dataLen = hton16(buf->dataLen);
    memset(filepath, 0x00, sizeof(filepath));
    memcpy(filepath, buf->data, dataLen);

    ret = updateMutex.tryLock();
    if (ret == false) {
        qDebug() << "get lock failed";
        return;
    }
    timer.start();
#ifdef INCREASE_UPDATE
    IncreaseUpdate(uartDev, g_updateWidgets.lineEdit->text().toStdString().c_str(), g_updateWidgets.progressBar);
#endif

    ret = FirmwareUpdate(&g_serialPort, (const char *)filepath);
    qDebug() << "CmdUpdate: FirmwareUpdate returned" << ret;

    // create a new window to show the result of update
    if (ret == 0) {
        // send ok
        QString usedTime = "执行成功，用时";
        usedTime += QString::number((int)(timer.elapsed() / 1000 / 60)) + "分" + QString::number((int)((timer.elapsed() / 1000) % 60)) + "秒";
        SyncUpdateRet(usedTime);
    } else {
        qDebug() << "CmdUpdate: update failed, ret=" << ret;
        // send failed with specific error message
        QString usedTime = "执行失败，用时";
        usedTime += QString::number((int)(timer.elapsed() / 1000 / 60)) + "分" + QString::number((int)((timer.elapsed() / 1000) % 60)) + "秒";

        QString errorMsg;
        switch (ret) {
        case -1:
            errorMsg = "设备握手失败，请检查设备连接";
            break;
        case -2:
            errorMsg = "发送更新信息失败，请检查设备状态";
            break;
        case -3:
            errorMsg = "内存分配失败";
            break;
        case -4:
            errorMsg = "读取升级文件失败";
            break;
        default:
            errorMsg = QString("未知错误，错误代码: %1").arg(ret);
            break;
        }

        usedTime += "\n错误原因: " + errorMsg;
        SyncUpdateRet(usedTime);
    }

    updateMutex.unlock();
}

void UpdateFwTask(void)
{
    // get current task
    ThreadManager *thread = g_threadList->Get(THREAD_ID_UPDATE_FW);
    uint16_t dataLen = 0;
    uint16_t crc = 0;

    while (1) {
        if (! thread->IsQueueEmpty()) {
            // TODO: suspend other threads

            ThreadDataPkt *buf = (ThreadDataPkt *)(thread->DequeueData().data());
            dataLen = hton16(buf->dataLen);
            if (dataLen > THREAD_PACKET_DATA_LEN) {
                continue;
            }

            dataLen += 8; // head, type, subtype, seq, dataLen
            crc = *(uint16_t *)((uint8_t *)buf + dataLen);

            // check crc
            if (! check_crc16((uint8_t *)buf, dataLen, hton16(crc))) {
                continue;
            }

            // check head
            if (buf->head != hton16(THREAD_PACKET_HEAD)) {
                continue;
            }

            // check type
            if (buf->type != THREAD_TYPE_CMD_UPDATE_FW) {
                continue;
            }

            // check subtype
            if (buf->subType > THREAD_SUBTYPE_UPDATE_CMD_FIRST && buf->subType < THREAD_SUBTYPE_UPDATE_CMD_LAST) {
                if (g_updateCmdCbPkt[buf->subType].cb != nullptr) {
                    g_updateCmdCbPkt[buf->subType].cb(buf);
                }
            }

            // TODO: resume other threads
        } else {
            QThread::msleep(300);
        }
    }
}

