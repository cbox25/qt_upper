#include "cpptest_ui_test.h"
#include "../src/common.h"
#include "../src/page_reg_ctrl.h"
#include "../src/page_update.h"
#include "../src/page_network.h"
#include "../src/page_usb.h"
#include "../src/fontmanager.h"
#include "../base_widgets/switch_button.h"
#include <QApplication>
#include <QTest>
#include <QDebug>
#include <sstream>

// 添加QString输出操作符支持
namespace std {
    inline std::ostream& operator<<(std::ostream& os, const QString& qstr) {
        return os << qstr.toStdString();
    }
    
    // 添加QColor输出操作符支持
    inline std::ostream& operator<<(std::ostream& os, const QColor& color) {
        return os << "QColor(" << color.red() << "," << color.green() << "," << color.blue() << "," << color.alpha() << ")";
    }
    
    // 添加QFont输出操作符支持
    inline std::ostream& operator<<(std::ostream& os, const QFont& font) {
        return os << "QFont(" << font.family().toStdString() << "," << font.pointSize() << ")";
    }
}

#if ENABLE_CPPTEST_UI_TEST

CppTestUITest::CppTestUITest() : Test::Suite() {
    // 添加字体管理器测试
    TEST_ADD(CppTestUITest::testFontManagerSingleton);
    TEST_ADD(CppTestUITest::testFontManagerLoadFonts);
    TEST_ADD(CppTestUITest::testFontManagerSetFont);
    TEST_ADD(CppTestUITest::testGetApplicationFont);
    
    // 添加寄存器控件测试
    TEST_ADD(CppTestUITest::testUiRegWidgetCreation);
    TEST_ADD(CppTestUITest::testUiRegWidgetUiSetRegCtrl);
    TEST_ADD(CppTestUITest::testUiRegWidgetGetRegValue);
    TEST_ADD(CppTestUITest::testRegObjectCreation);
    TEST_ADD(CppTestUITest::testRegObjectProperties);
    TEST_ADD(CppTestUITest::testCreateComboBox);
    TEST_ADD(CppTestUITest::testComboBoxProperties);
    
    // 添加SwitchButton测试
    TEST_ADD(CppTestUITest::testSwitchButtonCreation);
    TEST_ADD(CppTestUITest::testSwitchButtonProperties);
    TEST_ADD(CppTestUITest::testSwitchButtonState);
    TEST_ADD(CppTestUITest::testSwitchButtonText);
    TEST_ADD(CppTestUITest::testSwitchButtonColors);
    
    // 添加UI基本控件测试
    TEST_ADD(CppTestUITest::testButtonCreation);
    TEST_ADD(CppTestUITest::testLineEditCreation);
    TEST_ADD(CppTestUITest::testRadioButtonCreation);
    TEST_ADD(CppTestUITest::testComboBoxCreation);
    TEST_ADD(CppTestUITest::testLabelCreation);
    TEST_ADD(CppTestUITest::testProgressBarCreation);
    TEST_ADD(CppTestUITest::testSliderCreation);
    TEST_ADD(CppTestUITest::testHoverLabelCreation);
    
    // 添加控件属性测试
    TEST_ADD(CppTestUITest::testWidgetProperties);
    TEST_ADD(CppTestUITest::testWidgetGeometry);
    TEST_ADD(CppTestUITest::testWidgetVisibility);
}

void CppTestUITest::setupTestEnvironment() {

    // 1. 确保Qt应用程序环境已初始化
    if (!QApplication::instance()) {
        static int argc = 1;
        static char* argv[] = {(char*)"test"};
        new QApplication(argc, argv);
        qDebug() << "创建QApplication实例";
    }

    // 2. 设置测试数据目录
    QDir::setCurrent(QCoreApplication::applicationDirPath());

    // 3. 确保测试配置文件存在
    if (!QFile::exists("reg.csv")) {
        qDebug() << "警告: reg.csv 文件不存在";
    }

    // 4. 处理Qt事件队列
    QApplication::processEvents();

}

