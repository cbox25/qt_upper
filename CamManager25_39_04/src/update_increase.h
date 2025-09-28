#ifndef UPDATE_INCREASE_H   // 该文件作用于嵌入式系统固件更新的实现
#define UPDATE_INCREASE_H

#include <QSerialPort> // 负责串口通信的QT库
#include <QProgressBar>// 负责进度条的现实

#define HANDSHAKE_TIMEOUT_MS   5000 /* 5000ms */ // 握手超时时间设置为5秒
#define BLOCK_SIZE  4096// 块大小设为4096字节
#define PACKET_DATA_SIZE  512// 数据包数据部分大小设为512字节
#define UART_PACKET_LEN_MAX  524 /* 2B head + 1B type + 1B ziptype + 2B reserved + 2B serial num + 2B data len + 512B data + 2B crc16 */ // 最大的数据包长度，更新协议后为524
#define UART_PACKET_HEAD    ((uint16_t) 0xEB90)// 定义包头

/* for read and write register */
#define OPERA_REG_READ   0x01 // 读标志
#define OPERA_REG_WRITE  0x02 // 写标志
#define OPERA_REG_DATA_LEN_MAX  503 /* 512 - 1 - 4 - 4 */ // 最大可操作数据长度 1为操作类型，4为寄存器地址，4为操作长度

/* update type and compress type */
enum {// 更新类型的枚举 分别表示完整更新和增量更新
    UPDATE_TYPE_FULL = 0x01,
    UPDATE_TYPE_INCREASE = 0x02
};

/* register address of operating */
#define ALG_REG_BASE_ADDR 0x44A20000// 寄存器基地址

#ifdef _WIN32 // 判断使用的操作系统版本
#pragma pack(push, 1)// 字节对齐
typedef struct {//
    uint16_t head;// 包头
    uint16_t seqNum;// 序列号
    uint16_t addrOffset;// 地址偏移量
    uint16_t dataLen;// 数据长度
    uint16_t data[PACKET_DATA_SIZE];// 存放数据的数组
    uint16_t crc;// 校验
} PatchPacket;

typedef struct {
    uint16_t head;// 包头
    uint8_t type;// 包类型
    uint8_t compressType;// 压缩种类
    uint8_t reserved[2];// 预留位
    uint16_t seqNum;// 序列号
    uint16_t dataLen;// 数据长度
    uint8_t data[PACKET_DATA_SIZE];// 存放数据的数组
    uint16_t crc;// 校验
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
    uint8_t compressType;// 压缩种类
    uint16_t Reserved;// 预留值
    uint16_t seqNum;
    uint16_t dataLen;
    uint8_t data[PACKET_DATA_SIZE];
    uint16_t crc;
} UartPacket;
#endif

void UpdateFwTask(void);// 更新固件的主任务函数
void CmdUpdate(void *param);// 处理更新命令函数

int GetUpdateFileSize(const char* filePath);
int IncreaseUpdate(QSerialPort *hSerial, const char *filePath, uint8_t updateFile, QProgressBar *progressBar);// 执行增量更新
int FullUpdate(QSerialPort *dev, const char *filePath, QProgressBar *progressBar);// 执行全量更新
int FirmwareUpdate(QSerialPort *dev, const char *filePath, QProgressBar *progressBar);// 固件更新的一个封装函数
void SyncUpdateRet(const QString &str);// 同步更新返回信息

#endif // UPDATE_INCREASE_H
