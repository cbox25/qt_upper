#include "page_network.h"
#include <QWidget>
#include <QApplication>
#include <QObject>
#include <QDebug>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QUdpSocket>
#include <QTextEdit>
#include <QRadioButton>
#include <QMutex>
#include <QLabel>
#include <QProcess>
#include "common.h"
#include "page_reg_ctrl.h"
#include "../base_widgets/switch_button.h"
#include <QEvent>
#include <QResizeEvent>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QSizePolicy>
#include <QFormLayout>
#include <qdir.h>

#ifdef PRJ201

QLineEdit *g_posEditX[IMAGE_BOARD_NUM_MAX];
QLineEdit *g_posEditY[IMAGE_BOARD_NUM_MAX];
QMutex g_sendMsgMutex;
CmdData g_cmdData[IMAGE_BOARD_NUM_MAX];
TestInfoWidget g_testInfoWidget[44];

SwitchButton *g_powerSwitch;
SwitchButton *g_gpioSwitch;

NetworkInfo g_netInfo[2] = {
    {"192.9.200.144", 5000},
    {"192.9.200.145", 5001}
};

QString g_testInfoLabelName[44] = {
    "ARM版本",
    "FPGA版本",
    "方位左边缘",
    "DDR初始化状态",
    "DDR初始化时间",
    "DVI1 长",
    "DVI1 宽",
    "DVI1 帧频",
    "DVI2 长",
    "DVI2 宽",
    "DVI2 帧频",
    "DVI2 数据出错数量",
    "DVI2 时序出错数量",
    "DVI1 数据出错数量",
    "DVI1 时序出错数量",
    "SDI1 长",
    "SDI1 宽",
    "SDI1 帧频",
    "SDI2 长",
    "SDI2 宽",
    "SDI2 帧频",
    "SRIO初始化状态",
    "视频大小",
    "视频类型",
    "是否请求前视视频",
    "是否请求后视视频",
    "是否请求全景视频",
    "SRIO给头盔前视视频最大地址",
    "SRIO给头盔后视视频最大地址",
    "SRIO给头盔全景视频最大地址",
    "SRIO给主板门铃频率",
    "SRIO给头盔门铃频率",
    "SRIO接收通道1帧频",
    "SRIO接收通道0帧频",
    "GTX状态",
    "预处理心跳",
    "预处理版本号",
    "预处理通道状态",
    "引导方位/高低坐标",
    "识别控制",
    "指令计数",
    "接收指令内容",
    "分布式1版本号",
    "分布式2-8版本号"
};

NetworkBtnInfo g_cmdBtnInfo[2][NETWORK_CMD_NUM_MAX] = {
    {
        {0, NULL, "向左滚动", RollLeftBtnClickedEvent},
        {1, NULL, "向右滚动", RollRightBtnClickedEvent},
        {2, NULL, "滚动加快", RollSpeedUpBtnClickedEvent},
        {3, NULL, "滚动变慢", RollSpeedDownBtnClickedEvent},
        {4, NULL, "单路图像", SingleChnImageBtnClickedEvent},
        {5, NULL, "左侧相机", TurnLeftCamBtnClickedEvent},
        {6, NULL, "右侧相机", TurnRightCamBtnClickedEvent},
        {7, NULL, "周视镜可见光视频", ShowPeriscopeVisibleBtnClickedEvent},
        {8, NULL, "周视镜热像视频", ShowPeriscopeIrBtnClickedEvent},
        {9, NULL, "瞄准镜可见光视频", ShowSightVisibleBtnClickedEvent},
        {10, NULL, "瞄准镜热像视频", ShowSightIrBtnClickedEvent},
        {11, NULL, "导弹视频", ShowMissileBtnClickedEvent},
        {12, NULL, "触摸显示", TouchDisplayBtnClickedEvent},
        {13, NULL, "图像增强", ImageEnhanceBtnClickedEvent},
        {14, NULL, "方位引导", DirectGuideBtnClickedEvent},
        {15, NULL, "识别控制", CtrlRecognizeBtnClickedEvent},
        {16, NULL, "版本号查询", GetVersionBtnClickedEvent},
        {17, NULL, "测试信息", GetTestInfoBtnClickedEvent}
    },

    {
        {0, NULL, "向左滚动", RollLeftBtnClickedEvent},
        {1, NULL, "向右滚动", RollRightBtnClickedEvent},
        {2, NULL, "滚动加快", RollSpeedUpBtnClickedEvent},
        {3, NULL, "滚动变慢", RollSpeedDownBtnClickedEvent},
        {4, NULL, "单路图像", SingleChnImageBtnClickedEvent},
        {5, NULL, "左侧相机", TurnLeftCamBtnClickedEvent},
        {6, NULL, "右侧相机", TurnRightCamBtnClickedEvent},
        {7, NULL, "周视镜可见光视频", ShowPeriscopeVisibleBtnClickedEvent},
        {8, NULL, "周视镜热像视频", ShowPeriscopeIrBtnClickedEvent},
        {9, NULL, "瞄准镜可见光视频", ShowSightVisibleBtnClickedEvent},
        {10, NULL, "瞄准镜热像视频", ShowSightIrBtnClickedEvent},
        {11, NULL, "导弹视频", ShowMissileBtnClickedEvent},
        {12, NULL, "触摸显示", TouchDisplayBtnClickedEvent},
        {13, NULL, "图像增强", ImageEnhanceBtnClickedEvent},
        {14, NULL, "方位引导", DirectGuideBtnClickedEvent},
        {15, NULL, "识别控制", CtrlRecognizeBtnClickedEvent},
        {16, NULL, "版本号查询", GetVersionBtnClickedEvent},
        {17, NULL, "测试信息", GetTestInfoBtnClickedEvent}
    }
};
QUdpSocket *g_udpSocket[2];
QTextEdit *g_textEdit[2];

// 定义统一按钮QSS
const char* btnQss =
    "QPushButton {"
    " color: #FFFFFF !important;"
    " background: transparent !important;"
    " border: 1px solid #888 !important;"
    " font-size: 16px;"
    " min-height: 40px;"
    " min-width: 120px;"
    "}"
    "QPushButton:pressed, QPushButton:checked {"
    " color: #FFFFFF !important;"
    " background: #333 !important;"
    " border: 1px solid #888 !important;"
    " font-size: 16px;"
    " min-height: 40px;"
    " min-width: 120px;"
    "}";

// 声明自适应布局函数
void UpdateTestInfoLayout(QWidget *pageNetwork, int zeroPointX, int zeroPointY, int w, int btnRowNum, int btnColNum);

// eventFilter类定义
class TestInfoResizeFilter : public QObject {
    QWidget* pageNetwork;
    int zeroPointX, zeroPointY, btnW, btnRowNum, btnColNum;
public:
    TestInfoResizeFilter(QWidget* page, int zx, int zy, int bw, int br, int bc)
        : QObject(page), pageNetwork(page), zeroPointX(zx), zeroPointY(zy), btnW(bw), btnRowNum(br), btnColNum(bc) {}

    bool eventFilter(QObject* obj, QEvent* event) override {
        if (obj == pageNetwork && event->type() == QEvent::Resize) {
            UpdateTestInfoLayout(pageNetwork, zeroPointX, zeroPointY, btnW, btnRowNum, btnColNum);
        }
        return QObject::eventFilter(obj, event);
    }
};

void ClearReadBuffer(QUdpSocket *udpSocket)
{
    // 检查socket是否有效
    if (!udpSocket || !udpSocket->isValid()) {
        return;
    }
    
    while (udpSocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(udpSocket->pendingDatagramSize());
        udpSocket->readDatagram(datagram.data(), datagram.size());
    }
}

void WriteDateToTextEdit(QString &direction, const char *buf, int len, QTextEdit *textEdit)
{
    // QByteArray byteArray(reinterpret_cast<const char*>(buf), len);
    QString hexString = direction;

    if (len > 0) {
        for (int i = 0; i < len; ++i) {
            if (i > 0) {
                hexString += ", ";
            }
            hexString += QString("0x%1").arg(static_cast<unsigned char>(buf[i]),
                                             2, 16, QLatin1Char('0')).toUpper().replace("0X", "0x");
        }
    }

    hexString += "\n";

    // 将转换后的字符串设置到QTextEdit
    textEdit->append(hexString);
}

void UdpSendData(const char *buf, int len, int boardNum)
{
    qDebug() << "UdpSendData called, len=" << len << ", boardNum=" << boardNum;
    qDebug() << "g_textEdit[boardNum] is null?" << (g_textEdit[boardNum] == nullptr);
    // 添加空指针检查
    if (!g_gpioSwitch) {
        qDebug() << "UdpSendData: g_gpioSwitch is null";
        return;
    }

    if (!g_udpSocket[boardNum]) {
        qDebug() << "UdpSendData: creating UDP socket for boardNum" << boardNum;
        g_udpSocket[boardNum] = new QUdpSocket();
    }

    QString send = "Send to: ";
    QHostAddress hostAddr;
    quint16 port = 0;
    QString dstIp;

    int idx = g_gpioSwitch->checked() == false ? 0 : 1;
    
    if (idx == 0) {
        dstIp = "192.9.200.144";
        port = 5000;
    } else {
        dstIp = "192.9.200.145";
        port = 5001;
    }

    g_udpSocket[boardNum]->bind(port);
    hostAddr.setAddress(dstIp);

    // clear recv buffer of udp before sending
    ClearReadBuffer(g_udpSocket[boardNum]);

    qint64 bytesWritten = g_udpSocket[boardNum]->writeDatagram(buf, len, hostAddr, port); // send data

    if (bytesWritten == len) {
        qDebug() << "Data sent successfully";
    } else {
        qDebug() << "Failed to send data";
    }

    // show send data in textEdit
    send += dstIp + ":" + QString::number(port) + "\n";
    if (g_textEdit[boardNum]) {
        qDebug() << "UdpSendData: writing to textEdit, boardNum=" << boardNum;
    WriteDateToTextEdit(send, buf, bytesWritten, g_textEdit[boardNum]);
    } else {
        qDebug() << "UdpSendData: g_textEdit[boardNum] is null";
    }
}

