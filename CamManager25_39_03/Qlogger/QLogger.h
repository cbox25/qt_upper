#ifndef QLOGGER_H
#define QLOGGER_H

#include <QMutex>
#include <QMap>
#include <QVariant>
#include "QLoggerLevel.h"
#include "QLoggerWriter.h"
#include "QLoggerManager.h"

namespace QLogger
{
extern void QLog_(const QString &module, LogLevel level, const QString &message, const QString &function,
                  const QString &file = QString(), int line = -1);

#define QLog_Trace(module, message) QLogger::QLog_(module, QLogger::LogLevel::Trace, message, __FUNCTION__, __FILE__, __LINE__)
#define QLog_Debug(module, message) QLogger::QLog_(module, QLogger::LogLevel::Debug, message, __FUNCTION__, __FILE__, __LINE__)
#define QLog_Info(module, message) QLogger::QLog_(module, QLogger::LogLevel::Info, message, __FUNCTION__, __FILE__, __LINE__)
#define QLog_Warning(module, message) QLogger::QLog_(module, QLogger::LogLevel::Warning, message, __FUNCTION__, __FILE__, __LINE__)
#define QLog_Error(module, message) QLogger::QLog_(module, QLogger::LogLevel::Error, message, __FUNCTION__, __FILE__, __LINE__)
#define QLog_Fatal(module, message) QLogger::QLog_(module, QLogger::LogLevel::Fatal, message, __FUNCTION__, __FILE__, __LINE__)
}

#endif // QLOGGER_H
