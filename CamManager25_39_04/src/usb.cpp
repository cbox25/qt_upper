#include <iostream>
#include <QSharedMemory>
#include <QMutex>
#include "libusb.h"
#include "usb.h"
#include "common.h"
#include "multi_threads.h"
#include "crc.h"
#include "mainwindow.h"
#include "../Qlogger/QLogger.h"
#include "../Qlogger/QLoggerLevel.h"
#include "../Qlogger/QLoggerManager.h"
#include "../Qlogger/QLoggerProtocol.h"
#include "../Qlogger/QLoggerWriter.h"

#if defined(MODULE_USB3)
UsbOperate g_usbOp;

CmdCbPkt g_usbCmdCbPkt[THREAD_SUBTYPE_USB_CMD_LAST] = {
    {THREAD_SUBTYPE_USB_CMD_FIRST, nullptr},
    {THREAD_SUBTYPE_USB_CMD_START, UsbStart},
    {THREAD_SUBTYPE_USB_CMD_STOP, UsbStop},
    };

UsbOperate::UsbOperate(void)
{
    sharedMem[0].setKey(sharedMemKey[0]);
    sharedMem[0].create(sharedMemSize, QSharedMemory::ReadWrite);

    sharedMem[1].setKey(sharedMemKey[1]);
    sharedMem[1].create(sharedMemSize, QSharedMemory::ReadWrite);
}

UsbOperate::~UsbOperate(void)
{
    sharedMem[0].detach();
    sharedMem[1].detach();
}

void UsbOperate::SetUsbAcqFlag(bool flag)
{
    mutexAckFlag.lock();
    usbAcqFlag = flag;
    mutexAckFlag.unlock();
}

bool UsbOperate::GetUsbAcqFlag(void)
{
    return usbAcqFlag;
}

void UsbOperate::SetUsbSharedMemSize(uint32_t size)
{
    sharedMemSize = size;
}

uint32_t UsbOperate::GetUsbSharedMemSize(void)
{
    return sharedMemSize;
}

void UsbOperate::ReverseSharedMemIdx(void)
{
    if (sharedMemIdx < 0 || sharedMemIdx > 1) {
        sharedMemIdx = 0;
    } else {
        sharedMemIdx = 1 - sharedMemIdx;
    }
}

int UsbOperate::GetSharedMemIdx(void)
{
    return sharedMemIdx;
}

QSharedMemory *UsbOperate::GetSharedMem(int idx)
{
    return &sharedMem[idx];
}

QSharedMemory *UsbOperate::GetSharedMem(void)
{
    return &sharedMem[sharedMemIdx];
}

QString UsbOperate::GetSharedMemKey(int idx)
{
    return sharedMemKey[idx];
}

QString UsbOperate::GetSharedMemKey(void)
{
    return sharedMemKey[sharedMemIdx];
}

void UsbOperate::SetImgSize(uint32_t size)
{
    imgSize = size;
}
uint32_t UsbOperate::GetImgSize()
{
    return imgSize;
}

void UsbOperate::CalcImgPktNum(uint32_t w, uint32_t h, uint8_t bytePerPixel)
{
    imgPktNum = (w * h * bytePerPixel + USB_COMM_DATA_LEN_MAX - 1) / USB_COMM_DATA_LEN_MAX;
}

static void print_devs(libusb_device **devs)
{
    libusb_device *dev;
    int i = 0, j = 0;
    uint8_t path[8];

    while ((dev = devs[i++]) != NULL) {
        struct libusb_device_descriptor desc;
        int r = libusb_get_device_descriptor(dev, &desc);
        if (r < 0) {
            qDebug() << "failed to get device descriptor";
            QLog_Error("CamManager", "Failed to get device descriptor");
            return;
        }

        QString idVendor = "0x" + QString::number(desc.idVendor, 16);
        QString idProduct = "0x" + QString::number(desc.idProduct, 16);
        QString busNum = "0x" + QString::number(libusb_get_bus_number(dev), 16);
        QString addr = "0x" + QString::number(libusb_get_device_address(dev), 16);

        qDebug() << "usb info," << "VID:" << idVendor << "PID:" << idProduct << "bus:" << busNum << "dev:" << addr;
        QLog_Debug("CamManager", QString(" usb info, VID: %1 ,PID: %2  bus: %3  dev: %4 ")
                                     .arg(idVendor).arg(idProduct).arg(busNum).arg(addr));

        r = libusb_get_port_numbers(dev, path, sizeof(path));
        if (r > 0) {
            qDebug() << "path 0: " << path[0];
            QLog_Debug("CamManager", QString("path 0: %1").arg(path[0]));
            for (j = 1; j < r; j++)
                qDebug() << "path j: " << path[j];
            QLog_Debug("CamManager", QString("path 0: %1").arg(path[j]));
        }
    }
}

