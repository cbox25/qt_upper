#include <QApplication>
#include <QDebug>
#include <QTimer>
#include <QMutex>
#include <QFontDatabase>
#include <QDir>
#include <QResource>
#include "mainwindow.h"
#include "../src/fontmanager.h"
#include "../src/init_main_window.h"
#include "../src/multi_threads.h"
#include "../src/page_update.h"
#include "../QLogger/QLogger.h"
#include "../src/usb.h"
#ifdef ENABLE_CPPTEST_UI_TEST

#include "../test/cpptest_ui_test.h"

// 前向声明，避免链接错误
void runCppTestUITests();
#endif

#ifdef ENABLE_UI_TEST
#include "../test/ui_test.h"

// 前向声明，避免链接错误
void testCommonUIFunctions();
#endif

extern QStackedWidget *g_stackedWidget;

LinkedList<ThreadManager *> *g_threadList = nullptr;
MainWindow *g_window = nullptr;
QApplication *g_app = nullptr;

static QMutex uiInitMutex;

void runTestsInGui();

void SetAttributeForDisplayZoom(void) {
    // 重新启用高DPI缩放以确保文字正常显示
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling, true);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps, true);
    QApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
}

void CreateUi(int argc, char *argv[]) {
    QMutexLocker locker(&uiInitMutex);

    if (!g_threadList) {
        qDebug() << "Error: g_threadList is null";
        return;
    }
    if (g_window) {
        qDebug() << "Warning: g_window already exists, deleting old instance";
        delete g_window;
        g_window = nullptr;
    }
    MainWindow *w = new MainWindow();
    if (!w) {
        qDebug() << "Error: Failed to create MainWindow";
        return;
    }
    InitMainWindowWidgets(*w); // 确保在 show 之前初始化
    w->show();
    //qDebug() << "CreateUi: Window shown with size" << w->width() << "x" << w->height();
    g_window = w;
    uiInitialized = true;
    QCoreApplication::processEvents();
    // 调用 runTestsInGui
    QTimer::singleShot(100, runTestsInGui); // 延迟 100ms 确保 UI 初始化完成
}

void runTestsInGui() {
    if (g_window) {
        static_cast<MainWindow*>(g_window)->testsRunning = true;
    }
    if (!g_window || !g_window->isVisible() || !uiInitialized) {
        qDebug() << "Error: GUI not fully initialized, g_window =" << g_window
                 << ", isVisible =" << (g_window ? g_window->isVisible() : false)
                 << ", uiInitialized =" << uiInitialized;
        if (!uiInitialized) {
            qDebug() << "Retrying test in 500ms due to uninitialized GUI";
            QTimer::singleShot(500, runTestsInGui);
            return;
        }
    }
    if (!g_threadList) {
        qDebug() << "Error: Thread list not initialized, skipping tests";
        return;
    }
    if (!g_stackedWidget || !g_scrollArea) {
        qDebug() << "Error: g_stackedWidget or g_scrollArea not initialized, skipping tests";
        return;
    }
// 运行自定义UI测试
#ifdef ENABLE_UI_TEST
    qDebug() << "Starting custom UI tests...";
    try {
        testCommonUIFunctions();
        qDebug() << "Custom UI tests completed successfully";
    } catch (const std::exception& e) {
        qDebug() << "Exception in custom UI tests:" << e.what();
    }
#endif

// 运行CppTest UI测试
#ifdef ENABLE_CPPTEST_UI_TEST
    qDebug() << "Starting CppTest UI tests...";
    try {
        runCppTestUITests();
        qDebug() << "CppTest UI tests completed successfully";
    } catch (const std::exception& e) {
        qDebug() << "Exception in CppTest UI tests:" << e.what();
    }
    qDebug() << "All tests completed";
#else
    qDebug() << "CppTest UI tests disabled";
#endif
    if (g_window) {
        static_cast<MainWindow*>(g_window)->testsRunning = false;
    }
}



int main(int argc, char *argv[]) {
    SetAttributeForDisplayZoom();
    g_app = new QApplication(argc, argv);
    
    // 使用字体管理类加载和设置字体
    FontManager::instance().loadApplicationFonts();
    FontManager::instance().setApplicationFont();
    
    // 设置全局样式，包括右键菜单
    g_app->setStyleSheet(
        "QMenu {"
        "    background-color: #2b2b2b;"
        "    color: #ffffff;"
        "    border: 1px solid #555555;"
        "    padding: 5px;"
        "}"
        "QMenu::item {"
        "    background-color: transparent;"
        "    padding: 5px 20px;"
        "}"
        "QMenu::item:selected {"
        "    background-color: #555555;"
        "}"
        "QMenu::separator {"
        "    height: 1px;"
        "    background-color: #555555;"
        "    margin: 2px 0px;"
        "}"
    );

    g_threadList = new LinkedList<ThreadManager *>();
    if (!g_threadList) {
        qDebug() << "Error: Failed to allocate g_threadList";
        delete g_app;
        return -1;
    }


    // 使用带参数的构造函数创建 THREAD_ID_UPDATE_FW 线程
    ThreadManager *updateThread = new ThreadManager(THREAD_ID_UPDATE_FW, UpdatePageFw);
    RegisterThread(*g_threadList, *updateThread);


    // 初始化 THREAD_ID_UPDATE_UI 线程
    ThreadManager *uiThread = new ThreadManager(THREAD_ID_UPDATE_UI, UpdatePageFw);
    RegisterThread(*g_threadList, *uiThread);

#if defined(MODULE_USB3)
    ThreadManager *usbThread = new ThreadManager(THREAD_ID_USB3, UsbTask);
    g_threadList->Append(THREAD_ID_USB3, usbThread);
#endif

    CreateMultiThreads(*g_threadList);


    CreateUi(argc, argv); // 调用 CreateUi 初始化 UI 和 reg.csv

    int result = g_app->exec();

    if (g_app) {
        delete g_threadList;
        delete g_window;
        delete g_app;
    }
    return result;
}
