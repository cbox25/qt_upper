#include "page_update.h"
#include <QWidget>
#include <QApplication>
#include <QObject>
#include <QDebug>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QProgressBar>
#include <QMessageBox>
#include <QtCore>
#include <QFile>
#include <winsock2.h>
#include "page_reg_ctrl.h"
#include "update_increase.h"
#include "uart.h"
#include "common.h"
#include "multi_threads.h"
#include "crc.h"
#include "mainwindow.h"
#include "../QLogger/QLogger.h"
#include "../QLogger/QLoggerWriter.h"
#include "../QLogger/QLoggerLevel.h"
#include <QStandardItemModel>
#include <QStandardItem>
#include <QVBoxLayout>
#include <QHBoxLayout>

UpdateWidgets g_updateWidgets;
CmdCb g_cmdCbUpdate[THREAD_SUBTYPE_UPDATE_CMD_LAST] = {
    nullptr, // THREAD_SUBTYPE_UPDATE_CMD_FIRST
    UpdateFile, // THREAD_SUBTYPE_UPDATE_CMD_UPDATE_FW
};
AckCbPkt g_ackCbUpdate[THREAD_SUBTYPE_UPDATE_ACK_LAST] = {
    {THREAD_SUBTYPE_UPDATE_ACK_FIRST, nullptr},
    {THREAD_SUBTYPE_UPDATE_ACK_PROGRESS_BAR, UpdateProgressBar},
    {THREAD_SUBTYPE_UPDATE_ACK_RET, UpdateRet},
    };

void SelectBtnClickedEvent(QWidget *sender)
{
    QWidget *parent = sender->parentWidget();
    QFileDialog *fileDlg = new QFileDialog();
    QString filePath = fileDlg->getOpenFileName(parent, "Select File", "", "Binary Files (*.bin);;All Files (*)");

    // set file path to line edit
    g_updateWidgets.lineEdit->setText(filePath);

    // Check the file path
    if (filePath.isEmpty()) {
        QLog_Warning("CamManager", "未选择文件");
        return;
    }
}

void UpdateBtnClickedEvent(QPushButton *btn)
{
    if (!g_updateWidgets.lineEdit) {
        qDebug() << "Error: g_updateWidgets.lineEdit is null";
        return;
    }

    QByteArray filepath = g_updateWidgets.lineEdit->text().toUtf8();
    qDebug() << "UpdateBtnClickedEvent: filepath length:" << filepath.size();
    if (filepath.isEmpty()) {
        qDebug() << "UpdateBtnClickedEvent: Filepath is empty, showing message";
        QLog_Error("CamManager", "文件路径为空");
        QMetaObject::invokeMethod(qApp, []() {
            QMessageBox box;
            box.setWindowTitle("升级失败");
            box.setText("文件路径为空");
            box.setIcon(QMessageBox::Critical);
            box.setStyleSheet(
                "QMessageBox { background-color: #000; color: #FFF; }"
                "QLabel { color: #FFF; }"
                "QPushButton { background-color: #000; color: #FFF; border: 1px solid #FFF; }"
                "QPushButton:focus { border: 1px solid #00BFFF; }"
                );
            box.exec();
        }, Qt::QueuedConnection);
        return;
    }

    // Check whether the serial port is connected to the device
    if (!g_serialPort.isOpen()) {
        qDebug() << "UpdateBtnClickedEvent: Serial port is not open, showing message";
        QLog_Error("CamManager", "串口未打开");
        QMetaObject::invokeMethod(qApp, []() {
            QMessageBox box;
            box.setWindowTitle("升级失败");
            box.setText("设备未连接，请先连接设备并选择串口");
            box.setIcon(QMessageBox::Critical);
            box.setStyleSheet(
                "QMessageBox { background-color: #000; color: #FFF; }"
                "QLabel { color: #FFF; }"
                "QPushButton { background-color: #000; color: #FFF; border: 1px solid #FFF; }"
                "QPushButton:focus { border: 1px solid #00BFFF; }"
                );
            box.exec();
        }, Qt::QueuedConnection);
        return;
    }

    if (!g_threadList) {
        qDebug() << "Error: g_threadList is null";
        return;
    }

    ThreadManager *thread = g_threadList->Get(THREAD_ID_UPDATE_FW);
    if (!thread) {
        qDebug() << "Error: Failed to get thread for THREAD_ID_UPDATE_FW";
        return;
    }

    if (filepath.size() > FILE_PATH_LEN_MAX) {
        qDebug() << "Error: Filepath size" << filepath.size() << "exceeds FILE_PATH_LEN_MAX" << FILE_PATH_LEN_MAX;
        return;
    }

    QByteArray buf(sizeof(ThreadDataPkt), 0);
    MakeThreadDataPkt(THREAD_TYPE_CMD_UPDATE_FW, THREAD_SUBTYPE_UPDATE_CMD_UPDATE_FW, filepath, buf);
    qDebug() << "Enqueuing data, buf size:" << buf.size() << ", filepath:" << filepath;
    thread->EnqueueData(buf);
    qDebug() << "Data enqueued for update with filepath:" << filepath;
}

