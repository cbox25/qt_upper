#ifndef PAGE_UPDATE_H
#define PAGE_UPDATE_H

#include <QApplication>
#include <QWidget>
#include <QFileDialog>
#include <QDebug>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QProgressBar>
#include <QMessageBox>
#include "multi_threads.h"

#define FILE_PATH_LEN_MAX THREAD_PACKET_DATA_LEN

class UpdateWidgets{
public:
    QLineEdit *lineEdit;
    QPushButton *selectBtn;
    QPushButton *updateBtn;
    QProgressBar *progressBar;
    QMessageBox *msgBox;
    QComboBox *uartComBox;
};

extern UpdateWidgets g_updateWidgets;

void CreateUpdateWidgets(QWidget *pageUpdate);
void UpdatePageFw(void *param);
void UpdateRet(void *param);
void UpdateProgressBar(void *param);
void SelectBtnClickedEvent(QWidget *parent);
void UpdateBtnClickedEvent(QPushButton *sender);
void UpdatePageFw(void *param);
void UpdateFile(void *param);

#endif // PAGE_UPDATE_H
