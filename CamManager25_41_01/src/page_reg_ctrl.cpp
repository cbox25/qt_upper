#include <stdio.h>
#include <stdint.h>
#include <cmath>
#include <string>
#include <cstdint>
#include <QApplication>
#include <QObject>
#include <QComboBox>
#include <QtSerialPort/QSerialPort>
#include <QSerialPortInfo>
#include <QDebug>
#include <QStyledItemDelegate>
#include <QMutex>
#include <QElapsedTimer>
#include <QRadioButton>
#include <QToolTip>
#include <../base_widgets/switch_button.h>
#include "page_reg_ctrl.h"
#include "qcheckbox.h"
#include "uart.h"
#include "common.h"
#include "page_update.h"
#include "init_main_window.h"
#include "../QLogger/QLogger.h"
#include <QDateTime>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include "update_increase.h"
#include <QTextCodec>

Reg g_regs[REG_NUM_MAX];
UiRegWidgetsInfo g_regWidgetsInfo[REG_CTRL_PAGE_NUM] = { {nullptr} }; // Explicitly initialize to nullptr
QMutex g_uartMutex;
extern int g_tabWidgetIndex;
int g_regCnt = 0;
bool g_versionParse = false; // Record whether the version number is read and parsed when the serial port number remains unchanged

#if defined(PRJ201)
    void *g_pageWidgets2[REG_NUM_MAX];
    extern SwitchButton *g_powerSwitch;
    extern SwitchButton *g_gpioSwitch;
    QCheckBox *g_gtxChkBox[8];
#endif

#if defined(PRJ201)
typedef struct {
    int value;
    QString text;
} ComboBoxData;

ComboBoxData g_vpxGa[2] = {
    {4, "图像处理卡1"},
    {3, "图像处理卡2"}
};

ComboBoxData g_srioImgFormat[4] = {
    {0, "1600*1200"},
    {1, "768*576"},
    {3, "1280*1024"},
    {4, "720*576"}
};

ComboBoxData g_dviMask[2] = {
    {0, "1600*1200掩码"},
    {3, "1920*1080掩码"}
};

ComboBoxData g_dviDataChoose[3] = {
    {0, "SRIO与GTX数据"},
    {3, "测试数据"},
    {24, "SDI数据"}
};

UiLabelData g_labelData[2][READ_ONLY_DATA_NUM_MAX] = {
    {
        {NULL, NULL, NULL, "门铃数据",     0x44A00128, ""},
        {NULL, NULL, NULL, "nwrite数据", 0x44A0012C, ""},
        {NULL, NULL, NULL, "版本号",     0x44A00800, ""}
    },

    {
        {NULL, NULL, NULL, "门铃数据",     0x44A00128, ""},
        {NULL, NULL, NULL, "nwrite数据", 0x44A0012C, ""},
        {NULL, NULL, NULL, "版本号",     0x44A00800, ""}
    }
};
#elif defined(PRJ_CIOMP)
UiLabelData g_labelData[READ_ONLY_DATA_NUM_MAX] = {
    // label, lineEdit, name, addr, data
    {NULL, NULL, NULL, "温度", 0x44A209F4, ""},
    {NULL, NULL, NULL, "图像均值", 0x44A20A08, ""}
};
#elif defined(INTERNAL)
UiLabelData g_labelData[READ_ONLY_DATA_NUM_MAX] = {
    /* label, lineEdit, name, addr, data */
    {NULL, NULL, NULL, "温度 ℃", 0x44A209F4, "0"},
    {NULL, NULL, NULL, "k", 0x00, "0"},
    {NULL, NULL, NULL, "b", 0x00, "0"},
    {NULL, NULL, NULL, "图像均值", 0x44A20A08, "0"}
};
#endif

Reg::Reg()
{
    this->name = "";
    this->addr = 0;
    this->value = 0;
    this->defaultValue = 0;
    this->max = 0;
    this->min = 0;
}

Reg::Reg(const QString name, uint32_t addr, uint32_t defaultValue, uint32_t max, uint32_t min)
{
    this->name = name;
    this->addr = addr;
    this->value = defaultValue;
    this->defaultValue = defaultValue;
    this->max = max;
    this->min = min;
}

UiRegWidget::UiRegWidget()
{
    label = nullptr;
    lineEdit = nullptr;
#ifdef REG_CTRL_SLIDER
    slider = nullptr;
#endif
}

void UiRegWidget::UiSetRegCtrl(const QString &name, uint32_t defaultValue)
{
    if (label) {
        label->setText(name);
    }
    if (lineEdit) {
        QString hexStr = QString("0x%1").arg(defaultValue, 8, 16, QLatin1Char('0')).toUpper();
        hexStr.replace("0X", "0x");
        lineEdit->setText(hexStr);
    }

    qDebug() << "UiSetRegCtrl: name=" << name << ", defaultValue=" << defaultValue
             << ", label=" << (label ? "valid" : "null")
             << ", lineEdit=" << (lineEdit ? "valid" : "null");
}

int UiRegWidget::GetRegValue()
{
    if (!lineEdit) {
        qDebug() << "GetRegValue: lineEdit is null";
        return 0;
    }

    QString text = lineEdit->text().trimmed();
    qDebug() << "GetRegValue: text=" << text;

    if (text.startsWith("0x") || text.startsWith("0X")) {
        int value = text.mid(2).toUInt(nullptr, 16);
        qDebug() << "GetRegValue: hex value=" << value;
        return value;
    } else {
        int value = text.toInt();
        qDebug() << "GetRegValue: decimal value=" << value;
        return value;
    }
}

int IsEmptyLine(char *line)
{
    while (*line) {
        if (!isspace(*(unsigned char *)line)) {
            return 0; // not empty line
        }
        line ++;
    }

    return 1; // empty line
}

void ParseLine(const QString& line)
{
    const int COMMA_NUM = 9;

    if (g_regCnt >= REG_NUM_MAX) {
        qDebug() << "Error: regCnt exceeded REG_NUM_MAX:" << g_regCnt << ", REG_NUM_MAX:" << REG_NUM_MAX;
        return;
    }

    // Find comma positions
    QVector<int> commaPos;
    int pos = 0;
    for (const QChar& ch : line) {
        if (ch == ',') {
            if (commaPos.size() < COMMA_NUM) {
                commaPos.append(pos);
            }
        } else if (ch == '\r' || ch == '\n') {
            break;
        }
        pos++;
    }

    if (commaPos.size() < COMMA_NUM) {
        qDebug() << "Error: Insufficient commas in line:" << line;
        return;
    }

    // Assign name
    QString nameStr = line.left(commaPos[0]).trimmed();
    if (nameStr.isEmpty()) {
        qDebug() << "Error: Empty name in line:" << line;
        return;
    }
    g_regs[g_regCnt].name = nameStr;

    // Assign addr
    QString addrStr = line.mid(commaPos[0] + 1, commaPos[1] - commaPos[0] - 1).trimmed();
    if (addrStr.isEmpty() || !addrStr.startsWith("0x")) {
        qDebug() << "Error: Invalid addr format:" << addrStr;
        return;
    }
    g_regs[g_regCnt].addr = addrStr.mid(2).toUInt(nullptr, 16);

    // Assign max
    QString maxStr = line.mid(commaPos[1] + 1, commaPos[2] - commaPos[1] - 1).trimmed();
    if (maxStr.isEmpty()) {
        qDebug() << "Error: Empty max value:" << line;
        return;
    }
    g_regs[g_regCnt].max = maxStr.toUInt();

    // Assign min
    QString minStr = line.mid(commaPos[2] + 1, commaPos[3] - commaPos[2] - 1).trimmed();
    if (minStr.isEmpty()) {
        qDebug() << "Error: Empty min value:" << line;
        return;
    }
    g_regs[g_regCnt].min = minStr.toUInt();

    // Assign default
    QString defaultStr = line.mid(commaPos[3] + 1, commaPos[4] - commaPos[3] - 1).trimmed();
    if (defaultStr.isEmpty()) {
        qDebug() << "Error: Empty default value:" << line;
        return;
    }
    g_regs[g_regCnt].defaultValue = defaultStr.toUInt();

    // Assign value
    QString valueStr = line.mid(commaPos[4] + 1, commaPos[5] - commaPos[4] - 1).trimmed();
    if (valueStr.isEmpty()) {
        qDebug() << "Error: Empty value:" << line;
        return;
    }
    g_regs[g_regCnt].value = valueStr.toUInt();

    // Assign save
    QString saveStr = line.mid(commaPos[5] + 1, commaPos[6] - commaPos[5] - 1).trimmed();
    if (saveStr.isEmpty()) {
        qDebug() << "Error: Empty save value:" << line;
        return;
    }
    g_regs[g_regCnt].save = saveStr.toUInt();

    // Assign visible
    QString visibleStr = line.mid(commaPos[6] + 1, commaPos[7] - commaPos[6] - 1).trimmed();
    if (visibleStr.isEmpty()) {
        qDebug() << "Error: Empty visible value:" << line;
        return;
    }
    g_regs[g_regCnt].visible = visibleStr.toUInt();

    // Assign delay
    QString delayStr = line.mid(commaPos[7] + 1, commaPos[8] - commaPos[7] - 1).trimmed();
    if (delayStr.isEmpty()) {
        qDebug() << "Error: Empty delay value:" << line;
        return;
    }
    g_regs[g_regCnt].delay = delayStr.toUInt();

    // Assign tips
    QString tipText = line.mid(commaPos[8] + 1).trimmed();

    if (tipText.startsWith('"') && tipText.endsWith('"')) {
        tipText = tipText.mid(1, tipText.length() - 2);
    }
    g_regs[g_regCnt].tipText = tipText;

    g_regCnt++;
}

int ReadRegFile(const QString& filename)
{
    g_regCnt = 0;
    QString fullPath = QDir(QCoreApplication::applicationDirPath()).filePath(filename);

    QFile file(fullPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Error: Cannot open" << fullPath << ", error:" << file.errorString();
        return -1;
    }

    QTextStream textFile(&file);
    textFile.setCodec("GBK");

    int lineCnt = 0;
    while (!textFile.atEnd()) {
        QString line = textFile.readLine().trimmed();
        if (line.isEmpty()) {
            qDebug() << "Skipping empty line in reg.csv";
            continue;
        }
        lineCnt++;
        if (lineCnt != 1) {
            ParseLine(line);
        }
    }
    file.close();

    return g_regCnt;
}

void SendWriteRegBuf(uint32_t addr, uint32_t value)
{
    uint8_t buf[UART_PACKET_LEN_MAX];
    QSerialPort *serialPort = &g_serialPort;

    memset(buf, 0x00, sizeof(buf));
    // make buf
    int len = MakeUartPacket(OPERA_REG_WRITE, addr, value, 0, buf);

    qDebug() << "a:" << QString::number(addr, 16) << "v:" << value;

    // send buf
    UartWriteBuf(serialPort, (const char *)buf, len);
}

