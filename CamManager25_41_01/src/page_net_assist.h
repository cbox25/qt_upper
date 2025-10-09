#ifndef PAGE_NET_ASSIST_H
#define PAGE_NET_ASSIST_H

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QTextEdit>
#include "common.h"

#ifdef PRJ201

#define LABEL_LINEEDIT_GROUP_NUM    4

class LabelLineEditGroup {
public:
    QLabel *label;
    QLineEdit *edit;
};

class NetAssist {
public:
    LabelLineEditGroup lableEditGrp[LABEL_LINEEDIT_GROUP_NUM];
    QLabel *sendLabel;
    QTextEdit *sendEdit;
    QLabel *recvLabel;
    QTextEdit *recvEdit;
    QPushButton *sendBtn;
    QCheckBox *periodChkBox;
    QLineEdit *peroidDataEdit;
};

void CreateNetAssistWidgets(QWidget *pageNetwork);

#endif // PRJ201

#endif // PAGE_NET_ASSIST_H