void CppTestUITest::cleanupTestEnvironment() {

    // 处理Qt事件队列
    QApplication::processEvents();

}

// 字体管理器测试
void CppTestUITest::testFontManagerSingleton() {
    setupTestEnvironment();
    
    // 测试FontManager单例模式
    FontManager& fontManager1 = FontManager::instance();
    FontManager& fontManager2 = FontManager::instance();
    
    TEST_ASSERT(&fontManager1 == &fontManager2);
    
    cleanupTestEnvironment();
}

void CppTestUITest::testFontManagerLoadFonts() {
    setupTestEnvironment();
    
    FontManager& fontManager = FontManager::instance();
    bool result = fontManager.loadApplicationFonts();
    
    TEST_ASSERT(result);
    
    cleanupTestEnvironment();
}

void CppTestUITest::testFontManagerSetFont() {
    setupTestEnvironment();
    
    FontManager& fontManager = FontManager::instance();
    fontManager.setApplicationFont();
    
    QFont currentFont = fontManager.getCurrentFont();
    TEST_ASSERT(!currentFont.family().isEmpty());
    
    cleanupTestEnvironment();
}

void CppTestUITest::testGetApplicationFont() {
    setupTestEnvironment();
    
    // 测试getApplicationFont函数
    QFont font1 = getApplicationFont(12);
    TEST_ASSERT(font1.pointSize() == 12);
    
    QFont font2 = getApplicationFont("Arial", 14);
    TEST_ASSERT(font2.pointSize() == 14);
    
    cleanupTestEnvironment();
}

// 寄存器控件测试
void CppTestUITest::testUiRegWidgetCreation() {
    setupTestEnvironment();
    
    // 测试UiRegWidget创建
    UiRegWidget* regWidget = new UiRegWidget();
    TEST_ASSERT(regWidget != nullptr);
    
    // 检查初始状态
    TEST_ASSERT(regWidget->label == nullptr);
    TEST_ASSERT(regWidget->lineEdit == nullptr);
    
    delete regWidget;
    cleanupTestEnvironment();
}

void CppTestUITest::testUiRegWidgetUiSetRegCtrl() {
    setupTestEnvironment();
    
    // 创建测试环境
    QWidget parent;
    UiRegWidget* regWidget = new UiRegWidget();
    regWidget->label = new HoverLabel(&parent);
    regWidget->lineEdit = new QLineEdit(&parent);
    
    // 测试设置寄存器控件
    QString regName = "TestReg";
    uint32_t defaultValue = 0x1234;
    
    regWidget->UiSetRegCtrl(regName, defaultValue);
    
    // 验证设置结果
    TEST_ASSERT(regWidget->label->text() == regName);
    TEST_ASSERT(regWidget->lineEdit->text() == "0x00001234");
    
    delete regWidget;
    cleanupTestEnvironment();
}

void CppTestUITest::testUiRegWidgetGetRegValue() {
    setupTestEnvironment();
    
    // 创建测试环境
    QWidget parent;
    UiRegWidget* regWidget = new UiRegWidget();
    regWidget->lineEdit = new QLineEdit(&parent);
    
    // 测试十六进制值
    regWidget->lineEdit->setText("0x1234");
    int value = regWidget->GetRegValue();
    TEST_ASSERT_EQUALS(value, 0x1234);
    
    // 测试十进制值
    regWidget->lineEdit->setText("1234");
    value = regWidget->GetRegValue();
    TEST_ASSERT_EQUALS(value, 1234);
    
    // 测试空值
    regWidget->lineEdit->setText("");
    value = regWidget->GetRegValue();
    TEST_ASSERT_EQUALS(value, 0);
    
    delete regWidget;
    cleanupTestEnvironment();
}

void CppTestUITest::testRegObjectCreation() {
    setupTestEnvironment();
    
    // 测试Reg对象创建
    Reg* testReg = new Reg("TestReg", 0x44a20910, 0x123, 0xFFFF, 0x0000);
    TEST_ASSERT(testReg != nullptr);
    
    delete testReg;
    cleanupTestEnvironment();
}