#if defined(PRJ201)
void SyncDataToPage1(int i, int value)
{
    qDebug() << "SyncDataToPage1";

    if (!g_regs[i].visible)
        return;

// set slider
#ifdef REG_CTRL_SLIDER
    float t = (float)(value - g_regs[i].min) * 100 / (g_regs[i].max - g_regs[i].min) + 0.5; // round
    g_regWidgetsInfo[0].regWidgets[i].slider->setValue((int)t);
#endif

    // set line edit
    // convert regValue to hex string
    if (g_regWidgetsInfo[0].hexBtn->isChecked()) {
        // hex
        QString hexStr = QString("0x%1").arg(value, 8, 16, QLatin1Char('0')).toUpper();
        hexStr.replace("0X", "0x");
        g_regWidgetsInfo[0].regWidgets[i].lineEdit->setText(hexStr);
    } else {
        // decimal
        g_regWidgetsInfo[0].regWidgets[i].lineEdit->setText(QString::number(value));
    }
}

void SyncDataToPage2(int i, int regValue)
{
    // sync reg value to page 2 and 4

    qDebug() << "SyncDataToPage2";

    QComboBox *comBox = NULL;
    if (i == 0 || i == 2 || i == 13 || i == 15) {
        // switch button
        SwitchButton *swBtn = (SwitchButton *)g_pageWidgets2[i];
        bool chked = (regValue == 0) ? false : true;
        swBtn->setChecked(chked);
    } else {
        // combobox
        switch (i) {
        case 1:
            comBox = (QComboBox *)g_pageWidgets2[i];
            for (int j = 0; j < sizeof(g_vpxGa) / sizeof(g_vpxGa[0]); j ++) {
                if (regValue == g_vpxGa[j].value) {
                    comBox->setCurrentIndex(j);
                }
            }
            break;
        case 3:
            comBox = (QComboBox *)g_pageWidgets2[i];
            for (int j = 0; j < sizeof(g_srioImgFormat) / sizeof(g_srioImgFormat[0]); j ++) {
                if (regValue == g_srioImgFormat[j].value) {
                    comBox->setCurrentIndex(j);
                }
            }
            break;
        case 11:
            comBox = (QComboBox *)g_pageWidgets2[i];
            for (int j = 0; j < sizeof(g_dviMask) / sizeof(g_dviMask[0]); j ++) {
                if (regValue == g_dviMask[j].value) {
                    comBox->setCurrentIndex(j);
                }
            }
            break;
        case 12:
            comBox = (QComboBox *)g_pageWidgets2[i];
            for (int j = 0; j < sizeof(g_dviDataChoose) / sizeof(g_dviDataChoose[0]); j ++) {
                if (regValue == g_dviDataChoose[j].value) {
                    comBox->setCurrentIndex(j);
                }
            }
            break;
        default:
            break;
        };
    }
}

void SyncDataToPage4(int idx, int regValue)
{
    qDebug() << "SyncDataToPage4";

    if (idx == 0) {
        bool chked = (regValue == 0) ? false : true;
        g_powerSwitch->setChecked(chked);
    } else if (idx == 1) {
        bool chked = (regValue == 4) ? false : true;
        g_gpioSwitch->setChecked(chked);
    } else {
        ;
    }
}
#endif

#ifdef REG_CTRL_SLIDER
void SliderReleasedEvent(QObject *sender)
{
    uint32_t value = 0;
    uint32_t regValue = 0;
    int pos = 0;
    uint64_t t = 0;
    float step = 0.0;
    QSlider *slider = qobject_cast<QSlider *>(sender);
    UiRegWidgetsInfo *uiRegWidgets = NULL;
    uiRegWidgets = &g_regWidgetsInfo[g_tabWidgetIndex];

    if (! slider) {
        return;
    }

    value = slider->sliderPosition();

    for (int i = 0; i < uiRegWidgets->regTotalNum; i ++) {
        if (sender == uiRegWidgets->regWidgets[i].slider) {
            step = 100 / (float)(g_regs[i].max - g_regs[i].min);
            pos = round((float)value / step) * step;
            slider->setValue(pos);

            // convert slider value to reg value
            t = (uint64_t)pos * (uint64_t)(g_regs[i].max - g_regs[i].min);
            regValue = (uint32_t)((t + 50) / 100 + (uint64_t)g_regs[i].min);

            // convert regValue to hex string
            if (uiRegWidgets->hexBtn->isChecked()) {
                // hex
                QString hexStr = QString("0x%1").arg(regValue, 8, 16, QLatin1Char('0')).toUpper();
                hexStr.replace("0X", "0x");
                uiRegWidgets->regWidgets[i].lineEdit->setText(hexStr);
            } else {
                // decimal
                uiRegWidgets->regWidgets[i].lineEdit->setText(QString::number(regValue));
            }

            // send buf
            SendWriteRegBuf(g_regs[i].addr, regValue);
#if defined(PRJ201)
            SyncDataToPage2(i, regValue);
            SyncDataToPage4(i, regValue);
            if (i == 1) {
                DelayMs(20);
                SendWriteRegBuf(g_regs[0].addr, 0);
                SyncDataToPage1(0, 0);
                SyncDataToPage2(0, 0);
                SyncDataToPage4(0, 0);
            }
#endif
            break;
        }
    }
}
#endif

void LineEditReturnPressedEvent(QObject *sender, const QString &hexStr)
{
    uint32_t value = 0;
    QLineEdit *lineEdit = qobject_cast<QLineEdit *>(sender);
    UiRegWidgetsInfo *uiRegWidgets = NULL;
    uiRegWidgets = &g_regWidgetsInfo[g_tabWidgetIndex];

    if (! lineEdit) {
        return;
    }

    // convert QString to const char *
    g_uartMutex.lock();
    const char *str = lineEdit->text().toStdString().c_str();
    g_uartMutex.unlock();

    if (lineEdit->text().startsWith("0x")) {
        // hex
        str += 2; // ignore "0x"
        value = strtoul(str, NULL, 16);
    } else {
        // decimal
        value = strtoul(str, NULL, 10);
    }

    // set slider position
    for (int i = 0; i < uiRegWidgets->regTotalNum; i ++) {
        if (sender == uiRegWidgets->regWidgets[i].lineEdit) {
            // check the max and min
            value = value <= g_regs[i].max ? value : g_regs[i].max;
            value = value >= g_regs[i].min ? value : g_regs[i].min;

#ifdef REG_CTRL_SLIDER
            if (g_tabWidgetIndex == TAB_PAGE_REG) {
                // set slider value
                float t = (float)(value - g_regs[i].min) * 100 / (g_regs[i].max - g_regs[i].min) + 0.5; // round
                uiRegWidgets->regWidgets[i].slider->setValue((int)t);
            }
#endif

            // send buf
            SendWriteRegBuf(g_regs[i].addr, value);

            // Record the adjustment results to the log file
            QString logMessage;
            if (uiRegWidgets->hexBtn->isChecked()) {
                logMessage = QString("调整寄存器: 名称=%4, 地址=0x%5, 新值=0x%6")
                                 .arg(g_regs[i].name)
                                 .arg(g_regs[i].addr, 8, 16, QLatin1Char('0')).toUpper()
                                 .arg(value, 8, 16, QLatin1Char('0')).toUpper();
            } else {
                logMessage = QString("调整寄存器: 名称=%4, 地址=0x%5, 新值=%6")
                                 .arg(g_regs[i].name)
                                 .arg(g_regs[i].addr, 8, 16, QLatin1Char('0')).toUpper()
                                 .arg(value);
            }
            QLog_Debug("CamManager", logMessage);

#if defined(PRJ201)
            SyncDataToPage2(i, value);
            SyncDataToPage4(i, value);
            if (i == 1) {
                DelayMs(20);
                SendWriteRegBuf(g_regs[0].addr, 0);
                SyncDataToPage1(0, 0);
                SyncDataToPage2(0, 0);
                SyncDataToPage4(0, 0);
            }
#endif

            break;
        }
    }
}

void SyncComboBoxIndex(int index)
{
    UiRegWidgetsInfo *uiRegWidgets = NULL, *uiRegWidgets2 = NULL;
    uiRegWidgets = &g_regWidgetsInfo[0];
#if defined(PRJ201)
    uiRegWidgets2 = &g_regWidgetsInfo[1];
#endif

    if (g_updateWidgets.uartComBox != NULL) {
        g_updateWidgets.uartComBox->setCurrentIndex(index);
    }

    if (uiRegWidgets->comboBox != NULL) {
        uiRegWidgets->comboBox->setCurrentIndex(index);
    }

#if defined(PRJ201)
    if (uiRegWidgets2->comboBox != NULL) {
        uiRegWidgets2->comboBox->setCurrentIndex(index);
    }
#endif
}

void UartComboBoxActivatedEvent(QComboBox* comboBox, const QString &text)
{
    QSerialPort *serialPort = &g_serialPort;
    static QString lastText = "关闭";
    int ret = 0;
    uint16_t pktNum = 0, regNum = 0;

    if (text == lastText) {
        return;
    }
    g_versionParse = false;
    g_fpgaVersion = "";
    g_softKernelVersion = "";

    if (lastText != "关闭") {
        // close the last uart and delete
        qDebug() << "close serial port: " << serialPort->portName();
        serialPort->close();
    }

    lastText = text;

    if (text != "关闭") {
        ConfigureSerialPort(serialPort, 115200, text);
        if (OpenSerialPort(serialPort)) { // open serial port
            qDebug() << "Serial port opened successfully" << serialPort->portName() << comboBox->currentIndex();
            QLog_Debug("CamManager", "Serial port opened successfully");
        } else {
            qDebug() << "Failed to open serial port" << serialPort->portName() << comboBox->currentIndex();
            QLog_Debug("CamManager", "Serial port opened Failed");
        }
        SyncComboBoxIndex(comboBox->currentIndex());

        if ((ret = SendCmdVersionRead()) < 0) {
            debugNoQuote() << "read softkernel & fpga Version error";
        } else {
            g_versionParse = true;
        }

        if ((ret = ReadCsvCmd(&pktNum)) < 0) {
            debugNoQuote() << "read csv sheet error";
            return;
        }

        if ((ret = ReadCsvData(pktNum, &regNum)) < 0) {
            debugNoQuote() << "read csv sheet error";
            return;
        }

        debugNoQuote() << "read csv sheet successfully";

        // Add security checks
        if (g_tabWidgetIndex >= REG_CTRL_PAGE_NUM || !uiInitialized) {
            return;
        }

        UiRegWidgetsInfo *uiRegWidgets = &g_regWidgetsInfo[g_tabWidgetIndex];
        QWidget *scroll = nullptr;

        // Ensure the page is valid
        if (!g_stackedWidget || g_stackedWidget->count() <= g_tabWidgetIndex) {
            return;
        }

        if (g_tabWidgetIndex == TAB_PAGE_REG) { // Get the current scroll area
            scroll = g_scrollArea ? g_scrollArea->widget() : nullptr;
        } else {
            scroll = g_stackedWidget->widget(g_tabWidgetIndex);
        }

        if (!scroll || !uiRegWidgets) {
            return;
        }

        int labelX = 0, labelY = 65; // Calculate the appropriate position
        RefreshRegList(uiRegWidgets, scroll, regNum, labelX, labelY); // Refresh the register list
    } else {
        SyncComboBoxIndex(comboBox->currentIndex());
    }
}

