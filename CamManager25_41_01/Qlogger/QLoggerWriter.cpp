#include "QLoggerWriter.h"
#include <QDir>
#include <QDateTime>
#include <QMutexLocker>

/**
 * @brief QLoggerWriter 构造函数
 * @param fileDest         日志文件名
 * @param level            日志级别过滤
 * @param fileFolderDest   日志文件夹路径
 * @param mode             日志模式（仅文件、仅控制台等）
 * @param fileSuffixIfFull 日志文件满时的重命名策略
 * @param messageOptions   日志消息显示选项（是否显示时间、线程、模块等）
 * 
 * 该类负责日志的最终写入（文件/控制台），支持多线程安全、文件自动分割等功能。
 */
QLoggerWriter::QLoggerWriter(const QString &fileDest, QLogger::LogLevel level, const QString &fileFolderDest,
                             QLogger::LogModes mode, QLogger::LogFileDisplay fileSuffixIfFull,
                             QLogger::LogMessageDisplays messageOptions)
    : QThread(nullptr), mFileDest(fileDest), mLevel(level), mMode(mode), mFileFolderDest(fileFolderDest),
    mFileSuffixIfFull(fileSuffixIfFull), mMessageOptions(messageOptions), threadManager(nullptr), mMaxFileSize(1024 * 1024), mIsStop(false)
{
    //qDebug() << "QLoggerWriter构造函数 - 文件目标:" << fileDest << "文件夹:" << fileFolderDest;
    
    // 确保日志文件夹路径有效
    if (fileFolderDest.isEmpty()) {
        mFileFolderDest = QDir::currentPath() + "/logs/";
    }
    if (!mFileFolderDest.endsWith("/")) {
        mFileFolderDest.append("/");
    }
    // 构建完整文件路径
    QString fullPath = QDir(mFileFolderDest).filePath(mFileDest);
    //qDebug() << "完整文件路径:" << fullPath;
    // 确保日志目录存在
    QDir dir(mFileFolderDest);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
}

QLoggerWriter::~QLoggerWriter()
{
    closeDestination();
}

/**
 * @brief 日志写入线程主循环
 *        持续从ThreadManager队列中取出日志数据，解析后写入文件或控制台
 */
void QLoggerWriter::run()
{
    //qDebug() << "QLoggerWriter线程启动，ThreadManager:" << (threadManager ? "已设置" : "未设置");
    while (!isInterruptionRequested()) {
        try {
            if (threadManager) {
                // 从线程安全队列取出日志数据
                QByteArray data = threadManager->DequeueData();
                if (!data.isEmpty()) {
                    //qDebug() << "QLoggerWriter收到数据，大小:" << data.size();
                    uint8_t type, subtype;
                    QByteArray payload;
                    if (QLogger::parseLogFrame(data, type, subtype, payload)) {
                        if (type == 0x01) { // 只对CMD类型写日志
                            write(QString::fromUtf8(payload));
                            // 自动回ACK包
                            if (threadManager) {
                                uint8_t ackType = 0x84;
                                QByteArray ackFrame = QLogger::createLogFrame(ackType, subtype, payload);
                                threadManager->EnqueueData(ackFrame);
                                //qDebug() << "自动回发ACK帧:" << ackFrame.toHex();
                            }
                        }
                        // type==0x84（ACK）时，不写日志
                    } else {
                        qDebug() << "Failed to parse log frame, size:" << data.size();
                    }
                } else {
                    msleep(100); // 队列为空时休眠
                }
            } else {
                qDebug() << "ThreadManager为nullptr，等待...";
                msleep(1000);
            }
        } catch (const std::exception &e) {
            qDebug() << "Exception in QLoggerWriter::run:" << e.what();
        } catch (...) {
            qDebug() << "Unknown exception in QLoggerWriter::run";
        }
    }
    qDebug() << "QLoggerWriter::run stopped";
}

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
 * 
 * 根据配置拼接日志字符串，并调用write写入
 */