void CreateUpdateWidgets(QWidget *pageUpdate)
{
    if (!pageUpdate) {
        qDebug() << "Error: pageUpdate is null";
        return;
    }
    if (pageUpdate->width() <= 0 || pageUpdate->height() <= 0) {
        pageUpdate->resize(800, 600); // 默认大小
        qDebug() << "Warning: pageUpdate size invalid, set to 800x600";
    }

    int spaceW = 20;
    int comBoxX = 0, comBoxY = 150, comBoxW = 120, comBoxH = 50;
    int editW = 400;
    int selectBtnW = comBoxW;
    int updateBtnW = comBoxW;
    // Use the global font of the application instead of hard-coded fonts
    QFont font = getApplicationFont(14);

    comBoxX = (pageUpdate->width() - 140 - comBoxW - editW - selectBtnW - updateBtnW - 3 * spaceW) / 2;
    // create widgets of page update
    QLineEdit *lineEdit = new QLineEdit(pageUpdate);
    QPushButton *selectBtn = new QPushButton("选择文件", pageUpdate);
    QPushButton *updateBtn = new QPushButton("开始升级", pageUpdate);
    QProgressBar *progressBar = new QProgressBar(pageUpdate);
    QMessageBox *msgBox = new QMessageBox();
    QComboBox *uartComBox = CreateComboBox(pageUpdate, comBoxX, comBoxY, comBoxW, comBoxH, font);

    // Create a main vertical layout
    QVBoxLayout *mainLayout = new QVBoxLayout(pageUpdate);

    // Create a horizontal layout for the top row
    QHBoxLayout *topLayout = new QHBoxLayout();
    topLayout->addWidget(uartComBox);
    topLayout->addWidget(lineEdit);
    topLayout->addWidget(selectBtn);
    topLayout->addWidget(updateBtn);

    // Add to the main layout
    mainLayout->addLayout(topLayout);
    mainLayout->addWidget(progressBar);
    mainLayout->setContentsMargins(100, 80, 100, 120); // Left top right bottom distance

    // Set the main layout
    pageUpdate->setLayout(mainLayout);
    // Set a border for the "Select File" button
    selectBtn->setStyleSheet("QPushButton { background: transparent; color: #FFF; border: 2px solid #888; }");

    // Set a border for the "Start Upgrade" button
    updateBtn->setStyleSheet("QPushButton { background: transparent; color: #FFF; border: 2px solid #888; }");

    // assign global variables
    g_updateWidgets.lineEdit = lineEdit;
    g_updateWidgets.selectBtn = selectBtn;
    g_updateWidgets.updateBtn = updateBtn;
    g_updateWidgets.progressBar = progressBar;
    g_updateWidgets.uartComBox = uartComBox;
    g_updateWidgets.msgBox = msgBox;

    // set comboBox
    uartComBox->setMinimumWidth(100);
    uartComBox->setMaximumWidth(100);
    uartComBox->setMinimumHeight(30);
    uartComBox->setMaximumHeight(100);

    // create LineEdit for patch file path
    lineEdit->setFont(font);
    lineEdit->setReadOnly(false);
    lineEdit->setMinimumWidth(320);
    lineEdit->setMaximumWidth(800);
    lineEdit->setMinimumHeight(30);
    lineEdit->setMaximumHeight(50);
    //lineEdit->setGeometry(editX, editY, editW, editH);
    lineEdit->setStyleSheet("QLineEdit { background: transparent; color: #FFF; border: 2px solid #888; }");        //文件地址框

    // create button to start FileDialog to select patch file
    selectBtn->setFont(font);
    selectBtn->setMinimumWidth(100);
    selectBtn->setMaximumWidth(100);
    selectBtn->setMinimumHeight(30);
    selectBtn->setMaximumHeight(50);
    //selectBtn->setGeometry(selectBtnX, selectBtnY, selectBtnW, selectBtnH);

    // create button to start update
    updateBtn->setFont(font);
    updateBtn->setMinimumWidth(100);
    updateBtn->setMaximumWidth(100);
    updateBtn->setMinimumHeight(30);
    updateBtn->setMaximumHeight(50);
    //updateBtn->setGeometry(updateBtnX, updateBtnY, updateBtnW, updateBtnH);

    // create a progress bar
    progressBar->setRange(0, 100); // 设置进度条范围
    progressBar->setValue(0);     // 初始值为0
    progressBar->setMinimumHeight(30);
    //progressBar->setMaximumWidth(660);
    //progressBar->setGeometry(progBarX, progBarY, progBarW, progBarH / 2);

    // 设置串口下拉项字体颜色为白色
    QStandardItemModel* model = qobject_cast<QStandardItemModel*>(uartComBox->model());
    if (model) {
        for (int i = 0; i < model->rowCount(); ++i) {
            QStandardItem* item = model->item(i);
            if (item) {
                item->setForeground(QBrush(Qt::white));
            }
        }
    }

    // slots
    //  QObject::connect(selectBtn, &QPushButton::clicked, [selectBtn]() {
    //      SelectBtnClickedEvent(selectBtn);
    //  });
    //  QObject::connect(updateBtn, &QPushButton::clicked, [updateBtn]() {
    //      UpdateBtnClickedEvent(updateBtn);
    //  });
    // ... existing code ...
    // slots
    QObject::connect(selectBtn, &QPushButton::clicked, [selectBtn]() {
        SelectBtnClickedEvent(selectBtn);
    });
    QObject::connect(updateBtn, &QPushButton::clicked, [updateBtn]() {
        UpdateBtnClickedEvent(updateBtn);
    });
}