QComboBox *CreateComboBox(QWidget *parent, int x, int y, int w, int h, QFont &font)
{
    QComboBox *comboBox = new QComboBox(parent);
    comboBox->setGeometry(x, y, w, h);
    const auto serialPortInfos = QSerialPortInfo::availablePorts();
    // clear QComboBox
    comboBox->clear();
    // traversal all of the uarts and add to QComboBox
    comboBox->addItem("关闭");
    for (const QSerialPortInfo &serialPortInfo : serialPortInfos) {
        comboBox->addItem(serialPortInfo.portName());
    }
    comboBox->setFont(font);
    comboBox->setMinimumHeight(40);
    comboBox->setStyleSheet("QComboBox { background: transparent !important; color: #FFFFFF; border: 2px solid #888; }"
                            "QComboBox QAbstractItemView { background: transparent !important; color: #FFFFFF; border: 2px solid #888; }");

    // slot
    QObject::connect(comboBox, QOverload<const QString &>::of(&QComboBox::activated), [comboBox](const QString &text) {
        UartComboBoxActivatedEvent(comboBox, comboBox->currentText());
    });

    return comboBox;
}

QString hexToBytes(uint8_t *buf, uint32_t len) { // Calculate the size of the array
    // Convert the uint8 t array to std::string
    QString str = QString::fromUtf8(reinterpret_cast<const char*>(buf), len);

    return str;
}

int CheckReadRegAckBuf(uint8_t *buf, int len, uint8_t *regDataBuf)
{
    uint8_t *p = NULL;
    int i = 0;

    p = buf;

    for (i = 0; i < len; i ++) {
        if (*(p + i) == ((UART_PACKET_HEAD >> 8) & 0xFF) && *(p + i + 1) == (UART_PACKET_HEAD & 0xFF)) {
            p += i;
            break;
        }
    }

    // exceed the receive buf
    if (i + 10 > len)
        return -1;

    uint16_t dataLen = *(uint16_t *)(p + 8);

    opera_reg_reply_t *op_pkt = (opera_reg_reply_t *)(p + 10);

    if (op_pkt->op != OPERA_REG_READ) {
        return -2;
    }

    if (op_pkt->err_code != 0) {
        return op_pkt->err_code;
    }

    int opLen = op_pkt->len;

    memcpy(regDataBuf, op_pkt->data, opLen);

    return opLen;
}

int ReadReg(uint32_t addr, int readLen, const char *readBuf)
{
    uint8_t buf[UART_PACKET_LEN_MAX];
    QSerialPort *serialPort = &g_serialPort;
    int readUartLen = 0;
    int ret = 0;
    QElapsedTimer timer;

    // make buf
    int sendLen = MakeUartPacket(OPERA_REG_READ, addr, 0, readLen, buf);
    // send buf
    UartWriteBuf(serialPort, (const char *)buf, sendLen);

    PrintBuf((const char *)buf, sendLen, "ReadReg:");

    // read buf
    memset(buf, 0x00, sizeof(buf));
    timer.start();
    do {
        readUartLen = UartReadBuf(serialPort, (const char *)buf);
        ret = CheckReadRegAckBuf(buf, readUartLen, (uint8_t *)readBuf);
        if (ret < 0) {
            DelayMs(50);
        }
    } while (ret < 0 && timer.elapsed() < 1000);

    return readUartLen;
}

void ParseSheetPacket(uint8_t *buf, uint32_t filesize, int *regnum)
{
    *regnum = 0;

    if (buf == nullptr || filesize == 0) {
        return;
    }

    QByteArray data(reinterpret_cast<const char*>(buf), filesize);
    QTextStream stream(&data);
    stream.setCodec("UTF-8");
    QString line;
    bool isHeader = true;
    int index = 0;

    while (stream.readLineInto(&line) && index < REG_NUM_MAX) {
        // Skip blank lines
        if (line.isEmpty()) {
            continue;
        }

        // Skip header
        if (isHeader) {
            // Check the header format
            if (line.startsWith("NAME,ADDR,MAX,MIN,DEFAULT,VALUE,SAVE,VISIBLE,DELAY,TIPS")) {
                isHeader = false;
            }
            continue;
        }

        // Separated by commas
        QStringList fields = line.split(',');
        if (fields.size() < 10) {
            qDebug() << "Skipping incomplete line:" << line;
            continue; // Skip the rows with incomplete fields
        }

        Reg &reg = g_regs[index];
        bool parseOk = true;

        reg.name = fields[0].trimmed();
        if (reg.name.isEmpty()) {
            qDebug() << "Empty name field in line:" << line;
            parseOk = false;
        }

        // Parsing address (hexadecimal starting with 0x)
        QString addrStr = fields[1].trimmed();
        bool ok;
        if (addrStr.startsWith("0x", Qt::CaseInsensitive)) {
            reg.addr = addrStr.toUInt(&ok, 16);
            if (!ok) {
                qDebug() << "Invalid hex address format:" << addrStr << "in line:" << line;
                parseOk = false;
            }
        } else {
            reg.addr = addrStr.toUInt(&ok);
            if (!ok) {
                qDebug() << "Invalid decimal address format:" << addrStr << "in line:" << line;
                parseOk = false;
            }
        }

        QString maxStr = fields[2].trimmed();
        if (maxStr.startsWith("0x", Qt::CaseInsensitive)) {
            reg.max = maxStr.toUInt(&ok, 16);
            if (!ok) {
                qDebug() << "Invalid hex max value format:" << maxStr << "in line:" << line;
                parseOk = false;
            }
        } else {
            reg.max = maxStr.toUInt(&ok);
            if (!ok) {
                qDebug() << "Invalid decimal max value format:" << maxStr << "in line:" << line;
                parseOk = false;
            }
        }

        QString minStr = fields[3].trimmed();
        if (minStr.startsWith("0x", Qt::CaseInsensitive)) {
            reg.min = minStr.toUInt(&ok, 16);
            if (!ok) {
                qDebug() << "Invalid hex min value format:" << minStr << "in line:" << line;
                parseOk = false;
            }
        } else {
            reg.min = minStr.toUInt(&ok);
            if (!ok) {
                qDebug() << "Invalid decimal min value format:" << minStr << "in line:" << line;
                parseOk = false;
            }
        }

        QString defaultStr = fields[4].trimmed();
        if (defaultStr.startsWith("0x", Qt::CaseInsensitive)) {
            reg.defaultValue = defaultStr.toUInt(&ok, 16);
            if (!ok) {
                qDebug() << "Invalid hex default value format:" << defaultStr << "in line:" << line;
                parseOk = false;
            }
        } else {
            reg.defaultValue = defaultStr.toUInt(&ok);
            if (!ok) {
                qDebug() << "Invalid decimal default value format:" << defaultStr << "in line:" << line;
                parseOk = false;
            }
        }

        QString valueStr = fields[5].trimmed();
        if (valueStr.startsWith("0x", Qt::CaseInsensitive)) {
            reg.value = valueStr.toUInt(&ok, 16);
            if (!ok) {
                qDebug() << "Invalid hex value format:" << valueStr << "in line:" << line;
                parseOk = false;
            }
        } else {
            reg.value = valueStr.toUInt(&ok);
            if (!ok) {
                qDebug() << "Invalid decimal value format:" << valueStr << "in line:" << line;
                parseOk = false;
            }
        }

        reg.save = static_cast<uint8_t>(fields[6].trimmed().toUInt(&ok));
        if (!ok) {
            qDebug() << "Invalid save field format:" << fields[6] << "in line:" << line;
            parseOk = false;
        }

        reg.visible = static_cast<uint8_t>(fields[7].trimmed().toUInt(&ok));
        if (!ok) {
            qDebug() << "Invalid visible field format:" << fields[7] << "in line:" << line;
            parseOk = false;
        }

        reg.delay = fields[8].trimmed().toUInt(&ok);
        if (!ok) {
            qDebug() << "Invalid delay field format:" << fields[8] << "in line:" << line;
            parseOk = false;
        }

        reg.tipText = fields[9].trimmed();
        for (int i = 10; i < fields.size(); i++) {
            reg.tipText += "," + fields[i];
        }

        // If the parsing is successful, add the index
        if (parseOk) {
            index++;
        } else {
            qDebug() << "Failed to parse line, skipping:" << line;
        }
    }

    *regnum = index;

    qDebug() << "Successfully parsed" << index << "registers";
}

void WriteCsvFileByBufData(const char *filename, const uint8_t *buf, uint32_t filesize)
{
    FILE *file = fopen(filename, "wb"); // Open the file in binary write mode

    if (file == NULL) {
        qDebug() << "Error: Unable to open file" << filename << "for writing.";
        return;
    }

    // Write the buffer content to the file
    size_t bytes_written = fwrite(buf, 1, filesize, file);

    if (bytes_written != filesize) {
        debugNoQuoteNoSpace() << "create scv error";
    } else {
        debugNoQuoteNoSpace() << "create scv successfully";
    }

    fclose(file);
}