bool IsValidIPv4Addr(const QString &ipAddress) {
    // 定义正则表达式，用于匹配 IPv4 地址
    QRegExp ipv4Regex(R"(^(\d{1,3}\.){3}\d{1,3}$)");

    // 检查是否匹配正则表达式
    if (!ipv4Regex.exactMatch(ipAddress)) {
        return false;
    }

    // 进一步检查每组数字是否在 0 �?255 之间
    QStringList parts = ipAddress.split('.');
    for (const QString &part : parts) {
        int value = part.toInt();
        if (value < 0 || value > 255) {
            return false;
        }
    }

    return true;
}

#include <QNetworkInterface>

QString GetLocalIp(QString eth)
{
    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    QHostAddress ip;

    foreach (const QNetworkInterface &interface, interfaces) {
        // check if the network interface is "eth1"
        if (interface.name() == eth) {
            qDebug() << "find out the nerwork interface" << interface.name();
            foreach (const QNetworkAddressEntry &entry, interface.addressEntries()) {
                ip = entry.ip();
                if (ip.protocol() == QAbstractSocket::IPv4Protocol) { // 过滤�?IPv4 地址
                    qDebug() << "IPv4 address:" << ip.toString();
                }
            }
        }
    }

    return ip.toString();
}

void UdpSendDataWithIp(const char *buf, int len, QString srcIp, uint16_t srcPort, QString dstIp, uint16_t dstPort, QTextEdit *edit)
{
    qDebug() << "[DEBUG] UdpSendDataWithIp called with srcIp:" << srcIp << "dstIp:" << dstIp;
    QString send = "Send to: ";
    QHostAddress hostAddr;
    quint16 port = 0;
    QProcess process;
    hostAddr.setAddress(dstIp);
    port = dstPort;

    if (! IsValidIPv4Addr(srcIp)) {
        qDebug() << "invalid src ip";
        return;
    }

    if (! IsValidIPv4Addr(dstIp)) {
        qDebug() << "invalid dst ip";
        return;
    }

    // set local ip
#ifndef _WIN32
#if 1
    QString localIp = GetLocalIp("eth1");

    if(localIp != srcIp) {
        QString cmd = QString("ifconfig eth1 %1 netmask 255.255.255.0\n").arg(srcIp);
        qDebug() << "cmd: " << cmd;
        process.start(cmd);
        process.waitForStarted();
        process.waitForFinished();
        process.close();

        // make gateway
        int lastDotIdx = srcIp.lastIndexOf('.');
        QString gateway;
        if (lastDotIdx != -1) {
            gateway = srcIp.left(lastDotIdx + 1) + "1";
        }
        cmd = "route add default gw " + gateway + " eth1\n";
        qDebug() << "cmd: " << cmd;
        process.start(cmd);
        process.waitForStarted();
        process.waitForFinished();
        process.close();
    }
#else
    char cmd[128] = {0};
    memset(cmd, 0x00, sizeof(cmd));
    sprintf(cmd, "ifconfig eth1 %s netmask 255.255.255.0", srcIp.toStdString().c_str());
    qDebug() << cmd;
    system(cmd);
#endif
#endif // _WIN32

    // 检查并绑定socket
    if (!g_udpSocket[0]) {
        g_udpSocket[0] = new QUdpSocket();
    }
    
    // 绑定socket到本地端口
    if (g_udpSocket[0]->state() != QAbstractSocket::BoundState) {
        QHostAddress localAddr;
        if (srcIp.isEmpty()) {
            localAddr = QHostAddress::Any;
        } else {
            localAddr = QHostAddress(srcIp);
        }
        g_udpSocket[0]->bind(localAddr, srcPort);
    }

    // clear recv buffer of udp before sending
    ClearReadBuffer(g_udpSocket[0]);

    qint64 bytesWritten = g_udpSocket[0]->writeDatagram(buf, len, hostAddr, port); // send data

    if (bytesWritten == len) {
        qDebug() << "Data sent successfully";
    } else {
        qDebug() << "Failed to send data";
    }

    // show send data in textEdit
    send += dstIp + ":" + QString::number(dstPort) + "\n";
    WriteDateToTextEdit(send, buf, bytesWritten, edit);
}

int UdpRecvData(uint8_t *buf, int len, int boardNum)
{
    qDebug() << "UdpRecvData: starting, boardNum=" << boardNum;

    // 添加空指针检查
    if (!g_gpioSwitch) {
        qDebug() << "UdpRecvData: g_gpioSwitch is null";
        return 0;
    }

    if (!g_udpSocket[boardNum]) {
        qDebug() << "UdpRecvData: creating UDP socket for boardNum" << boardNum;
        g_udpSocket[boardNum] = new QUdpSocket();
    }

    QString recv = "Receive from: ";
    QHostAddress hostAddr;
    quint16 targetPort = 0, port = 0;
    int64_t bytesRead = 0;
    int timeout = 0, delay = 50;
    QString recvIp;

    int idx = g_gpioSwitch->checked() == false ? 0 : 1;
    qDebug() << "UdpRecvData: idx=" << idx;

    if (idx == 0) {
        targetPort = 5000;
    } else {
        targetPort = 5001;
    }
    g_udpSocket[boardNum]->bind(port);

    qDebug() << "UdpRecvData: waiting for data...";
    do {
        DelayMs(delay);
        bytesRead = g_udpSocket[boardNum]->readDatagram((char *)buf, len, &hostAddr, &port);
        timeout += delay;
    } while (bytesRead <= 0 && timeout < 1000);

    qDebug() << "UdpRecvData: bytesRead=" << bytesRead << ", timeout=" << timeout;

    if (port != targetPort) {
        memset(buf, 0x00, len);
        bytesRead = 0;
    }

    if (hostAddr.toString().startsWith("::ffff:")) {
        recvIp = hostAddr.toString().mid(7); // 截取 IPv4 地址部分
    } else {
        recvIp = hostAddr.toString();
    }

    // show send data in textEdit
    recv += recvIp + ":" + QString::number(port) + "\n";
    if (g_textEdit[boardNum]) {
        qDebug() << "UdpRecvData: writing to textEdit";
    WriteDateToTextEdit(recv, (const char *)buf, bytesRead, g_textEdit[boardNum]);
    } else {
        qDebug() << "UdpRecvData: g_textEdit[boardNum] is null";
    }

    qDebug() << "UdpRecvData: completed, returning" << (int)bytesRead;
    return (int)bytesRead;
}

void UdpRecvDataWithIp(uint8_t *buf, int len, QString localIp, uint16_t localPort, QString remoteIp, uint16_t remotePort, QTextEdit *edit)
{
    // 检查socket是否初始化
    if (!g_udpSocket[0]) {
        g_udpSocket[0] = new QUdpSocket();
    }
    
    // 绑定socket到本地端口
    if (g_udpSocket[0]->state() != QAbstractSocket::BoundState) {
        QHostAddress localAddr;
        if (localIp.isEmpty()) {
            localAddr = QHostAddress::Any;
        } else {
            localAddr = QHostAddress(localIp);
        }
        g_udpSocket[0]->bind(localAddr, localPort);
    }
    
    QString recv = "Receive from: ";
    QHostAddress hostAddr;
    quint16 targetPort = 0, port = 0;
    int64_t bytesRead = 0;
    int timeout = 0, delay = 50;
    targetPort = remotePort;
    QString recvIp;

    do {
        DelayMs(delay);
        bytesRead = g_udpSocket[0]->readDatagram((char *)buf, len, &hostAddr, &port);
        timeout += delay;
    } while (bytesRead <= 0 && timeout < 1000);

    if (port != targetPort) {
        memset(buf, 0x00, len);
        bytesRead = 0;
    }

    if (hostAddr.toString().startsWith("::ffff:")) {
        recvIp = hostAddr.toString().mid(7); // 截取 IPv4 地址部分
    } else {
        recvIp = hostAddr.toString();
    }

    // show send data in textEdit
    recv += recvIp + ":" + QString::number(port) + "\n";
    WriteDateToTextEdit(recv, (const char *)buf, bytesRead, edit);
}

void RollLeftStart(int idx, int boardNum)
{
    // if no style is set, set as green
    g_cmdBtnInfo[boardNum][idx].btn->setStyleSheet("QPushButton { color: #FFFFFF !important; background: transparent !important; border: 1px solid #888 !important; }");

    g_cmdData[boardNum].cnt ++;
    g_cmdData[boardNum].cmd = 0x01;
    g_cmdData[boardNum].data1 = hton16(0x0021); // roll left
    g_cmdData[boardNum].data2 = hton16(0x0000);

    // send cmd data through udp
    UdpSendData((const char *)&g_cmdData[boardNum], sizeof(g_cmdData[boardNum]), boardNum);
}