int ListUsb(void)
{
    libusb_device **devs;
    int r;
    ssize_t cnt;

    r = libusb_init_context(/*ctx=*/NULL, /*options=*/NULL, /*num_options=*/0);
    if (r < 0)
        return r;

    cnt = libusb_get_device_list(NULL, &devs);
    if (cnt < 0){
        libusb_exit(NULL);
        return (int) cnt;
    }
    print_devs(devs);

    // free and exit
    libusb_free_device_list(devs, 1);
    libusb_exit(NULL);
    return 0;
}

void NotifyToRefreshImage(uint8_t type, uint32_t sharedMemIdx)
{
    ThreadManager *thread = NULL;
    QByteArray queueData(sizeof(ThreadDataPkt), 0);
    QByteArray srcArray;

    srcArray.append(reinterpret_cast<char *>(&type), sizeof(type));
    srcArray.append(reinterpret_cast<char *>(&sharedMemIdx), sizeof(sharedMemIdx));

    thread = g_threadList->Get(THREAD_ID_UPDATE_UI); // send data to queue of update UI thread
    MakeThreadDataPkt(THREAD_TYPE_ACK_USB3, THREAD_SUBTYPE_USB_ACK_IMAGE, srcArray, queueData);
    thread->EnqueueData(queueData);

    EmitUpdateUiSignal();
}

int InitLibUsbInterface(libusb_context **context, libusb_device_handle **device_handle)
{
    //qDebug() << "Enter InitLibUsbInterface";

    int r = 0;
    // Initialize libusb
    r = libusb_init(context);
    if (r < 0) {
        qDebug() << "Failed to initialize libusb";
        QLog_Error("CamManager", "Failed to initialize libusb");
        return -1;
    }

    // Set debug level
    libusb_set_debug(*context, 3);

    // Open the device
    *device_handle = libusb_open_device_with_vid_pid(*context, 0x706D, 0x807C);
    if (*device_handle == NULL) {
        qDebug() << "Failed to open usb device";
        QLog_Error("CamManager", "Failed to open usb device");
        libusb_exit(*context);
        return -2;
    }

    // Detach kernel driver if active
    if (libusb_kernel_driver_active(*device_handle, 0) == 1) {
        if (libusb_detach_kernel_driver(*device_handle, 0) == 0) {
            qDebug() << "Kernel driver detached";
        } else {
            qDebug() << "Failed to detach kernel driver";
            QLog_Error("CamManager", "Failed to detach kernel driver");
            libusb_close(*device_handle);
            libusb_exit(*context);
            return -3;
        }
    }

    // Claim the interface
    if (libusb_claim_interface(*device_handle, 0) < 0) {
        qDebug() << "Failed to claim interface";
        QLog_Error("CamManager", "Failed to claim interface");
        libusb_close(*device_handle);
        libusb_exit(*context);
        return -4;
    }

    return 0;
}

void UsbOperate::UsbAcqStart(void *param)
{
    //qDebug() << "Enter UsbAcqStart";

    libusb_context* context = NULL; // a libusb session
    libusb_device_handle* device_handle = NULL; // a device handle
    uint8_t *data = NULL; // buffer to store the data read from the device
    int transferred = 0; // number of bytes transferred
    int timeout = 0; // timeout in milliseconds
    QSharedMemory *currentMem = nullptr;
    uint8_t dataType = 0;
    int r = 0;
    const int pktNumPerFrame = 2473;
    uint32_t dataSize = 0;

    dataSize = sizeof(UsbCommProtocolPkt) * pktNumPerFrame * 20;
    if ((data = (uint8_t *)malloc(dataSize)) == NULL) {
        qDebug() << "usb malloc space failed";
        QLog_Error("CamManager", "usb malloc space failed");
        return;
    }

    if ((r = InitLibUsbInterface(&context, &device_handle)) < 0) {
        free(data);
        return;
    }

    SetUsbAcqFlag(true);
    while(GetUsbAcqFlag()) {
        memset(data, 0x00, dataSize);
        currentMem = &sharedMem[sharedMemIdx];
        char *to = static_cast<char*>(currentMem->data()); // get shared memory address
#if 0
        r = libusb_bulk_transfer(device_handle, USB_DATA_END_POINT, data,
                                 dataSize, &transferred, 0);
#else \
    // r = libusb_interrupt_transfer(device_handle, USB_DATA_END_POINT, \
    //                               data, dataSize, &transferred, timeout);
        r = libusb_interrupt_transfer(device_handle, USB_DATA_END_POINT,
                                      (unsigned char *)to, dataSize, &transferred, timeout);
#endif

        if (r == 0) {
#if 0 \
    // write data to shared memory
            currentMem->lock();
            char *to = static_cast<char*>(currentMem->data()); // get shared memory address
            memcpy(to, data, dataSize); // write data to shared memory
            currentMem->unlock();
#endif

            NotifyToRefreshImage(dataType, hton32(sharedMemIdx)); // send big endian data
            ReverseSharedMemIdx(); // for ping pang operation
        } else {
            qDebug() << "Failed to read data, ret:" << r;
            QLog_Error("CamManager", QString("Failed to read data, ret: %1").arg(r));
            continue;
        }
    }

    free(data);
    // Release the interface and close the device
    libusb_release_interface(device_handle, 0);
    libusb_close(device_handle);
    libusb_exit(context);
}

