#include <QDebug>
#include <QTabWidget>
#include "mainwindow.h"
#include "qevent.h"
#include "ui_mainwindow.h"
#include "../src/uart.h"
#include "../src/init_main_window.h"
#include "../src/multi_threads.h"
#include "../src/crc.h"
#include "../src/page_update.h"
#include "../src/page_usb.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    g_window = this; // 确保全局指针设置
    this->setWindowIcon(QIcon(":/logo.png"));
}

MainWindow::~MainWindow()
{
    qDebug() << "close mainwindow";
    if (g_serialPort.isOpen()) {
        qDebug() << "close serial port";
        g_serialPort.waitForBytesWritten(500);
        g_serialPort.close();
    }
    delete ui;
}

void MainWindow::resizeEvent(QResizeEvent *event) {

    QMainWindow::resizeEvent(event);
    emit resized();
}

void MainWindow::resizeStackedWidgetAndScrollArea(QStackedWidget *stackedWidget, QScrollArea *scrollArea)
{
    if (!stackedWidget || !scrollArea) {
        qDebug() << "Error: stackedWidget or scrollArea is null";
        return;
    }
    QWidget *regWidget = stackedWidget->widget(TAB_PAGE_REG);
    if (regWidget) {
        regWidget->resize(stackedWidget->size());
    } else {
        qDebug() << "Error: TAB_PAGE_REG widget not found";
    }
    scrollArea->resize(size().width(), size().height() - 50);
}

void EmitUpdateUiSignal(void)
{
    if (g_window) {
        qDebug() << "Emitting updateUi signal";
        emit g_window->updateUi();
    } else {
        qDebug() << "Error: g_window is null";
    }
}

AckCbPkt g_ackCbPkt[THREAD_TYPE_ACK_NUM] = {
    {THREAD_TYPE_ACK_IDX(THREAD_TYPE_ACK_FIRST), nullptr},
    {THREAD_TYPE_ACK_IDX(THREAD_TYPE_ACK_REG_CTRL), nullptr},
    {THREAD_TYPE_ACK_IDX(THREAD_TYPE_ACK_UPDATE_FW), UpdatePageFw}
#if defined(MODULE_USB3)
    ,{THREAD_TYPE_ACK_IDX(THREAD_TYPE_ACK_USB3), UpdateUsbPageImage}
#endif
};

void MainWindow::UpdateUi(void)
{
    ThreadManager *thread = g_threadList->Get(THREAD_ID_UPDATE_UI);
    if (!thread) {
        qDebug() << "Error: Update UI thread not found";
        return;
    }

    if (thread->IsQueueEmpty()) {
        return;
    }

    ThreadDataPkt *buf = (ThreadDataPkt *)(thread->DequeueData().data());
    uint16_t dataLen = hton16(buf->dataLen);
    dataLen += 8; // head, type, subtype, seq, dataLen
    uint16_t crc = *(uint16_t *)((uint8_t *)buf + dataLen);

    if (!check_crc16((uint8_t *)buf, dataLen, hton16(crc))) {
        return;
    }

    if (buf->head != hton16(THREAD_PACKET_HEAD)) {
        return;
    }

    if (buf->type > THREAD_TYPE_ACK_FIRST && buf->type < THREAD_TYPE_ACK_LAST) {
        if (g_ackCbPkt[THREAD_TYPE_ACK_IDX(buf->type)].cb) {
            g_ackCbPkt[THREAD_TYPE_ACK_IDX(buf->type)].cb(buf);
        }
    }
}

void UpdateUiTask(void)
{
    // 仅处理数据队列，不更新 UI
}

// mainwindow.cpp (添加 closeEvent 实现)
void MainWindow::closeEvent(QCloseEvent *event) {

    if (testsRunning) {
        event->ignore();
        qDebug() << "Close event ignored, waiting for tests to finish";
    } else {
        event->accept();

        if (g_serialPort.isOpen()) {
            qDebug() << "close serial port";
            g_serialPort.waitForBytesWritten(500);
            g_serialPort.close();
        }
    }
}
