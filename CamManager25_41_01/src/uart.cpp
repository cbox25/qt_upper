#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "crc.h"
#include "uart.h"
#include "common.h"
#include <QSerialPort>
#include <string.h>
#include <QElapsedTimer>
#include <QDateTime>
#include "../QLogger/QLogger.h"
#include "lz4.h"

QSerialPort g_serialPort;  //  Create a global serial port object that is common throughout the entire project

bool OpenSerialPort(QSerialPort *serialPort) { // Opening a serial port
    return serialPort->open(QIODevice::ReadWrite);
}

bool ConfigureSerialPort(QSerialPort *serialPort, int baudRate, const QString &text) // Configure the serial port
{
    serialPort->setPortName(text); // designated port

    serialPort->setBaudRate(baudRate); // Specify baud rate
    serialPort->setDataBits(QSerialPort::Data8); // Specify 8 bits
    serialPort->setParity(QSerialPort::NoParity);
    serialPort->setStopBits(QSerialPort::OneStop); // A stop bit
    serialPort->setFlowControl(QSerialPort::NoFlowControl); // No verification

    return true;
}

void UartWriteBuf(QSerialPort *serialPort, const char *buf, int len) // send
{
    if (serialPort->isOpen()) {
        serialPort->clear(); // clear
        serialPort->write(buf, len); // write
        DelayMs(3); // delay
        // serialPort->waitForBytesWritten(50); // wait for send finished
    } else {
        ; // TODO
    }
}

int UartReadBuf(QSerialPort *serialPort, const char *buf)
{
    if (!serialPort || !buf) {
        return -1;
    }

    QByteArray data;
    if (serialPort->isOpen()) {
        serialPort->waitForReadyRead(5);
        if(serialPort->bytesAvailable()) { // Determine whether there is data inside the serial port cache area. If there is data, read it out
            data = serialPort->read(UART_PACKET_LEN_MAX);
            memcpy((void *)buf, data.data(), data.size());
        }
    } else {
        ; // TODO
    }

    return data.size();
}

int UartReadBuf(QSerialPort *serialPort, char *buf, int timeOutMs)
{
    if (!serialPort || !buf || timeOutMs < 0) {
        return -1;
    }

    QByteArray data;
    if (serialPort->isOpen()) {
        serialPort->waitForReadyRead(timeOutMs);
        if(serialPort->bytesAvailable()) { // Determine whether there is data inside the serial port cache area. If there is data, read it out
            data = serialPort->read(UART_PACKET_LEN_MAX);
            memcpy((void *)buf, data.data(), data.size());
        }
    } else {
        ; // TODO
    }

    return data.size();
}

void ClearUart(QSerialPort *dev) // Clear the serial port cache area
{
    if (dev->isOpen()) {
        dev->clear();
    }
}

int MakeUartPacket(uint8_t op, uint32_t addr, uint32_t writeValue, int readLen, uint8_t *uartBuf) // Package protocol frame
{ // Entry parameters: Read and write instructions, register start address, write 32-bit value, read byte count, output cache
    /* opera reg */
    opera_reg_t op_pkt;
    int send_len = 0;
    uint16_t dataLen = 0; // data len of data segment

    /* usart */
    uart_packet_t uart_pkt;
    uint16_t serial_num = 0;
    uint32_t len_tmp = 0; // The frame header fixes the length of the part

    memset(&op_pkt, 0x00, sizeof(op_pkt));

    /* make reg op packet*/
    op_pkt.op = op;
    op_pkt.reg_start_addr = addr;

    if (op_pkt.op == OPERA_REG_WRITE) { // Determine whether it is a write
        op_pkt.len = 4; // The write register is fixed at 4 bytes
        memcpy(op_pkt.data, &writeValue, sizeof(writeValue)); // Write the data to be written to the data in the structure
    } else {
        op_pkt.len = readLen; // The length of reading
    }

    uart_pkt.head = hton16(UART_PACKET_HEAD);
    uart_pkt.type = READ_WRITE_REG;
    uart_pkt.compressType = COMPRESS_TYPE_NONE;
    uart_pkt.reserved[0] = 0x00;
    uart_pkt.reserved[1] = 0x00;
    uart_pkt.serial_num = serial_num;
    if (op_pkt.op == OPERA_REG_WRITE) { // Judge the read and write to obtain the corresponding length
        dataLen = sizeof(op_pkt.op) + sizeof(op_pkt.reg_start_addr) + sizeof(op_pkt.len) + op_pkt.len; // 1,4,4 + the actual 4 bytes written
    } else {
        dataLen = sizeof(op_pkt.op) + sizeof(op_pkt.reg_start_addr) + sizeof(op_pkt.len); // Data length calculation: 1,4,4
    }
    uart_pkt.data_len = dataLen;

    len_tmp = (uint8_t *)uart_pkt.data - (uint8_t *)&uart_pkt.head;
    memcpy(uartBuf, &uart_pkt, len_tmp);
    memcpy(uartBuf + len_tmp, &op_pkt, dataLen);
    uart_pkt.crc16 = calc_crc16(uartBuf, len_tmp + dataLen);
    memcpy(uartBuf + len_tmp + dataLen, &uart_pkt.crc16, sizeof(uart_pkt.crc16));
    send_len = len_tmp + dataLen + sizeof(uart_pkt.crc16);

    return send_len;
}