void RollLeftStop(int idx, int boardNum)
{
    // remove style
    g_cmdBtnInfo[boardNum][idx].btn->setStyleSheet("QPushButton { color: #FFFFFF !important; background: transparent !important; border: 1px solid #888 !important; }");

    g_cmdData[boardNum].cnt ++;
    g_cmdData[boardNum].cmd = 0x01;
    g_cmdData[boardNum].data1 = hton16(0x0023); // stop roll
    g_cmdData[boardNum].data2 = hton16(0x0000);

    // send cmd data through udp
    UdpSendData((const char *)&g_cmdData[boardNum], sizeof(g_cmdData[boardNum]), boardNum);
}

void RollRightStart(int idx, int boardNum)
{
    // if no style is set, set as green
    g_cmdBtnInfo[boardNum][idx].btn->setStyleSheet("QPushButton { color: #FFFFFF !important; background: transparent !important; border: 1px solid #888 !important; }");

    g_cmdData[boardNum].cnt ++;
    g_cmdData[boardNum].cmd = 0x01;
    g_cmdData[boardNum].data1 = hton16(0x0022); // roll right
    g_cmdData[boardNum].data2 = hton16(0x0000);

    // send cmd data through udp
    UdpSendData((const char *)&g_cmdData[boardNum], sizeof(g_cmdData[boardNum]), boardNum);
}

void RollRightStop(int idx, int boardNum)
{
    // remove style
    g_cmdBtnInfo[boardNum][idx].btn->setStyleSheet("QPushButton { color: #FFFFFF !important; background: transparent !important; border: 1px solid #888 !important; }");

    g_cmdData[boardNum].cnt ++;
    g_cmdData[boardNum].cmd = 0x01;
    g_cmdData[boardNum].data1 = hton16(0x0023); // stop roll
    g_cmdData[boardNum].data2 = hton16(0x0000);

    // send cmd data through udp
    UdpSendData((const char *)&g_cmdData[boardNum], sizeof(g_cmdData[boardNum]), boardNum);
}

// add a button to send cmd of roll left
void RollLeftBtnClickedEvent(QObject *sender, int idx, int boardNum)
{
    if (g_cmdBtnInfo[boardNum][idx].btn->styleSheet().isEmpty()) {
        RollLeftStart(idx, boardNum);

        /* exclusion 2,5,6,7 */
        if (! g_cmdBtnInfo[boardNum][1].btn->styleSheet().isEmpty()) {
            g_cmdBtnInfo[boardNum][1].btn->setStyleSheet("QPushButton { color: #FFFFFF !important; background: transparent !important; border: 1px solid #888 !important; }");
        }

        if (! g_cmdBtnInfo[boardNum][4].btn->styleSheet().isEmpty()) {
            g_cmdBtnInfo[boardNum][4].btn->setStyleSheet("QPushButton { color: #FFFFFF !important; background: transparent !important; border: 1px solid #888 !important; }");
        }

        if (! g_cmdBtnInfo[boardNum][12].btn->styleSheet().isEmpty()) {
            g_cmdBtnInfo[boardNum][12].btn->setStyleSheet("QPushButton { color: #FFFFFF !important; background: transparent !important; border: 1px solid #888 !important; }");
        }
    } else {
        RollLeftStop(idx, boardNum);
    }
}

void RollRightBtnClickedEvent(QObject *sender, int idx, int boardNum)
{
    if (g_cmdBtnInfo[boardNum][idx].btn->styleSheet().isEmpty()) {
        RollRightStart(idx, boardNum);

        /* exclusion 1,5,6,7 */
        if (! g_cmdBtnInfo[boardNum][0].btn->styleSheet().isEmpty()) {
            g_cmdBtnInfo[boardNum][0].btn->setStyleSheet("QPushButton { color: #FFFFFF !important; background: transparent !important; border: 1px solid #888 !important; }");
        }

        if (! g_cmdBtnInfo[boardNum][4].btn->styleSheet().isEmpty()) {
            g_cmdBtnInfo[boardNum][4].btn->setStyleSheet("QPushButton { color: #FFFFFF !important; background: transparent !important; border: 1px solid #888 !important; }");
        }

        if (! g_cmdBtnInfo[boardNum][12].btn->styleSheet().isEmpty()) {
            g_cmdBtnInfo[boardNum][12].btn->setStyleSheet("QPushButton { color: #FFFFFF !important; background: transparent !important; border: 1px solid #888 !important; }");
        }
    } else {
        RollRightStop(idx, boardNum);
    }
}

void RollSpeedUpBtnClickedEvent(QObject *sender, int idx, int boardNum)
{
    g_cmdData[boardNum].cnt ++;
    g_cmdData[boardNum].cmd = 0x01;
    g_cmdData[boardNum].data1 = hton16(0x0024); // speed up
    g_cmdData[boardNum].data2 = hton16(0x0000);

    // send cmd data through udp
    UdpSendData((const char *)&g_cmdData[boardNum], sizeof(g_cmdData[boardNum]), boardNum);
}

void RollSpeedDownBtnClickedEvent(QObject *sender, int idx, int boardNum)
{
    g_cmdData[boardNum].cnt ++;
    g_cmdData[boardNum].cmd = 0x01;
    g_cmdData[boardNum].data1 = hton16(0x0025); // speed down
    g_cmdData[boardNum].data2 = hton16(0x0000);

    // send cmd data through udp
    UdpSendData((const char *)&g_cmdData[boardNum], sizeof(g_cmdData[boardNum]), boardNum);
}

void SingleChnImageStart(int idx, int boardNum)
{
    // if no style is set, set as green
    g_cmdBtnInfo[boardNum][idx].btn->setStyleSheet("QPushButton { color: #FFFFFF !important; background: transparent !important; border: 1px solid #888 !important; }");

    g_cmdData[boardNum].cnt ++;
    g_cmdData[boardNum].cmd = 0x01;
    g_cmdData[boardNum].data1 = hton16(0x0010); // single channel image
    g_cmdData[boardNum].data2 = hton16(0x0000);

    // send cmd data through udp
    UdpSendData((const char *)&g_cmdData[boardNum], sizeof(g_cmdData[boardNum]), boardNum);

    /* exclusion 1,2 */
    if (! g_cmdBtnInfo[boardNum][0].btn->styleSheet().isEmpty()) {
        g_cmdBtnInfo[boardNum][0].btn->setStyleSheet("QPushButton { color: #FFFFFF !important; background: transparent !important; border: 1px solid #888 !important; }");
    }

    if (! g_cmdBtnInfo[boardNum][1].btn->styleSheet().isEmpty()) {
        g_cmdBtnInfo[boardNum][1].btn->setStyleSheet("QPushButton { color: #FFFFFF !important; background: transparent !important; border: 1px solid #888 !important; }");
    }

    if (! g_cmdBtnInfo[boardNum][12].btn->styleSheet().isEmpty()) {
        g_cmdBtnInfo[boardNum][12].btn->setStyleSheet("QPushButton { color: #FFFFFF !important; background: transparent !important; border: 1px solid #888 !important; }");
    }
}

void SingleChnImageStop(int idx, int boardNum)
{
    // remove style
    g_cmdBtnInfo[boardNum][idx].btn->setStyleSheet("QPushButton { color: #FFFFFF !important; background: transparent !important; border: 1px solid #888 !important; }");
}

void SingleChnImageBtnClickedEvent(QObject *sender, int idx, int boardNum)
{
    qDebug() << "SingleChnImageBtnClickedEvent called, idx=" << idx << ", boardNum=" << boardNum;
    if (g_cmdBtnInfo[boardNum][idx].btn->isEnabled()) {
        SingleChnImageStart(idx, boardNum);
    }
}

void TurnLeftCamBtnClickedEvent(QObject *sender, int idx, int boardNum)
{
    g_cmdData[boardNum].cnt ++;
    g_cmdData[boardNum].cmd = 0x01;
    g_cmdData[boardNum].data1 = hton16(0x0019); // left camera
    g_cmdData[boardNum].data2 = hton16(0x0000);

    // send cmd data through udp
    UdpSendData((const char *)&g_cmdData[boardNum], sizeof(g_cmdData[boardNum]), boardNum);

    /* exclusion 1,2 */
    if (! g_cmdBtnInfo[boardNum][0].btn->styleSheet().isEmpty()) {
        g_cmdBtnInfo[boardNum][0].btn->setStyleSheet("QPushButton { color: #FFFFFF !important; background: transparent !important; border: 1px solid #888 !important; }");
    }

    if (! g_cmdBtnInfo[boardNum][1].btn->styleSheet().isEmpty()) {
        g_cmdBtnInfo[boardNum][1].btn->setStyleSheet("QPushButton { color: #FFFFFF !important; background: transparent !important; border: 1px solid #888 !important; }");
    }

    if (g_cmdBtnInfo[boardNum][4].btn->styleSheet().isEmpty()) {
        g_cmdBtnInfo[boardNum][4].btn->setStyleSheet("QPushButton { color: #FFFFFF !important; background: transparent !important; border: 1px solid #888 !important; }");
    }

    if (! g_cmdBtnInfo[boardNum][12].btn->styleSheet().isEmpty()) {
        g_cmdBtnInfo[boardNum][12].btn->setStyleSheet("QPushButton { color: #FFFFFF !important; background: transparent !important; border: 1px solid #888 !important; }");
    }
}