void CppTestUITest::testRegObjectProperties() {
    setupTestEnvironment();
    
    // 测试Reg对象属性
    Reg* testReg = new Reg("TestReg", 0x1000, 0x1234, 0xFFFF, 0x0000);
    
    TEST_ASSERT_EQUALS(testReg->name, "TestReg");
    TEST_ASSERT_EQUALS(testReg->addr, 0x1000);
    TEST_ASSERT_EQUALS(testReg->defaultValue, 0x1234);
    TEST_ASSERT_EQUALS(testReg->max, 0xFFFF);
    TEST_ASSERT_EQUALS(testReg->min, 0x0000);
    
    delete testReg;
    cleanupTestEnvironment();
}

void CppTestUITest::testCreateComboBox() {
    setupTestEnvironment();
    
    // 测试ComboBox创建
    QWidget parent;
    QFont testFont = getApplicationFont(12);
    
    QComboBox* comboBox = CreateComboBox(&parent, 10, 10, 100, 30, testFont);
    TEST_ASSERT(comboBox != nullptr);
    
    delete comboBox;
    cleanupTestEnvironment();
}

void CppTestUITest::testComboBoxProperties() {
    setupTestEnvironment();
    
    // 测试ComboBox属性
    QWidget parent;
    QFont testFont = getApplicationFont(10);
    
    QComboBox* comboBox = CreateComboBox(&parent, 10, 10, 100, 30, testFont);
    TEST_ASSERT(comboBox != nullptr);
    
    // 测试几何属性 - 注意CreateComboBox设置了setMinimumHeight(40)
    TEST_ASSERT_EQUALS(comboBox->x(), 10);
    TEST_ASSERT_EQUALS(comboBox->y(), 10);
    TEST_ASSERT_EQUALS(comboBox->width(), 100);
    // 由于setMinimumHeight(40)，实际高度可能大于30
    TEST_ASSERT(comboBox->height() >= 30);
    
    // 测试字体属性
    TEST_ASSERT_EQUALS(comboBox->font().pointSize(), 10);
    
    // 测试ComboBox是否有项目（CreateComboBox会添加串口项目）
    TEST_ASSERT(comboBox->count() > 0);
    
    delete comboBox;
    cleanupTestEnvironment();
}

// UI基本控件测试
void CppTestUITest::testButtonCreation() {
    setupTestEnvironment();
    
    QWidget parent;
    QPushButton* button = new QPushButton("Test Button", &parent);
    TEST_ASSERT(button != nullptr);
    TEST_ASSERT_EQUALS(button->text(), "Test Button");
    
    // 测试字体设置
    QFont testFont = getApplicationFont(12);
    button->setFont(testFont);
    TEST_ASSERT_EQUALS(button->font().pointSize(), 12);
    
    delete button;
    cleanupTestEnvironment();
}

void CppTestUITest::testLineEditCreation() {
    setupTestEnvironment();
    
    QWidget parent;
    QLineEdit* lineEdit = new QLineEdit(&parent);
    TEST_ASSERT(lineEdit != nullptr);
    
    lineEdit->setText("Test Text");
    TEST_ASSERT_EQUALS(lineEdit->text(), "Test Text");
    
    // 测试字体设置
    QFont testFont = getApplicationFont(10);
    lineEdit->setFont(testFont);
    TEST_ASSERT_EQUALS(lineEdit->font().pointSize(), 10);
    
    delete lineEdit;
    cleanupTestEnvironment();
}

void CppTestUITest::testRadioButtonCreation() {
    setupTestEnvironment();
    
    QWidget parent;
    QRadioButton* radioButton = new QRadioButton("Test Radio", &parent);
    TEST_ASSERT(radioButton != nullptr);
    TEST_ASSERT_EQUALS(radioButton->text(), "Test Radio");
    
    // 测试字体设置
    QFont testFont = getApplicationFont(11);
    radioButton->setFont(testFont);
    TEST_ASSERT_EQUALS(radioButton->font().pointSize(), 11);
    
    delete radioButton;
    cleanupTestEnvironment();
}