int SendCmdSaveArgs(QSerialPort *dev, uint8_t *data , int dataLen) // Save args
{
    int len = 0;
    uart_packet_t packet;
    uint8_t buf[UART_PACKET_LEN_MAX] = {0};

    if (dev == NULL) {
        return -1;
    }
    memset(buf, 0x5A, sizeof(buf));

    packet.head = hton16(UART_PACKET_HEAD);
    packet.type = CMD;
    packet.compressType = COMPRESS_TYPE_NONE;
    packet.reserved[0] = 0x00;
    packet.reserved[1] = 0x00;
    packet.serial_num = 0;
    packet.data_len = dataLen + 1; // 1: sizeof CMD_SAVE_ARGS
    len = (uint8_t *)packet.data - (uint8_t *)&packet.head;
    packet.data[0] = CMD_SAVE_ARGS;
    len += 1;

    memcpy(buf, &packet, len);
    memcpy(buf + len, data, dataLen);
    len += dataLen;
    packet.crc16 = calc_crc16(buf, len);
    memcpy(buf + len, &packet.crc16, sizeof(packet.crc16));
    len += sizeof(packet.crc16);

    UartWriteBuf(dev, (const char *)buf, len);
    PrintBuf((const char *)buf, len, "CMD_SAVE_ARGS:");

    /* TODO: parse the response data from mcu */

    return 0;
}

int ParseResponseCmdSheetWriteAck(QSerialPort *dev)
{
    uint8_t readBuf[UART_PACKET_LEN_MAX] = {0};
    int readLen = 0;
    QElapsedTimer timer;

    if (dev == NULL) {
        return -1;
    }
    /* read data from uart */
    memset(readBuf, 0x00, sizeof(readBuf));
    timer.start();
    do {
        readLen = UartReadBuf(dev, (char *)readBuf);
        DelayMs(10);
    } while (readLen == 0 && timer.elapsed() < 2000);

    if (readLen <= 0) {
        QLog_Error("CamManager", QString("[%1] [%2:%3] get uart read data failed")
                       .arg(QDateTime::currentDateTime()
                                .toString("yyyy-MM-dd HH:mm:ss.zzz"))
                       .arg(__FILE__).arg(__LINE__));
        return -2;
    }

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

    if (*(p + 2) == RESPONSE_CMD && *(p + 10) == CMD_SHEET_WRITE && *(p + 11) == 0x01) {
        qDebug() << "parse response sheet write info OK";
        PrintBuf((const char *)p, readLen - i, "info: ");
    } else {
        qDebug() << "parse response sheet write info error";
        return -4;
    }
    return 0;
}