void UpdateProgressBar(void *param)
{
    ThreadDataPkt *buf = NULL;
    uint8_t barValue = 0;

    if (param == NULL) {
        return;
    }

    buf = (ThreadDataPkt *)param;
    barValue = *(uint8_t *)(buf->data);

    g_updateWidgets.progressBar->setValue((int)barValue);
    QApplication::processEvents();  // make sure UI will update
}

void UpdateRet(void *param)
{
    ThreadDataPkt *buf = NULL;
    uint16_t dataLen = 0;

    if (param == NULL) {
        return;
    }

    buf = (ThreadDataPkt *)param;
    dataLen = hton16(buf->dataLen);

    QByteArray ret(reinterpret_cast<const char*>(buf->data), (int)dataLen);
    QString str = QString::fromUtf8(ret);
    qDebug() << "UpdateRet: received content:" << str;
    QLog_Debug("CamManager", QString("UpdateRet: received content: %1").arg(str));
    if (str.trimmed().isEmpty() || str.trimmed().length() < 5 || str == "\n" || str == "\r\n") {
        qDebug() << "UpdateRet: content empty, whitespace, or too short, skip messagebox";
        QLog_Warning("CamManager", "UpdateRet: content empty, whitespace, or too short, skip messagebox");
        return;
    }

    // 使用QMetaObject::invokeMethod确保在主线程中显示弹窗
    QMetaObject::invokeMethod(qApp, [str]() {
        qDebug() << "UpdateRet: showing messagebox in main thread with content:" << str;
        QMessageBox box;
        box.setWindowTitle("执行结果");
        box.setText(str);
        box.setIcon(QMessageBox::Information);
        box.setStyleSheet(
            "QMessageBox { background-color: #000; color: #FFF; }"
            "QLabel { color: #FFF; }"
            "QPushButton { background-color: #222; color: #FFF; border: 1px solid #FFF; min-width: 60px; min-height: 28px; }"
            "QPushButton:focus { border: 1px solid #00BFFF; }"
            );
        box.exec();
        qDebug() << "UpdateRet: messagebox closed";
    }, Qt::QueuedConnection);

    QLog_Error("CamManager", QString("%4").arg(str));
}

void UpdatePageFw(void *param)
{
    qDebug() << "UpdatePageFw called, param:" << param;
    uint8_t subType = 0;
    ThreadDataPkt *buf = NULL;

    if (param == NULL) {
        qDebug() << "UpdatePageFw: param is null";
        return;
    }

    buf = (ThreadDataPkt *)param;
    qDebug() << "UpdatePageFw: received type:" << (int)buf->type << ", subType:" << (int)buf->subType
             << ", dataLen:" << ntohs(buf->dataLen);

    // check subtype if necessary
    if (buf->type == THREAD_TYPE_CMD_UPDATE_FW) {
        subType = buf->subType;
        if (subType < THREAD_SUBTYPE_UPDATE_CMD_LAST) {
            if (g_cmdCbUpdate[subType]) {
                qDebug() << "UpdatePageFw: calling callback for subType" << (int)subType;
                g_cmdCbUpdate[subType](param);
            } else {
                qDebug() << "UpdatePageFw: no callback for subType" << (int)subType;
            }
        }
    } else if (buf->type == THREAD_TYPE_ACK_UPDATE_FW) {
        subType = buf->subType;
        if (subType < THREAD_SUBTYPE_UPDATE_ACK_LAST) {
            if (g_ackCbUpdate[subType].cb) {
                qDebug() << "UpdatePageFw: will call g_ackCbUpdate[" << (int)subType << "]";
                g_ackCbUpdate[subType].cb(param);
            } else {
                qDebug() << "UpdatePageFw: no callback for ACK subType" << (int)subType;
            }
        }
    }
}

