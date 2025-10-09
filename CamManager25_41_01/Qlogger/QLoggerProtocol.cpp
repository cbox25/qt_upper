#include "QLoggerProtocol.h"
#include <QByteArray>
#include <QDebug>
#include <QtEndian>

namespace QLogger {

// CRC16-CCITT (0x1021) 实现
static quint16 crc16(const QByteArray &data)
{
    quint16 crc = 0xFFFF;
    for (char byte : data) {
        crc ^= (quint8)byte << 8;
        for (int i = 0; i < 8; ++i) {
            if (crc & 0x8000)
                crc = (crc << 1) ^ 0x1021;
            else
                crc = crc << 1;
        }
    }
    return crc;
}

QByteArray createLogFrame(uint8_t type, uint8_t subtype, const QByteArray &data)
{
    QByteArray frame;
    frame.append((char)0xEB); frame.append((char)0x90); // 帧头
    frame.append((char)type);                           // 类型
    frame.append((char)subtype);                        // 子类型
    frame.append((char)0x00); frame.append((char)0x00); // 序号/预留
    quint16 dataLen = qMin((int)data.size(), 512);
    frame.append((char)((dataLen >> 8) & 0xFF));        // 数据长度高字节
    frame.append((char)(dataLen & 0xFF));               // 数据长度低字节
    frame.append(data.left(512));                       // 数据
    quint16 crc = crc16(frame);
    frame.append((char)((crc >> 8) & 0xFF));            // CRC高字节
    frame.append((char)(crc & 0xFF));                   // CRC低字节
    //qDebug() << "createLogFrame: " << frame.toHex();
    return frame;
}

bool parseLogFrame(const QByteArray &frame, uint8_t &type, uint8_t &subtype, QByteArray &data)
{
    if (frame.size() < 10) return false;
    if ((quint8)frame[0] != 0xEB || (quint8)frame[1] != 0x90) return false;
    type = (quint8)frame[2];
    subtype = (quint8)frame[3];
    // 跳过预留
    quint16 dataLen = ((quint8)frame[6] << 8) | (quint8)frame[7];
    if (frame.size() < 8 + dataLen + 2) return false;
    data = frame.mid(8, dataLen);
    quint16 crcRecv = ((quint8)frame[8 + dataLen] << 8) | (quint8)frame[8 + dataLen + 1];
    quint16 crcCalc = crc16(frame.left(8 + dataLen));
    if (crcRecv != crcCalc) {
        qDebug() << "CRC校验失败:" << crcRecv << crcCalc;
        return false;
    }
    return true;
}

} // namespace QLogger