void QLoggerWriter::enqueue(const QDateTime &datetime, const QString &threadId, const QString &module,
                            QLogger::LogLevel level, const QString &function, const QString &file, int line,
                            const QString &message)
{
    //qDebug() << "enqueue被调用 - 消息级别:" << static_cast<int>(level) << "设置级别:" << static_cast<int>(mLevel);
    //qDebug() << "模式检查 - 当前模式:" << static_cast<int>(mMode) << "Disabled标志:" << mMode.testFlag(QLogger::LogMode::Disabled);
    
    // 日志级别过滤和禁用模式判断
    if (mLevel > level || mMode.testFlag(QLogger::LogMode::Disabled)) {
        //qDebug() << "消息被过滤掉 - 级别不匹配或模式被禁用";
        return;
    }

    //qDebug() << "开始构建日志消息";
    QString logMessage;
    // 按配置拼接日志内容
    if (mMessageOptions.testFlag(QLogger::LogMessageDisplay::DateTime))
        logMessage += QString("[%1]").arg(datetime.toString("yyyy-MM-dd HH:mm:ss.zzz"));
    if (mMessageOptions.testFlag(QLogger::LogMessageDisplay::ThreadId))
        logMessage += QString(" [%1]").arg(threadId);
    if (mMessageOptions.testFlag(QLogger::LogMessageDisplay::Module))
        logMessage += QString(" %1").arg(module);
    if (mMessageOptions.testFlag(QLogger::LogMessageDisplay::File))
        logMessage += QString(" %1:%2").arg(file).arg(line);
    if (mMessageOptions.testFlag(QLogger::LogMessageDisplay::Function))
        logMessage += QString(" %1").arg(function);
    logMessage += QString(" %1").arg(message);

    //qDebug() << "构建的日志消息:" << logMessage;
    write(logMessage);
}

/**
 * @brief 实际写入日志到文件或控制台
 * @param message 日志内容
 * 
 * 支持文件自动分割、UTF-8编码、每条日志自动换行
 */
void QLoggerWriter::write(const QString &message)
{
    if (mMode.testFlag(QLogger::LogMode::Disabled)) return;

    // 控制台输出
    if (mMode.testFlag(QLogger::LogMode::OnlyConsole)) {
        //qDebug().noquote() << message;
    }

    // 文件输出
    if (mMode.testFlag(QLogger::LogMode::OnlyFile)) {
        //qDebug() << "准备写入文件，路径:" << QDir(mFileFolderDest).filePath(mFileDest);
        
        // 确保目录存在
        QDir dir(mFileFolderDest);
        if (!dir.exists()) {
            dir.mkpath(".");
        }
        
        QString fullPath = QDir(mFileFolderDest).filePath(mFileDest);
        //qDebug() << "完整文件路径:" << fullPath;
        
        // 文件大小检查，自动分割
        if (renameFileIfFull()) {
            mFile.setFileName(fullPath);
        }
        if (!mFile.isOpen()) {
            mFile.setFileName(fullPath);
            //qDebug() << "尝试打开文件:" << mFile.fileName();
            if (!mFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
                //qDebug() << "Failed to open log file:" << mFile.fileName() << "Error:" << mFile.errorString();
                return;
            }
            //qDebug() << "文件打开成功:" << mFile.fileName();
        }
        QTextStream out(&mFile);
        out.setCodec("UTF-8"); // 保证UTF-8编码
        out << message << "\n"; // 每条日志后加换行
        out.flush();
        //qDebug() << "日志写入成功:" << message;
        //qDebug() << "[QLoggerWriter] 写入日志内容:" << message;
        //qDebug() << "[QLoggerWriter] mMode:" << static_cast<int>(mMode) << "mFileDest:" << mFileDest;
    }
}

/**
 * @brief 日志文件大小检查与自动重命名
 * @return 是否重命名成功
 */
bool QLoggerWriter::renameFileIfFull()
{
    if (mFile.size() >= mMaxFileSize) {
        mFile.close();
        QFileInfo fileInfo(mFile.fileName());
        QString newName = fileInfo.baseName();
        if (mFileSuffixIfFull == QLogger::LogFileDisplay::DateTime) {
            newName += QDateTime::currentDateTime().toString(".yyyyMMddHHmmss");
        } else {
            newName += QString(".%1").arg(QFileInfo(mFile.fileName()).size());
        }
        newName += "." + fileInfo.suffix();
        return mFile.rename(QDir(mFileFolderDest).filePath(newName));
    }
    return false;
}

/**
 * @brief 关闭日志文件
 */
void QLoggerWriter::closeDestination()
{
    if (mFile.isOpen()) {
        mFile.close();
    }
}

