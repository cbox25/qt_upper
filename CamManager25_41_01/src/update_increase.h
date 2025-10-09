#ifndef UPDATE_INCREASE_H   // This file is used for the implementation of firmware updates in embedded systems
#define UPDATE_INCREASE_H

#include <QSerialPort> // The QT library responsible for serial communication
#include <QProgressBar> // Be responsible for the reality of the progress bar

#define HANDSHAKE_TIMEOUT_MS   5000 /* 5000ms */ // The handshake timeout is set to 5 seconds
#define BLOCK_SIZE  4096 // Set the block size to 4096 bytes
#define PACKET_DATA_SIZE  512 // Set the size of the data part of the data packet to 512 bytes
#define UART_PACKET_LEN_MAX  524 /* 2B head + 1B type + 1B ziptype + 2B reserved + 2B serial num + 2B data len + 512B data + 2B crc16 */
#define UART_PACKET_HEAD    ((uint16_t) 0xEB90) // Define the header

/* for read and write register */
#define OPERA_REG_READ   0x01 // read
#define OPERA_REG_WRITE  0x02 // write
#define OPERA_REG_DATA_LEN_MAX  503 /* 512 - 1 - 4 - 4 */
/* The maximum operable data length, where 1 represents the operation type, 4 represents the register address, and 4 represents the operation length */

/* update type and compress type */
enum {
    UPDATE_TYPE_FULL = 0x01,
    UPDATE_TYPE_INCREASE = 0x02
};

/* register address of operating */
#define ALG_REG_BASE_ADDR 0x44A20000 // Register base address

#ifdef _WIN32 // Determine the version of the operating system being used
#pragma pack(push, 1) // 1-byte alignment
typedef struct {
    uint16_t head;
    uint16_t seqNum;
    uint16_t addrOffset; // Address offset
    uint16_t dataLen;
    uint16_t data[PACKET_DATA_SIZE];
    uint16_t crc;
} PatchPacket;

typedef struct {
    uint16_t head;
    uint8_t type;
    uint8_t compressType;
    uint8_t reserved[2];
    uint16_t seqNum;
    uint16_t dataLen;
    uint8_t data[PACKET_DATA_SIZE];
    uint16_t crc;
} UartPacket;
#pragma pack(pop)
#else
typedef struct __attribute__((__packed__)){
    uint16_t head;
    uint16_t seqNum;
    uint16_t addrOffset;
    uint16_t dataLen;
    uint16_t data[PACKET_DATA_SIZE];
    uint16_t crc;
} PatchPacket;

typedef struct __attribute__((__packed__)){
    uint16_t head;
    uint8_t type;
    uint8_t compressType;
    uint16_t Reserved;
    uint16_t seqNum;
    uint16_t dataLen;
    uint8_t data[PACKET_DATA_SIZE];
    uint16_t crc;
} UartPacket;
#endif

void UpdateFwTask(void); // The main task of updating the firmware
void CmdUpdate(void *param); // Handle update commands

int GetUpdateFileSize(const char* filePath);
int IncreaseUpdate(QSerialPort *hSerial, const char *filePath, uint8_t updateFile, QProgressBar *progressBar); // Perform incremental updates
int FullUpdate(QSerialPort *dev, const char *filePath, QProgressBar *progressBar); // Perform a full update
int FirmwareUpdate(QSerialPort *dev, const char *filePath, QProgressBar *progressBar); // firmware update
void SyncUpdateRet(const QString &str); // Synchronize and update the returned information

#endif // UPDATE_INCREASE_H