void TurnRightCamBtnClickedEvent(QObject *sender, int idx, int boardNum)
{
    g_cmdData[boardNum].cnt ++;
    g_cmdData[boardNum].cmd = 0x01;
    g_cmdData[boardNum].data1 = hton16(0x001A); // right camera
    g_cmdData[boardNum].data2 = hton16(0x0000);

    // send cmd data through udp
    UdpSendData((const char *)&g_cmdData[boardNum], sizeof(g_cmdData[boardNum]), boardNum);

    /* exclusion 1,2 */
    if (! g_cmdBtnInfo[boardNum][0].btn->styleSheet().isEmpty()) {
        g_cmdBtnInfo[boardNum][0].btn->setStyleSheet("QPushButton { color: #FFFFFF !important; background: transparent !important; border: 1px solid #888 !important; }");
    }

    if (! g_cmdBtnInfo[boardNum][1].btn->styleSheet().isEmpty()) {
        g_cmdBtnInfo[boardNum][1].btn->setStyleSheet("QPushButton { color: #FFFFFF !important; background: transparent !important; border: 1px solid #888 !important; }");
    }

    // belongs to 4
    if (g_cmdBtnInfo[boardNum][4].btn->styleSheet().isEmpty()) {
        g_cmdBtnInfo[boardNum][4].btn->setStyleSheet("QPushButton { color: #FFFFFF !important; background: transparent !important; border: 1px solid #888 !important; }");
    }

    if (! g_cmdBtnInfo[boardNum][12].btn->styleSheet().isEmpty()) {
        g_cmdBtnInfo[boardNum][12].btn->setStyleSheet("QPushButton { color: #FFFFFF !important; background: transparent !important; border: 1px solid #888 !important; }");
    }
}

void ShowPeriscopeVisibleBtnClickedEvent(QObject *sender, int idx, int boardNum)
{
    g_cmdData[boardNum].cnt ++;
    g_cmdData[boardNum].cmd = 0x01;
    g_cmdData[boardNum].data1 = hton16(0x0080); // periscope visible
    g_cmdData[boardNum].data2 = hton16(0x0000);

    // send cmd data through udp
    UdpSendData((const char *)&g_cmdData[boardNum], sizeof(g_cmdData[boardNum]), boardNum);
}

void ShowPeriscopeIrBtnClickedEvent(QObject *sender, int idx, int boardNum)
{
    g_cmdData[boardNum].cnt ++;
    g_cmdData[boardNum].cmd = 0x01;
    g_cmdData[boardNum].data1 = hton16(0x0081); // periscope ir
    g_cmdData[boardNum].data2 = hton16(0x0000);

    // send cmd data through udp
    UdpSendData((const char *)&g_cmdData[boardNum], sizeof(g_cmdData[boardNum]), boardNum);
}

void ShowSightVisibleBtnClickedEvent(QObject *sender, int idx, int boardNum)
{
    g_cmdData[boardNum].cnt ++;
    g_cmdData[boardNum].cmd = 0x01;
    g_cmdData[boardNum].data1 = hton16(0x0082); // sight visible
    g_cmdData[boardNum].data2 = hton16(0x0000);

    // send cmd data through udp
    UdpSendData((const char *)&g_cmdData[boardNum], sizeof(g_cmdData[boardNum]), boardNum);
}

void ShowSightIrBtnClickedEvent(QObject *sender, int idx, int boardNum)
{
    g_cmdData[boardNum].cnt ++;
    g_cmdData[boardNum].cmd = 0x01;
    g_cmdData[boardNum].data1 = hton16(0x0083); // sight ir
    g_cmdData[boardNum].data2 = hton16(0x0000);

    // send cmd data through udp
    UdpSendData((const char *)&g_cmdData[boardNum], sizeof(g_cmdData[boardNum]), boardNum);
}

void ShowMissileBtnClickedEvent(QObject *sender, int idx, int boardNum)
{
    g_cmdData[boardNum].cnt ++;
    g_cmdData[boardNum].cmd = 0x01;
    g_cmdData[boardNum].data1 = hton16(0x0084); // missile
    g_cmdData[boardNum].data2 = hton16(0x0000);

    // send cmd data through udp
    UdpSendData((const char *)&g_cmdData[boardNum], sizeof(g_cmdData[boardNum]), boardNum);
}

void TouchDisplayBtnClickedEvent(QObject *sender, int idx, int boardNum)
{
    uint16_t coor_x = 0, coor_y = 0;
    bool ok;

    coor_x = g_posEditX[boardNum]->text().toInt(&ok);
    if (! ok) {
        coor_x = 0;
    }

    coor_y = g_posEditY[boardNum]->text().toInt(&ok);
    if (! ok) {
        coor_y = 0;
    }

    // set as green
    g_cmdBtnInfo[boardNum][idx].btn->setStyleSheet("QPushButton { color: #FFFFFF !important; background: transparent !important; border: 1px solid #888 !important; }");
    g_cmdData[boardNum].cnt ++;
    g_cmdData[boardNum].cmd = 0x03;
    g_cmdData[boardNum].data1 = hton16(coor_x);
    g_cmdData[boardNum].data2 = hton16(coor_y);

    // exclude
    g_cmdBtnInfo[boardNum][0].btn->setStyleSheet("QPushButton { color: #FFFFFF !important; background: transparent !important; border: 1px solid #888 !important; }");
    g_cmdBtnInfo[boardNum][1].btn->setStyleSheet("QPushButton { color: #FFFFFF !important; background: transparent !important; border: 1px solid #888 !important; }");
    g_cmdBtnInfo[boardNum][4].btn->setStyleSheet("QPushButton { color: #FFFFFF !important; background: transparent !important; border: 1px solid #888 !important; }");

    // send cmd data through udp
    UdpSendData((const char *)&g_cmdData[boardNum], sizeof(g_cmdData[boardNum]), boardNum);
}

void ImageEnhanceBtnClickedEvent(QObject *sender, int idx, int boardNum)
{
    if (g_cmdBtnInfo[boardNum][idx].btn->styleSheet().isEmpty()) {
        // if no style is set, set as green
        g_cmdBtnInfo[boardNum][idx].btn->setStyleSheet("QPushButton { color: #FFFFFF !important; background: transparent !important; border: 1px solid #888 !important; }");

        g_cmdData[boardNum].cnt ++;
        g_cmdData[boardNum].cmd = 0x02;
        g_cmdData[boardNum].data1 = hton16(0x0011); // image enhance
        g_cmdData[boardNum].data2 = hton16(0x0000);
    } else {
        // remove style
        g_cmdBtnInfo[boardNum][idx].btn->setStyleSheet("QPushButton { color: #FFFFFF !important; background: transparent !important; border: 1px solid #888 !important; }");

        g_cmdData[boardNum].cnt ++;
        g_cmdData[boardNum].cmd = 0x02;
        g_cmdData[boardNum].data1 = hton16(0x0010); // disable
        g_cmdData[boardNum].data2 = hton16(0x0000);
    }

    // send cmd data through udp
    UdpSendData((const char *)&g_cmdData[boardNum], sizeof(g_cmdData[boardNum]), boardNum);
}

void DirectGuideBtnClickedEvent(QObject *sender, int idx, int boardNum)
{
    uint16_t coor_x = 0, coor_y = 0;
    bool ok;

    coor_x = g_posEditX[boardNum]->text().toInt(&ok);
    if (! ok) {
        coor_x = 0;
    }

    coor_y = g_posEditY[boardNum]->text().toInt(&ok);
    if (! ok) {
        coor_y = 0;
    }

    // set as green
    // g_cmdBtnInfo[boardNum][idx].btn->setStyleSheet("background-color: #transparent;");

    g_cmdData[boardNum].cnt ++;
    g_cmdData[boardNum].cmd = 0x04;
    g_cmdData[boardNum].data1 = hton16(coor_x); // other mode
    g_cmdData[boardNum].data2 = hton16(coor_y);

    // send cmd data through udp
    UdpSendData((const char *)&g_cmdData[boardNum], sizeof(g_cmdData[boardNum]), boardNum);

    // exclude

}

void CtrlRecognizeBtnClickedEvent(QObject *sender, int idx, int boardNum)
{
    if (g_cmdBtnInfo[boardNum][idx].btn->styleSheet().isEmpty()) {
        // if no style is set, set as green
        g_cmdBtnInfo[boardNum][idx].btn->setStyleSheet("QPushButton { color: #FFFFFF !important; background: transparent !important; border: 1px solid #888 !important; }");

        g_cmdData[boardNum].cnt ++;
        g_cmdData[boardNum].cmd = 0x02;
        g_cmdData[boardNum].data1 = hton16(0x0021); // control recognize
        g_cmdData[boardNum].data2 = hton16(0x0000);
    } else {
        // remove style
        g_cmdBtnInfo[boardNum][idx].btn->setStyleSheet("QPushButton { color: #FFFFFF !important; background: transparent !important; border: 1px solid #888 !important; }");

        g_cmdData[boardNum].cnt ++;
        g_cmdData[boardNum].cmd = 0x02;
        g_cmdData[boardNum].data1 = hton16(0x0020); // disable
        g_cmdData[boardNum].data2 = hton16(0x0000);
    }

    // send cmd data through udp
    UdpSendData((const char *)&g_cmdData[boardNum], sizeof(g_cmdData[boardNum]), boardNum);
}