void MakeSheetPacket(uint8_t *buf, uint32_t *filesize, int regNum)
{
    char sheetHead[] = "NAME,ADDR,MAX,MIN,DEFAULT,VALUE,SAVE,VISIBLE,DELAY,TIPS";

    // Calculate the total size of the CSV file
    uint32_t totalSize = 0;

    // Add a header row
    totalSize += strlen(sheetHead);
    totalSize += 2; // header row "\r\n"

    // Add data rows
    for (int i = 0; i < regNum; i++) {
        Reg reg = g_regs[i];

        // Calculate the length of each field
        totalSize += reg.name.toUtf8().length();  // NAME
        totalSize += QString::number(reg.max).length();   // MAX
        totalSize += QString::number(reg.min).length();   // MIN
        totalSize += QString::number(reg.defaultValue).length();  // DEFAULT
        totalSize += QString::number(reg.value).length(); // VALUE
        totalSize += QString::number(reg.save).length();  // SAVE
        totalSize += QString::number(reg.visible).length();  // VISIBLE
        totalSize += QString::number(reg.delay).length(); // DELAY
        totalSize += reg.tipText.toUtf8().length();  // TIPS
        totalSize += 9 + 2 + 10; // Commas and the "\r\n" at the end of lines and addr
    }

    *filesize = totalSize;

    if (buf == nullptr) {
        return;
    }

    // Write to the header
    memcpy(buf, sheetHead, strlen(sheetHead));
    buf += strlen(sheetHead);
    memcpy(buf, "\r\n", 2);
    buf += 2;

    // Write data rows
    for (int i = 0; i < REG_NUM_MAX; i++) {
        Reg reg = g_regs[i];

        // NAME
        QByteArray nameBytes = reg.name.toUtf8();
        memcpy(buf, nameBytes.constData(), nameBytes.length());
        buf += nameBytes.length();
        memcpy(buf, ",", 1);
        buf += 1;

        // ADDR
        QString addrHex = QString("0x%1").arg(reg.addr, 8, 16, QLatin1Char('0')).toUpper();
        QByteArray addrBytes = addrHex.toUtf8();
        memcpy(buf, addrBytes.constData(), addrBytes.length());
        buf += addrBytes.length();
        memcpy(buf, ",", 1);
        buf += 1;

        // MAX
        QByteArray maxBytes = QString::number(reg.max).toUtf8();
        memcpy(buf, maxBytes.constData(), maxBytes.length());
        buf += maxBytes.length();
        memcpy(buf, ",", 1);
        buf += 1;

        // MIN
        QByteArray minBytes = QString::number(reg.min).toUtf8();
        memcpy(buf, minBytes.constData(), minBytes.length());
        buf += minBytes.length();
        memcpy(buf, ",", 1);
        buf += 1;

        // DEFAULT
        QByteArray defaultBytes = QString::number(reg.defaultValue).toUtf8();
        memcpy(buf, defaultBytes.constData(), defaultBytes.length());
        buf += defaultBytes.length();
        memcpy(buf, ",", 1);
        buf += 1;

        // VALUE
        QByteArray valueBytes = QString::number(reg.value).toUtf8();
        memcpy(buf, valueBytes.constData(), valueBytes.length());
        buf += valueBytes.length();
        memcpy(buf, ",", 1);
        buf += 1;

        // SAVE
        QByteArray saveBytes = QString::number(reg.save).toUtf8();
        memcpy(buf, saveBytes.constData(), saveBytes.length());
        buf += saveBytes.length();
        memcpy(buf, ",", 1);
        buf += 1;

        // VISIBLE
        QByteArray visibleBytes = QString::number(reg.visible).toUtf8();
        memcpy(buf, visibleBytes.constData(), visibleBytes.length());
        buf += visibleBytes.length();
        memcpy(buf, ",", 1);
        buf += 1;

        // DELAY
        QByteArray delayBytes = QString::number(reg.delay).toUtf8();
        memcpy(buf, delayBytes.constData(), delayBytes.length());
        buf += delayBytes.length();
        memcpy(buf, ",", 1);
        buf += 1;

        // TIPS
        QByteArray tipsBytes = reg.tipText.toUtf8();
        memcpy(buf, tipsBytes.constData(), tipsBytes.length());
        buf += tipsBytes.length();

        // Line end character
        memcpy(buf, "\r\n", 2);
        buf += 2;
    }
}

int ReadCsvCmd(uint16_t *pktNum)
{
    int ret = 0;

    if ((ret = SendCmdSheetRead(&g_serialPort, pktNum)) < 0) {
        qDebug() << "parse response sheet read info error";
        return -1;
    }
    return 0;
}

int ReadCsvData(uint16_t pktNum, uint16_t *regNum)
{
    uint16_t remainNum = pktNum;
    uint16_t last_seq = 0;
    uint8_t* file_space = NULL;
    uint8_t* unzipfile_space = NULL;
    uint8_t readBuf[UART_PACKET_LEN_MAX];
    int readLen = 0;
    uint32_t file_size = 0;
    int upzipfile_size = 0;
    uint32_t file_size_real = 0;
    uint16_t crc16 = 0;
    QElapsedTimer timer;
    uint8_t hash_buf[HASH_BUF_SIZE];
    uint32_t offset = 0;
    uint8_t head_len = 10;
    uint16_t data_len = 0;
    uint8_t crc_len = 2;
    UartPacket packet;

    if ((file_space = (uint8_t *)calloc(SHEET_SIZE_MAX, sizeof(char))) == NULL) {
        QLog_Error("CamManager", QString("[%1] [%2:%3] malloc space failed")
                       .arg(QDateTime::currentDateTime()
                                .toString("yyyy-MM-dd HH:mm:ss.zzz"))
                       .arg(__FILE__).arg(__LINE__));
        SyncUpdateRet(QString("内存分配失败"));
        debugNoQuoteNoSpace() << "malloc space failed";
        debugNoQuote() << "ERROR 1";
        return -1;
    }

    do {
        memset(readBuf, 0x00, sizeof(readBuf)); /* clear buf */

        timer.start();
        do {
            readLen = UartReadBuf(&g_serialPort, (char *)readBuf, 90);
            DelayMs(10);
        } while (readLen == 0 && timer.elapsed() < 2000);

        if (readLen <= 0) {
            QLog_Error("CamManager", QString("[%1] [%2:%3] get uart read data failed")
                           .arg(QDateTime::currentDateTime()
                                    .toString("yyyy-MM-dd HH:mm:ss.zzz"))
                           .arg(__FILE__).arg(__LINE__));
            debugNoQuote() << "ERROR 2" << "readLen" << readLen;
            return -2;
        }
        uint8_t *buf = NULL;
        int i = 0;
        for (i = 0; i < readLen - 1; i ++) {
            if (readBuf[i] == ((UART_PACKET_HEAD >> 8) & 0xFF) && readBuf[i + 1] == (UART_PACKET_HEAD & 0xFF)) {
                buf = readBuf + i;
                break;
            }
        }

        // packet len is not enough
        if (i + 11 > readLen) {
            qDebug() << "len is short:" << readLen << "head offset:" << i;
            debugNoQuote() << "ERROR 3";
            return -3;
        }

        data_len = *(uint16_t *)(buf + 8);
        if (readLen > i + head_len + data_len + crc_len) {
            qDebug() << "ReadCsvData len error";
            debugNoQuote() << "ERROR 4" << "readLen" << readLen;
            return -4;
        }

        memcpy(&packet, buf, head_len + data_len);
        memcpy(&packet.crc, buf + head_len + data_len, crc_len);
        crc16 = calc_crc16(buf, head_len + data_len); /* calc crc16 */
        if (crc16 != packet.crc) {
            PrintBuf((const char *)buf, data_len + 12, "info:");
            qDebug() << "ReadCsvData crc error";
            debugNoQuote() << "ERROR 5" << "readLen" << readLen;
            return -5;
        }

        if (packet.seqNum != last_seq) {
            memcpy(file_space + offset, packet.data, data_len);
        } else {
            qDebug() << "ReadCsvData seqNum error";
            debugNoQuote() << "ERROR 6";
            return -6;
        }
        debugNoQuote() << "CMD_SHEET_READ_DATA_PACKET:" << packet.seqNum;
        PrintBuf((const char *)buf, 10, "head:");
        PrintBuf((const char *)buf + 10, readLen - 12, "data:");
        debugNoQuote() << "CRCcal:" << QString("%1").arg(crc16, 4, 16, QChar('0')).toUpper() << "CRCpkt:" << QString("%1").arg(packet.crc, 4, 16, QChar('0')).toUpper();

        remainNum --;
        offset += data_len;
        last_seq ++;
        file_size_real += data_len;
    } while (remainNum > 0);
    file_size = *(uint32_t *)file_space;
    if (file_size != file_size_real) {
        qDebug() << "ReadCsvData total_len error";
        debugNoQuote() << "ERROR 7";
        return -7;
    }
    if ((unzipfile_space = (uint8_t *)calloc(SHEET_SIZE_MAX, sizeof(char))) == NULL) {
        QLog_Error("CamManager", QString("[%1] [%2:%3] malloc zipspace failed")
                       .arg(QDateTime::currentDateTime()
                                .toString("yyyy-MM-dd HH:mm:ss.zzz"))
                       .arg(__FILE__).arg(__LINE__));
        SyncUpdateRet(QString("内存分配失败"));
        debugNoQuoteNoSpace() << "malloc zipspace failed";
        free(file_space);
        debugNoQuote() << "ERROR 8";
        return -8;
    }
    upzipfile_size = LZ4_decompress_safe((const char *)(file_space + 128), (char *)unzipfile_space, (int)(file_size - 128), (int)SHEET_SIZE_MAX);
    if (upzipfile_size < 0) {
        qDebug() << "ReadCsvData unzip file error";
        free(file_space);
        free(unzipfile_space);
        debugNoQuote() << "ERROR 9";
        return -9;
    }
    free(file_space);

    ParseSheetPacket(unzipfile_space, (uint32_t)upzipfile_size, (int *)regNum);
    free(unzipfile_space);
    return 0;
}

