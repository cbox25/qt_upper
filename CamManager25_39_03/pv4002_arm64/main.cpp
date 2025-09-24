#include "mainwindow.h"
#include <QApplication>
#include <QToolBar>
#include <QAction>
#include <QStackedWidget>
#include "../src/InitMainWindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;

    InitMainWindowWidgets(w);

    w.show();
    return a.exec();
}
