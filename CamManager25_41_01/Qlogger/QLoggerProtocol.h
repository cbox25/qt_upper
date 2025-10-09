#ifndef QLOGGERPROTOCOL_H
#define QLOGGERPROTOCOL_H

#include <QByteArray>
#include <QString>
#include <QMutex>
#include <QWaitCondition>
#include <QThread>
#include <QVector>
#include <QFile>
#include <QDir>
#include <QDateTime>
#include <cstring>
#include <algorithm>
#include "../src/multi_threads.h"
#include "qendian.h"
#include "QLoggerLevel.h"

namespace QLogger
{
/**
 * @brief 生成日志帧（二进制格式，便于线程/进程间传递）
 * @param type     日志类型
 * @param subtype  日志子类型
 * @param data     日志数据
 * @return QByteArray 日志帧
 */
QByteArray createLogFrame(uint8_t type, uint8_t subtype, const QByteArray &data);

/**
 * @brief 解析日志帧
 * @param frame    输入的二进制帧
 * @param type     输出类型
 * @param subtype  输出子类型
 * @param data     输出数据
 * @return 是否解析成功
 */
bool parseLogFrame(const QByteArray &frame, uint8_t &type, uint8_t &subtype, QByteArray &data);
}

#endif // QLOGGER_PROTOCOL_H