void GetVersionBtnClickedEvent(QObject *sender, int idx, int boardNum)
{
    uint8_t buf[1024] = {0};
    g_cmdData[boardNum].cnt ++;
    g_cmdData[boardNum].cmd = 0xBB;
    g_cmdData[boardNum].data1 = hton16(0x0071); // missile
    g_cmdData[boardNum].data2 = hton16(0x0000);

    // send cmd data through udp
    UdpSendData((const char *)&g_cmdData[boardNum], sizeof(g_cmdData[boardNum]), boardNum);
    // DelayMs(50);

    // read ack data and show in textEdit
    memset(buf, 0x00, sizeof(buf));
    UdpRecvData(buf, sizeof(buf), boardNum);
}

QString ConvertTestInfoData(uint8_t *buf, int len)
{
    QString hexStr;
    for (int cnt = 0; cnt < len; cnt ++) {
        hexStr += QString("%1 ").arg(static_cast<unsigned int>(buf[cnt]), 2, 16, QLatin1Char('0')).toUpper();
    }

    return hexStr;
}

void ParseTestInfo(uint8_t *buf, int len)
{
    TestInfo *info = NULL;
    int i = 0;

    if (len <= 0) {
        return;
    }

    info = (TestInfo *)buf;

    // �?QString 显示�?QLineEdit �?
    // lineEdit->setText(hexString.trimmed());

    int offset = 0;
    g_testInfoWidget[i++].edit->setText(ConvertTestInfoData(buf + offset, sizeof(info->armVersion)).trimmed());
    offset += sizeof(info->armVersion);

    g_testInfoWidget[i++].edit->setText(ConvertTestInfoData(buf + offset, sizeof(info->fpgaVersion)).trimmed());
    offset += sizeof(info->fpgaVersion);

    g_testInfoWidget[i++].edit->setText(ConvertTestInfoData(buf + offset, sizeof(info->curDirect)).trimmed());
    offset += sizeof(info->curDirect);

    g_testInfoWidget[i++].edit->setText(QString::number(info->ddrInitStaus));
    offset += sizeof(info->ddrInitStaus);

    g_testInfoWidget[i++].edit->setText(QString::number(hton32(info->ddrInitTime)));
    offset += sizeof(info->ddrInitTime);

    g_testInfoWidget[i++].edit->setText(QString::number(hton16(info->dviInfo1.width)));
    offset += sizeof(info->dviInfo1.width);

    g_testInfoWidget[i++].edit->setText(QString::number(hton16(info->dviInfo1.height)));
    offset += sizeof(info->dviInfo1.height);

    g_testInfoWidget[i++].edit->setText(QString::number(info->dviInfo1.frameFreq));
    offset += sizeof(info->dviInfo1.frameFreq);

    g_testInfoWidget[i++].edit->setText(QString::number(hton16(info->dviInfo2.width)));
    offset += sizeof(info->dviInfo2.width);

    g_testInfoWidget[i++].edit->setText(QString::number(hton16(info->dviInfo2.height)));
    offset += sizeof(info->dviInfo2.height);

    g_testInfoWidget[i++].edit->setText(QString::number(info->dviInfo2.frameFreq));
    offset += sizeof(info->dviInfo2.frameFreq);

    g_testInfoWidget[i++].edit->setText(QString::number(info->dviRecvDataErrCnt2));
    offset += sizeof(info->dviRecvDataErrCnt2);

    g_testInfoWidget[i++].edit->setText(QString::number(info->dviRecvTimingErrCnt2));
    offset += sizeof(info->dviRecvTimingErrCnt2);

    g_testInfoWidget[i++].edit->setText(QString::number(info->dviRecvDataErrCnt1));
    offset += sizeof(info->dviRecvDataErrCnt1);

    g_testInfoWidget[i++].edit->setText(QString::number(info->dviRecvTimingErrCnt1));
    offset += sizeof(info->dviRecvTimingErrCnt1);

    g_testInfoWidget[i++].edit->setText(QString::number(hton16(info->sdiInfo1.width)));
    offset += sizeof(info->sdiInfo1.width);

    g_testInfoWidget[i++].edit->setText(QString::number(hton16(info->sdiInfo1.height)));
    offset += sizeof(info->sdiInfo1.height);

    g_testInfoWidget[i++].edit->setText(QString::number(info->sdiInfo1.frameFreq));
    offset += sizeof(info->sdiInfo1.frameFreq);

    g_testInfoWidget[i++].edit->setText(QString::number(hton16(info->sdiInfo2.width)));
    offset += sizeof(info->sdiInfo2.width);

    g_testInfoWidget[i++].edit->setText(QString::number(hton16(info->sdiInfo2.height)));
    offset += sizeof(info->sdiInfo2.height);

    g_testInfoWidget[i++].edit->setText(QString::number(info->sdiInfo2.frameFreq));
    offset += sizeof(info->sdiInfo2.frameFreq);

    g_testInfoWidget[i++].edit->setText(QString::number(info->srioInitStatus));
    offset += sizeof(info->srioInitStatus);

    g_testInfoWidget[i++].edit->setText(QString::number(hton32(info->videoSize)));
    g_testInfoWidget[i++].edit->setText(QString::number(info->videoType));
    // g_testInfoWidget[i++].edit->setText(QString::number(info->reserve));
    offset += sizeof(info->videoInfo);

    g_testInfoWidget[i++].edit->setText(QString::number(info->isForwardVideoReq));
    g_testInfoWidget[i++].edit->setText(QString::number(info->isBackwardVideoReq));
    g_testInfoWidget[i++].edit->setText(QString::number(info->isPanoramaVideoReq));
    offset += sizeof(info->videoReq);

    g_testInfoWidget[i++].edit->setText(ConvertTestInfoData(buf + offset, sizeof(info->srioForwardVideoAddrMax)).trimmed());
    offset += sizeof(info->srioForwardVideoAddrMax);

    g_testInfoWidget[i++].edit->setText(ConvertTestInfoData(buf + offset, sizeof(info->srioBackwardVideoAddrMax)).trimmed());
    offset += sizeof(info->srioBackwardVideoAddrMax);

    g_testInfoWidget[i++].edit->setText(ConvertTestInfoData(buf + offset, sizeof(info->srioPanoramaVideoAddrMax)).trimmed());
    offset += sizeof(info->srioPanoramaVideoAddrMax);

    g_testInfoWidget[i++].edit->setText(QString::number(info->srioSendToMainBoardBellFreq));
    g_testInfoWidget[i++].edit->setText(QString::number(info->srioSendToHelmetBellFreq));
    g_testInfoWidget[i++].edit->setText(QString::number(info->srioRecvVideoFrameFreqChn1));
    g_testInfoWidget[i++].edit->setText(QString::number(info->srioRecvVideoFrameFreqChn0));
    offset += sizeof(info->srioInfo);

    g_testInfoWidget[i++].edit->setText(QString::number(info->gtxStatus));
    offset += sizeof(info->gtxStatus);

    g_testInfoWidget[i++].edit->setText(QString::number(hton16(info->preProcessHeartbeat)));
    offset += sizeof(info->preProcessHeartbeat);

    g_testInfoWidget[i++].edit->setText(QString::number(hton32(info->preProcessVersion)));
    offset += sizeof(info->preProcessVersion);

    g_testInfoWidget[i++].edit->setText(QString::number(hton16(info->preProcessChnStatus)));
    offset += sizeof(info->preProcessChnStatus);

    g_testInfoWidget[i++].edit->setText(QString::number(hton32(info->guideDirect)));
    offset += sizeof(info->guideDirect);

    g_testInfoWidget[i++].edit->setText(QString::number(info->recognizeCtrl));
    offset += sizeof(info->recognizeCtrl);

    g_testInfoWidget[i++].edit->setText(QString::number(info->cmdCnt));
    offset += sizeof(info->cmdCnt);

    g_testInfoWidget[i++].edit->setText(ConvertTestInfoData(buf + offset, sizeof(info->recvCmd)).trimmed());
    offset += sizeof(info->recvCmd);

    g_testInfoWidget[i++].edit->setText(ConvertTestInfoData(buf + offset, sizeof(info->distributedVersion1)).trimmed());
    offset += sizeof(info->distributedVersion1);

    g_testInfoWidget[i++].edit->setText(ConvertTestInfoData(buf + offset, sizeof(info->distributedVersion2to8)).trimmed());
}

void GetTestInfoBtnClickedEvent(QObject *sender, int idx, int boardNum)
{
    uint8_t buf[1024] = {0};
    int recvLen = 0;
    // g_cmdData[boardNum].cnt ++;
    uint8_t sendBuf[] = {0x6D, 0x65, 0x73, 0x73, 0x61, 0x67, 0x65}; // message

    // send cmd data through udp
    UdpSendData((const char *)sendBuf, sizeof(sendBuf), boardNum);
    // DelayMs(50);

    // read ack data and show in textEdit
    memset(buf, 0x00, sizeof(buf));
    recvLen = UdpRecvData(buf, sizeof(buf), boardNum);

    // parse test info
    ParseTestInfo(buf, recvLen);
}

void HexBtnClickedEvent(QRadioButton *hexBtn, QLineEdit *inputEdit)
{
    // if ascii button is selected, change the content in input edit to hex
    QString input = inputEdit->text().toUtf8().toHex().toUpper();
    QString outStr;

    for (int i = 0; i < input.length(); i += 2) {
        if (i + 2 <= input.length()) {
            outStr += input.mid(i, 2) + " ";
        } else {
            outStr += input.mid(i);
        }
    }

    inputEdit->setText(outStr);
}