#if defined(PRJ201)
void RefreshBtnClickedEvent(QObject *sender)
{
    uint8_t readBuf[64] = {0};
    uint32_t dbData = 0;
    UiRegWidgetsInfo *uiRegWidgets = NULL;
    uiRegWidgets = &g_regWidgetsInfo[g_tabWidgetIndex];
    // sync data to another page
    int tabIndex = 1 - g_tabWidgetIndex;
    UiRegWidgetsInfo *uiRegWidgets2 = &g_regWidgetsInfo[tabIndex];

    // send cmd of reading regs
    for (int i = 0; i < READ_ONLY_DATA_NUM_MAX - 1; i ++) {
        memset(readBuf, 0x00, sizeof(readBuf));
        int readLen = ReadReg(uiRegWidgets->labelData[i].addr, 4, (const char *)readBuf);

        // set text of reg line edit
        QString dataStr = QString("0x%1").arg(*readBuf, 8, 16, QLatin1Char('0')).toUpper().replace("0X", "0x");
        uiRegWidgets->labelData[i].lineEdit->setText(dataStr);
        uiRegWidgets2->labelData[i].lineEdit->setText(dataStr);

        DelayMs(100);
    }

    memset(readBuf, 0x00, sizeof(readBuf));
    int readLen = ReadReg(uiRegWidgets->labelData[2].addr, 16, (const char *)readBuf);

    // set text of reg line edit
    QString version = hexToBytes(readBuf, 16);
    uiRegWidgets->labelData[2].lineEdit->setText(version);
    uiRegWidgets2->labelData[2].lineEdit->setText(version);
}
#elif defined(PRJ_CIOMP) || defined(INTERNAL)
void RefreshTempBtnClickedEvent(void)
{
    uint8_t readBuf[8] = {0};
    int readLen = 0;
    float k = 0, b = 0;
    UiRegWidgetsInfo *uiRegWidgets = &g_regWidgetsInfo[g_tabWidgetIndex];

    k = uiRegWidgets->labelData[1].lineEdit->text().toFloat();
    b = uiRegWidgets->labelData[2].lineEdit->text().toFloat();

    // read temperature register twice according to FPGA
    // send cmd of reading regs
    memset(readBuf, 0x00, sizeof(readBuf));
    readLen = ReadReg(uiRegWidgets->labelData[0].addr, 4, (const char *)readBuf);

    int t = *(uint32_t *)readBuf;
    float temp = k * (float)t + b;
    uiRegWidgets->labelData[0].lineEdit->setText(QString::number(temp, 'f', 2));

    // Record temperature
    QString logMessage = QString("刷新寄存器: 名称=%1, 地址=0x%2, 新值=%3")
                             .arg(uiRegWidgets->labelData[0].name)
                             .arg(uiRegWidgets->labelData[0].addr, 8, 16, QLatin1Char('0')).toUpper()
                             .arg(QString::number(temp, 'f', 2));
    QLog_Debug("CamManager", logMessage);

    // Record k
    logMessage = QString("刷新寄存器: 名称=%1, 新值=%2")
                     .arg(uiRegWidgets->labelData[1].name)
                     .arg(QString::number(k, 'f', 2));
    QLog_Debug("CamManager", logMessage);

    // Record b
    logMessage = QString("刷新寄存器: 名称=%1, 新值=%2")
                     .arg(uiRegWidgets->labelData[2].name)
                     .arg(QString::number(b, 'f', 2));
    QLog_Debug("CamManager", logMessage);
}

void RefreshImageAvgValueBtnClickedEvent(void)
{
    uint8_t readBuf[8] = {0};
    int readLen = 0, t = 0;
    UiRegWidgetsInfo *uiRegWidgets = &g_regWidgetsInfo[g_tabWidgetIndex];

    // read image average value
    memset(readBuf, 0x00, sizeof(readBuf));
    readLen = ReadReg(uiRegWidgets->labelData[3].addr, 4, (const char *)readBuf);
    if (readLen < 0)
        return;
    t = *(uint32_t *)readBuf;
    uiRegWidgets->labelData[3].lineEdit->setText(QString::number(t));

    // Record the mean value of the image
    QString logMessage = QString("刷新图像均值: 名称=%1, 地址=0x%2, 新值=%3")
                             .arg(uiRegWidgets->labelData[3].name)
                             .arg(uiRegWidgets->labelData[3].addr, 8, 16, QLatin1Char('0')).toUpper()
                             .arg(t);
    QLog_Debug("CamManager", logMessage);
}

void RefreshSheet()
{
    int ret = 0;
    uint8_t* file_space = NULL;
    uint8_t* zipfile_space = NULL;
    uint8_t buf[UART_PACKET_LEN_MAX] = { 0 };
    uint32_t file_size = 0;
    int zipfile_size = 0;
    uint32_t total_size = 0;
    uint16_t filePktNum = 0;
    uint16_t serial_num = 1;
    UartPacket packet;
    uint32_t offset = 0;
    uint16_t data_len = 0, buf_len = 0;
    uint32_t remain = 0;
    uint16_t crc16 = 0;
    QElapsedTimer recvTimer;
    uint8_t hash_buf[HASH_BUF_SIZE];
    uint8_t hash_size = 0;

    MakeSheetPacket(NULL, &file_size, g_regWidgetsInfo[0].regTotalNum);
    if ((file_space = (uint8_t *)calloc(file_size * 2, sizeof(char))) == NULL) {
        QLog_Error("CamManager", QString("[%1] [%2:%3] malloc space failed")
                       .arg(QDateTime::currentDateTime()
                                .toString("yyyy-MM-dd HH:mm:ss.zzz"))
                       .arg(__FILE__).arg(__LINE__));
        SyncUpdateRet(QString("内存分配失败"));
        debugNoQuoteNoSpace() << "malloc space failed";
        return;
    }
    MakeSheetPacket(file_space, &file_size, g_regWidgetsInfo[0].regTotalNum);

    if ((zipfile_space = (uint8_t *)calloc(file_size * 2, sizeof(char))) == NULL) {
        QLog_Error("CamManager", QString("[%1] [%2:%3] malloc zipspace failed")
                       .arg(QDateTime::currentDateTime()
                                .toString("yyyy-MM-dd HH:mm:ss.zzz"))
                       .arg(__FILE__).arg(__LINE__));
        SyncUpdateRet(QString("内存分配失败"));
        debugNoQuoteNoSpace() << "malloc zipspace failed";
        free(file_space);
        return;
    }
    zipfile_size = LZ4_compress_default((const char *)file_space, (char *)(zipfile_space + 128), file_size, file_size * 2 - 128);
    PrintBuf((const char *)(zipfile_space + 128), zipfile_size, "zipfile info: ");

    if (zipfile_size == 0) {
        debugNoQuoteNoSpace() << "compress error";
        free(file_space);
        free(zipfile_space);
        return;
    }
    free(file_space);

    total_size = zipfile_size + 128;
    memcpy(zipfile_space, &total_size, sizeof(total_size));
    calculate_hash(zipfile_space + 128, (uint32_t)zipfile_size, hash_buf, &hash_size);
    memcpy(zipfile_space + 4, hash_buf, hash_size);

    if (total_size % UART_PACKET_LEN_MAX) {
        filePktNum = total_size / PACKET_DATA_SIZE + 1;
    } else {
        filePktNum = total_size / PACKET_DATA_SIZE;
    }

    debugNoQuote() << "filesize info:"<< file_size << zipfile_size << total_size << filePktNum << endl;
    if ((ret = SendCmdSheetWrite(&g_serialPort, filePktNum)) < 0) {
        qDebug() << "parse response sheet write info error";
        return;
    }

    packet.head = hton16(UART_PACKET_HEAD);
    packet.type = CMD;
    packet.compressType = COMPRESS_TYPE_LZ4;
    packet.reserved[0] = 0x00;
    packet.reserved[1] = 0x00;
    remain = total_size;

    do {
        memset(buf, 0x00, sizeof(buf)); /* clear buf */
        data_len = remain >= PACKET_DATA_SIZE - 1 ? PACKET_DATA_SIZE - 1 : remain;
        packet.seqNum = serial_num;
        packet.dataLen = data_len + 1;
        memcpy(packet.data, zipfile_space + offset, data_len);
        buf_len = 10; /* 10 = len of head/type/compress_file/reserved/serial_num/dataLen */
        memcpy(buf, &packet, buf_len);
        memset(buf + 10, CMD_SHEET_WRITE, 1);
        memcpy(buf + 11, packet.data, data_len);

        crc16 = calc_crc16(buf, buf_len + data_len + 1); /* calc crc16 */
        memcpy(buf + buf_len + data_len + 1, &crc16, sizeof(crc16));
        buf_len += sizeof(crc16);

        /* send data by uart */
        UartWriteBuf(&g_serialPort, (const char*)buf, buf_len + data_len + 1);

        debugNoQuote() << "CMD_SHEET_WRITE_DATA_PACKET:" << serial_num;
        PrintBuf((const char *)buf, 10, "head:");
        PrintBuf((const char *)buf + 10, data_len + 1, "data:");
        PrintBuf((const char *)buf + data_len + 11, 2, "crc:");

        recvTimer.start();
        /* wait a specified time so that microblaze responsed data can be get */
        do {
            ret = ParseResponseCmdSheetWriteAck(&g_serialPort); /* wait for response from mcu */
        } while(ret < 0 && recvTimer.elapsed() < 5000);

        if (ret == 0) {
            remain -= data_len;
            offset += data_len;
            serial_num ++;
        } else {
            qDebug() << "parse response data failed, seq: " << serial_num;
            /* if ret < 0, means that packet crc or length is wrong, so delay 1000ms and resend this packet */
        }
    } while (remain > 0);

    free(zipfile_space);
}
#endif

bool WriteCsvFileByRegData(const char *filePathCsv)
{
    static QMutex mutex;
    mutex.lock();

    FILE* fp = fopen(filePathCsv, "w");
    if (fp == NULL) {
        qDebug() << "Failed to open file for writing:" << filePathCsv;
        return false;
    }

    // write csv header
    fprintf(fp, "Name,Address,Value,Default,Max,Min,Save,Visible,Delay,TipText\r\n");

    // Traverse the g regs array and write the data
    for (int i = 0; i < g_regWidgetsInfo[0].regTotalNum; i++) {
        const Reg& reg = g_regs[i];

        // Handle special characters in strings (such as commas and quotation marks)
        QString nameEscaped = reg.name;
        nameEscaped.replace("\"", "\"\""); // Escape double quotes
        if (nameEscaped.contains(",") || nameEscaped.contains("\"")) {
            nameEscaped = "\"" + nameEscaped + "\"";
        }

        QString tipEscaped = reg.tipText;
        tipEscaped.replace("\"", "\"\""); // Escape double quotes
        if (tipEscaped.contains(",") || tipEscaped.contains("\"")) {
            tipEscaped = "\"" + tipEscaped + "\"";
        }

        // Write a line of data
        fprintf(fp, "%s,%u,%u,%u,%u,%u,%u,%u,%u,%s\n",
                nameEscaped.toUtf8().constData(),
                reg.addr,
                reg.value,
                reg.defaultValue,
                reg.max,
                reg.min,
                static_cast<uint32_t>(reg.save),
                static_cast<uint32_t>(reg.visible),
                reg.delay,
                tipEscaped.toUtf8().constData());
    }

    fclose(fp);
    mutex.unlock();

    return true;
}

