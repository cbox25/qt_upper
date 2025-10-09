#ifndef QLOGGER_MANAGER_H
#define QLOGGER_MANAGER_H

#include <QObject>
#include <QMutex>
#include <QVector>
#include "QLoggerProtocol.h"
#include "QLoggerWriter.h"
#include "../src/multi_threads.h"

class QLoggerWriter;

/**
 * @brief QLoggerManager 日志管理器单例
 * 
 * 负责管理所有日志写入线程、日志目的地、日志队列等，提供统一的日志接口。
 */
class QLoggerManager : public QObject {
    Q_OBJECT
public:
    // 获取单例
    static QLoggerManager *getInstance();

    // 设置线程管理器（用于跨线程日志队列）
    void setThreadManager(ThreadManager *manager) { threadManager = manager; }

    /**
     * @brief 添加日志写入目的地（单模块）
     * @param fileDest 日志文件名
     * @param module   日志模块名
     * @param level    日志级别
     * @param fileFolderDestination 日志文件夹
     * @param mode     日志模式
     * @param fileSuffixIfFull 文件满时重命名策略
     * @param messageOptions   日志显示选项
     * @param notify  是否通知
     */
    bool addDestination(const QString &fileDest, const QString &module, QLogger::LogLevel level = QLogger::LogLevel::Warning,
                        const QString &fileFolderDestination = QString(), QLogger::LogModes mode = QLogger::LogMode::OnlyFile,
                        QLogger::LogFileDisplay fileSuffixIfFull = QLogger::LogFileDisplay::DateTime,
                        QLogger::LogMessageDisplays messageOptions = QLogger::LogMessageDisplays(QLogger::LogMessageDisplay::Default),
                        bool notify = true);

    /**
     * @brief 添加日志写入目的地（多模块）
     */
    bool addDestination(const QString &fileDest, const QStringList &modules, QLogger::LogLevel level = QLogger::LogLevel::Warning,
                        const QString &fileFolderDestination = QString(), QLogger::LogModes mode = QLogger::LogMode::OnlyFile,
                        QLogger::LogFileDisplay fileSuffixIfFull = QLogger::LogFileDisplay::DateTime,
                        QLogger::LogMessageDisplays messageOptions = QLogger::LogMessageDisplays(QLogger::LogMessageDisplay::Default),
                        bool notify = true);

    /**
     * @brief 日志消息入队
     */
    void enqueueMessage(const QString &module, QLogger::LogLevel level, const QString &message, const QString &function,
                        const QString &file, int line);

    // 日志管理器暂停/恢复
    bool isPaused() const { return mIsStop; }
    void pause();
    void resume();

    // 获取/设置默认参数
    QString getDefaultFileDestinationFolder() const { return mDefaultFileDestinationFolder; }
    QLogger::LogModes getDefaultMode() const { return mDefaultMode; }
    QLogger::LogLevel getDefaultLevel() const { return mDefaultLevel; }
    void setDefaultFileDestinationFolder(const QString &fileDestinationFolder);
    void setDefaultFileDestination(const QString &fileDestination);
    void setDefaultFileSuffixIfFull(QLogger::LogFileDisplay fileSuffixIfFull);
    void setDefaultLevel(QLogger::LogLevel level);
    void setDefaultMode(QLogger::LogModes mode);
    void setDefaultMaxFileSize(int maxFileSize);
    void setDefaultMessageOptions(QLogger::LogMessageDisplays messageOptions);

    // 批量覆盖所有日志写入线程的参数
    void overwriteLogMode(QLogger::LogModes mode);
    void overwriteLogLevel(QLogger::LogLevel level);
    void overwriteMaxFileSize(int maxSize);

    // 日志文件夹迁移、清理
    void moveLogsWhenClose(const QString &newLogsFolder);
    static void clearFileDestinationFolder(const QString &fileFolderDestination, int days = -1);

    // 获取所有日志写入线程
    const QVector<QLoggerWriter *> &getWriters() const { return writers; }

private:
    QLoggerManager();
    ~QLoggerManager();

    // 创建日志写入对象
    QLoggerWriter *createWriter(const QString &fileDest, QLogger::LogLevel level, const QString &fileFolderDestination,
                                QLogger::LogModes mode, QLogger::LogFileDisplay fileSuffixIfFull, QLogger::LogMessageDisplays messageOptions) const;
    // 启动日志写入线程
    void startWriter(const QString &module, QLoggerWriter *log, QLogger::LogModes mode, bool notify);
    // 写入并清空队列
    void writeAndDequeueMessages(const QString &module);

    static QLoggerManager *instance; // 单例指针
    ThreadManager *threadManager;    // 线程管理器
    QVector<QLoggerWriter *> writers; // 所有日志写入线程
    QMap<QString, QLoggerWriter *> mModuleDest; // 模块到写入线程的映射
    QMultiMap<QString, QVector<QVariant>> mNonWriterQueue; // 非写入队列
    QString mDefaultFileDestinationFolder; // 默认日志文件夹
    QString mDefaultFileDestination;       // 默认日志文件名
    QLogger::LogFileDisplay mDefaultFileSuffixIfFull; // 默认重命名策略
    QLogger::LogModes mDefaultMode;        // 默认日志模式
    QLogger::LogLevel mDefaultLevel;       // 默认日志级别
    int mDefaultMaxFileSize;               // 默认最大文件大小
    QLogger::LogMessageDisplays mDefaultMessageOptions; // 默认显示选项
    QString mNewLogsFolder;                // 日志迁移目标
    QMutex mutex;                          // 互斥锁
    quint16 sequence;                      // 日志序号
    bool mIsStop;                          // 停止标志
};

#endif // QLOGGER_MANAGER_H
