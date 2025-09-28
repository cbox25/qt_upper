#ifndef QLOGGER_LEVEL_H
#define QLOGGER_LEVEL_H

#include <QObject>
#include <QFlags>

namespace QLogger
{
/**
 * @brief 日志级别枚举
 */
enum class LogLevel
{
    Trace = 0,   // 追踪，最详细
    Debug = 1,   // 调试
    Info = 2,    // 信息
    Warning = 3, // 警告
    Error = 4,   // 错误
    Fatal = 5    // 致命
};

/**
 * @brief 日志输出模式
 */
enum class LogMode
{
    Disabled = 0,   // 禁用日志
    OnlyFile = 1,   // 仅写文件
    OnlyConsole = 2,// 仅控制台
    Both = OnlyFile | OnlyConsole // 文件+控制台
};
Q_DECLARE_FLAGS(LogModes, LogMode)

/**
 * @brief 日志文件重命名策略
 */
enum class LogFileDisplay
{
    Number = 0,   // 按编号
    DateTime = 1  // 按时间戳
};

/**
 * @brief 日志消息显示选项（可组合）
 */
enum class LogMessageDisplay
{
    Default = 0x00,   // 默认
    DateTime = 0x01,  // 显示时间
    ThreadId = 0x02,  // 显示线程ID
    Module = 0x04,    // 显示模块
    Function = 0x08,  // 显示函数
    File = 0x10,      // 显示文件
    Line = 0x20       // 显示行号
};
Q_DECLARE_FLAGS(LogMessageDisplays, LogMessageDisplay)

} // namespace QLogger

Q_DECLARE_METATYPE(QLogger::LogLevel)

#endif // QLOGGER_LEVEL_H
