#ifndef USB_H
#define USB_H

#include "prj_config.h"
#include <QMutex>
#include <QSharedMemory>
#include <stdint.h>

#if defined(MODULE_USB3)

#define USB_IMAGE_HEIGHT_MAX (4096)
#define USB_IMAGE_WIDTH_MAX (4096)
#define USB_DATA_END_POINT  (0x87)
#define USB_IMAGE_SIZE_MAX (16777216) // 4096 * 4096
#define USB_COMM_DATA_LEN_MAX (2028)
#define USB_COMM_HEAD  (0x55aa9966)

enum {
    USB_COMM_DATA_TYPE_MONO8 = 0x00,
    USB_COMM_DATA_TYPE_MONO10,
    USB_COMM_DATA_TYPE_MONO12,
    USB_COMM_DATA_TYPE_RGB8,
    USB_COMM_DATA_TYPE_RGB10,
    USB_COMM_DATA_TYPE_RGB12,
    USB_COMM_DATA_TYPE_BAYER8,
    USB_COMM_DATA_TYPE_BAYER10,
    USB_COMM_DATA_TYPE_BAYER12,
    USB_COMM_DATA_TYPE_YUV444,
    USB_COMM_DATA_TYPE_YUV422
};

#ifdef __WIN32
#pragma pack(push, 1)
typedef struct {
    uint32_t head;
    uint8_t dataType;
    uint8_t reserve;
    uint16_t totalPktNum;
    uint16_t seqNum;
    uint16_t imgHeight;
    uint16_t imgWidth;
    uint16_t dataLen;
    uint8_t data[USB_COMM_DATA_LEN_MAX];
    uint32_t chkSum;
} UsbCommProtocolPkt;
#pragma pack(pop)
#else
typedef struct __attribute__((__packed__)){
    uint32_t head;
    uint8_t dataType;
    uint8_t reserve;
    uint16_t totalPktNum;
    uint16_t seqNum;
    uint16_t imgHeight;
    uint16_t imgWidth;
    uint16_t dataLen;
    uint8_t data[USB_COMM_DATA_LEN_MAX];
    uint32_t chkSum;
} UsbCommProtocolPkt;
#endif

class UsbOperate {
public:
    UsbOperate();
    ~UsbOperate();
    void SetUsbAcqFlag(bool flag);
    bool GetUsbAcqFlag(void);
    void UsbAcqStart(void *param);
    void UsbAcqStop(void *param);
    void SetUsbSharedMemSize(uint32_t size);
    uint32_t GetUsbSharedMemSize(void);
    void ReverseSharedMemIdx();
    int GetSharedMemIdx();
    QSharedMemory *GetSharedMem(int idx);
    QSharedMemory *GetSharedMem(void);
    QString GetSharedMemKey(int idx);
    QString GetSharedMemKey(void);
    void SetImgSize(uint32_t imgSize);
    uint32_t GetImgSize();
    void CalcImgPktNum(uint32_t w, uint32_t h, uint8_t bytePerPixel);

private:
    volatile bool usbAcqFlag = false;
    mutable QMutex mutexAckFlag;
    QSharedMemory sharedMem[2]; // ping pang
    mutable QMutex mutexSharedMem;
    int sharedMemIdx = 0;
    uint32_t sharedMemSize = USB_IMAGE_SIZE_MAX * 8; // default 4K * 4K * 1Byte * 4
    QString sharedMemKey[2] = {"usbSharedMem0", "usbSharedMem1"};
    uint32_t imgSize;
    uint32_t imgPktNum;
};

extern UsbOperate g_usbOp;

int ListUsb(void);
int ReadUsbData(void);
void UsbTask(void *param);
void UsbStart(void *param);
void UsbStop(void *param);

#endif // MODULE_USB3
#endif // USB_H