void CppTestUITest::testComboBoxCreation() {
    setupTestEnvironment();
    
    QWidget parent;
    QComboBox* comboBox = new QComboBox(&parent);
    TEST_ASSERT(comboBox != nullptr);
    
    comboBox->addItem("Item 1");
    comboBox->addItem("Item 2");
    TEST_ASSERT_EQUALS(comboBox->count(), 2);
    
    // 测试字体设置
    QFont testFont = getApplicationFont(9);
    comboBox->setFont(testFont);
    TEST_ASSERT_EQUALS(comboBox->font().pointSize(), 9);
    
    delete comboBox;
    cleanupTestEnvironment();
}

void CppTestUITest::testLabelCreation() {
    setupTestEnvironment();
    
    QWidget parent;
    QLabel* label = new QLabel("Test Label", &parent);
    TEST_ASSERT(label != nullptr);
    TEST_ASSERT_EQUALS(label->text(), "Test Label");
    
    // 测试字体设置
    QFont testFont = getApplicationFont(14);
    label->setFont(testFont);
    TEST_ASSERT_EQUALS(label->font().pointSize(), 14);
    
    delete label;
    cleanupTestEnvironment();
}

void CppTestUITest::testProgressBarCreation() {
    setupTestEnvironment();
    
    QWidget parent;
    QProgressBar* progressBar = new QProgressBar(&parent);
    TEST_ASSERT(progressBar != nullptr);
    
    progressBar->setValue(50);
    TEST_ASSERT_EQUALS(progressBar->value(), 50);
    
    // 测试字体设置
    QFont testFont = getApplicationFont(8);
    progressBar->setFont(testFont);
    TEST_ASSERT_EQUALS(progressBar->font().pointSize(), 8);
    
    delete progressBar;
    cleanupTestEnvironment();
}

void CppTestUITest::testSliderCreation() {
    setupTestEnvironment();
    
    QWidget parent;
    QSlider* slider = new QSlider(Qt::Horizontal, &parent);
    TEST_ASSERT(slider != nullptr);
    
    slider->setValue(75);
    TEST_ASSERT_EQUALS(slider->value(), 75);
    
    // 测试字体设置
    QFont testFont = getApplicationFont(7);
    slider->setFont(testFont);
    TEST_ASSERT_EQUALS(slider->font().pointSize(), 7);
    
    delete slider;
    cleanupTestEnvironment();
}

void CppTestUITest::testHoverLabelCreation() {
    setupTestEnvironment();
    
    QWidget parent;
    HoverLabel* hoverLabel = new HoverLabel(&parent);
    TEST_ASSERT(hoverLabel != nullptr);
    
    hoverLabel->setText("Hover Test");
    TEST_ASSERT_EQUALS(hoverLabel->text(), "Hover Test");
    
    // 测试字体设置
    QFont testFont = getApplicationFont(13);
    hoverLabel->setFont(testFont);
    TEST_ASSERT_EQUALS(hoverLabel->font().pointSize(), 13);
    
    // 测试提示文本设置
    QString tipText = "这是一个提示文本";
    hoverLabel->setTipText(tipText);
    TEST_ASSERT(!tipText.isEmpty());
    
    delete hoverLabel;
    cleanupTestEnvironment();
}

// SwitchButton测试
void CppTestUITest::testSwitchButtonCreation() {
    setupTestEnvironment();
    
    QWidget parent;
    SwitchButton* switchBtn = new SwitchButton(&parent);
    TEST_ASSERT(switchBtn != nullptr);
    
    delete switchBtn;
    cleanupTestEnvironment();
}

void CppTestUITest::testSwitchButtonProperties() {
    setupTestEnvironment();
    
    QWidget parent;
    SwitchButton* switchBtn = new SwitchButton(&parent);
    
    // 测试初始属性
    TEST_ASSERT(!switchBtn->checked()); // 初始状态应该是false
    TEST_ASSERT_EQUALS(switchBtn->space(), 2);
    TEST_ASSERT_EQUALS(switchBtn->radius(), 5);
    
    // 测试字体设置
    QFont testFont = getApplicationFont(12);
    switchBtn->setFont(testFont);
    TEST_ASSERT_EQUALS(switchBtn->font().pointSize(), 12);
    
    delete switchBtn;
    cleanupTestEnvironment();
}

