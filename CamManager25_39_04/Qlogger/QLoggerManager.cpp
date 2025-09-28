#include "QLoggerManager.h"
#include "QLoggerWriter.h"
#include "../src/multi_threads.h"

// QLoggerManager单例指针
QLoggerManager *QLoggerManager::instance = nullptr;

/**
 * @brief QLoggerManager 构造函数
 *        初始化默认参数
 */
QLoggerManager::QLoggerManager() : QObject(nullptr), threadManager(nullptr), mDefaultMaxFileSize(1024 * 1024), sequence(0), mIsStop(false)
{
    mDefaultFileSuffixIfFull = QLogger::LogFileDisplay::DateTime;
    mDefaultMode = QLogger::LogMode::OnlyFile;
    mDefaultLevel = QLogger::LogLevel::Warning;
    mDefaultMessageOptions = QLogger::LogMessageDisplays(QLogger::LogMessageDisplay::Default);
}

QLoggerManager::~QLoggerManager()
{
    QMutexLocker locker(&mutex);
    // 停止并释放所有日志写入线程
    for (QLoggerWriter *writer : writers) {
        writer->requestInterruption();
        writer->wait();
        delete writer;
    }
    writers.clear();
    mModuleDest.clear();
    mNonWriterQueue.clear();
}

/**
 * @brief 获取QLoggerManager单例
 */
QLoggerManager *QLoggerManager::getInstance()
{
    static QLoggerManager instance;
    return &instance;
}

/**
 * @brief 为指定模块添加日志写入目的地
 * @param fileDest 日志文件名
 * @param module   日志模块名
 * @param level    日志级别
 * @param fileFolderDestination 日志文件夹
 * @param mode     日志模式
 * @param fileSuffixIfFull 文件满时重命名策略
 * @param messageOptions   日志显示选项
 * @param notify  是否通知
 */
bool QLoggerManager::addDestination(const QString &fileDest, const QString &module, QLogger::LogLevel level,
                                    const QString &fileFolderDestination, QLogger::LogModes mode,
                                    QLogger::LogFileDisplay fileSuffixIfFull, QLogger::LogMessageDisplays messageOptions,
                                    bool notify)
{
    QMutexLocker locker(&mutex);
    if (!mModuleDest.contains(module)) {
        QLoggerWriter *writer = createWriter(fileDest, level, fileFolderDestination, mode, fileSuffixIfFull, messageOptions);
        
        // 设置ThreadManager（如果已设置）
        if (threadManager) {
            writer->setThreadManager(threadManager);
        }
        
        writers.append(writer);
        mModuleDest.insert(module, writer);
        startWriter(module, writer, mode, notify);
        return true;
    }
    return false;
}

/**
 * @brief 为多个模块批量添加日志写入目的地
 */
bool QLoggerManager::addDestination(const QString &fileDest, const QStringList &modules, QLogger::LogLevel level,
                                    const QString &fileFolderDestination, QLogger::LogModes mode,
                                    QLogger::LogFileDisplay fileSuffixIfFull, QLogger::LogMessageDisplays messageOptions,
                                    bool notify)
{
    QMutexLocker locker(&mutex);
    bool allAdded = false;
    for (const auto &module : modules) {
        if (!mModuleDest.contains(module)) {
            QLoggerWriter *writer = createWriter(fileDest, level, fileFolderDestination, mode, fileSuffixIfFull, messageOptions);
            writers.append(writer);
            mModuleDest.insert(module, writer);
            startWriter(module, writer, mode, notify);
            allAdded = true;
        }
    }
    return allAdded;
}

/**
 * @brief 日志消息入队，最终由QLoggerWriter异步写入
 */