int SendCmdSheetWrite(QSerialPort *dev, uint16_t pktNum) // cmd sheet write
{
    int len = 0, ret = 0;
    uart_packet_t packet;
    uint8_t buf[UART_PACKET_LEN_MAX] = {0};

    if (dev == NULL) {
        return -1;
    }
    memset(buf, 0x5A, sizeof(buf));

    packet.head = hton16(UART_PACKET_HEAD);
    packet.type = CMD;
    packet.compressType = COMPRESS_TYPE_NONE;
    packet.reserved[0] = 0x00;
    packet.reserved[1] = 0x00;
    packet.serial_num = 0;
    packet.data_len = 3;
    len = (uint8_t *)packet.data - (uint8_t *)&packet.head;
    packet.data[0] = CMD_SHEET_WRITE;
    packet.data[1] = pktNum & 0xFF;
    packet.data[2] = (pktNum >> 8) & 0xFF;

    memcpy(buf, &packet, len);
    memcpy(buf + len, packet.data, packet.data_len);

    len += packet.data_len;
    packet.crc16 = calc_crc16(buf, len);
    memcpy(buf + len, &packet.crc16, sizeof(packet.crc16));
    len += sizeof(packet.crc16);

    UartWriteBuf(dev, (const char *)buf, len);
    PrintBuf((const char *)buf, len, "CMD_SHEET_WRITE_PACKET:");

    /* wait for response from mcu */
    ret = ParseResponseCmdSheetWriteAck(dev);

    if (ret < 0) {
        return -2;
    }
    return 0;
}

int ParseResponseCmdSheetReadAck(QSerialPort *dev, uint16_t *pktNum)
{
    uint8_t readBuf[UART_PACKET_LEN_MAX] = {0};
    int readLen = 0;
    QElapsedTimer timer;

    if (dev == NULL) {
        qDebug() << "error 1";
        return -1;
    }
    /* read data from uart */
    memset(readBuf, 0x00, sizeof(readBuf));
    timer.start();
    do {
        readLen = UartReadBuf(dev, (char *)readBuf);
        DelayMs(10);
    } while (readLen == 0 && timer.elapsed() < 2000);

    if (readLen <= 0) {
        QLog_Error("CamManager", QString("[%1] [%2:%3] get uart read data failed")
                       .arg(QDateTime::currentDateTime()
                                .toString("yyyy-MM-dd HH:mm:ss.zzz"))
                       .arg(__FILE__).arg(__LINE__));
        qDebug() << "error 2";
        return -2;
    }

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
        qDebug() << "error 3";
        return -3;
    }

    if (*(p + 2) == RESPONSE_CMD && *(p + 10) == CMD_SHEET_READ && *(p + 11) == 0x01) {
        *pktNum = *(p + 12) + (*(p + 13) << 8);
        qDebug() << "parse response sheet read info OK " << *pktNum;
    } else {
        PrintBuf((const char *)p, readLen - i, "parse info: ");
        qDebug() << "parse response sheet read info error";
        qDebug() << "error 4";
        return -4;
    }
    return 0;
}

int SendCmdSheetRead(QSerialPort *dev, uint16_t *pktNum) // cmd sheet read
{
    int len = 0, ret = 0;
    uart_packet_t packet;
    uint8_t buf[UART_PACKET_LEN_MAX] = {0};

    if (dev == NULL) {
        return -1;
    }
    memset(buf, 0x5A, sizeof(buf));

    packet.head = hton16(UART_PACKET_HEAD);
    packet.type = CMD;
    packet.compressType = COMPRESS_TYPE_NONE;
    packet.reserved[0] = 0x00;
    packet.reserved[1] = 0x00;
    packet.serial_num = 0;
    packet.data_len = 1;
    len = (uint8_t *)packet.data - (uint8_t *)&packet.head;
    packet.data[0] = CMD_SHEET_READ;

    memcpy(buf, &packet, len);
    memcpy(buf + len, packet.data, packet.data_len);

    len += packet.data_len;
    packet.crc16 = calc_crc16(buf, len);
    memcpy(buf + len, &packet.crc16, sizeof(packet.crc16));
    len += sizeof(packet.crc16);

    UartWriteBuf(dev, (const char *)buf, len);
    PrintBuf((const char *)buf, len, "CMD_SHEET_READ:");

    /* wait for response from mcu */
    ret = ParseResponseCmdSheetReadAck(dev, pktNum);

    if (ret < 0) {
        debugNoQuote() << "errorcode:" << ret;
        return -2;
    }
    return 0;
}

