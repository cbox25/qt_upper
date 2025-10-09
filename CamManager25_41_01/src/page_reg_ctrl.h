#ifndef PAGE_REG_CTRL_H
#define PAGE_REG_CTRL_H

#include <QWidget>
#include <stdint.h>
#include <QLabel>
#include <QSlider>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QSerialPort>
#include <QComboBox>
#include <QPushButton>
#include <QRadioButton>
#include <QToolTip>
#include <QTimer>
#include "common.h"
#include "init_main_window.h"
#include "../base_widgets/switch_button.h"

// Frontend
#if defined(PRJ201)
#define READ_ONLY_DATA_NUM_MAX 3
#elif defined(PRJ_CIOMP)
#define READ_ONLY_DATA_NUM_MAX 2
#elif defined(INTERNAL)
#define READ_ONLY_DATA_NUM_MAX 4
#endif

#define LINE_LEN_MAX 1024

#if defined(PRJ201)
#define REG_CTRL_PAGE_NUM   2
#else
#define REG_CTRL_PAGE_NUM   1
#endif

class HoverLabel : public QLabel {
    Q_OBJECT
public:
    HoverLabel(QWidget *parent = nullptr) : QLabel(parent), hoverTimer(new QTimer(this)) {
        setMouseTracking(true); // Enable mouse tracking
        hoverTimer->setSingleShot(true); // Timer should only fire once
        connect(hoverTimer, &QTimer::timeout, this, &HoverLabel::showTooltip);
    }

    void setTipText(QString &text) {
        hoverText = QString("<div style='width: 200px; white-space: normal; font-size: 14px;'>%1</div>").arg(text.replace("\n", "<br>"));
    }

protected:
    void enterEvent(QEvent *event) override {
        QLabel::enterEvent(event);
        hoverTimer->start(1000); // Start timer for 1 second
    }

    void leaveEvent(QEvent *event) override {
        QLabel::leaveEvent(event);
        hoverTimer->stop(); // Stop timer if mouse leaves before timeout
        QToolTip::hideText(); // Hide any tooltip that might be shown
    }

private slots:
    void showTooltip() {
        // Get the current mouse position
        QPoint mousePos = QCursor::pos();

        // Calculate the position for the tooltip (bottom-right of the mouse cursor)
        QPoint tooltipPos = mousePos + QPoint(20, 5); // Adjust the position as needed
        QToolTip::showText(tooltipPos, hoverText);
    }

private:
    QTimer *hoverTimer;
    QString hoverText; // Store the tooltip text
};

class UiRegWidget {
public:
    HoverLabel *label;
#ifdef REG_CTRL_SLIDER
    QSlider *slider;
#endif
    QLineEdit *lineEdit;

    UiRegWidget();
    int GetRegValue();
    void UiSetRegCtrl(const QString &name, uint32_t defaultValue);
};

class UiLabelData {
public:
    QLabel *label;
    QPushButton *btn;
    QLineEdit *lineEdit;
    QString name;
    uint32_t addr;
    QString data;
};

class UiRegWidgetsInfo {
public:
    UiRegWidget *regWidgets;
    QComboBox *comboBox;
    UiLabelData *labelData;
    int regTotalNum;
    QRadioButton *hexBtn;
    QRadioButton *decBtn;
};

// Backend
#define REG_NUM_MAX 64

class Reg {
public:
    QString name;
    uint32_t addr;
    uint32_t value;
    uint32_t defaultValue;
    uint32_t max;
    uint32_t min;
    uint8_t save;
    uint8_t visible;
    uint32_t delay;
    QString tipText;

    Reg();
    Reg(const QString name, uint32_t addr, uint32_t defaultValue, uint32_t max, uint32_t min);
};

extern Reg g_regs[];
extern QStackedWidget *g_stackedWidget;
QScrollArea *CreateRegCtrlWidgets(QWidget *pageRegCtrl, int tabNum);
void SendWriteRegBuf(uint32_t addr, uint32_t value);
void SyncDataToPage1(int i, int value);
void SyncDataToPage2(int i, int value);
QComboBox *CreateComboBox(QWidget *parent, int x, int y, int w, int h, QFont &font);
void LineEditReturnPressedEvent(QObject *sender, const QString &hexStr);
void RegCtrlHexBtnClickedEvent(QRadioButton *hexBtn);
void RegCtrlDecBtnClickedEvent(QRadioButton *decBtn);
void SaveArgsBtnClickedEvent(QObject *sender);

void RefreshTempBtnClickedEvent(void);
void RefreshImageAvgValueBtnClickedEvent(void);
void RefreshSheet(void);
int ReadCsvCmd(uint16_t *pktNum);
int ReadCsvData(uint16_t pktNum, uint16_t *regNum);
void RefreshRegList(UiRegWidgetsInfo *uiRegWidgets, QWidget *scroll, int regNum, int labelX, int labelY);
bool WriteCsvFileByRegData(const char* filePathCsv);
void CreateBufToCsvFile(const char *filename, const uint8_t *buf, uint32_t filesize);

QWidget *CreateScrollWindow(QWidget *page, QScrollArea *scrollArea, int regNum);
void UartComboBoxActivatedEvent(QComboBox *comboBox, const QString &text);
void RegComboBoxActivatedEvent(QComboBox *comboBox, int idx);
void SwitchBtnValueChangedEvent(SwitchButton *btn, int idx);
void GtxBtnClickedEvent(QPushButton *gtxBtn);
#ifdef REG_CTRL_SLIDER
void SliderReleasedEvent(QObject *sender);
#endif

#endif // PAGE_REG_CTRL_H
