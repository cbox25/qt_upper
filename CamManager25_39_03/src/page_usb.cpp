#include <QWidget>
#include <QApplication>
#include <QObject>
#include <QDebug>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QtCore>
#include <QImage>
#include <QVideoWidget>
#include "common.h"
#include "multi_threads.h"
#include "crc.h"
#include "mainwindow.h"
#include "qboxlayout.h"
#include "usb.h"
#include "page_usb.h"

#if defined(MODULE_USB3)
UsbWidgets g_usbWidgets;

AckCbPkt g_ackCbUsb[THREAD_SUBTYPE_USB_ACK_LAST] = {
    {THREAD_SUBTYPE_USB_ACK_FIRST, nullptr},
    {THREAD_SUBTYPE_USB_ACK_IMAGE, UpdateImage}
};

UsbWidgets::UsbWidgets()
{
    // Use the global font of the application instead of hard-coded fonts
    font = getApplicationFont(10);
    imgWidth = 2448;
    imgHeight = 2048;
}

void UsbWidgets::UsbStartStopBtnClickedEvent(void)
{
    // send data to thread usb backend
    QByteArray buf(sizeof(ThreadDataPkt), 0);
    QByteArray src(1, 0);
    ThreadManager *thread = g_threadList->Get(THREAD_ID_USB3);

    // FIXME: If the button is clicked too rapidly, it may corrupt the application.
    if (!isStarted) {
        MakeThreadDataPkt(THREAD_TYPE_CMD_USB3, THREAD_SUBTYPE_USB_CMD_START, src, buf);
        thread->EnqueueData(buf);
        isStarted = true;
    } else {
        UsbStop(nullptr);
        isStarted = false;
    }
    if (startStopBtn) startStopBtn->setText(isStarted ? "停止" : "开始");
}

void UsbWidgets::CreateStartStopBtn(QWidget *parent)
{
    startStopBtn = new QPushButton("开始", parent);
    startStopBtn->setFont(font);
    startStopBtn->setStyleSheet("QPushButton { background-color: #000; color: #FFF; text-align: left;} ");
    // Add the button to the layout of the parent widget
    QVBoxLayout *layout = qobject_cast<QVBoxLayout*>(parent->layout());
    if (!layout) {
        layout = new QVBoxLayout(parent);
        layout->setContentsMargins(0, 0, 0, 0);
        parent->setLayout(layout);
    }
    layout->addWidget(startStopBtn, 0, Qt::AlignTop);
    QObject::connect(startStopBtn, &QPushButton::clicked, this, &UsbWidgets::UsbStartStopBtnClickedEvent);
}

void UsbWidgets::CreateImageWidthEdit(QWidget *parent)
{
    imgWidthEdit = new QLineEdit(parent);
    imgWidthEdit->setGeometry(5, 45, 50, 25);
    imgWidthEdit->setFont(font);
}

void UsbWidgets::CreateImageHeightEdit(QWidget *parent)
{
    imgHeightEdit = new QLineEdit(parent);
    imgHeightEdit->setGeometry(5, 80, 50, 25);
    imgHeightEdit->setFont(font);
}

void CalcFrameRate(void)
{
    // get current time
    qint64 now = QDateTime::currentDateTime().toMSecsSinceEpoch();
    static qint64 start = 0;

    // print current time
    if (now - start >= 1000) {
        qDebug() << "frame rate:" << g_usbWidgets.frameRate << "lost:" << g_usbWidgets.lostPkt << "discard:" << g_usbWidgets.discardPkt;
        g_usbWidgets.frameRate = 0;
        start = now;
    }
}

