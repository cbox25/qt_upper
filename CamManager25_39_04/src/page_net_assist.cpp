#include <QWidget>
#include <QTextEdit>
#include <QObject>
#include <QElapsedTimer>
#include <QLabel>
#include "page_net_assist.h"
#include "page_network.h"
#include "qboxlayout.h"
#include <QDebug> // Added for qDebug

#ifdef PRJ201

NetAssist g_netAssist;
TestInfoWidget g_topInfoWidget[4];

class ConfigWidgets {
public:
    int idx;
    QString name;
    int width;
    int height;
};

#define LABEL_WIDTH     100
#define LABEL_HEIGHT    50

#define LINE_EDIT_WIDTH     200
#define LINE_EDIT_HEIGHT    50

QString g_netAssistWidgets[4] = {
    { "本机IP地址"},
    { "目标IP地址"},
    { "本机端口"},
    { "目标端口"}
};

void NetAssistSendBtnClickedEvent(QObject *sendBtn)
{
    // 空指针检查和调试输出
    if (!g_netAssist.lableEditGrp[0].edit || !g_netAssist.lableEditGrp[1].edit ||
        !g_netAssist.lableEditGrp[2].edit || !g_netAssist.lableEditGrp[3].edit ||
        !g_netAssist.sendEdit || !g_netAssist.peroidDataEdit || !g_netAssist.periodChkBox || !g_netAssist.recvEdit) {
        qDebug() << "[NetAssist] Error: One or more widgets are nullptr!";
        return;
    }
    QString localIp = g_netAssist.lableEditGrp[0].edit->text();
    QString localPortStr = g_netAssist.lableEditGrp[1].edit->text();
    QString dstIp = g_netAssist.lableEditGrp[2].edit->text();
    QString dstPortStr = g_netAssist.lableEditGrp[3].edit->text();
    QString text = g_netAssist.sendEdit->toPlainText();
    QString delayMsStr = g_netAssist.peroidDataEdit->text();
    qDebug() << "[NetAssist] localIp:" << localIp << "localPort:" << localPortStr << "dstIp:" << dstIp << "dstPort:" << dstPortStr;
    qDebug() << "[NetAssist] sendText:" << text << "delayMs:" << delayMsStr;
    if (localIp.isEmpty() || localPortStr.isEmpty() || dstIp.isEmpty() || dstPortStr.isEmpty()) {
        qDebug() << "[NetAssist] Error: IP或端口为空";
        return;
    }
    bool ok1 = false, ok2 = false;
    uint16_t localPort = localPortStr.toUShort(&ok1);
    uint16_t dstPort = dstPortStr.toUShort(&ok2);
    if (!ok1 || !ok2) {
        qDebug() << "[NetAssist] Error: 端口号格式错误";
        return;
    }
    if (text.isEmpty()) {
        qDebug() << "[NetAssist] Error: 发送内容为空";
        return;
    }
    // 检查hex字符串合法性
    QString hexStrInput = text;
    hexStrInput.replace(" ", "");
    if (hexStrInput.length() % 2 != 0 || !hexStrInput.contains(QRegExp("^[0-9A-Fa-f]+$"))) {
        qDebug() << "[NetAssist] Error: 发送内容不是合法的十六进制字符串";
        return;
    }
    int len = 0;
    uint8_t hexStr[1024];
    uint8_t buf[1024];
    int flag = 0;
    int delayMs = delayMsStr.toInt();
    do {
        flag = g_netAssist.periodChkBox->isChecked();
        memset(hexStr, 0x00, sizeof(hexStr));
        QByteArray byteArray = QByteArray::fromHex(hexStrInput.toLatin1());
        len = hexStrInput.length() / 2;
        memcpy(hexStr, byteArray.data(), len);
        qDebug() << "[NetAssist] Send to:" << dstIp << ":" << dstPort << ", data:" << byteArray.toHex();
        UdpSendDataWithIp((const char *)hexStr, len, localIp, localPort, dstIp, dstPort, g_netAssist.recvEdit);
        memset(buf, 0x00, sizeof(buf));
        UdpRecvDataWithIp(buf, sizeof(buf), localIp, localPort, dstIp, dstPort, g_netAssist.recvEdit);
        if (flag) {
            DelayMs(delayMs);
        }
    } while (flag);
}