void WriteRegFile(const char* filePathCsv, const char* filePathTxt, uint8_t *buf)
{
    if (buf == NULL)
        return;

    static QMutex mutex;

    char fileLine[REG_NUM_MAX][LINE_LEN_MAX];
    char line[LINE_LEN_MAX];
    FILE *fp = NULL;
    int pos1 = 0, pos2 = 0, valueCnt = 0, commaCnt = 0, i = 0, lineCnt = 0;
    char data[16];

    // read file
    if ((fp = fopen(filePathCsv, "r")) == NULL) {
        if ((fp = fopen(filePathTxt, "r")) == NULL) {
            qDebug() << "open reg file for read failed";
            return;
        }
    }

    valueCnt = 0;
    while (fgets(line, sizeof(line), fp) != NULL) {
        memset(fileLine[lineCnt], 0x00, sizeof(fileLine[lineCnt]));
        if (lineCnt == 0) {
            strcpy(fileLine[lineCnt], line);
            lineCnt ++;
            continue; // jump first line
        }

        memset(data, 0x00, sizeof(data));

        if (IsEmptyLine(line)) {
            continue;
        }

        // search the 5th and 6th comma
        commaCnt = 0;
        for (i = 0; i < (int)strlen(line); i ++) {
            if (line[i] == ',' || line[i] == '\r' || line[i] == '\n'){
                commaCnt ++;
                if (commaCnt == 5) {
                    pos1 = i;
                }
                if (commaCnt == 6) {
                    pos2 = i;
                    break;
                }
            }
        }

        sprintf(data, "%u", *(uint32_t *)(buf + valueCnt * 4)); // 12: sizeof(addr + value + delay), 4: sizeof(addr)
        strncpy(fileLine[lineCnt], line, pos1 + 1);
        strcat(fileLine[lineCnt], data);
        strcat(fileLine[lineCnt], line + pos2);

        valueCnt ++;
        lineCnt ++;

        memset(line, 0x00, sizeof(line));
    }
    fclose(fp);

    mutex.lock();
    // write file
    if ((fp = fopen(filePathCsv, "w")) == NULL) {
        if ((fp = fopen(filePathTxt, "w")) == NULL) {
            qDebug() << "open reg file for write failed";
            return;
        }
    }

    for (i = 0; i < lineCnt; i ++) {
        fwrite(fileLine[i], strlen(fileLine[i]), 1, fp);
    }

    fclose(fp);
    mutex.unlock();
}

void SaveArgsBtnClickedEvent(QObject *sender)
{
    uint8_t buf[UART_PACKET_DATA_LEN];
    int len = 0;
    uint32_t value = 0;
    UiRegWidgetsInfo *uiRegWidgets = &g_regWidgetsInfo[g_tabWidgetIndex];
    QLineEdit *lineEdit = NULL;

    memset(buf, 0x00, sizeof(buf));

    debugNoQuoteNoSpace() << uiRegWidgets->regTotalNum;

    for (int i = 0; i < uiRegWidgets->regTotalNum; i ++) {
        if (!uiRegWidgets->regWidgets[i].lineEdit)
            continue;
        lineEdit = uiRegWidgets->regWidgets[i].lineEdit;
        if (lineEdit->text().isEmpty()) {
            if (g_regWidgetsInfo[0].hexBtn->isChecked()) {
                QString hexStr;
                hexStr = "0x" + QString::number(g_regs[i].min, 16).toUpper().rightJustified(8, '0');
                lineEdit->setText(hexStr);
            } else {
                lineEdit->setText(QString::number(g_regs[i].min));
            }
        }

        // read addr and value of all of the regs
        if (lineEdit->text().startsWith("0x")) {
            QString valueStr = lineEdit->text();
            const char *str = valueStr.toStdString().c_str();
            str += 2; // ignore "0x"
            value = strtoul(str, NULL, 16);
        } else {
            value = strtoul(lineEdit->text().toStdString().c_str(), NULL, 10);
        }

        value = value < g_regs[i].max ? value : g_regs[i].max;
        value = value > g_regs[i].min ? value : g_regs[i].min;
        g_regs[i].value = value;

        memcpy(buf + len, &g_regs[i].addr, sizeof(g_regs[i].addr));
        len += sizeof(g_regs[i].addr);
        if (g_regs[i].save) {
            memcpy(buf + len, &g_regs[i].defaultValue, sizeof(g_regs[i].defaultValue));
            len += sizeof(g_regs[i].defaultValue);
        } else {
            memcpy(buf + len, &value, sizeof(value));
            len += sizeof(value);
        }
        memcpy(buf + len, &g_regs[i].delay, sizeof(g_regs[i].delay));
        len += sizeof(g_regs[i].delay);
    }

    SendCmdSaveArgs(&g_serialPort, buf, len);
    DelayMs(500);
#ifndef PRJ201
    RefreshSheet();
#endif
}

void RegCtrlHexBtnClickedEvent(QRadioButton *hexBtn)
{
    QLineEdit *edit = NULL;
    QString hexStr;
    const char *decStr = NULL;
    uint32_t decValue = 0;
    UiRegWidgetsInfo *uiRegWidgets = NULL;
    uiRegWidgets = &g_regWidgetsInfo[g_tabWidgetIndex];

    for (int i = 0; i < uiRegWidgets->regTotalNum; i ++) {
#if defined(PRJ201)
        if (g_tabWidgetIndex == TAB_PAGE_REG2 && g_regs[i].max <= 10) {
            continue;
        }
#endif
        if (uiRegWidgets->regWidgets[i].lineEdit) {
            edit = uiRegWidgets->regWidgets[i].lineEdit;
            decStr = edit->text().toStdString().c_str();

            decValue = strtoul(decStr, NULL, 10);
            hexStr = "0x";
            hexStr += QString::number(decValue, 16).toUpper().rightJustified(8, '0');
            edit->setText(hexStr);
        }
    }
}

void RegCtrlDecBtnClickedEvent(QRadioButton *decBtn)
{
    // Switch to decimal mode
    if (g_tabWidgetIndex >= REG_CTRL_PAGE_NUM) {
        qDebug() << "Error: Invalid tab index" << g_tabWidgetIndex;
        return;
    }

    UiRegWidgetsInfo *uiRegWidgets = &g_regWidgetsInfo[g_tabWidgetIndex];
    if (!uiRegWidgets || !uiRegWidgets->regWidgets) {
        qDebug() << "Error: uiRegWidgets or regWidgets is null";
        return;
    }

    // Update the display format of all registers to decimal
    for (int i = 0; i < uiRegWidgets->regTotalNum; i++) {
        if (uiRegWidgets->regWidgets[i].lineEdit) {
            int value = uiRegWidgets->regWidgets[i].GetRegValue();
            uiRegWidgets->regWidgets[i].lineEdit->setText(QString::number(value));
        }
    }

    qDebug() << "Switched to decimal mode";
}

QHBoxLayout* CreateDecSwitchRadioBtn(QWidget *parent, int x, int y)
{
    QHBoxLayout *radioLayout = new QHBoxLayout();

    QRadioButton *hexBtn = new QRadioButton("16进制", parent);
    QRadioButton *decBtn = new QRadioButton("10进制", parent);
    UiRegWidgetsInfo *uiRegWidgets = NULL;
    uiRegWidgets = &g_regWidgetsInfo[g_tabWidgetIndex];

    uiRegWidgets->hexBtn = hexBtn;
    uiRegWidgets->decBtn = decBtn;

    // Use the global font of the application instead of hard-coded fonts
    QFont font = getApplicationFont(12);
    hexBtn->setFont(font);
    decBtn->setFont(font);

    decBtn->setChecked(true); // default use ascii

    // Set the style to make the text color white
    hexBtn->setStyleSheet("QRadioButton { color: #FFFFFF; }");
    decBtn->setStyleSheet("QRadioButton { color: #FFFFFF; }");

    // Add to the layout
    radioLayout->addWidget(hexBtn);
    radioLayout->addWidget(decBtn);
    radioLayout->addStretch();

    // slot
    QObject::connect(hexBtn, &QRadioButton::clicked, [hexBtn]() {
        RegCtrlHexBtnClickedEvent(hexBtn);
    });
    QObject::connect(decBtn, &QRadioButton::clicked, [decBtn]() {
        RegCtrlDecBtnClickedEvent(decBtn);
    });

    return radioLayout;
}

void CreateRegList(QWidget *scroll, UiRegWidgetsInfo *uiRegWidgets, int regNum, int labelX, int labelY)
{
    // Use the global font of the application instead of hard-coded fonts
    QFont font = getApplicationFont(14);

    // Get the left layout
    if (!scroll->layout())
        return;
    QHBoxLayout *mainLayout = qobject_cast<QHBoxLayout*>(scroll->layout());
    if (!mainLayout || mainLayout->count() <= 0)
        return;
    QVBoxLayout *leftLayout = qobject_cast<QVBoxLayout*>(mainLayout->itemAt(0)->layout());
    if (!leftLayout) {
        qDebug() << "Error: Could not find left layout";
        return;
    }

    // Create a horizontal layout for each register
    for (int i = 0; i < regNum; i ++) {
        if (!g_regs[i].visible)
            continue;

        QHBoxLayout *rowLayout = new QHBoxLayout();

        HoverLabel *label = new HoverLabel(scroll);
        QLineEdit *edit = new QLineEdit(scroll);

        // set label
        label->setText(g_regs[i].name);
        label->setMinimumSize(200, 40);
        label->setMaximumSize(200, 40);
        label->setStyleSheet("QLabel { border: 2px solid #4A4A4D; background-color: #141519; color: #FFFFFF; }");
        label->setFont(font);
        label->setTipText(g_regs[i].tipText);

        // Set the input box
        edit->setMinimumSize(200, 40);
        edit->setMaximumSize(300, 40);
        edit->setStyleSheet("QLineEdit { border: 2px solid #4A4A4D; background-color: #141519; color: #FFFFFF; }");
        edit->setFont(font);
        edit->setReadOnly(false);
        edit->setText(QString::number(g_regs[i].value));

        // Add to the row layout
        rowLayout->addWidget(label);
        rowLayout->addWidget(edit);
        rowLayout->addStretch();

        // Save the control reference
        uiRegWidgets->regWidgets[i].label = label;
        uiRegWidgets->regWidgets[i].lineEdit = edit;

        // Connect the signal slot
        QObject::connect(edit, &QLineEdit::returnPressed, [edit]() {
            LineEditReturnPressedEvent(edit, edit->text());
        });

        leftLayout->addLayout(rowLayout);
    }
}

#if defined(PRJ201)
void RegComboBoxActivatedEvent(QComboBox* comboBox, int idx)
{
    int comboIdx = 0;
    int value = 0;
    SwitchButton *swBtn = NULL;

    qDebug() << "RegComboBoxActivatedEvent";
    comboIdx = comboBox->currentIndex();

    switch (idx) {
    case 1:
        value = g_vpxGa[comboIdx].value;
        // power off
        SendWriteRegBuf(g_regs[0].addr, 0);
        swBtn = (SwitchButton *)g_pageWidgets2[0];
        swBtn->setChecked(false);
        SyncDataToPage1(0, 0);
        SyncDataToPage4(0, 0);
        DelayMs(20);
        break;
    case 3:
        value = g_srioImgFormat[comboIdx].value;
        break;
    case 11:
        value = g_dviMask[comboIdx].value;
        break;
    case 12:
        value = g_dviDataChoose[comboIdx].value;
        break;
    default:
        break;
    }

    SendWriteRegBuf(g_regs[idx].addr, value);
    // sync data to other pages
    SyncDataToPage1(idx, value);
    SyncDataToPage4(idx, value);
}

