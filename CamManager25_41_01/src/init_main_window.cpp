#include <QGridLayout>
#include <QScrollArea>
#include <QPushButton>
#include <QVBoxLayout>
#include <QTabWidget>
#include "page_reg_ctrl.h"
#include "page_update.h"
#include "page_network.h"
#include "page_net_assist.h"
#include "page_usb.h"
#include "init_main_window.h"
#include "mainwindow.h"
#include "common.h"
#include "libusb.h"
#include "usb.h"
#include "uart.h"
#include "../QLogger/QLoggerLevel.h"
#include "../QLogger/QLoggerWriter.h"
#include "../QLogger/QLogger.h"
#include "../QLogger/QLoggerManager.h"
#include "multi_threads.h"
#include "../base_widgets/customtabwidget.h"
#include <QToolBar>
#include <QAction>
#include <QIcon>
#include <QLabel>
#include <QPixmap>
#include <QSplitter>
#include <QTabBar>
#include <QStackedWidget>
#include <QTreeWidget>
#include <QProxyStyle>
#include <QPainter>
#include <QClipboard>
#include <QToolTip>
#include <QMouseEvent>
#include <QApplication>
#include <QCursor>
#include <QDir>
#include "toolbar_ctrl.h"

// Customize the white arrow style
class WhiteArrowStyle : public QProxyStyle {
public:
    using QProxyStyle::QProxyStyle;
    void drawPrimitive(PrimitiveElement element, const QStyleOption *option,
                       QPainter *painter, const QWidget *widget = nullptr) const override {
        if (element == QStyle::PE_IndicatorBranch) {
            // get current item
            const QTreeView *tree = qobject_cast<const QTreeView *>(widget);
            if (tree) {
                QModelIndex index = tree->indexAt(option->rect.center());
                if (index.isValid()) {
                    QString text = index.data().toString();
                    // Only draw an arrow for "USB Image"
                    if (text == "USB图像") {
                        painter->save();
                        painter->setRenderHint(QPainter::Antialiasing, true);
                        QRect r = option->rect;
                        QPolygonF arrow;
                        if (option->state & QStyle::State_Open) {
                            arrow << QPointF(r.center().x() - 5, r.center().y() - 2)
                            << QPointF(r.center().x() + 5, r.center().y() - 2)
                            << QPointF(r.center().x(),     r.center().y() + 5);
                        } else {
                            arrow << QPointF(r.center().x() - 2, r.center().y() - 5)
                            << QPointF(r.center().x() - 2, r.center().y() + 5)
                            << QPointF(r.center().x() + 5, r.center().y());
                        }
                        painter->setBrush(Qt::white);
                        painter->setPen(Qt::NoPen);
                        painter->drawPolygon(arrow);
                        painter->restore();
                    }
                    // For other items, do not draw arrows; just return directly
                    return;
                }
            }
        }
        QProxyStyle::drawPrimitive(element, option, painter, widget);
    }
};

int g_tabWidgetIndex;
QTabWidget *g_tabWidgets = nullptr;
QScrollArea *g_scrollArea = nullptr;
bool isInitDone = false;
bool uiInitialized = false;
QSplitter *g_splitter = nullptr;
QTabBar *g_tabBar = nullptr;
QStackedWidget *g_stackedWidget = nullptr;
QTreeWidget *g_tabList = nullptr;
QString g_fpgaVersion, g_softKernelVersion; // The version number read through the serial port

MainWindowTab g_mainWindowTab[TAB_PAGE_NUM] = {
    { TAB_PAGE_REG, "寄存器控制" },
#if defined(PRJ201)
    { TAB_PAGE_REG2, "寄存器控制2" },
#endif
    { TAB_PAGE_UPDATE, "在线升级" },
#if defined(PRJ201)
    { TAB_PAGE_IMAGE1, "图像处理板" },
    { TAB_PAGE_NETASSIST, "网络助手" },
#endif
#if defined(MODULE_USB3)
    { TAB_PAGE_USB, "USB图像"}
#endif
};