void UpdateFile(void *param)
{
    if (!param) {
        qDebug() << "UpdateFile: param is null";
        return;
    }
    ThreadDataPkt *pkt = (ThreadDataPkt *)param;
    int dataLen = ntohs(pkt->dataLen);

    QByteArray rawPath(reinterpret_cast<const char*>(pkt->data), dataLen);
    QString filepath = QString::fromUtf8(rawPath).trimmed();

    qDebug() << "UpdateFile: received type:" << (int)pkt->type << ", subType:" << (int)pkt->subType
             << ", dataLen:" << dataLen << ", filepath:" << filepath;

    if (filepath.isEmpty() || filepath.contains(QChar('\0')) || filepath.contains('?')) {
        qDebug() << "UpdateFile: invalid filepath, aborting";
        QLog_Warning("CamManager", "UpdateFile: invalid filepath, aborting");
        QMetaObject::invokeMethod(qApp, []() {
            QMessageBox box;
            box.setWindowTitle("升级失败");
            box.setText("文件路径无效，请重新选择升级文件。");
            box.setIcon(QMessageBox::Critical);
            box.exec();
        }, Qt::QueuedConnection);
        return;
    }

    if (ntohs(pkt->dataLen) > 0) {
        qDebug() << "UpdateFile: processing file:" << filepath;

        if (!g_serialPort.isOpen()) {
            qDebug() << "UpdateFile: UART is not open, upgrade aborted";
            QMetaObject::invokeMethod(qApp, [filepath]() {
                QMessageBox box;
                box.setWindowTitle("升级失败");
                box.setText("串口未打开，无法进行升级: " + filepath);
                box.setIcon(QMessageBox::Critical);
                box.setStyleSheet(
                    "QMessageBox { background-color: #000; color: #FFF; }"
                    "QLabel { color: #FFF; }"
                    "QPushButton { background-color: #000; color: #FFF; border: 1px solid #FFF; }"
                    "QPushButton:focus { border: 1px solid #00BFFF; }"
                    );
                box.exec();
            }, Qt::QueuedConnection);
            return;
        }

        QFile file(filepath);
        if (!file.open(QIODevice::ReadOnly)) {
            qDebug() << "UpdateFile: failed to open file" << filepath << file.errorString();
            QLog_Error("CamManager", "UpdateFile: failed to open file");
            // 在主线程显示弹窗
            QMetaObject::invokeMethod(qApp, [filepath]() {
                QMessageBox box;
                box.setWindowTitle("升级失败");
                box.setText("无法打开文件: " + filepath);
                box.setIcon(QMessageBox::Critical);
                box.setStyleSheet(
                    "QMessageBox { background-color: #000; color: #FFF; }"
                    "QLabel { color: #FFF; }"
                    "QPushButton { background-color: #000; color: #FFF; border: 1px solid #FFF; }"
                    "QPushButton:focus { border: 1px solid #00BFFF; }"
                    );
                box.exec();
            }, Qt::QueuedConnection);
            return;
        }

        // 验证文件是否为升级文件（例如检查扩展名）
        QFileInfo fileInfo(filepath);
        QString ext = fileInfo.suffix().toLower();
        if (ext != "bin") {
            qDebug() << "UpdateFile: invalid file format, expected .bin, got" << ext;
            file.close();
            QMetaObject::invokeMethod(qApp, [filepath]() {
                QMessageBox box;
                box.setWindowTitle("升级失败");
                box.setText("无效的文件格式: " + filepath + "\n预期 .bin 文件");
                box.setIcon(QMessageBox::Critical);
                box.setStyleSheet(
                    "QMessageBox { background-color: #000; color: #FFF; }"
                    "QLabel { color: #FFF; }"
                    "QPushButton { background-color: #000; color: #FFF; border: 1px solid #FFF; }"
                    "QPushButton:focus { border: 1px solid #00BFFF; }"
                    );
                box.exec();
            }, Qt::QueuedConnection);
            return;
        }

        file.close();
        qDebug() << "UpdateFile: file opened successfully";

        // 调用主升级流程，错误弹窗将由主流程负责
        CmdUpdate(param);
        return;
    } else {
        qDebug() << "UpdateFile: empty filepath";
        QMetaObject::invokeMethod(qApp, []() {
            QMessageBox box;
            box.setWindowTitle("升级失败");
            box.setText("文件路径为空");
            box.setIcon(QMessageBox::Critical);
            box.setStyleSheet(
                "QMessageBox { background-color: #000; color: #FFF; }"
                "QLabel { color: #FFF; }"
                "QPushButton { background-color: #000; color: #FFF; border: 1px solid #FFF; }"
                "QPushButton:focus { border: 1px solid #00BFFF; }"
                );
            box.exec();
        }, Qt::QueuedConnection);
    }
}
