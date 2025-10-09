#include "toolbar_ctrl.h"
#include <QApplication>
#include <QClipboard>
#include <QToolTip>
#include <QCursor>
#include <QDir>
#include <QPixmap>
#include "prj_config.h"

/* This.cpp file stores classes and others in the toolbar */

// Draggable Dialog implementation
DraggableDialog::DraggableDialog(QWidget* parent) : QDialog(parent)
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    installEventFilter(this);
}

bool DraggableDialog::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == this) {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        switch (event->type()) {
        case QEvent::MouseButtonPress:
            if (mouseEvent->button() == Qt::LeftButton) {
                m_bDrag = true;
                m_DragPosition = mouseEvent->globalPos() - frameGeometry().topLeft();
                return true;
            }
            break;
        case QEvent::MouseButtonRelease:
            if (mouseEvent->button() == Qt::LeftButton) {
                m_bDrag = false;
                return true;
            }
            break;
        case QEvent::MouseMove:
            if (m_bDrag && (mouseEvent->buttons() & Qt::LeftButton)) {
                move(mouseEvent->globalPos() - m_DragPosition);
                return true;
            }
            break;
        default:
            break;
        }
    }
    return QDialog::eventFilter(obj, event);
}



/* ===== Toolbar :: Setting Start ===== */

// Setting Dialog implementation
SettingDialog::SettingDialog(QWidget* parent)
    : DraggableDialog(parent)
{
    setWindowTitle("设置");
    setFixedSize(500, 400);
    setModal(true);
    InitializeStyles();
    InitializeUI();
    SetupConnections();
}

void SettingDialog::InitializeUI(void)
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    mainLayout->addWidget(CreateTitleBar());
    mainLayout->addWidget(CreateContentArea());
}

void SettingDialog::InitializeStyles(void)
{
    setStyleSheet(
        "QDialog { "
        "   background-color: #222; "
        "   border: 2px solid #3c3f41; "
        "   border-radius: 8px; "
        "}"
        "QLabel { "
        "   color: #FFF; "
        "   background: transparent; "
        "   font-size: 13px; "
        "}"
        "QLabel#title { "
        "   font-size: 18px; "
        "   font-weight: bold; "
        "   color: #FFF; "
        "}"
        "QPushButton { "
        "   background-color: #222; "
        "   color: #FFF; "
        "   border: 1px solid #888; "
        "   border-radius: 6px; "
        "   padding: 5px 15px; "
        "   font-size: 12px; "
        "   width: 70px; "
        "}"
        "QPushButton:hover { "
        "   background-color: #4c4f51; "
        "   border-color: #666666; "
        "}"
        "QPushButton:pressed { "
        "   background-color: #2b2b2b; "
        "}"
        "QPushButton#closeButton { "
        "   background-color: transparent; "
        "   border: none; "
        "   color: #bbbbbb; "
        "   font-size: 20px; "
        "   font-weight: bold; "
        "   width: 46px; "
        "   height: 32px; "
        "   border-radius: 0px; "
        "   margin: 0px; "
        "   padding: 0px; "
        "}"
        "QPushButton#closeButton:hover { "
        "   background-color: #e81123; "
        "   color: white; "
        "   border: none; "
        "}"
        "QPushButton#closeButton:pressed { "
        "   background-color: #f1707a; "
        "   color: white; "
        "}"
        "QWidget#titleBar { "
        "   background-color: #3c3f41; "
        "   border-bottom: 1px solid #555555; "
        "}"
        "QLabel#hintLabel { "
        "   font-size: 14px; "
        "   color: #AAA; "
        "   font-style: italic; "
        "}"
        );
}

QWidget* SettingDialog::CreateTitleBar(void)
{
    QWidget* titleBar = new QWidget();
    titleBar->setObjectName("titleBar");
    titleBar->setFixedHeight(30);
    titleBar->setCursor(Qt::SizeAllCursor);

    QHBoxLayout* titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(12, 0, 0, 0);
    titleLayout->setSpacing(0);

    QLabel* titleLabel = new QLabel("设置");
    titleLabel->setStyleSheet("color: #FFF; font-weight: bold; background: transparent;");
    titleLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    QPushButton* closeButton = new QPushButton("×");
    closeButton->setObjectName("closeButton");
    closeButton->setCursor(Qt::ArrowCursor);
    closeButton->setFocusPolicy(Qt::NoFocus);

    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch();
    titleLayout->addWidget(closeButton);

    connect(closeButton, &QPushButton::clicked, this, &SettingDialog::OnCancelButtonClicked);

    return titleBar;
}