void AsciiBtnClickedEvent(QRadioButton *asciiBtn, QLineEdit *inputEdit)
{
    // if hex button is selected, change the content in input edit to ascii
    QString hexStr = inputEdit->text();
    QByteArray byteArray = QByteArray::fromHex(hexStr.replace(" ", "").toLatin1());

    // convert QByteArray to QString
    QString data = QString::fromUtf8(byteArray);

    inputEdit->setText(data);
}

void SendBtnClickedEvent(QObject *sendBtn, QRadioButton *hexBtn, QLineEdit *inputEdit, int boardNum)
{
    // 添加空指针检查
    if (!hexBtn || !inputEdit) {
        qDebug() << "SendBtnClickedEvent: hexBtn or inputEdit is null";
        return;
    }

    qDebug() << "SendBtnClickedEvent: starting...";

    uint8_t buf[1024];
    int len = 0;
    char hexStr[128];

    g_sendMsgMutex.lock();
    memset(hexStr, 0x00, sizeof(hexStr));

    QString inputText = inputEdit->text().trimmed();
    if (inputText.isEmpty()) {
        qDebug() << "SendBtnClickedEvent: input text is empty";
        g_sendMsgMutex.unlock();
        return;
    }

    qDebug() << "SendBtnClickedEvent: input text =" << inputText;

    if (hexBtn->isChecked()) {
        // 十六进制模式
        QString cleanHex = inputText.replace(" ", "");
        if (cleanHex.length() % 2 != 0) {
            qDebug() << "SendBtnClickedEvent: invalid hex string length";
            g_sendMsgMutex.unlock();
            return;
        }
        QByteArray byteArray = QByteArray::fromHex(cleanHex.toLatin1());
        len = byteArray.length();
        if (len > sizeof(hexStr) - 1) {
            len = sizeof(hexStr) - 1;
        }
        memcpy(hexStr, byteArray.data(), len);
        qDebug() << "SendBtnClickedEvent: hex mode, len =" << len;
    } else {
        // ASCII模式
        QByteArray utf8Data = inputText.toUtf8();
        len = utf8Data.length();
        if (len > sizeof(hexStr) - 1) {
            len = sizeof(hexStr) - 1;
        }
        memcpy(hexStr, utf8Data.data(), len);
        qDebug() << "SendBtnClickedEvent: ascii mode, len =" << len;
    }
    g_sendMsgMutex.unlock();

    if (len > 0) {
        qDebug() << "SendBtnClickedEvent: about to send data, len =" << len;

        // 恢复UdpSendData调用，继续注释UdpRecvData
    UdpSendData(hexStr, len, boardNum);

        // 恢复UdpRecvData调用
        qDebug() << "SendBtnClickedEvent: about to receive data";
    memset(buf, 0x00, sizeof(buf));
    UdpRecvData(buf, sizeof(buf), boardNum);

        qDebug() << "SendBtnClickedEvent: data processing completed successfully";
    }
}

void posEditXReturnPressedEvent(QLineEdit *posEditX, int boardNum)
{
    // check touch or direction guide button is pressed
    if (! g_cmdBtnInfo[boardNum][12].btn->styleSheet().isEmpty()) {
        TouchDisplayBtnClickedEvent(NULL, 12, boardNum);
    }

    if (! g_cmdBtnInfo[boardNum][14].btn->styleSheet().isEmpty()) {
        DirectGuideBtnClickedEvent(NULL, 14, boardNum);
    }
}

void posEditYReturnPressedEvent(QLineEdit *posEditY, int boardNum)
{
    // check touch or direction guide button is pressed
    if (! g_cmdBtnInfo[boardNum][12].btn->styleSheet().isEmpty()) {
        TouchDisplayBtnClickedEvent(NULL, 12, boardNum);
    }

    if (! g_cmdBtnInfo[boardNum][14].btn->styleSheet().isEmpty()) {
        DirectGuideBtnClickedEvent(NULL, 14, boardNum);
    }
}

// 自适应布局函数
void UpdateTestInfoLayout(QWidget *pageNetwork, int zeroPointX, int zeroPointY, int w, int btnRowNum, int btnColNum)
{
    // 使用应用程序全局字体，而不是硬编码字体
    QFont testInfoFont = getApplicationFont(9);

    int spaceW = 10, spaceH = 5, bottomH = 60;
    int testInfoLabelH = (pageNetwork->height() - zeroPointY - bottomH) * 2 / (sizeof(g_testInfoWidget) / sizeof(g_testInfoWidget[0])) - 5;
    int testInfoEditW = 120; // 固定输入框宽度

    zeroPointX += (w + spaceW) * btnColNum + spaceW;

    // 计算可用宽度和布局策略
    int availableWidth = pageNetwork->width() - zeroPointX - 20; // 留出20像素边距
    int totalItems = sizeof(g_testInfoWidget) / sizeof(g_testInfoWidget[0]);

    // 根据可用宽度决定布局：单列或双列
    bool useTwoColumns = (availableWidth > 600); // 如果可用宽度大于600，使用双列布局

    if (useTwoColumns) {
        // 双列布局
        int itemsPerColumn = totalItems / 2;
        int columnWidth = (availableWidth - spaceW) / 2;
        int labelWidth = columnWidth - testInfoEditW - spaceW;

        // 第一列
        for (uint32_t i = 0; i < itemsPerColumn; i++) {
            if (g_testInfoWidget[i].label && g_testInfoWidget[i].edit) {
                g_testInfoWidget[i].label->setGeometry(zeroPointX, zeroPointY + (spaceH + testInfoLabelH) * i, labelWidth, testInfoLabelH);
                g_testInfoWidget[i].edit->setGeometry(zeroPointX + labelWidth + spaceW, zeroPointY + (spaceH + testInfoLabelH) * i, testInfoEditW, testInfoLabelH);
            }
        }

        // 第二列
        int secondColumnX = zeroPointX + columnWidth + spaceW;
        for (uint32_t i = itemsPerColumn; i < totalItems; i++) {
            if (g_testInfoWidget[i].label && g_testInfoWidget[i].edit) {
                g_testInfoWidget[i].label->setGeometry(secondColumnX, zeroPointY + (spaceH + testInfoLabelH) * (i - itemsPerColumn), labelWidth, testInfoLabelH);
                g_testInfoWidget[i].edit->setGeometry(secondColumnX + labelWidth + spaceW, zeroPointY + (spaceH + testInfoLabelH) * (i - itemsPerColumn), testInfoEditW, testInfoLabelH);
            }
        }
    } else {
        // 单列布局
        int labelWidth = availableWidth - testInfoEditW - spaceW;

        for (uint32_t i = 0; i < totalItems; i++) {
            if (g_testInfoWidget[i].label && g_testInfoWidget[i].edit) {
                g_testInfoWidget[i].label->setGeometry(zeroPointX, zeroPointY + (spaceH + testInfoLabelH) * i, labelWidth, testInfoLabelH);
                g_testInfoWidget[i].edit->setGeometry(zeroPointX + labelWidth + spaceW, zeroPointY + (spaceH + testInfoLabelH) * i, testInfoEditW, testInfoLabelH);
            }
        }
    }
}

