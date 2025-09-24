#ifndef PAGE_USB_H
#define PAGE_USB_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>

class UsbStats {
public:
    int frameRate = 0;
    int lostPkt = 0;
    int discardPkt = 0;
};

class UsbWidgets : public QObject, public UsbStats{
public:
    UsbWidgets();
    void CreateStartStopBtn(QWidget *pageUsb);
    void CreateImageLabel(QWidget *parent);
    void CreateImageWidthEdit(QWidget *parent);
    void CreateImageHeightEdit(QWidget *parent);
    void RefreshImage(void *param);
    bool isStarted = false;
public slots:
    void UsbStartStopBtnClickedEvent(void);

private:
    QFont font;
    QLabel *imageLabel;
    QPushButton *startStopBtn;
    QLineEdit *imgWidthEdit;
    QLineEdit *imgHeightEdit;
    uint32_t imgWidth = 2448;
    uint32_t imgHeight = 2048;
};

void UpdateImage(void *param);
void UpdateUsbPageImage(void *param);
void CreateUsbWidgets(QWidget *pageUsb);

#endif // PAGE_USB_H