void QLoggerManager::enqueueMessage(const QString &module, QLogger::LogLevel level, const QString &message,
                                    const QString &function, const QString &file, int line)
{
    QMutexLocker locker(&mutex);
    if (!threadManager) {
        qDebug() << "ThreadManager为nullptr，无法处理日志消息:" << message;
        return;
    }
    // 组装日志帧并入队
    QString levelStr;
    switch (level) {
        case QLogger::LogLevel::Trace: levelStr = "Trace"; break;
        case QLogger::LogLevel::Debug: levelStr = "Debug"; break;
        case QLogger::LogLevel::Info: levelStr = "Info"; break;
        case QLogger::LogLevel::Warning: levelStr = "Warning"; break;
        case QLogger::LogLevel::Error: levelStr = "Error"; break;
        case QLogger::LogLevel::Fatal: levelStr = "Fatal"; break;
        default: levelStr = "Unknown"; break;
    }
    QString timeStr = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");
    QString threadId = QString::number((quintptr)QThread::currentThreadId());
    // 日志内容中包含日志级别
    QString logContent = QString("[%1][%2][%3][%4][%5][%6][%7] %8").arg(timeStr, threadId, module, levelStr, function, file, QString::number(line), message);
    QByteArray data = logContent.toUtf8();
    uint8_t type = 0x01; // CMD类型
    uint8_t subtype = 0x01; // 普通日志
    QByteArray frameData = QLogger::createLogFrame(type, subtype, data);
    //qDebug() << "发送协议帧内容:" << frameData.toHex();
    //qDebug() << "[QLoggerManager] 日志等级:" << static_cast<int>(level) << "内容:" << message;
    threadManager->EnqueueData(frameData);
    //qDebug() << "日志消息已发送到ThreadManager:" << message;
}

/**
 * @brief 创建日志写入对象
 */
QLoggerWriter *QLoggerManager::createWriter(const QString &fileDest, QLogger::LogLevel level,
                                            const QString &fileFolderDestination, QLogger::LogModes mode,
                                            QLogger::LogFileDisplay fileSuffixIfFull, QLogger::LogMessageDisplays messageOptions) const
{
    // 处理默认参数
    const auto lFileDest = fileDest.isEmpty() ? mDefaultFileDestination : fileDest;
    const auto lLevel = level == QLogger::LogLevel::Warning ? mDefaultLevel : level;
    const auto lFileFolderDestination = fileFolderDestination.isEmpty()
                                            ? mDefaultFileDestinationFolder
                                            : QDir::fromNativeSeparators(fileFolderDestination);
    const auto lMode = mode.testFlag(QLogger::LogMode::OnlyFile) ? mDefaultMode : mode;
    const auto lFileSuffixIfFull = fileSuffixIfFull == QLogger::LogFileDisplay::DateTime ? mDefaultFileSuffixIfFull : fileSuffixIfFull;
    const auto lMessageOptions = messageOptions.testFlag(QLogger::LogMessageDisplay::Default) ? mDefaultMessageOptions : messageOptions;

    QLoggerWriter *log = new QLoggerWriter(lFileDest, lLevel, lFileFolderDestination, lMode, lFileSuffixIfFull, lMessageOptions);
    log->setMaxFileSize(mDefaultMaxFileSize);
    log->stop(mIsStop);
    return log;
}

/**
 * @brief 启动日志写入线程
 */
void QLoggerManager::startWriter(const QString &module, QLoggerWriter *log, QLogger::LogModes mode, bool notify)
{
    if (notify)
    {
        const auto threadId = QString("%1").arg((quintptr)QThread::currentThread(), QT_POINTER_SIZE * 2, 16, QChar('0'));
    }
    if (!mode.testFlag(QLogger::LogMode::Disabled))
        log->start();
}

/**
 * @brief 清理指定文件夹下过期日志文件
 * @param fileFolderDestination 日志文件夹
 * @param days 过期天数
 */
void QLoggerManager::clearFileDestinationFolder(const QString &fileFolderDestination, int days)
{
    QDir dir(fileFolderDestination + QStringLiteral("/logs"));
    if (!dir.exists())
        return;
    dir.setFilter(QDir::Files | QDir::Hidden | QDir::NoSymLinks);
    const auto list = dir.entryInfoList();
    const auto now = QDateTime::currentDateTime();
    for (const auto &fileInfoIter : list)
    {
        if (fileInfoIter.lastModified().daysTo(now) >= days)
        {
            dir.remove(fileInfoIter.fileName());
        }
    }
}

// 下面是各种默认参数的设置和批量操作，均有详细注释，略