void ShowUsbImageData(QByteArray &pktData, QLabel *imageLabel)
{
    QByteArray imgData(pktData.size(), 0);
    UsbCommProtocolPkt *pkt = nullptr;
    uint8_t *data = (uint8_t *)pktData.data();
    uint8_t *img = (uint8_t *)imgData.data();
    // uint8_t dataType = 0;
    static uint16_t totalPktNum = 0;
    static uint16_t seqNum = 0;
    static int showImg = 0;
    static int lastSeqNum = 0;
    uint16_t pktSeqNum = 0;
    uint32_t pktTotalNum = 0;

    if (imageLabel == nullptr) {
        return;
    }

    g_usbWidgets.discardPkt = 0;
#if 1
    static QTimer *timer = new QTimer();

    QObject::connect(timer, &QTimer::timeout, []() {
        CalcFrameRate();
    });
    timer->start();
#endif

    pktTotalNum = pktData.size() / sizeof(UsbCommProtocolPkt);

    for (uint32_t pktCnt = 0; pktCnt < pktTotalNum; pktCnt ++) {
        pkt = (UsbCommProtocolPkt *)(data + pktCnt * sizeof(UsbCommProtocolPkt));

        if (pkt->head != hton32(USB_COMM_HEAD) ||
            hton16(pkt->dataLen) > USB_COMM_DATA_LEN_MAX) {
            qDebug() << "packet head error:" << pkt->head;
            seqNum = 0;
            return;
            // continue;
        }

        pktSeqNum = hton16(pkt->seqNum);

        if (pktCnt == 0) {
            if (pktSeqNum >= lastSeqNum)
                g_usbWidgets.lostPkt = pktSeqNum - lastSeqNum;
            else
                g_usbWidgets.lostPkt = pktSeqNum + 255 - lastSeqNum;
        }

        if (pktSeqNum == 0xFFFF) {
            // qDebug() << "ignore this packet";
            seqNum = 0;
            // g_discardPkt ++;
            continue; // ignore the packet
        }

        if (pktSeqNum == 0) {
            // qDebug() << "recv first pkt";
            // receive the first pakcet, then parse the data type / total packet num
            if (pkt->dataType > USB_COMM_DATA_TYPE_YUV422) {
                qDebug() << "packet data type error:" << pkt->dataType;
                continue;
            }
            // dataType = pkt->dataType;
            totalPktNum = hton16(pkt->totalPktNum);
            seqNum = 0;
        }

        if (pktSeqNum != seqNum) {
            qDebug() << "usb image data seqNum error:" << pktSeqNum << seqNum;
            seqNum = 0; // wait for next start packet of frame
            g_usbWidgets.discardPkt ++;
            continue;
        }

        if (CheckSum(pkt->data, USB_COMM_DATA_LEN_MAX, pkt->chkSum) != 0) {
            qDebug() << "packet check sum error:" << pkt->chkSum;
            seqNum = 0;
            continue;
        }

        memcpy(img + seqNum * USB_COMM_DATA_LEN_MAX, pkt->data, USB_COMM_DATA_LEN_MAX);
        seqNum ++;
        if (seqNum == totalPktNum) {
            if (hton16(pkt->imgHeight) > USB_IMAGE_HEIGHT_MAX ||
                hton16(pkt->imgWidth) > USB_IMAGE_WIDTH_MAX) {
                qDebug() << "packet image height width error:" << pkt->imgHeight << pkt->imgWidth;
                continue;
            }

            if ((showImg & 0x1) == 0) {
                // update image of label, create QImage to load image data
                QImage image((uint8_t *)imgData.data(), hton16(pkt->imgWidth), hton16(pkt->imgHeight), QImage::Format_Grayscale8);
                imageLabel->setPixmap(QPixmap::fromImage(image));
                showImg++;
            }

            g_usbWidgets.frameRate ++;
            seqNum = 0;
        }
    }

    lastSeqNum = seqNum;
    g_usbWidgets.discardPkt += seqNum;
}

void ReadImage(QByteArray &data, int sharedMemIdx)
{
    QSharedMemory *sharedMem = nullptr;
    QString key;
    char *to = nullptr;
    const char *from = nullptr;
    uint32_t size = 0;

    key = g_usbOp.GetSharedMemKey(sharedMemIdx);
    sharedMem = new QSharedMemory(key);

    // try to connect to shared memory
    if (!sharedMem->attach(QSharedMemory::ReadOnly)) {
        qDebug() << "connect to shared memory failed" << sharedMem->errorString();
        return;
    }

    // lock the current shared memory
    if (!sharedMem->lock()) {
        qDebug() << "lock shared memory failed" << sharedMem->errorString();
        return;
    }

    // get the shared memory address
    to = data.data();
    from = static_cast<const char *>(sharedMem->constData());
    size = (uint32_t)data.size() < g_usbOp.GetUsbSharedMemSize() ? data.size() : g_usbOp.GetUsbSharedMemSize();
    memcpy(to, from, size);

    // unlock shared memory
    if (!sharedMem->unlock()) {
        qDebug() << "unlock shared memory failed" << sharedMem->errorString();
        return;
    }

    delete(sharedMem);
}