void SwitchBtnValueChangedEvent(SwitchButton *btn, int idx)
{
    int value = btn->checked() == true ? 1 : 0;

    // send data through uart
    SendWriteRegBuf(g_regs[idx].addr, value);

    // sync data to page 1 and 4
    SyncDataToPage1(idx, value);
    SyncDataToPage4(idx, value);
}

void GtxBtnClickedEvent(QPushButton *gtxBtn)
{
    uint32_t addr = 0x44A00304;
    uint32_t value = 0;
    int chkBoxNum = sizeof(g_gtxChkBox) / sizeof(g_gtxChkBox[0]);

    for (int i = 0; i < chkBoxNum; i ++) {
        if (g_gtxChkBox[i]->isChecked()) {
            value |= 1 << i;
        }
    }

    // send value by uart
    SendWriteRegBuf(addr, value);
}

void CreateRegList1(QWidget *scroll, UiRegWidgetsInfo *uiRegWidgets, int regNum, int labelX, int labelY)
{
    int labelW = 200, labelH = 40, spaceW = 30, spaceH = 35;
    // Use the global font of the application instead of hard-coded fonts
    QFont font = getApplicationFont(15);
    SwitchButton *switchBtn = NULL;
    QComboBox *regCombox = NULL;

    // Get the left layout
    QVBoxLayout *leftLayout = nullptr;
    if (scroll->layout()) {
        QHBoxLayout *mainLayout = qobject_cast<QHBoxLayout*>(scroll->layout());
        if (mainLayout && mainLayout->count() > 0) {
            leftLayout = qobject_cast<QVBoxLayout*>(mainLayout->itemAt(0)->layout());
        }
    }
    if (!leftLayout) {
        qDebug() << "Error: Could not find left layout";
        return;
    }
    leftLayout->setContentsMargins(30, 30, 30, 30);
    leftLayout->setSpacing(spaceH);

    for (int i = 0; i < regNum; i ++) {
        // 只保留有 SwitchButton 和 ComboBox 的寄存器
        if (!(i == 0 || i == 1 || i == 2 || i == 3 || i == 11 || i == 12 || i == 13 || i == 15))
            continue;

        HoverLabel *label = new HoverLabel(scroll);
        QHBoxLayout *rowLayout = new QHBoxLayout();
        label->setText(g_regs[i].name);
        label->setFixedSize(labelW, labelH);
        label->setFont(font);
        label->setStyleSheet("QLabel { background: transparent !important; color: #FFF; border: 1px solid #888; border-radius: 6px; padding-left: 8px; }");

        if (i == 0 || i == 2 || i == 13 || i == 15) {
            // create switch
            switchBtn = new SwitchButton(scroll);
            switchBtn->setFont(font);
            switchBtn->setFixedSize(100, labelH);
            switchBtn->setStyleSheet("SwitchButton { background: #222; color: #FFF; border: 1px solid #888; border-radius: 6px; }");
            g_pageWidgets2[i] = switchBtn;
            QObject::connect(switchBtn, &SwitchButton::statusChanged, [switchBtn, i]() {
                SwitchBtnValueChangedEvent(switchBtn, i);
            });
            rowLayout->addWidget(label);
            rowLayout->addWidget(switchBtn);
            rowLayout->addStretch();
        } else if (i == 1 || i == 3 || i == 11 || i == 12) {
            // create comboBox
            regCombox = new QComboBox(scroll);
            regCombox->setFont(font);
            regCombox->setFixedSize(200, labelH);
            regCombox->setStyleSheet("QComboBox { background: transparent !important; color: #FFF; border: 1px solid #888; border-radius: 6px; } QComboBox QAbstractItemView { background: #222; color: #FFF; }");
            if (i == 1) {
                regCombox->addItem(g_vpxGa[0].text);
                regCombox->addItem(g_vpxGa[1].text);
            }
            if (i == 3) {
                regCombox->addItem(g_srioImgFormat[0].text);
                regCombox->addItem(g_srioImgFormat[1].text);
                regCombox->addItem(g_srioImgFormat[2].text);
                regCombox->addItem(g_srioImgFormat[3].text);
            }
            if (i == 11) {
                regCombox->addItem(g_dviMask[0].text);
                regCombox->addItem(g_dviMask[1].text);
            }
            if (i == 12) {
                regCombox->addItem(g_dviDataChoose[0].text);
                regCombox->addItem(g_dviDataChoose[1].text);
                regCombox->addItem(g_dviDataChoose[2].text);
            }
            g_pageWidgets2[i] = regCombox;
            QObject::connect(regCombox, QOverload<const QString &>::of(&QComboBox::activated), [regCombox, i](const QString &text) {
                RegComboBoxActivatedEvent(regCombox, i);
            });
            rowLayout->addWidget(label);
            rowLayout->addWidget(regCombox);
            rowLayout->addStretch();
        }

        uiRegWidgets->regWidgets[i].label = label;
        leftLayout->addLayout(rowLayout);
    }

    // GTX开关区
    QHBoxLayout *gtxRowLayout = new QHBoxLayout();
    QLabel *gtxLabel = new QLabel(scroll);
    gtxLabel->setText("GTX 开关");
    gtxLabel->setFixedSize(labelW, labelH);
    gtxLabel->setFont(font);
    gtxLabel->setStyleSheet("QLabel { background: transparent !important; color: #FFF; border: 1px solid #888; border-radius: 6px; padding-left: 8px; }");
    gtxRowLayout->addWidget(gtxLabel);
    for (int i = 0; i < 8; i ++) {
        g_gtxChkBox[i] = new QCheckBox(scroll);
        g_gtxChkBox[i]->setText(QString::number(i + 1));
        g_gtxChkBox[i]->setStyleSheet("QCheckBox { background: transparent !important; color: #FFF; font-size: 15px; }");
        g_gtxChkBox[i]->setFont(font);
        g_gtxChkBox[i]->setFixedSize(50, labelH);
        gtxRowLayout->addWidget(g_gtxChkBox[i]);
    }
    QPushButton *gtxBtn = new QPushButton(scroll);
    gtxBtn->setText("确定");
    gtxBtn->setFont(font);
    gtxBtn->setFixedSize(labelW / 2, labelH);
    gtxBtn->setStyleSheet("QPushButton { background: transparent !important; color: #FFF; border: 1px solid #888; border-radius: 6px; }");
    QObject::connect(gtxBtn, &QPushButton::clicked, [gtxBtn]() {
        GtxBtnClickedEvent(gtxBtn);
    });
    gtxRowLayout->addWidget(gtxBtn);
    gtxRowLayout->addStretch();
    leftLayout->addLayout(gtxRowLayout);

}
#endif

void RefreshRegList(UiRegWidgetsInfo *uiRegWidgets, QWidget *scroll, int regNum, int labelX, int labelY)
{
    int spaceH = 15, labelH = 40;
    int regNumVisible = 0;

    // Get the left layout
    if (!scroll->layout())
        return;
    QHBoxLayout *mainLayout = qobject_cast<QHBoxLayout*>(scroll->layout());
    if (!mainLayout || mainLayout->count() <= 0)
        return;
    QVBoxLayout *leftLayout = qobject_cast<QVBoxLayout*>(mainLayout->itemAt(0)->layout());
    if (!leftLayout) {
        qDebug() << "Error: Could not find left layout for refresh";
        return;
    }

    // Clear the old regWidgets
    if (uiRegWidgets && uiRegWidgets->regWidgets) {
        for (int i = 0; i < uiRegWidgets->regTotalNum; i++) {
            // Delete the object directly
            if (uiRegWidgets->regWidgets[i].label) {
                delete uiRegWidgets->regWidgets[i].label;
                uiRegWidgets->regWidgets[i].label = nullptr;
            }
            if (uiRegWidgets->regWidgets[i].lineEdit) {
                delete uiRegWidgets->regWidgets[i].lineEdit;
                uiRegWidgets->regWidgets[i].lineEdit = nullptr;
            }
        }
        free(uiRegWidgets->regWidgets);
        uiRegWidgets->regWidgets = nullptr;
    }

    // again malloc memory
    uiRegWidgets->regWidgets = (UiRegWidget *)calloc(regNum, sizeof(UiRegWidget));
    if (!uiRegWidgets->regWidgets) {
        qDebug() << "Error: Failed to allocate memory for regWidgets during refresh";
        return;
    }

    uiRegWidgets->regTotalNum = regNum;

    // Calculate the number of displayed registers
    for (int i = 0; i < regNum; i++) {
        if (g_regs[i].visible) {
            regNumVisible++;
        }
    }

    // Handle the event loop to ensure that all deletion operations are completed
    QApplication::processEvents();

    // Recreate the register list
    if (regNumVisible > 0) {
        if (g_tabWidgetIndex == 0) {
            CreateRegList(scroll, uiRegWidgets, regNum, labelX, labelY);
        }
#if defined(PRJ201)
        else {
            CreateRegList1(scroll, uiRegWidgets, regNum, labelX, labelY);
        }
#endif
    }

    // Update the content of the scrolling area
    if (g_tabWidgetIndex == 0 && g_scrollArea) {
        // Dynamically calculate the height of the scrolling content
        int totalHeight = labelY + (labelH + spaceH) * regNumVisible + spaceH;
        scroll->setMinimumHeight(totalHeight);
        g_scrollArea->viewport()->update();
    }

    // Reprocess the event loop to ensure that all UI updates are completed
    QApplication::processEvents();

    debugNoQuote() << "interface refresh successfully";
    debugNoQuote() << "reg_num:" << regNum << "visible_reg_num:" << regNumVisible;
}

QWidget *CreateScrollWindow(QWidget *page, QScrollArea *scrollArea, int regNum)
{
    // Set the scrolling area size strategy
    scrollArea->setWidgetResizable(true);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    QWidget *scrollContent = new QWidget();
    scrollArea->setWidget(scrollContent);

    return scrollContent;
}