QWidget* SettingDialog::CreateContentArea(void)
{
    QWidget* contentWidget = new QWidget();
    contentWidget->setStyleSheet("background-color: #222;");

    QVBoxLayout* contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setSpacing(20);
    contentLayout->setContentsMargins(30, 20, 30, 0);

    m_settingHintLabel = new QLabel("设置功能开发中...");
    m_settingHintLabel->setObjectName("hintLabel");
    m_settingHintLabel->setAlignment(Qt::AlignCenter);
    m_settingHintLabel->setMinimumHeight(200);

    contentLayout->addWidget(m_settingHintLabel);
    contentLayout->addStretch();
    contentLayout->addWidget(CreateSeparator());
    contentLayout->addLayout(CreateButtonLayout());

    return contentWidget;
}

QHBoxLayout* SettingDialog::CreateButtonLayout(void)
{
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(15);
    buttonLayout->setContentsMargins(0, 0, 0, 15);

    QPushButton* okButton = new QPushButton("确定");
    okButton->setFixedWidth(80);

    QPushButton* cancelButton = new QPushButton("取消");
    cancelButton->setFixedWidth(80);

    QPushButton* applyButton = new QPushButton("应用");
    applyButton->setFixedWidth(80);
    applyButton->setEnabled(false); // Temporarily disable the application button

    buttonLayout->addStretch();
    buttonLayout->addWidget(applyButton);
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addWidget(okButton);

    connect(okButton, &QPushButton::clicked, this, &SettingDialog::OnOkButtonClicked);
    connect(cancelButton, &QPushButton::clicked, this, &SettingDialog::OnCancelButtonClicked);
    connect(applyButton, &QPushButton::clicked, this, &SettingDialog::OnApplyButtonClicked);

    return buttonLayout;
}

QFrame* SettingDialog::CreateSeparator(void)
{
    QFrame* line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    line->setStyleSheet("background-color: #3c3f41; margin: 10px 0;");
    return line;
}

void SettingDialog::SetupConnections(void)
{

}

void SettingDialog::OnOkButtonClicked(void)
{
    accept();
}

void SettingDialog::OnCancelButtonClicked(void)
{
    reject();
}

void SettingDialog::OnApplyButtonClicked(void)
{
    QToolTip::showText(QCursor::pos(), "设置已应用", this);
}

/* ===== Toolbar :: Setting End ===== */



/* ===== Toolbar :: About Start ===== */

// About Dialog implementation
AboutDialog::AboutDialog(QWidget* parent, const QString& fpgaVersion,
                         const QString& softKernelVersion)
    : DraggableDialog(parent)
    , m_fpgaVersion(fpgaVersion)
    , m_softKernelVersion(softKernelVersion)
{
    setWindowTitle("关于");
    setFixedSize(400, 350);
    setModal(true);
    // true - The dialog box must be processed before proceeding, false - The dialog box and the main window can be operated simultaneously
    InitializeStyles();
    InitializeUI();
    SetupConnections();
}

void AboutDialog::InitializeUI(void)
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    mainLayout->addWidget(CreateTitleBar());
    mainLayout->addWidget(CreateContentArea());
}

void AboutDialog::InitializeStyles(void)
{
    setStyleSheet(
        "QDialog { "
        "   background-color: #222; "
        "   border: 2px solid #3c3f41; "
        "   border-radius: 8px; "
        "}"
        "QLabel { "
        "   color: #FFF; "
        "   background: transparent; "
        "   font-size: 13px; "
        "}"
        "QLabel#title { "
        "   font-size: 18px; "
        "   font-weight: bold; "
        "   color: #FFF; "
        "}"
        "QPushButton { "
        "   background-color: #222; "
        "   color: #FFF; "
        "   border: 1px solid #888; "
        "   border-radius: 6px; "
        "   padding: 5px 15px; "
        "   font-size: 12px; "
        "   width: 70px; "
        "}"
        "QPushButton:hover { "
        "   background-color: #4c4f51; "
        "   border-color: #666666; "
        "}"
        "QPushButton:pressed { "
        "   background-color: #2b2b2b; "
        "}"
        "QPushButton#closeButton { "
        "   background-color: transparent; "
        "   border: none; "
        "   color: #bbbbbb; "
        "   font-size: 20px; "
        "   font-weight: bold; "
        "   width: 46px; "
        "   height: 32px; "
        "   border-radius: 0px; "
        "   margin: 0px; "
        "   padding: 0px; "
        "}"
        "QPushButton#closeButton:hover { "
        "   background-color: #e81123; "
        "   color: white; "
        "   border: none; "
        "}"
        "QPushButton#closeButton:pressed { "
        "   background-color: #f1707a; "
        "   color: white; "
        "}"
        "QWidget#titleBar { "
        "   background-color: #3c3f41; "
        "   border-bottom: 1px solid #555555; "
        "}"
        );
}