int ParseResponseCmdVersionReadAck(QSerialPort *dev)
{
    uint8_t readBuf[UART_PACKET_LEN_MAX] = {0};
    int readLen = 0;
    QElapsedTimer timer;

    if (dev == NULL) {
        return -1;
    }
    /* read data from uart */
    memset(readBuf, 0x00, sizeof(readBuf));
    timer.start();
    do {
        readLen = UartReadBuf(dev, (char *)readBuf);
        DelayMs(10);
    } while (readLen == 0 && timer.elapsed() < 500);

    if (readLen <= 0) {
        QLog_Error("CamManager", QString("[%1] [%2:%3] get uart read data failed")
                       .arg(QDateTime::currentDateTime()
                                .toString("yyyy-MM-dd HH:mm:ss.zzz"))
                       .arg(__FILE__).arg(__LINE__));
        return -2;
    }

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

    if (*(p + 2) == RESPONSE_CMD && *(p + 10) == CMD_VERSION_READ && *(p + 11) == 0x01) {
        qDebug() << "parse response version read info OK ";

        int dataLength = readLen - i - 12;
        if (dataLength < 5) { // At least: 1 character + "/" + 1 character + 2 bytes of CRC
            qDebug() << "Data length too short:" << dataLength;
            return -4;
        }

        uint8_t *dataStart = p + 12;
        QString fullData = QString::fromLatin1(reinterpret_cast<const char*>(dataStart), dataLength);

        // Find the delimiter "/"
        int separatorPos = fullData.indexOf('|');
        if (separatorPos == -1) {
            qDebug() << "Separator '|' not found";
            return -5;
        }

        g_softKernelVersion = fullData.left(separatorPos);

        int secondStringLength = dataLength - separatorPos - 3; // Subtract the lengths of the delimiter and CRC
        if (secondStringLength < 0) {
            qDebug() << "Second string length invalid";
            return -6;
        }

        g_fpgaVersion = fullData.mid(separatorPos + 1, secondStringLength);

        uint16_t crc = dataStart[dataLength - 2] | (dataStart[dataLength - 1] << 8);

        debugNoQuote() << "Soft Kernel Version:" << g_softKernelVersion;
        debugNoQuote() << "FPGA Version:" << g_fpgaVersion;

        if (!check_crc16(p, readLen -i - 2, crc)) {
            qDebug() << "CRC check failed";
            return -7;
        }
    } else {
        PrintBuf((const char *)p, readLen - i, "parse info: ");
        qDebug() << "parse response version read info error";
        return -8;
    }
    return 0;
}

int SendCmdVersionRead(void) // cmd version read
{
    int len = 0, ret = 0;
    uart_packet_t packet;
    uint8_t buf[UART_PACKET_LEN_MAX] = {0};
    QSerialPort *dev = &g_serialPort;

    if (dev == NULL) {
        return -1;
    }
    memset(buf, 0x5A, sizeof(buf));

    packet.head = hton16(UART_PACKET_HEAD);
    packet.type = CMD;
    packet.compressType = COMPRESS_TYPE_NONE;
    packet.reserved[0] = 0x00;
    packet.reserved[1] = 0x00;
    packet.serial_num = 0;
    packet.data_len = 1;
    len = (uint8_t *)packet.data - (uint8_t *)&packet.head;
    packet.data[0] = CMD_VERSION_READ;

    memcpy(buf, &packet, len);
    memcpy(buf + len, packet.data, packet.data_len);

    len += packet.data_len;
    packet.crc16 = calc_crc16(buf, len);
    memcpy(buf + len, &packet.crc16, sizeof(packet.crc16));
    len += sizeof(packet.crc16);

    UartWriteBuf(dev, (const char *)buf, len);
    PrintBuf((const char *)buf, len, "CMD_VERSION_READ:");

    /* wait for response from mcu */
    ret = ParseResponseCmdVersionReadAck(dev);

    if (ret < 0) {
        debugNoQuote() << "errorcode:" << ret;
        return -2;
    }
    return 0;
}