void CreateTestInfoWidgets(QWidget *pageNetwork, int zeroPointX, int zeroPointY, int w, int btnRowNum, int btnColNum)
{
    // 使用应用程序全局字体，而不是硬编码字体
    QFont testInfoFont = getApplicationFont(9);
    int btnW = w;
    int spaceW = 10, spaceH = 5, bottomH = 60;
    int testInfoLabelH = (pageNetwork->height() - zeroPointY - bottomH) * 2 / (sizeof(g_testInfoWidget) / sizeof(g_testInfoWidget[0])) - 5;
    int testInfoEditW = 120; // 固定输入框宽度

    zeroPointX += (btnW + spaceW) * btnColNum + spaceW;

    // 计算可用宽度和布局策略
    int availableWidth = pageNetwork->width() - zeroPointX - 20; // 留出20像素边距
    int totalItems = sizeof(g_testInfoWidget) / sizeof(g_testInfoWidget[0]);

    // 根据可用宽度决定布局：单列或双列
    bool useTwoColumns = (availableWidth > 600); // 如果可用宽度大于600，使用双列布局

    if (useTwoColumns) {
        // 双列布局
        int itemsPerColumn = totalItems / 2;
        int columnWidth = (availableWidth - spaceW) / 2;
        int labelWidth = columnWidth - testInfoEditW - spaceW;

        // 第一列
        for (uint32_t i = 0; i < itemsPerColumn; i++) {
        g_testInfoWidget[i].label = new QLabel(pageNetwork);
        g_testInfoWidget[i].edit = new QLineEdit(pageNetwork);

            g_testInfoWidget[i].label->setGeometry(zeroPointX, zeroPointY + (spaceH + testInfoLabelH) * i, labelWidth, testInfoLabelH);
            g_testInfoWidget[i].edit->setGeometry(zeroPointX + labelWidth + spaceW, zeroPointY + (spaceH + testInfoLabelH) * i, testInfoEditW, testInfoLabelH);

        g_testInfoWidget[i].label->setText(g_testInfoLabelName[i]);
        g_testInfoWidget[i].label->setFont(testInfoFont);
        g_testInfoWidget[i].label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            g_testInfoWidget[i].label->setStyleSheet("QLabel { color: #FFFFFF !important; background: transparent !important; }");

        g_testInfoWidget[i].edit->setFont(testInfoFont);
            g_testInfoWidget[i].edit->setStyleSheet("QLineEdit { color: #FFFFFF !important; background: transparent !important; border: 1px solid #888 !important; }");
        }

        // 第二列
        int secondColumnX = zeroPointX + columnWidth + spaceW;
        for (uint32_t i = itemsPerColumn; i < totalItems; i++) {
        g_testInfoWidget[i].label = new QLabel(pageNetwork);
        g_testInfoWidget[i].edit = new QLineEdit(pageNetwork);

            g_testInfoWidget[i].label->setGeometry(secondColumnX, zeroPointY + (spaceH + testInfoLabelH) * (i - itemsPerColumn), labelWidth, testInfoLabelH);
            g_testInfoWidget[i].edit->setGeometry(secondColumnX + labelWidth + spaceW, zeroPointY + (spaceH + testInfoLabelH) * (i - itemsPerColumn), testInfoEditW, testInfoLabelH);

        g_testInfoWidget[i].label->setText(g_testInfoLabelName[i]);
        g_testInfoWidget[i].label->setFont(testInfoFont);
            g_testInfoWidget[i].label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            g_testInfoWidget[i].label->setStyleSheet("QLabel { color: #FFFFFF !important; background: transparent !important; }");

        g_testInfoWidget[i].edit->setFont(testInfoFont);
            g_testInfoWidget[i].edit->setStyleSheet("QLineEdit { color: #FFFFFF !important; background: transparent !important; border: 1px solid #888 !important; }");
        }
    } else {
        // 单列布局
        int labelWidth = availableWidth - testInfoEditW - spaceW;

        for (uint32_t i = 0; i < totalItems; i++) {
            g_testInfoWidget[i].label = new QLabel(pageNetwork);
            g_testInfoWidget[i].edit = new QLineEdit(pageNetwork);

            g_testInfoWidget[i].label->setGeometry(zeroPointX, zeroPointY + (spaceH + testInfoLabelH) * i, labelWidth, testInfoLabelH);
            g_testInfoWidget[i].edit->setGeometry(zeroPointX + labelWidth + spaceW, zeroPointY + (spaceH + testInfoLabelH) * i, testInfoEditW, testInfoLabelH);

            g_testInfoWidget[i].label->setText(g_testInfoLabelName[i]);
            g_testInfoWidget[i].label->setFont(testInfoFont);
            g_testInfoWidget[i].label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            g_testInfoWidget[i].label->setStyleSheet("QLabel { color: #FFFFFF !important; background: transparent !important; }");

            g_testInfoWidget[i].edit->setFont(testInfoFont);
            g_testInfoWidget[i].edit->setStyleSheet("QLineEdit { color: #FFFFFF !important; background: transparent !important; border: 1px solid #888 !important; }");
        }
    }
}

void PowerSwitchBtnValueChangedEvent(void)
{
    if (g_powerSwitch == NULL)
        return;

    int value = g_powerSwitch->checked() == true ? 1 : 0;

    // send data through uart
    SendWriteRegBuf(g_regs[0].addr, value);

    // sync data to page 1 and 2
    SyncDataToPage1(0, value);
    SyncDataToPage2(0, value);
}

void GpioSwitchBtnValueChangedEvent(void)
{
    if (g_gpioSwitch == NULL)
        return;

    qDebug() << "GpioSwitchBtnValueChangedEvent";

    int value = g_gpioSwitch->checked() == true ? 3 : 4;

    // power off
    SendWriteRegBuf(g_regs[0].addr, 0);
    g_powerSwitch->setChecked(false);
    DelayMs(20);

    // send data through uart
    SendWriteRegBuf(g_regs[1].addr, value);

    // delay 1s
    DelayMs(300);

    // power on
    // SendWriteRegBuf(g_regs[0].addr, 1);
    // g_powerSwitch->setChecked(true);

    // sync data to page 1 and 2
    SyncDataToPage1(1, value);
    SyncDataToPage2(1, value);
    SyncDataToPage1(0, 0);
    SyncDataToPage2(0, 0);
}

void CreateSwitch(QWidget *pageNetwork, int zeroPointX, int zeroPointY, QFont &font)
{
    int switchW = 100, switchH = 40, spaceW = 10, labelW = 50;
    g_powerSwitch = new SwitchButton(pageNetwork);
    g_gpioSwitch = new SwitchButton(pageNetwork);
    QLabel *powerLabel = new QLabel(pageNetwork);
    QLabel *gpioLabel = new QLabel(pageNetwork);

    // 使用应用程序全局字体，而不是硬编码字体
    QFont switchFont = getApplicationFont(10);

    powerLabel->setGeometry(zeroPointX, zeroPointY, labelW + 20, switchH);
    powerLabel->setText("POWER");
    powerLabel->setFont(switchFont);
    powerLabel->setStyleSheet("QLabel { color: #FFFFFF !important; background: transparent !important; }");
    g_powerSwitch->setGeometry(zeroPointX + labelW + 20, zeroPointY, switchW, switchH);
    g_powerSwitch->setFont(switchFont);

    gpioLabel->setGeometry(zeroPointX + (switchW + labelW + spaceW + 140), zeroPointY, labelW, switchH);
    gpioLabel->setText("GPIO");
    gpioLabel->setFont(switchFont);
    gpioLabel->setStyleSheet("QLabel { color: #FFFFFF !important; background: transparent !important; }");
    g_gpioSwitch->setGeometry(zeroPointX + (labelW * 2 + spaceW) + switchW + 140, zeroPointY, switchW, switchH);
    g_gpioSwitch->setFont(switchFont);
    g_gpioSwitch->setTextOn("槽位2");
    g_gpioSwitch->setTextOff("槽位1");
    g_gpioSwitch->setBgColorOff(QColor(71, 126, 229));

    // slot
    QObject::connect(g_powerSwitch, &SwitchButton::statusChanged, []() {
        PowerSwitchBtnValueChangedEvent();
    });
    QObject::connect(g_gpioSwitch, &SwitchButton::statusChanged, []() {
        GpioSwitchBtnValueChangedEvent();
    });

    g_powerSwitch->setTextColor(QColor(255,255,255));
    g_gpioSwitch->setTextColor(QColor(255,255,255));
}