void UsbWidgets::CreateImageLabel(QWidget *parent)
{
    // 创建QScrollArea和QLabel
    QScrollArea *scrollArea = new QScrollArea(parent);
    QString imagePath = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/Aimuon1.jpg");
    imageLabel = new QLabel;
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    scrollArea->setWidget(imageLabel);
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet(
        "QScrollArea { border: none; background: transparent; }"
        "QScrollArea > QWidget > QWidget { background: transparent; }"
        "QScrollArea QWidget { background: transparent; }"
        "QScrollArea > QViewport { background: transparent; border: none; }"
        "QScrollBar:vertical { background: transparent; border: none; }"
        "QScrollBar:horizontal { background: transparent; border: none; }"
    );

    // 将scrollArea添加到父widget的布局中
    QVBoxLayout *layout = qobject_cast<QVBoxLayout*>(parent->layout());
    if (!layout) {
        layout = new QVBoxLayout(parent);
        layout->setContentsMargins(0, 0, 0, 0);
        parent->setLayout(layout);
    }
    layout->addWidget(scrollArea);
}

void UsbWidgets::RefreshImage(void *param)
{
    QByteArray data;
    uint8_t dataType = 0;
    int sharedMemIdx = 0;
    ThreadDataPkt *buf = NULL;

    buf = (ThreadDataPkt *)param;

    dataType = *(uint8_t *)buf->data;
    sharedMemIdx = *(int *)(buf->data + 1);
    sharedMemIdx = hton32(sharedMemIdx);

    // read image from shared memory
    uint32_t pktNumPerFrame = 2473;
    uint32_t size = sizeof(UsbCommProtocolPkt) * pktNumPerFrame * 20;
    data.resize(size);
    ReadImage(data, sharedMemIdx);

    ShowUsbImageData(data, imageLabel);
}

void UpdateImage(void *param)
{
    g_usbWidgets.RefreshImage(param);
}

void UpdateUsbPageImage(void *param)
{
    uint8_t subType = 0;
    ThreadDataPkt *buf = NULL;

    if (param == NULL) {
        return;
    }

    buf = (ThreadDataPkt *)param;

    // check subtype if necessary
    subType = buf->subType;
    if (subType > THREAD_SUBTYPE_USB_ACK_FIRST && subType < THREAD_SUBTYPE_USB_ACK_LAST) {
        g_ackCbUsb[subType].cb(param);
    }
}

void CreateUsbWidgets(QWidget *pageUsb)
{
    if (pageUsb == NULL) {
        return;
    }
    // 主布局：水平分布
    QHBoxLayout *mainLayout = new QHBoxLayout(pageUsb);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    pageUsb->setLayout(mainLayout);

    // 左侧按钮区
    QWidget *buttonPanel = new QWidget(pageUsb);
    QVBoxLayout *btnLayout = new QVBoxLayout(buttonPanel);
    btnLayout->setContentsMargins(0, 0, 0, 10);
    btnLayout->setSpacing(20);
    buttonPanel->setLayout(btnLayout);
    buttonPanel->setStyleSheet(
        "QWidget { background: #222; height: 40px; width:75; font-size: 16px; color: #FFF; padding-left: 5px;}");
    // 右侧图像区
    QWidget *imagePanel = new QWidget(pageUsb);
    QVBoxLayout *imageLayout = new QVBoxLayout(imagePanel);
    imageLayout->setContentsMargins(0, 0, 0, 0);
    imageLayout->setSpacing(0);
    imagePanel->setLayout(imageLayout);
    imagePanel->setStyleSheet("QWidget { border: none; background: transparent; }");

    // 添加到主布局
    mainLayout->addWidget(buttonPanel, 0); // 左侧按钮区
    mainLayout->addWidget(imagePanel, 1);  // 右侧图像区，拉伸填充

    // 移除按钮创建，只保留图像显示区
    // g_usbWidgets.CreateStartStopBtn(buttonPanel);

    // 创建图像显示区并放入右侧
    g_usbWidgets.CreateImageLabel(imagePanel);
}
#endif
