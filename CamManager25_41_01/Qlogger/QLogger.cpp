#include "QLogger.h"
#include "QLoggerManager.h"
#include "QLoggerWriter.h"

#include <QDateTime>
#include <QDir>

namespace QLogger
{/**
 * @brief QLog_ 日志主入口实现
 *        实际调用QLoggerManager的enqueueMessage，将日志信息放入队列，由后台线程异步写入
 */
void QLog_(const QString &module, LogLevel level, const QString &message, const QString &function, const QString &file, int line)
{
    ::QLoggerManager::getInstance()->enqueueMessage(module, level, message, function, file, line);
}
}