void UsbOperate::UsbAcqStop(void *unused)
{
    SetUsbAcqFlag(false);
}

void UsbStart(void *param)
{
    //qDebug() << "Enter UsbStart";

    g_usbOp.UsbAcqStart(param);
}

void UsbStop(void *unused)
{
    g_usbOp.UsbAcqStop(unused);
}

void UsbTask(void *param)
{
    ThreadDataPkt *buf = (ThreadDataPkt *)param;
    if (!buf) {
        qDebug() << "UsbTask: param is null";
        QLog_Error("CamManager", "UsbTask: param is null");
        return;
    }
    uint16_t dataLen = ntoh16(buf->dataLen);
    if (dataLen > THREAD_PACKET_DATA_LEN) {
        qDebug() << "dataLen too large:" << dataLen;
        QLog_Error("CamManager", QString("dataLen too large: %1").arg(dataLen));
        return;
    }
    dataLen += 8;
    uint16_t crc = ntoh16(buf->crc);
    //qDebug() << "UsbTask: head=" << buf->head
    //         << "type=" << buf->type
    //         << "subType=" << buf->subType
    //         << "dataLen=" << dataLen - 8
    //         << "crc=" << crc;
    //QLog_Debug("CamManager", QString("UsbTask: head= %1, type= %2, subType= %3, dataLen= %4, crc= %5")
    //                             .arg(buf->head).arg(buf->type).arg(buf->subType).arg(dataLen - 8));
    PrintBuf(reinterpret_cast<const char*>(buf), dataLen);
    if (! check_crc16((uint8_t *)buf, dataLen, crc)) {
        qDebug() << "crc check failed";
        QLog_Error("CamManager", "crc check failed");
        return;
    }
    if (buf->head != hton16(THREAD_PACKET_HEAD)) {
        qDebug() << "head check failed, buf->head=" << buf->head << "expect" << hton16(THREAD_PACKET_HEAD);
        QLog_Error("CamManager", QString("head check failed, buf->head= %1, expect: %2")
                                     .arg(buf->head).arg(hton16(THREAD_PACKET_HEAD)));
        return;
    }
    if (buf->type != THREAD_TYPE_CMD_USB3) {
        qDebug() << "type check failed, buf->type=" << buf->type << "expect" << THREAD_TYPE_CMD_USB3;
        QLog_Error("CamManager", QString("type check failed, buf->type= %1, expect: %2")
                                     .arg(buf->type).arg(THREAD_TYPE_CMD_USB3));
        return;
    }
    //qDebug() << "before subType check, buf->subType =" << buf->subType;
    if (buf->subType > THREAD_SUBTYPE_USB_CMD_FIRST && buf->subType < THREAD_SUBTYPE_USB_CMD_LAST) {
        //qDebug() << "buf->subType =" << buf->subType;
        //qDebug() << "g_usbCmdCbPkt[buf->subType].cb =" << (void*)g_usbCmdCbPkt[buf->subType].cb;
        if (g_usbCmdCbPkt[buf->subType].cb != nullptr) {
            g_usbCmdCbPkt[buf->subType].cb(buf);
        }
    } else {
        qDebug() << "subType check not passed, buf->subType =" << buf->subType;
        QLog_Error("CamManager", QString("subType check not passed, buf->subType = %1").arg(buf->subType));
    }
}

#endif
