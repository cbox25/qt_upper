#ifndef INIT_MAIN_WINDOW_H
#define INIT_MAIN_WINDOW_H

#include "mainwindow.h"
#include "common.h"
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QMouseEvent>

typedef enum {
    TAB_PAGE_REG,
#if defined(PRJ201)
    TAB_PAGE_REG2,
#endif
    TAB_PAGE_UPDATE,
#if defined(PRJ201)
    TAB_PAGE_IMAGE1,
    TAB_PAGE_NETASSIST,
#endif

#if defined(MODULE_USB3)
    TAB_PAGE_USB,
#endif
    TAB_PAGE_NUM
} TabPage;

typedef struct {
    TabPage num;
    QString name;
} MainWindowTab;

void InitMainWindowWidgets(MainWindow &w);

extern bool g_versionParse;
extern bool isInitDone ;
extern bool uiInitialized;
extern QTabWidget *g_tabWidgets;
extern QScrollArea *g_scrollArea;

#endif // INIT_MAIN_WINDOW_H