QTabWidget* CreatePages(MainWindow &w) {

    QFont font;
    int page_h = 0, page_w = 0;
    CustomTabWidget *tabWidget = new CustomTabWidget;

    font = getApplicationFont(12);
    tabWidget->setTabShape(QTabWidget::Rounded);
    tabWidget->setTabPosition(QTabWidget::West);
    w.setStyleSheet("background-color: #222;");
    tabWidget->setStyleSheet(
        "QTabBar::tab { height: 30px; width: 130px; background-color: #222; color: #FFF; "
        "text-align: center; "
        "font-size: 18px; "
        "writing-mode: horizontal-tb; "
        "padding: 5px; "
        " }"
        "QTabWidget::pane { border: 0px solid #222; background: #888; }"
        );
    // Use the global font of the application instead of hard-coded fonts
    tabWidget->setFont(QApplication::font());

    page_h = w.height();
    page_w = w.width();

    for (int i = 0; i < TAB_PAGE_NUM; i++) {
        QWidget *pageWidget = new QWidget;
        pageWidget->resize(page_w, page_h);
        // Add ICONS only for "Register Control" and "Online Upgrade"
        if (g_mainWindowTab[i].num == TAB_PAGE_REG || g_mainWindowTab[i].num == TAB_PAGE_UPDATE) {
            QString iconPath = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/1.png");
            QIcon tabIcon(iconPath);
            tabWidget->insertTab(g_mainWindowTab[i].num, pageWidget, tabIcon, g_mainWindowTab[i].name);
        } else {
            tabWidget->insertTab(g_mainWindowTab[i].num, pageWidget, g_mainWindowTab[i].name);
        }
    }

    QObject::connect(tabWidget, &QTabWidget::currentChanged, [tabWidget]() {
        g_tabWidgetIndex = tabWidget->currentIndex();
        qDebug() << "Tab changed to index" << g_tabWidgetIndex;
    });

    //qDebug() << "CreatePages: Completed";
    return tabWidget;
}

QStackedWidget* CreateStackedWidget(MainWindow &w) {
    QStackedWidget *stackedWidget = new QStackedWidget;
    // Create each page
    for (int i = 0; i < TAB_PAGE_NUM; i++) {
        QWidget *pageWidget = new QWidget;
        pageWidget->resize(w.width(), w.height());
        stackedWidget->addWidget(pageWidget);
    }
    return stackedWidget;
}

QTreeWidget* CreateTabTree() {
    QTreeWidget *tree = new QTreeWidget;
    tree->setHeaderHidden(true);
    tree->setIconSize(QSize(24, 24));
    tree->setMinimumWidth(160);
    tree->setMaximumWidth(300);
    QString BrciconPathR = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/branch_right.png");
    QString BrciconPathD = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/branch_down.png");
    tree->setFocusPolicy(Qt::NoFocus);
    tree->setStyle(new WhiteArrowStyle());
    tree->setStyleSheet(
        "QTreeWidget { background: #222; border: none; font-size: 16px; }"
        "QTreeWidget::item { height: 40px;  color: #FFF; padding-left: 10px; outline: none; border: none;   }"
        "QTreeWidget::item:selected { background: #444; color: #FFF; outline: none; border: none;  padding-left: 10px; }"
        "QTreeWidget::item:focus { outline: none; border: none;  padding-left: 10px; }"
        "QTreeWidget::item:selected:focus { outline: none; border: none;  padding-left: 10px; }"
        "QTreeWidget::item:hover { background: #333; color: #FFF; outline: none; border: none;  padding-left: 10px; }"
    );

    QFont font = getApplicationFont(12);
    tree->setFont(font);
    tree->setIndentation(18);
    tree->setExpandsOnDoubleClick(false);

    QList<QTreeWidgetItem*> topItems;
    for (int i = 0; i < TAB_PAGE_NUM; ++i) {
        QString iconPath = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/1.png");
        QIcon tabIcon(iconPath);
        QTreeWidgetItem *item = new QTreeWidgetItem(tree);
        item->setText(0, g_mainWindowTab[i].name);
        item->setIcon(0, tabIcon);
        item->setData(0, Qt::UserRole, i); // 存储tab索引
        // USB图像下加子项
        if (g_mainWindowTab[i].name == "USB图像") {
            QTreeWidgetItem *sub = new QTreeWidgetItem(item);
            sub->setText(0, " 开始");
            sub->setIcon(0, QIcon());
            sub->setData(0, Qt::UserRole, 1000); // 特殊标记
            QFont subFont = getApplicationFont(10);
            sub->setFont(0, subFont);
            item->addChild(sub);
            item->setExpanded(true);
        }
        topItems.append(item);
    }
    tree->addTopLevelItems(topItems);
    tree->setCurrentItem(topItems[0]);
    return tree;
}

