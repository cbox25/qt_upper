#ifndef UART_H   // The underlying layer of the serial communication protocol
#define UART_H

#include <stdint.h>
#include <QSerialPort>     // Reference the serial data QT library

enum {
    FIRMWARE_UPDATE 	    = 0x01,// firmware upgrade
    READ_WRITE_REG 		    = 0x02,// Read and write registers
    READ_MEMMORY_DATA       = 0x03,// Read memory data
    HANDSHAKE			    = 0x04,// Handshake/connection request
    CMD                     = 0X05,// Upper computer control command
    WRITE_XML // The XML writing command has not been used yet
};

enum {
    RESPONSE_FIRMWARE_UPDATE    = 0x80,// Firmware upgrade response
    RESPONSE_READ_WRITE_REG     = 0x81,// Read/write register response
    RESPONSE_READ_MEMMORY_DATA  = 0x82,// Read the memory data response
    RESPONSE_HANDSHAKE		    = 0x83,// Handshake response
    RESPONSE_CMD                = 0x84,// The response of the upper computer control command
    RESPONSE_WRITE_XML // The XML command response for writing has not been used yet
};

enum {
    HANDSHAKE_PC_REQUEST_LINK   = 0x01, // The PC requests to establish a connection
    HANDSHAKE_PC_REQUEST_RELINK = 0x02 // The PC requests to reconnect
};

enum {
    CMD_SAVE_ARGS = 0x01, // save args
    CMD_SET_BAUDRATE = 0x02, // Set the baud rate
    CMD_FFC_CALIBRATION = 0x03,
    CMD_SHEET_WRITE = 0x04,
    CMD_SHEET_READ = 0X05
};

enum {
    BAUD_9600_VALUE = 0, // Set the serial port baud rate value
    BAUD_115200_VALUE,
    BAUD_921600_VALUE,
    BAUD_230400_VALUE,
    BAUD_460800_VALUE
};

enum {
    COMPRESS_TYPE_NONE = 0, // Uncompress
    COMPRESS_TYPE_LZ4 = 1 // LZ4 compress
};

enum { // 升级文件类型
    ONLINE_FILE = 0x00, // online file
    XML_FILE = 0x01 // xml file
};

#define SUCCESS  (uint8_t)0x00

#define SHEET_SIZE_MAX    ((uint16_t)(1024 * 8)) // The maximum size of the CSV table

#define UART_PACKET_DATA_LEN	512 // Set the constant length of the data segment
#define UART_PACKET_LEN_MAX		524 /* 2B head + 1B type + 1B compress type + 2B reserved + 2B serial num + 2B data len + 512B data + 2B crc16 */
#define UART_PACKET_HEAD	((uint16_t) 0xEB90) // packet header

#define LOOPBACK_FORMAT "loopback: %s\r\n" // echo
#define LOOPBACK_FORMAT_LEN strlen(LOOPBACK_FORMAT) // lookback len
#define MAX_READ_SIZE 235 // A maximum of 235 bytes can be received at a time
#define MAX_LOOPBACK_SIZE MAX_READ_SIZE + LOOPBACK_FORMAT_LEN // Ensure not to cross the line

/* for read and write register */
#define OPERA_REG_READ   0x01// Read the register instruction code
#define OPERA_REG_WRITE  0x02// Write the register instruction code
#define OPERA_REG_DATA_LEN_MAX  503 /* 512 - 1 - 4 - 4 */

/* register address of operating */
#define ALG_REG_BASE_ADDR 0x44A20000 // The base address of the read and write register

typedef struct __attribute__((__packed__)){
    uint16_t head; // header
    uint8_t type; // packet type
    uint8_t compressType; // Whether to compress
    uint8_t reserved[2]; // reserved
    uint16_t serial_num; // serial number
    uint16_t data_len; // The array size of the data segment
    uint8_t data[UART_PACKET_DATA_LEN]; // data segment
    uint16_t crc16; // check bit
} uart_packet_t;

struct uart_device {
    char filename[32]; // Serial port device path
    int rate; // Baud rate value
    int fd; // file descriptor
    struct termios *tty; // Point to terminal configuration
};

typedef struct __attribute__((__packed__)){// PC -> The device register requests the payload
    uint8_t op; /* read: 0x01, write: 0x02 */ // operation code
    uint32_t reg_start_addr; // The starting address of the device register
    uint32_t len; // operate len
    uint8_t data[OPERA_REG_DATA_LEN_MAX]; // data
} opera_reg_t;

typedef struct __attribute__((__packed__)){ // device -> PC device register responds to the payload
    uint8_t op; /* read: 0x01, write: 0x02 */ // operation code
    uint32_t reg_start_addr; // The address of the starting register used during the request
    uint8_t rw_status; // Represents the status of the read and write results
    uint32_t err_code; // Error code, indicating what type of error occurred
    uint32_t len; // Return the length of the data
    uint8_t data[OPERA_REG_DATA_LEN_MAX]; // data

} opera_reg_reply_t;

typedef struct { // addr, value
    uint32_t addr;
    uint32_t value;
} reg_t;

extern QSerialPort g_serialPort; // Declare an external class
uint32_t hton32(uint32_t host); // 32-bit network byte order processing
uint16_t hton16(uint16_t host); // 16-bit network byte order processing

int SendCmdSheetWrite(QSerialPort *dev, uint16_t pktNum); // cmd sheet write
int ParseResponseCmdSheetWriteAck(QSerialPort *dev);
int SendCmdSheetRead(QSerialPort *dev, uint16_t *pktNum); // cmd sheet read

void ClearUart(QSerialPort *dev);
bool OpenSerialPort(QSerialPort *serialPort);
void UartWriteBuf(QSerialPort *serialPort, const char *buf, int len);
int UartReadBuf(QSerialPort *serialPort, const char *buf);
int UartReadBuf(QSerialPort *serialPort, char *buf, int maxSize, int timeoutMs);
void ClearUart(QSerialPort *dev);
bool ConfigureSerialPort(QSerialPort *serialPort, int baudRate, const QString &text);
int MakeUartPacket(uint8_t op, uint32_t addr, uint32_t value, int readLen, uint8_t *uartBuf);
int SendCmdSaveArgs(QSerialPort *dev, uint8_t *data, int dataLen);

#endif // UART_H
