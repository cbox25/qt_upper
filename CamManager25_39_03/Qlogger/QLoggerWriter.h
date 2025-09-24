#ifndef QLOGGER_WRITER_H
#define QLOGGER_WRITER_H

#include <QThread>
#include <QFile>
#include <QString>
#include "QLoggerProtocol.h"
#include "../src/multi_threads.h"
#include "QLoggerLevel.h"

/**
 * @brief QLoggerWriter 日志写入线程类
 * 
 * 负责将日志内容异步写入文件或输出到控制台，支持日志级别过滤、自动分割文件、多线程安全等功能。
 */
class QLoggerWriter : public QThread
{
    Q_OBJECT
public:
    /**
     * @brief 构造函数
     * @param fileDest         日志文件名
     * @param level            日志级别过滤
     * @param fileFolderDest   日志文件夹路径
     * @param mode             日志模式（仅文件、仅控制台等）
     * @param fileSuffixIfFull 日志文件满时的重命名策略
     * @param messageOptions   日志消息显示选项（是否显示时间、线程、模块等）
     */
    QLoggerWriter(const QString &fileDest, QLogger::LogLevel level, const QString &fileFolderDest,
                  QLogger::LogModes mode, QLogger::LogFileDisplay fileSuffixIfFull,
                  QLogger::LogMessageDisplays messageOptions);
    ~QLoggerWriter();

    // 设置线程管理器（用于跨线程日志队列）
    void setThreadManager(ThreadManager *manager) { threadManager = manager; }
    // 设置最大日志文件大小
    void setMaxFileSize(int size) { mMaxFileSize = size; }
    // 获取/设置日志级别
    QLogger::LogLevel getLevel() const { return mLevel; }
    void setLogLevel(QLogger::LogLevel level) { mLevel = level; }
    // 设置日志模式
    void setLogMode(QLogger::LogModes mode) { mMode = mode; }
    // 停止标志
    bool isStop() const { return mIsStop; }
    void stop(bool stop) { mIsStop = stop; }

    /**
     * @brief 日志消息入队
     * @param datetime   日志时间
     * @param threadId   线程ID
     * @param module     日志模块
     * @param level      日志级别
     * @param function   函数名
     * @param file       文件名
     * @param line       行号
     * @param message    日志内容
     */
    void enqueue(const QDateTime &datetime, const QString &threadId, const QString &module,
                 QLogger::LogLevel level, const QString &function, const QString &file, int line,
                 const QString &message);

protected:
    // 线程主循环
    void run() override;
    // 实际写入日志
    void write(const QString &message);
    // 文件大小检查与自动重命名
    bool renameFileIfFull();
    // 关闭日志文件
    void closeDestination();

private:
    QFile mFile;                        // 日志文件对象
    QString mFileDest;                  // 日志文件名
    QLogger::LogLevel mLevel;           // 日志级别
    QLogger::LogModes mMode;            // 日志模式
    QString mFileFolderDest;            // 日志文件夹
    QLogger::LogFileDisplay mFileSuffixIfFull; // 文件重命名策略
    QLogger::LogMessageDisplays mMessageOptions; // 日志显示选项
    ThreadManager *threadManager;       // 线程管理器
    int mMaxFileSize;                   // 最大文件大小
    bool mIsStop;                       // 停止标志
};

#endif // QLOGGER_WRITER_H
