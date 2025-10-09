#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <QDebug>
#include <QLoggingCategory>
#include <QString>
#include "prj_config.h"
#include <QFont>
#include "hash.h"
#include "lz4.h"
#include "crc.h"


Q_DECLARE_LOGGING_CATEGORY(debugCategory)

enum {
    THREAD_ID_UPDATE_UI = 0,
    THREAD_ID_UPDATE_FW,
#if defined(MODULE_USB3)
    THREAD_ID_USB3,
#endif

    THREAD_ID_NUM
};

// #define __DEBUG

#ifdef __DEBUG
#define LOG_DEBUG(format, ...) do { \
printf("file: %s, func: %s, line: %d, " format, __FILE__, __func__, __LINE__, ##__VA_ARGS__); \
} while(0);
#else
#define LOG_DEBUG(format, ...) do {;}while(0);
#endif

#define ntoh16(x) hton16(x)
#define ntoh32(x) hton32(x)

extern int g_regCnt; /* The external variable declares, the number of regs */

class CustomDebug {
public:
    CustomDebug(bool noquote = false, bool nospace = false);

    template<typename T>
    CustomDebug& operator<<(const T& value) {
        m_stream << value;
        return *this;
    }

    ~CustomDebug() = default;

private:
    QDebug m_stream;
};

// Refactoring of the qDebug function
CustomDebug debugNoQuoteNoSpace();
CustomDebug debugNoSpace();
CustomDebug debugNoQuote();

uint16_t hton16(uint16_t host);
uint32_t hton32(uint32_t host);
void PrintBuf(const char *data, int size);
void PrintBuf(const char *data, int size, const QString &title);
void SaveDataToFile(const char *data, int size, QString &file);
void DelayMs(int ms);

// Font management function
QFont getApplicationFont(int pointSize = -1);
QFont getApplicationFont(const QString &family, int pointSize = -1);

#endif // COMMON_H