void CreateNetworkWidgets(QWidget *pageNetwork, int boardNum)
{
    // 使用应用程序全局字体，而不是硬编码字体
    QFont font = getApplicationFont(10);
    QFont radioFont = getApplicationFont(9);
    const int btnRowNum = 4;
    int btnColNum = (NETWORK_CMD_NUM_MAX + btnRowNum - 1) / btnRowNum;

    // 主布局：左功能区+右参数区
    QHBoxLayout* mainLayout = new QHBoxLayout(pageNetwork);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(30, 20, 30, 20);

    // 左侧功能区
    QVBoxLayout* leftLayout = new QVBoxLayout();
    leftLayout->setSpacing(15);

    // 1. 顶部开关区
    QHBoxLayout* switchLayout = new QHBoxLayout();
    QLabel *powerLabel = new QLabel("POWER", pageNetwork);
    powerLabel->setFont(font);
    powerLabel->setStyleSheet("QLabel { color: #FFFFFF !important; background: transparent !important; font-size: 16px; }");
    g_powerSwitch = new SwitchButton(pageNetwork);
    //g_powerSwitch->setFont(font);
    g_powerSwitch->setShowText(true);
    g_powerSwitch->setFixedSize(120, 40);
    g_powerSwitch->setTextColor(QColor(255,255,255));
    g_powerSwitch->setStyleSheet("SwitchButton { background: transparent !important;font-size: 16px; }");
    QLabel *gpioLabel = new QLabel("GPIO", pageNetwork);
    gpioLabel->setFont(font);
    gpioLabel->setStyleSheet("QLabel { color: #FFFFFF !important; background: transparent !important; font-size: 16px; }");
    g_gpioSwitch = new SwitchButton(pageNetwork);
    //g_gpioSwitch->setFont(font);
    g_gpioSwitch->setShowText(true);
    g_gpioSwitch->setFixedSize(120, 40);
    g_gpioSwitch->setStyleSheet("SwitchButton { background: transparent !important;font-size: 16px; }");
    g_powerSwitch->setTextOn("打开");
    g_powerSwitch->setTextOff("关闭");
    g_gpioSwitch->setTextOn("槽位2");
    g_gpioSwitch->setTextOff("槽位1");
    g_gpioSwitch->setTextColor(QColor(255,255,255));
    switchLayout->addWidget(powerLabel);
    switchLayout->addWidget(g_powerSwitch);
    switchLayout->addSpacing(30);
    switchLayout->addWidget(gpioLabel);
    switchLayout->addWidget(g_gpioSwitch);
    switchLayout->addStretch();
    leftLayout->addLayout(switchLayout);



    QObject::connect(g_powerSwitch, &SwitchButton::statusChanged, []() { PowerSwitchBtnValueChangedEvent(); });
    QObject::connect(g_gpioSwitch, &SwitchButton::statusChanged, []() { GpioSwitchBtnValueChangedEvent(); });

    // 2. 按钮区（QGridLayout）
    QGridLayout* btnGrid = new QGridLayout();
    btnGrid->setSpacing(10);
    // 使用应用程序全局字体，而不是硬编码字体
    QFont btnFont = getApplicationFont();
    btnFont.setPixelSize(16);
    for (int idx = 0; idx < NETWORK_CMD_NUM_MAX; ++idx) {
        QPushButton* btn = new QPushButton(g_cmdBtnInfo[boardNum][idx].name, pageNetwork);
        btn->setFont(btnFont);
        btn->setStyleSheet(
            "QPushButton { color: #FFFFFF; background: transparent; border: 1px solid #888; min-height: 25px; }"
            "QPushButton:pressed, QPushButton:checked { color: #FFFFFF; background: #333; border: 1px solid #888; }"
        );
        btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        btnGrid->addWidget(btn, idx / btnColNum, idx % btnColNum);
        g_cmdBtnInfo[boardNum][idx].btn = btn;
        QObject::connect(btn, &QPushButton::clicked, [idx, boardNum]() {
            if (g_cmdBtnInfo[boardNum][idx].clickedEventCb) {
                g_cmdBtnInfo[boardNum][idx].clickedEventCb((QObject*)&g_cmdBtnInfo[boardNum][idx], idx, boardNum);
    }
        });
    }
    // 对每一列设置拉伸因子
    for (int col = 0; col < btnColNum; ++col) {
        btnGrid->setColumnStretch(col, 1);
    }
    leftLayout->addLayout(btnGrid);

    // 3. 信息区（横纵坐标）
    QLabel* labelX = new QLabel("横", pageNetwork);
    QLineEdit* posEditX = new QLineEdit(pageNetwork);
    QLabel* labelY = new QLabel("纵", pageNetwork);
    QLineEdit* posEditY = new QLineEdit(pageNetwork);
    labelX->setFont(font);
    labelX->setStyleSheet("QLabel { color: #FFFFFF !important; "
                          "background: transparent !important; "
                          "font-size: 16px; }");
    posEditX->setFont(font);
    posEditX->setStyleSheet("QLineEdit { color: #FFFFFF !important; "
                            "background: transparent !important; "
                            "border: 1px solid #888 !important; "
                            "font-size: 16px;"
                            "min-height: 25px; }");
    posEditX->setMaximumWidth(85);
    labelY->setFont(font);
    labelY->setStyleSheet("QLabel { color: #FFFFFF !important; "
                          "background: transparent !important; "
                          "font-size: 16px; }");
    posEditY->setFont(font);
    posEditY->setMaximumWidth(85);
    posEditY->setStyleSheet("QLineEdit { color: #FFFFFF !important; "
                            "background: transparent !important; "
                            "border: 1px solid #888 !important; "
                            "font-size: 16px;"
                            "min-height: 25px }");
    g_posEditX[boardNum] = posEditX;
    g_posEditY[boardNum] = posEditY;

    // 横 [输入框] 行
    QWidget* xRow = new QWidget(pageNetwork);
    QHBoxLayout* xRowLayout = new QHBoxLayout(xRow);
    xRowLayout->setContentsMargins(0,0,0,0);
    xRowLayout->setSpacing(2);
    xRowLayout->addWidget(labelX);
    xRowLayout->addWidget(posEditX);
    xRow->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
    // 纵 [输入框] 行
    QWidget* yRow = new QWidget(pageNetwork);
    QHBoxLayout* yRowLayout = new QHBoxLayout(yRow);
    yRowLayout->setContentsMargins(0,0,0,0);
    yRowLayout->setSpacing(2);
    yRowLayout->addWidget(labelY);
    yRowLayout->addWidget(posEditY);
    yRow->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
    int colTest = 3; // 测试信息按钮所在列
    int colGuide = 5; // 方位引导按钮所在列
    btnGrid->addWidget(xRow, btnRowNum-1, colTest, 1, 1, Qt::AlignLeft);
    btnGrid->addWidget(yRow, btnRowNum-1, colGuide, 1, 1, Qt::AlignLeft);

    xRow->setMaximumWidth(labelX->sizeHint().width() + posEditX->sizeHint().width() + 8);
    yRow->setMaximumWidth(labelY->sizeHint().width() + posEditY->sizeHint().width() + 8);

    // 用QWidget+QHBoxLayout包裹横纵控件，横向紧凑排列
    QWidget* infoRow = new QWidget(pageNetwork);
    QHBoxLayout* infoHBox = new QHBoxLayout(infoRow);
    infoHBox->setContentsMargins(0,0,0,0);
    infoHBox->addWidget(labelX);
    infoHBox->addSpacing(35);
    infoHBox->addWidget(posEditX);
    infoHBox->addSpacing(5);
    infoHBox->addWidget(labelY);
    infoHBox->addSpacing(35);
    infoHBox->addWidget(posEditY);
    infoRow->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
    btnGrid->addWidget(infoRow, btnRowNum-1, colTest, 1, 2, Qt::AlignLeft);
    infoRow->setStyleSheet("QWidget {color: #FFFFFF !important;"
                           " font-size: 16px;"
                           " background: transparent !important;}");

    // 4. 单选按钮区
    QHBoxLayout* radioLayout = new QHBoxLayout();
    QRadioButton* hexBtn = new QRadioButton("Hex", pageNetwork);
    QRadioButton* asciiBtn = new QRadioButton("Ascii", pageNetwork);
    hexBtn->setFont(radioFont);
    asciiBtn->setFont(radioFont);
    hexBtn->setStyleSheet("QRadioButton { color: #FFFFFF !important; background: transparent !important; font-size: 16px; }");
    asciiBtn->setStyleSheet("QRadioButton { color: #FFFFFF !important; background: transparent !important; font-size: 16px; }");
    asciiBtn->setChecked(true);
    radioLayout->addWidget(hexBtn);
    radioLayout->addWidget(asciiBtn);
    radioLayout->addStretch();
    leftLayout->addLayout(radioLayout);

    // 5. 输入区和发送按钮
    QHBoxLayout* inputLayout = new QHBoxLayout();
    QLineEdit* inputEdit = new QLineEdit(pageNetwork);
    inputEdit->setFont(font);
    inputEdit->setStyleSheet("QLineEdit { color: #FFFFFF !important; "
                             "background: transparent !important; "
                             "font-size: 16px; "
                             "border: 1px solid #888 !important;"
                             "min-height: 25px; }");
    QPushButton* sendBtn = new QPushButton("发送", pageNetwork);
    sendBtn->setFont(btnFont);
    sendBtn->setStyleSheet(
        "QPushButton { color: #FFFFFF; background: transparent; border: 1px solid #888; min-height: 25px; }"
        "QPushButton:pressed, QPushButton:checked { color: #FFFFFF; background: #333; border: 1px solid #888; }"
    );
    sendBtn->setMinimumWidth(40);
    sendBtn->setMinimumHeight(20);
    inputLayout->addWidget(inputEdit);
    inputLayout->addWidget(sendBtn);
    leftLayout->addLayout(inputLayout);

    // 6. 下方文本编辑区
    QTextEdit* textEdit = new QTextEdit(pageNetwork);
    textEdit->setFont(font);
    textEdit->setStyleSheet("QTextEdit { color: #FFFFFF !important; "
                            "font-size: 16px; border: 1px solid #888 !important; "
                            "background-position: center !important;"
                            "background-attachment: fixed !important;}");

    leftLayout->addWidget(textEdit, 1);
    g_textEdit[boardNum] = textEdit;

    // 信号槽
    QObject::connect(hexBtn, &QPushButton::clicked, [hexBtn, inputEdit]() { HexBtnClickedEvent(hexBtn, inputEdit); });
    QObject::connect(asciiBtn, &QPushButton::clicked, [asciiBtn, inputEdit]() { AsciiBtnClickedEvent(asciiBtn, inputEdit); });
    QObject::connect(sendBtn, &QPushButton::clicked, [sendBtn, hexBtn, inputEdit, boardNum]() { SendBtnClickedEvent((QObject*)sendBtn, hexBtn, inputEdit, boardNum); });

    // 右侧参数/测试信息区分为两列
    QGridLayout* rightGrid = new QGridLayout();
    rightGrid->setSpacing(15);    //行距
    // 使用应用程序全局字体，而不是硬编码字体
    QFont testInfoFont = getApplicationFont(35);
    int rowCount = 22;
    for (int i = 0; i < 44; ++i) {
        int row = i % rowCount;
        int col = i / rowCount * 2; // 0或2
        rightGrid->addWidget(g_testInfoWidget[i].label = new QLabel(g_testInfoLabelName[i], pageNetwork), row, col);
        g_testInfoWidget[i].label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        g_testInfoWidget[i].label->setStyleSheet("QLabel { color: #FFFFFF !important; "
                                                 "font-size: 16px; "
                                                 "background: transparent !important; }");
        rightGrid->addWidget(g_testInfoWidget[i].edit = new QLineEdit(pageNetwork), row, col + 1);
        g_testInfoWidget[i].edit->setFont(testInfoFont);
        g_testInfoWidget[i].edit->setStyleSheet("QLineEdit { color: #FFFFFF !important; "
                                                "font-size: 16px; "
                                                "background: transparent !important; "
                                                "border: 1px solid #888 !important;"
                                                "min-height: 25px; }");
    }

    QVBoxLayout* rightLayout = new QVBoxLayout();
    rightLayout->addLayout(rightGrid);
    rightLayout->addStretch();
    mainLayout->addLayout(leftLayout, 3); // 左侧占3份
    mainLayout->addLayout(rightLayout, 3); // 右侧占3份
    pageNetwork->setLayout(mainLayout);

}
#endif
