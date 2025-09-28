#include "common.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QApplication>
#include <QFontDatabase>

CustomDebug::CustomDebug(bool noquote, bool nospace)
    : m_stream(qDebug())
{
    if (noquote && nospace) {
        m_stream = m_stream.noquote().nospace();
    } else if (noquote) {
        m_stream = m_stream.noquote();
    } else if (nospace) {
        m_stream = m_stream.nospace();
    }
}

CustomDebug debugNoQuoteNoSpace() {
    return CustomDebug(true, true);
}

CustomDebug debugNoSpace() {
    return CustomDebug(false, true);
}

CustomDebug debugNoQuote() {
    return CustomDebug(true, false);
}

void DelayMs(int ms)
{
    QElapsedTimer time;
    time.start();
    while(time.elapsed() < ms)
    {
        QCoreApplication::processEvents();
    }
}

uint16_t hton16(uint16_t host)
{
	uint16_t net = 0;

	net = ((host >> 8) & 0xFF) | ((host & 0xFF) << 8);

	return net;
}

uint32_t hton32(uint32_t host)
{
    uint32_t net = ((host & 0x000000FF) << 24 |
            (host & 0x0000FF00) << 8 |
            (host & 0x00FF0000) >> 8 |
            (host & 0xFF000000) >> 24);

    return net;
}

void PrintBuf(const char *data, int size)
{
    const int bytesPerLine = 16;
    for (int i = 0; i < size; i += bytesPerLine) {
        QString line;
        for (int j = 0; j < bytesPerLine && (i + j) < size; ++j) {
            if (j == 8) {
                line += "\t";
            }
            line += QString("%1 ").arg(static_cast<unsigned char>(data[i + j]), 2, 16, QLatin1Char('0')).toUpper();
        }
        qDebug() << line.trimmed().toStdString().c_str();
    }
}

void PrintBuf(const char *data, int size, const QString &title) {
    const int bytesPerLine = 16;

    // Output title (without quotation marks)
    qDebug().noquote() << title;

    for (int i = 0; i < size; i += bytesPerLine) {
        QString line;
        for (int j = 0; j < bytesPerLine && (i + j) < size; ++j) {
            // Add a TAB character after the 8th byte
            if (j == 8) {
                line += " ";
            }
            // Format it to two capital hexadecimal digits. If there are less than two digits, add zeros
            line += QString("%1 ").arg(static_cast<quint8>(data[i + j]), 2, 16, QLatin1Char('0')).toUpper();
        }

        qDebug().noquote() << line.trimmed();
    }
}

void SaveDataToFile(const char *data, int size, QString &file)
{
    QFile outFile(file);
    if (!outFile.open(QIODevice::WriteOnly))
    {
        qDebug() << "Failed to open file for writing:" << outFile.errorString();
        return;
    }

    // write data to file
    qint64 bytesWritten = outFile.write(data, size);
    if (bytesWritten != size)
    {
        qDebug() << "Failed to write data to file:" << outFile.errorString();
    }

    outFile.close();
}

// 字体管理函数实现
QFont getApplicationFont(int pointSize) {
    QFont appFont;
    
    // 检查QApplication是否已创建
    if (QApplication::instance()) {
        appFont = QApplication::font();
    } else {
        // 如果QApplication未创建，使用系统默认字体
        appFont = QFont();
    }
    
    if (pointSize > 0) {
        appFont.setPointSize(pointSize);
    }
    return appFont;
}

QFont getApplicationFont(const QString &family, int pointSize) {
    QFontDatabase fontDb;
    QFont appFont;
    
    // 检查指定的字体是否可用
    if (fontDb.hasFamily(family)) {
        appFont.setFamily(family);
    } else {
        // 如果指定字体不可用，使用应用程序全局字体
        if (QApplication::instance()) {
            appFont = QApplication::font();
        } else {
            appFont = QFont();
        }
        qDebug() << "Font family" << family << "not available, using application font:" << appFont.family();
    }
    
    if (pointSize > 0) {
        appFont.setPointSize(pointSize);
    }
    
    return appFont;
}