void InitMainWindowWidgets(MainWindow &w) {
    int ret = 0;

    if (!w.centralWidget()) {
        qDebug() << "Error: Central widget not set, creating default";
        w.setCentralWidget(new QWidget());
    }

    // 确保全局线程列表已初始化
    if (!g_threadList) {
        g_threadList = new LinkedList<ThreadManager *>();
    }

    // 初始化 QLogger
    static const QString module("CamManager");
    QLoggerManager *manager = QLoggerManager::getInstance();

    // 创建日志线程的 ThreadManager - 在添加目标之前
    ThreadManager *loggerThreadManager = new ThreadManager(THREAD_TYPE_CMD_LOGGER, nullptr);
    RegisterThread(*g_threadList, *loggerThreadManager);
    manager->setThreadManager(loggerThreadManager);

    // 启动线程
    CreateMultiThreads(*g_threadList);

    // 等待线程启动
    QThread::msleep(1000);

    QString logPath = QCoreApplication::applicationDirPath() + "/mylogs";
    if (!QDir().mkpath(logPath)) {
        qDebug() << "创建日志目录失败：" << logPath;
        logPath = QDir::homePath() + "/mylogs";
        QDir().mkpath(logPath);
    }
    QString logFile = QDir::cleanPath(logPath + "/test.log");
    manager->addDestination(logFile, module, QLogger::LogLevel::Debug, QString(), QLogger::LogMode::OnlyFile,
                            QLogger::LogFileDisplay::DateTime, QLogger::LogMessageDisplays(QLogger::LogMessageDisplay::Default));
    // qDebug() << "QLogger 初始化完成，日志文件路径：" << logFile;

    for (QLoggerWriter *writer : manager->getWriters()) {
        writer->setThreadManager(loggerThreadManager);
    }

    QLog_Debug(module,"This is a debug log .");

    QString TopName;
    int width = 0, height = 0;

#ifdef PRJ201
    w.setWindowTitle("PJ 工装");
    // 设置合适的窗口大小，支持自适应布局
    width = 1600;
    height = 800;
#else
    TopName = "CamManager";
    w.setWindowTitle(TopName);
    width = 1000;
    height = 600;
#endif
    w.resize(width, height);

    if (g_splitter) {
        delete g_splitter;
        g_splitter = nullptr;
    }
    g_splitter = new QSplitter(Qt::Horizontal, &w);
    g_tabList = CreateTabTree();
    g_stackedWidget = CreateStackedWidget(w);
    g_stackedWidget->setMinimumWidth(750); // Minimum width of the content area
    g_splitter->addWidget(g_tabList);
    g_splitter->addWidget(g_stackedWidget);
    g_splitter->setStretchFactor(0, 0);
    g_splitter->setStretchFactor(1, 1);
    // Set the window size range and support adaptive layout
    w.setMinimumWidth(1200);
    w.setMinimumHeight(700);
    int leftWidth = g_tabList->minimumWidth();
    int rightWidth = w.width() - leftWidth;
    if (rightWidth < 600) rightWidth = 600;
    g_splitter->setSizes({leftWidth, rightWidth});
    g_splitter->setCollapsible(0, false);
    w.setCentralWidget(g_splitter);

    g_splitter->setStyleSheet(
        "QSplitter { background: #000; }"
        "QSplitter::preview { background: #000; }"
        "QSplitter::handle, "
        "QSplitter::handle:pressed, "
        "QSplitter::handle:hover { "
        "    background: #000;"
        "    border: none;"
        "    margin: 0px;"
        "}"
        "QSplitter::handle:horizontal {"
        "    background: #000;"
        "    width: 3px;"
        "    border: none;"
        "}"
        "QSplitter::handle:vertical {"
        "    background: #000;"
        "    height: 3px;"
        "    border: none;"
        "}"
    );

    // Initialize the content of each page
    // TAB_PAGE_REG
    if (g_scrollArea) {
        delete g_scrollArea;
        g_scrollArea = nullptr;
    }

    g_scrollArea = CreateRegCtrlWidgets(g_stackedWidget->widget(TAB_PAGE_REG), TAB_PAGE_REG);
    if (!g_scrollArea) {
        qDebug() << "Error: g_scrollArea initialization failed, check reg.csv or memory";
        return;
    }

    QVBoxLayout *regLayout = new QVBoxLayout(g_stackedWidget->widget(TAB_PAGE_REG));
    regLayout->setContentsMargins(0, 0, 0, 0);
    regLayout->addWidget(g_scrollArea);
    g_stackedWidget->widget(TAB_PAGE_REG)->setLayout(regLayout);

    // TAB_PAGE_UPDATE
    QWidget *pageUpdate = g_stackedWidget->widget(TAB_PAGE_UPDATE);
    CreateUpdateWidgets(pageUpdate);

    // Other pages can be initialized as needed
#if defined(MODULE_USB3)
    CreateUsbWidgets(g_stackedWidget->widget(TAB_PAGE_USB));
#endif

#ifdef PRJ201
    CreateRegCtrlWidgets(g_stackedWidget->widget(TAB_PAGE_REG2), TAB_PAGE_REG2);
    CreateNetworkWidgets(g_stackedWidget->widget(TAB_PAGE_IMAGE1), 0);
    CreateNetAssistWidgets(g_stackedWidget->widget(TAB_PAGE_NETASSIST));
    g_tabWidgetIndex = TAB_PAGE_REG;
#endif

    // Set the background
    QString imagePath = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/Aimuon2.jpg");
    g_stackedWidget->widget(TAB_PAGE_REG)->setStyleSheet(
        "background-image: url(" + imagePath.replace("\\", "/") + ") !important; "
        "background-repeat: no-repeat !important; "
        "background-position: center !important; "
        "background-color: #141519 !important;"
    );

    // Set QScrollArea to transparent
    g_scrollArea->setStyleSheet("background: transparent;");
    g_scrollArea->viewport()->setStyleSheet("background: transparent;");

    g_stackedWidget->widget(TAB_PAGE_UPDATE)->setStyleSheet(
        "background-image: url(" + imagePath.replace("\\", "/") + ") !important; "
        "background-repeat: no-repeat !important; "
        "background-position: center !important; "
        "background-color: #141519 !important;"
    );

#ifdef PRJ201
    g_stackedWidget->widget(TAB_PAGE_REG2)->setStyleSheet(
        "background-image: url(" + imagePath.replace("\\", "/") + ") !important;"
        "background-repeat: no-repeat !important;"
        "background-position: center !important;"
        "background-color: #141519 !important;"
        );

    g_stackedWidget->widget(TAB_PAGE_IMAGE1)->setStyleSheet(
        //"background-image: url(" + imagePath.replace("\\", "/") + ") !important;"
        "background-repeat: no-repeat !important;"
        "background-position: center !important;"
        "background-color: #141519 !important;"
        );


    g_stackedWidget->widget(TAB_PAGE_NETASSIST)->setStyleSheet(
        //"background-image: url(" + imagePath.replace("\\", "/") + ") !important;"
        "background-repeat: no-repeat !important;"
        "background-position: center !important;"
        "background-color: #141519 !important;"
        );
#endif

#ifdef MODULE_USB3
    g_stackedWidget->widget(TAB_PAGE_USB)->setStyleSheet(
    "background-image: url(" + imagePath.replace("\\", "/") + ") !important;"
    "background-repeat: no-repeat !important;"
    "background-position: center !important;"
    "background-color: #141519 !important;"
    );
#endif

    // Add the top toolbar
    QToolBar *toolBar = new QToolBar(&w);
    toolBar->setMovable(false);
    toolBar->setIconSize(QSize(24, 24));

    // add logo png
    QLabel *logoLabel = new QLabel(toolBar);
    QString logoPath = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/logo.png");
    QPixmap logoPixmap(logoPath);
    if (!logoPixmap.isNull()) {
        logoLabel->setPixmap(logoPixmap.scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        logoLabel->setFixedSize(32, 32);
    } else {
        logoLabel->setText("LOGO");
        logoLabel->setFixedSize(32, 32);
        logoLabel->setStyleSheet("color: #FFF; background: #444;");
    }
    toolBar->addWidget(logoLabel); // add logo

    QAction *openAction = new QAction("打开", toolBar);
    QAction *saveAction = new QAction("保存", toolBar);
    QAction *settingsAction = new QAction("设置", toolBar);
    QAction *helpAction = new QAction("帮助", toolBar);
    QAction *aboutAction = new QAction("关于", toolBar);
    toolBar->addAction(openAction);
    toolBar->addAction(saveAction);
    toolBar->addAction(settingsAction);
    toolBar->addAction(helpAction);
    toolBar->addAction(aboutAction);
    w.addToolBar(Qt::TopToolBarArea, toolBar);

    toolBar->setStyleSheet(
        "QToolBar { background: #444; min-height: 36px; }"
        "QToolButton { background: #444; color: #FFF; font-size: 16px; padding: 6px 16px; }"
        "QToolButton:hover { background: #555; }"
    );

    /* toolbar setting function slot */
    QObject::connect(settingsAction, &QAction::triggered, [&w]() {
        SettingDialog settingDialog(&w);
        settingDialog.exec();
    });

    /* toolbar about function slot */
    QObject::connect(aboutAction, &QAction::triggered, [&w, &ret]() {
        if (!g_versionParse) {
            if ((ret = SendCmdVersionRead()) < 0) {
                debugNoQuote() << "read softkernel & fpga Version error";
            } else {
                g_versionParse = true;
            }
        }

        AboutDialog aboutDialog(&w, g_fpgaVersion, g_softKernelVersion);
        aboutDialog.exec();
    });

    QTimer::singleShot(0, [&w]() { // Delay the connection signal slot to avoid triggering during initialization

        QObject::connect(&w, &MainWindow::resized, [&w]() {

            w.resizeStackedWidgetAndScrollArea(g_stackedWidget, g_scrollArea);
        });
        QObject::connect(&w, &MainWindow::updateUi, &w, &MainWindow::UpdateUi);

    });
    QObject::connect(g_tabList, &QTreeWidget::itemClicked, [&](QTreeWidgetItem *item, int column){ // Tab switch signal
        int role = item->data(0, Qt::UserRole).toInt();

#ifdef MODULE_USB3
        if (role == 1000) { // "开始"子项

            // 触发USB开始/停止逻辑
            QWidget *usbPage = g_stackedWidget->widget(TAB_PAGE_USB);
            // 只调用按钮逻辑，不切换页面
            extern UsbWidgets g_usbWidgets;
            g_usbWidgets.UsbStartStopBtnClickedEvent();

            // 切换文本
            if (item->text(0) == " 开始") {
                item->setText(0, " 停止");
            } else {
                item->setText(0, " 开始");
            }
        }
#endif


        if (role >= 0 && role < TAB_PAGE_NUM) {
            g_stackedWidget->setCurrentIndex(role);
        }
    });
    // 在初始化完成后设置 uiInitialized
    uiInitialized = true;

    isInitDone = true;
}