QWidget* AboutDialog::CreateTitleBar(void)
{
    QWidget* titleBar = new QWidget();
    titleBar->setObjectName("titleBar");
    titleBar->setFixedHeight(30);
    titleBar->setCursor(Qt::SizeAllCursor);

    QHBoxLayout* titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(12, 0, 0, 0);
    titleLayout->setSpacing(0);

    QLabel* titleLabel = new QLabel("关于");
    titleLabel->setStyleSheet("color: #FFF; font-weight: bold; background: transparent;");
    titleLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    QPushButton* closeButton = new QPushButton("×");
    closeButton->setObjectName("closeButton");
    closeButton->setCursor(Qt::ArrowCursor);
    closeButton->setFocusPolicy(Qt::NoFocus);

    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch();
    titleLayout->addWidget(closeButton);

    connect(closeButton, &QPushButton::clicked, this, &AboutDialog::OnCloseButtonClicked);

    return titleBar;
}

QWidget* AboutDialog::CreateContentArea(void)
{
    QWidget* contentWidget = new QWidget();
    contentWidget->setStyleSheet("background-color: #222;");

    QVBoxLayout* contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setSpacing(15);
    contentLayout->setContentsMargins(20, 15, 20, 0);

    contentLayout->addLayout(CreateHeaderLayout());
    contentLayout->addWidget(CreateSeparator());
    contentLayout->addLayout(CreateInfoLayout());
    contentLayout->addStretch();
    contentLayout->addWidget(CreateSeparator());
    contentLayout->addLayout(CreateButtonLayout());

    return contentWidget;
}

QHBoxLayout* AboutDialog::CreateHeaderLayout(void)
{
    QHBoxLayout* headerLayout = new QHBoxLayout();

    QLabel* logoLabel = new QLabel(); // add Logo
    QString logoPath = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/logo1.png");
    QPixmap logoPixmap(logoPath);
    if (!logoPixmap.isNull()) {
        logoLabel->setPixmap(logoPixmap.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        logoLabel->setText("LOGO");
        logoLabel->setStyleSheet("color: #FFF; background: #444; border-radius: 5px; padding: 10px;");
        logoLabel->setFixedSize(64, 64);
        logoLabel->setAlignment(Qt::AlignCenter);
    }
    headerLayout->addWidget(logoLabel);

    QVBoxLayout* appTitleLayout = new QVBoxLayout();
    appTitleLayout->setSpacing(5);

    m_appTitleLabel = new QLabel("CamManager");
    m_appTitleLabel->setObjectName("title");
    m_appTitleLabel->setAlignment(Qt::AlignLeft);
    appTitleLayout->addWidget(m_appTitleLabel);

    m_versionLabel = new QLabel("version: " + QString(VERSION));
    m_versionLabel->setAlignment(Qt::AlignLeft);
    appTitleLayout->addWidget(m_versionLabel);

    headerLayout->addLayout(appTitleLayout);
    headerLayout->addStretch();

    return headerLayout;
}

QVBoxLayout* AboutDialog::CreateInfoLayout(void)
{
    QVBoxLayout* infoLayout = new QVBoxLayout();
    infoLayout->setSpacing(5);

    m_fpgaLabel = new QLabel("FPGA version: " + m_fpgaVersion);
    m_fpgaLabel->setAlignment(Qt::AlignLeft);
    infoLayout->addWidget(m_fpgaLabel);

    m_kernelLabel = new QLabel("Microblaze version: " + m_softKernelVersion);
    m_kernelLabel->setAlignment(Qt::AlignLeft);
    infoLayout->addWidget(m_kernelLabel);

    return infoLayout;
}

QHBoxLayout* AboutDialog::CreateButtonLayout(void)
{
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(10);
    buttonLayout->setContentsMargins(0, 0, 0, 15);

    QPushButton* copyButton = new QPushButton("复制");
    copyButton->setFixedWidth(80);

    QPushButton* okButton = new QPushButton("关闭");
    okButton->setFixedWidth(80);
    okButton->setDefault(true);

    buttonLayout->addStretch();
    buttonLayout->addWidget(copyButton);
    buttonLayout->addWidget(okButton);

    connect(copyButton, &QPushButton::clicked, this, &AboutDialog::OnCopyButtonClicked);
    connect(okButton, &QPushButton::clicked, this, &AboutDialog::OnCloseButtonClicked);

    return buttonLayout;
}

QFrame* AboutDialog::CreateSeparator(void)
{
    QFrame* line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    line->setStyleSheet("background-color: #3c3f41; margin: 10px 0;");
    return line;
}

void AboutDialog::SetupConnections(void)
{

}

void AboutDialog::OnCopyButtonClicked(void)
{
    QString versionInfo = m_appTitleLabel->text() + " " +
                          m_versionLabel->text() + "\n" +
                          m_fpgaLabel->text() + "\n" +
                          m_kernelLabel->text();

    QApplication::clipboard()->setText(versionInfo);
    QToolTip::showText(QCursor::pos(), "版本信息已复制到剪贴板", this);
}

void AboutDialog::OnCloseButtonClicked(void)
{
    accept();
}

/* ===== Toolbar :: About End ===== */