void CreateLabelLineEditGrp(QWidget *pageNetAssist)
{
    // 使用应用程序全局字体，而不是硬编码字体
    QFont font = getApplicationFont(14);
    QFont textEditFont = getApplicationFont(12);

    //主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(pageNetAssist);
    mainLayout->setContentsMargins(200,50,200,50);
    mainLayout->setSpacing(15);

    // 1. g_netAssistWidgets区
    QGridLayout *topGrid = new QGridLayout();
    topGrid->setSpacing(15);
    topGrid->setContentsMargins(0, 0, 0, 0);

    int rowCount = 2;
    for (int i= 0; i < 4; ++i) {
        int row = i % rowCount;
        int col = i / rowCount * 2;

        topGrid->addWidget(g_topInfoWidget[i].label = new QLabel(g_netAssistWidgets[i], pageNetAssist), row, col );
        g_topInfoWidget[i].label->setFont(font);
        //g_topInfoWidget[i].label->setAlignment(Qt::AlignVCenter);
        g_topInfoWidget[i].label->setStyleSheet("QLabel {color:#FFFFFF !important;"
                                                    "font-size: 18px;"
                                                    "background-color: transparent !important; }");
        topGrid->addWidget(g_topInfoWidget[i].edit = new QLineEdit(pageNetAssist), row, col + 1 );
        g_topInfoWidget[i].edit->setFont(textEditFont);
        g_topInfoWidget[i].edit->setStyleSheet("QLineEdit { color: #FFFFFF; "
                                                  "background-color: transparent !important;"
                                                  "border: 1px solid #888;"
                                                  "min-height: 50px; }");
    }

    QVBoxLayout *topLayout = new QVBoxLayout();
    topLayout->addLayout(topGrid);

    // 2. 发送数据区
    QVBoxLayout *SdataLayout = new QVBoxLayout();
    QLabel *sendLabel = new QLabel("发送数据", pageNetAssist);
    sendLabel->setFont(font);
    sendLabel->setStyleSheet("QLabel { color:#FFFFFF !important;"
                            "font-size: 18px;"
                            "background-color:#141519 !important; }");
    QTextEdit *sendEdit = new QTextEdit();
    sendEdit->setFont(textEditFont);
    sendEdit->setStyleSheet("QTextEdit { color: #FFFFFF; "
                            "background-color: transparent !important;"
                            "border: 1px solid #888; "
                            "min-height: 200px; }");
    SdataLayout->addWidget(sendLabel);
    SdataLayout->addWidget(sendEdit);

    // 3. 接收数据区
    QVBoxLayout *RdataLayout = new QVBoxLayout();
    QLabel *recevieLabel = new QLabel("接收数据", pageNetAssist);
    recevieLabel->setFont(font);
    recevieLabel->setStyleSheet("QLabel { color:#FFFFFF !important;"
                            "font-size: 18px;"
                            "background-color:#141519 !important; }");
    QTextEdit *recevieEdit = new QTextEdit();
    recevieEdit->setFont(textEditFont);
    recevieEdit->setStyleSheet("QTextEdit { color: #FFFFFF; "
                            "background-color: transparent !important;"
                            "border: 1px solid #888;"
                            "min-height: 200px; }");
    RdataLayout->addWidget(recevieLabel);
    RdataLayout->addWidget(recevieEdit);

    // 4. 周期发送区
    QHBoxLayout *periodLayout = new QHBoxLayout();
    periodLayout->setSpacing(10);
    QCheckBox *ChkBox = new QCheckBox("周期发送", pageNetAssist);
    ChkBox->setFont(font);
    ChkBox->setStyleSheet(R"(QCheckBox::indicator:unchecked {
                            background: white;
                            border: 1px solid #888;})"
                          "QCheckBox { color: #FFFFFF;"
                          "background-color: transparent !important;}");
    ChkBox->setMaximumWidth(100);
    QLineEdit *periodEdit = new QLineEdit();
    periodEdit->setFont(textEditFont);
    periodEdit->setStyleSheet("QLineEdit { color: #FFFFFF;"
                            "background-color: transparent !important;"
                            "border: 1px solid #888 !important;}");
    periodEdit->setMaximumWidth(100);
    QLabel *msLabel = new QLabel("ms", pageNetAssist);
    msLabel->setFont(font);
    msLabel->setStyleSheet("QLabel {color: #FFFFFF !important;"
                           "font-size: 18px;"
                           "background-color: transparent !important;}");
    msLabel->setMaximumWidth(40);
    QPushButton *sendBtn = new QPushButton("发送", pageNetAssist);
    sendBtn->setFont(font);
    sendBtn->setStyleSheet("QPushButton { background-color: transparent !important;"
                           " color: #FFFFFF;"
                           "border: 1px solid #ddd;"
                           "border-radius: 2px;}");
    sendBtn->setMaximumWidth(40);

    periodLayout->addWidget(ChkBox);
    periodLayout->addWidget(periodEdit);
    periodLayout->addWidget(msLabel);
    periodLayout->addStretch();
    periodLayout->addWidget(sendBtn);
    periodLayout->setAlignment(Qt::AlignBottom);

    mainLayout->addLayout(topLayout);
    mainLayout->addLayout(SdataLayout);
    mainLayout->addLayout(RdataLayout);
    mainLayout->addLayout(periodLayout);
    mainLayout->setAlignment(Qt::AlignTop);

    // 将控件赋值给g_netAssist结构体
    g_netAssist.sendEdit = sendEdit;
    g_netAssist.recvEdit = recevieEdit;
    g_netAssist.periodChkBox = ChkBox;
    g_netAssist.peroidDataEdit = periodEdit;
    for (int i = 0; i < 4; ++i) {
        g_netAssist.lableEditGrp[i].edit = g_topInfoWidget[i].edit;
    }

    // slots
    QObject::connect(sendBtn, &QPushButton::clicked, [sendBtn]() {
        NetAssistSendBtnClickedEvent((QObject *)sendBtn);
    });
}

void CreateNetAssistWidgets(QWidget *pageNetAssist)
{
    // create label and line edit groups
    CreateLabelLineEditGrp(pageNetAssist);
}
#endif