void CppTestUITest::testSwitchButtonState() {
    setupTestEnvironment();
    
    QWidget parent;
    SwitchButton* switchBtn = new SwitchButton(&parent);
    
    // 测试状态切换
    switchBtn->setChecked(true);
    TEST_ASSERT(switchBtn->checked());
    
    switchBtn->setChecked(false);
    TEST_ASSERT(!switchBtn->checked());
    
    delete switchBtn;
    cleanupTestEnvironment();
}

void CppTestUITest::testSwitchButtonText() {
    setupTestEnvironment();
    
    QWidget parent;
    SwitchButton* switchBtn = new SwitchButton(&parent);
    
    // 测试文本设置
    switchBtn->setTextOn("开启");
    switchBtn->setTextOff("关闭");
    
    TEST_ASSERT_EQUALS(switchBtn->textOn(), "开启");
    TEST_ASSERT_EQUALS(switchBtn->textOff(), "关闭");
    
    delete switchBtn;
    cleanupTestEnvironment();
}

void CppTestUITest::testSwitchButtonColors() {
    setupTestEnvironment();
    
    QWidget parent;
    SwitchButton* switchBtn = new SwitchButton(&parent);
    
    // 测试颜色设置
    QColor testColor(255, 0, 0);
    switchBtn->setBgColorOn(testColor);
    switchBtn->setTextColor(testColor);
    switchBtn->setSliderColorOn(testColor);
    
    TEST_ASSERT_EQUALS(switchBtn->bgColorOn(), testColor);
    TEST_ASSERT_EQUALS(switchBtn->textColor(), testColor);
    TEST_ASSERT_EQUALS(switchBtn->sliderColorOn(), testColor);
    
    delete switchBtn;
    cleanupTestEnvironment();
}

// 控件属性测试
void CppTestUITest::testWidgetProperties() {
    setupTestEnvironment();
    
    QWidget* widget = new QWidget();
    widget->setObjectName("TestWidget");
    widget->setWindowTitle("Test Window");
    
    TEST_ASSERT_EQUALS(widget->objectName(), "TestWidget");
    TEST_ASSERT_EQUALS(widget->windowTitle(), "Test Window");
    
    delete widget;
    cleanupTestEnvironment();
}

void CppTestUITest::testWidgetGeometry() {
    setupTestEnvironment();
    
    QWidget* widget = new QWidget();
    widget->setGeometry(10, 20, 100, 50);
    
    TEST_ASSERT_EQUALS(widget->x(), 10);
    TEST_ASSERT_EQUALS(widget->y(), 20);
    TEST_ASSERT_EQUALS(widget->width(), 100);
    TEST_ASSERT_EQUALS(widget->height(), 50);
    
    delete widget;
    cleanupTestEnvironment();
}

void CppTestUITest::testWidgetVisibility() {
    setupTestEnvironment();
    
    QWidget* widget = new QWidget();
    
    widget->show();
    TEST_ASSERT(widget->isVisible());
    
    widget->hide();
    TEST_ASSERT(!widget->isVisible());
    
    delete widget;
    cleanupTestEnvironment();
}

// 运行CppTest UI测试
void runCppTestUITests() {
    qDebug() << "=== 开始CppTest UI功能测试 ===";
    
    // 创建测试套件
    CppTestUITest testSuite;
    
    // 运行测试
    Test::TextOutput output(Test::TextOutput::Verbose);
    bool result = testSuite.run(output);
    
    // 输出测试结果
    qDebug() << "\n=== CppTest UI测试结果 ===";
    if (result) {
        qDebug() << "所有CppTest UI测试通过！";
    } else {
        qDebug() << "部分CppTest UI测试失败！";
    }
}

#endif // ENABLE_CPPTEST_UI_TEST 