QScrollArea *CreateRegCtrlWidgets(QWidget *pageRegCtrl, int tabNum)
{
    int labelX = 0, labelY = 65, spaceW = 30, spaceH = 15, labelW = 200, labelH = 40;
    int sliderW = 0;
    int editW = 200;
    int comboxW = labelW - 60;
    int regNum = 0, regNumVisible = 0;
    QFont font;
    QScrollArea *scrollArea = nullptr;
    QWidget *scroll = nullptr;
    UiRegWidgetsInfo *uiRegWidgets = nullptr;

    if (tabNum >= REG_CTRL_PAGE_NUM || tabNum < 0) {
        qDebug() << "Error: Invalid tabNum" << tabNum;
        return nullptr;
    }

    if (tabNum == TAB_PAGE_REG) {
        scrollArea = new QScrollArea(pageRegCtrl);
        if (!scrollArea) {
            qDebug() << "Error: Failed to create QScrollArea";
            return nullptr;}
        // Clear the original content
        if (scrollArea->widget()) {
            delete scrollArea->widget();
        }
        // Clear out the old regWidgets memory
        if (uiRegWidgets && uiRegWidgets->regWidgets) {
            free(uiRegWidgets->regWidgets);
            uiRegWidgets->regWidgets = nullptr;
        }
        scroll = CreateScrollWindow(pageRegCtrl, scrollArea, regNum);
        if (!scroll) {
            qDebug() << "Error: Failed to create scroll content";
            delete scrollArea;
            return nullptr;
        }
    } else {
        scroll = pageRegCtrl;
    }

    g_tabWidgetIndex = tabNum;
    uiRegWidgets = &g_regWidgetsInfo[g_tabWidgetIndex];
    if (!uiRegWidgets) {
        qDebug() << "Error: uiRegWidgets is null for tabNum" << tabNum;
        if (scrollArea) delete scrollArea;
        return nullptr;
    }

    regNum = ReadRegFile("reg_default.csv");

    if (regNum <= 0) {
        qDebug() << "Error: Failed to load reg.csv, regNum =" << regNum;
        if (scrollArea) delete scrollArea;
        return nullptr;
    }
    for (int i = 0; i < regNum; i ++) {
        if (g_regs[i].visible) {
            regNumVisible ++;
        }
    }

    debugNoQuote() << "parse total_reg:" << regNum << " visible reg:" << regNumVisible;
    // Use the global font of the application instead of hard-coded fonts
    font = getApplicationFont(14);

    // malloc space for g_uiWidgets including label, slider and line edit
    uiRegWidgets->regWidgets = (UiRegWidget *)calloc(regNum, sizeof(UiRegWidget));

    if (!uiRegWidgets->regWidgets) {
        qDebug() << "Error: Failed to allocate memory for regWidgets";
        if (scrollArea) delete scrollArea;
        return nullptr;
    }

    uiRegWidgets->regTotalNum = regNum;

    if (tabNum == TAB_PAGE_REG) {
        // Create the scroll after regNum is determined
        scroll = CreateScrollWindow(pageRegCtrl, scrollArea, regNumVisible); // 传递正确 regNumVisible
        if (!scroll) {
            qDebug() << "Error: Failed to create scroll content";
            delete scrollArea;
            free(uiRegWidgets->regWidgets);
            return nullptr;
        }
        scroll->setStyleSheet("QWidget { background: transparent !important; }"); // Set the style

        // Dynamically calculate the height of scrollContent
        int totalHeight = labelY + (labelH + spaceH) * regNumVisible + spaceH ;
        scroll->setMinimumHeight(totalHeight);
        scrollArea->setMinimumHeight(pageRegCtrl->height()); // Fix the height of the QScrollArea
        scrollArea->setWidget(scroll); // reset widget
        scrollArea->setStyleSheet(
            "QScrollArea { background: #222; border: none; }"
            "QScrollArea > QWidget > QWidget { background: #222; }"
            "QScrollArea QWidget { background: #222; }"
            "QScrollArea > QViewport { background: #222; }"
            );
    }

#if defined(PRJ201)
    labelX = scroll->width() - labelW - sliderW - editW - comboxW * 2 - spaceW * 5;
#else
    labelX = scroll->width() - 130 - labelW - sliderW - editW - comboxW - spaceW * 5;
#endif
    labelX /= 2;

    // Create the main horizontal layout (register list on the left, controls on the right)
    QHBoxLayout *mainLayout = new QHBoxLayout(scroll);
    mainLayout->setSpacing(20); // Set the distance between the left and right columns

    // Left: Register list area
    QVBoxLayout *leftLayout = new QVBoxLayout();
    leftLayout->setSpacing(30);
    leftLayout->setContentsMargins(50, 0, 0, 70); // 50 pixels on the left

    // Add the number selection button to the left
    if (tabNum == 0) {
        QHBoxLayout *radioLayout = CreateDecSwitchRadioBtn(scroll, 0, 0);
        leftLayout->addLayout(radioLayout);
    }

    // Right side: Control area
    QVBoxLayout *rightLayout = new QVBoxLayout();
    rightLayout->setSpacing(20);
    rightLayout->setAlignment(Qt::AlignTop); // Top alignment

    // Create a serial port drop-down box
    uiRegWidgets->comboBox = CreateComboBox(scroll, 0, 0, comboxW, labelH, font);
    if (!uiRegWidgets->comboBox) {
        qDebug() << "Error: Failed to create comboBox";
        free(uiRegWidgets->regWidgets);
        if (scrollArea) delete scrollArea;
        return nullptr;
    }
    uiRegWidgets->comboBox->setMinimumWidth(180);
    uiRegWidgets->comboBox->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    uiRegWidgets->comboBox->setStyleSheet("QComboBox { background: transparent !important; color: #FFF; border: 1px solid #888; border-radius: 6px; } QComboBox QAbstractItemView { background: #222; color: #FFF; }");
    rightLayout->addWidget(uiRegWidgets->comboBox);

    // Create the save parameter button
    QPushButton *saveArgsBtn = new QPushButton("保存参数", scroll);
    saveArgsBtn->setFont(font);
    saveArgsBtn->setMinimumHeight(40);
    saveArgsBtn->setMinimumWidth(180);
    saveArgsBtn->setStyleSheet("QPushButton { background: transparent !important; color: #FFF; border: 1px solid #888; border-radius: 6px; }");
    rightLayout->addWidget(saveArgsBtn);

    // Save args button slot
    QObject::connect(saveArgsBtn, &QPushButton::clicked, [saveArgsBtn]() {
        SaveArgsBtnClickedEvent(saveArgsBtn);
    });
// initialize labelData
#if defined(PRJ201)
    uiRegWidgets->labelData = g_labelData[g_tabWidgetIndex];
#elif defined(PRJ_CIOMP) || defined(INTERNAL)
    uiRegWidgets->labelData = g_labelData;
#endif
    // Add read-only data to the right side
    for (int i = 0; i < READ_ONLY_DATA_NUM_MAX; i ++) {
        QHBoxLayout *readOnlyRowLayout = new QHBoxLayout();

        uiRegWidgets->labelData[i].label = new QLabel(scroll);
        uiRegWidgets->labelData[i].btn = new QPushButton(scroll);
        if (i == 0 || i == 3) {
            uiRegWidgets->labelData[i].btn->setText(uiRegWidgets->labelData[i].name);
            uiRegWidgets->labelData[i].label->setVisible(false);
            uiRegWidgets->labelData[i].btn->setFont(font);
            uiRegWidgets->labelData[i].btn->setMinimumSize(100,40);
            uiRegWidgets->labelData[i].btn->setStyleSheet("QPushButton { background: transparent !important; color: #FFF; border: 1px solid #888; border-radius: 6px; }");
        } else {
            uiRegWidgets->labelData[i].label->setText(uiRegWidgets->labelData[i].name);
            uiRegWidgets->labelData[i].label->setAlignment(Qt::AlignCenter);
            uiRegWidgets->labelData[i].btn->setVisible(false);
            uiRegWidgets->labelData[i].label->setFont(font);
            uiRegWidgets->labelData[i].label->setMinimumSize(100,40);
            uiRegWidgets->labelData[i].label->setStyleSheet("QLabel { background: transparent !important; color: #FFF; border: 1px solid #888; border-radius: 6px; }");
        }

        uiRegWidgets->labelData[i].lineEdit = new QLineEdit(scroll);
        uiRegWidgets->labelData[i].lineEdit->setAlignment(Qt::AlignCenter);
        uiRegWidgets->labelData[i].lineEdit->setFont(font);
        uiRegWidgets->labelData[i].lineEdit->setMinimumSize(75,40);
#if defined(PRJ201)
        uiRegWidgets->labelData[i].lineEdit->setReadOnly(true);
        uiRegWidgets->labelData[i].lineEdit->setStyleSheet("QLineEdit { background: transparent !important; color: #FFF; border: 1px solid #888; border-radius: 6px; }");
#elif defined(PRJ_CIOMP) || defined(INTERNAL)
        if (i == 0 || i == 3) {
            uiRegWidgets->labelData[i].lineEdit->setReadOnly(true);
        }
        uiRegWidgets->labelData[i].lineEdit->setStyleSheet("QLineEdit { background: transparent !important; color: #FFF; border: 1px solid #888; border-radius: 6px; }");
#endif

        readOnlyRowLayout->addWidget(uiRegWidgets->labelData[i].label);
        readOnlyRowLayout->addWidget(uiRegWidgets->labelData[i].btn);
        readOnlyRowLayout->addWidget(uiRegWidgets->labelData[i].lineEdit);

        rightLayout->addLayout(readOnlyRowLayout);
    }

    // Initialize the temperature and average image data
#ifndef PRJ201
    uiRegWidgets->labelData[0].lineEdit->setText(QString::number(0.00, 'f', 2));
    uiRegWidgets->labelData[1].lineEdit->setText(QString::number(0.04, 'f', 2));
    uiRegWidgets->labelData[2].lineEdit->setText(QString::number(1.00, 'f', 2));
    uiRegWidgets->labelData[3].lineEdit->setText(QString::number(0));
#endif

    // refresh button slot
#ifndef PRJ201
    QObject::connect(uiRegWidgets->labelData[0].btn, &QPushButton::clicked, uiRegWidgets->labelData[0].btn, []() {
        RefreshTempBtnClickedEvent();
    });
    QObject::connect(uiRegWidgets->labelData[3].btn, &QPushButton::clicked, uiRegWidgets->labelData[3].btn, []() {
        RefreshImageAvgValueBtnClickedEvent();
    });
#endif

    rightLayout->setContentsMargins(0,50,50,0); // Top left, bottom right

    // The control area on the right is wrapped with QWidget to set the minimum width
    QWidget *rightWidget = new QWidget(scroll);
    rightWidget->setLayout(rightLayout);
    rightWidget->setMinimumWidth(220); // Adjust according to the actual content
    rightWidget->setMaximumWidth(260);
    rightWidget->setStyleSheet("background: transparent;");

    // Add the left and right layouts to the main layout
    mainLayout->addLayout(leftLayout, 4);  // Four are on the left side
    mainLayout->addStretch(4);             // Intermediate elastic space
    mainLayout->addWidget(rightWidget, 0); // The control area on the right is not stretched
    mainLayout->addStretch(2); // Right elastic space

    // Set the main layout
    scroll->setLayout(mainLayout);

    if (regNumVisible > 0) {
        if (g_tabWidgetIndex == 0) {
            CreateRegList(scroll, uiRegWidgets, regNum, labelX, labelY);
        }
#if defined(PRJ201)
        else {
            CreateRegList1(scroll, uiRegWidgets, regNum, labelX, labelY);
        }
#endif
    }

    return scrollArea;
}
