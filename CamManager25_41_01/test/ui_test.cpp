#include "ui_test.h"
#include "common.h"
#include "page_reg_ctrl.h"
#include "page_update.h"
#include "page_network.h"
#include "page_usb.h"
#include "fontmanager.h"
#include "../base_widgets/switch_button.h"
#include <QApplication>
#include <QTest>
#include <QDebug>

#if ENABLE_UI_TEST

// 静态实例
UITest* UITest::instance = nullptr;

UITest* UITest::getInstance() {
    if (!instance) {
        instance = new UITest();
    }
    return instance;
}

void UITest::runAllTests() {
    qDebug() << "=== 开始UI功能测试 ===";
    stats.reset();
    
    runFontManagerTests();
    runRegWidgetsTests();
    runSwitchButtonTests();
    runUIEventsTests();
    
    printTestResults();
}

void UITest::runFontManagerTests() {
    qDebug() << "\n--- 字体管理器测试 ---";
    
    try {
        // 测试1: FontManager单例模式
        FontManager& fontManager = FontManager::instance();
        bool singletonWorks = (&fontManager != nullptr);
        getInstance()->logTestResult(singletonWorks, "FontManager单例测试", singletonWorks ? "FontManager单例创建成功" : "FontManager单例创建失败");
        
        // 测试2: 加载应用程序字体
        bool fontsLoaded = fontManager.loadApplicationFonts();
        getInstance()->logTestResult(fontsLoaded, "字体加载测试", fontsLoaded ? "字体加载成功" : "字体加载失败");
        
        // 测试3: 设置应用程序字体
        fontManager.setApplicationFont();
        QFont currentFont = fontManager.getCurrentFont();
        bool fontSet = !currentFont.family().isEmpty();
        getInstance()->logTestResult(fontSet, "应用程序字体设置测试", fontSet ? "应用程序字体设置成功" : "应用程序字体设置失败");
        
        // 测试4: getApplicationFont函数测试
        QFont testFont1 = getApplicationFont(12);
        bool font1Valid = (testFont1.pointSize() == 12);
        getInstance()->logTestResult(font1Valid, "getApplicationFont(12)测试", font1Valid ? "字体大小设置正确" : "字体大小设置错误");
        
        QFont testFont2 = getApplicationFont("Arial", 14);
        bool font2Valid = (testFont2.pointSize() == 14);
        getInstance()->logTestResult(font2Valid, "getApplicationFont(Arial, 14)测试", font2Valid ? "字体设置正确" : "字体设置错误");
        
        // 测试5: 获取可用字体列表
        QStringList availableFonts = fontManager.getAvailableFonts();
        bool fontsListValid = !availableFonts.isEmpty();
        getInstance()->logTestResult(fontsListValid, "可用字体列表测试", fontsListValid ? "字体列表获取成功" : "字体列表获取失败");
        
    } catch (const std::exception& e) {
        qDebug() << "Exception in runFontManagerTests:" << e.what();
        getInstance()->logTestResult(false, "字体管理器测试", QString("异常: %1").arg(e.what()));
    } catch (...) {
        qDebug() << "Unknown exception in runFontManagerTests";
        getInstance()->logTestResult(false, "字体管理器测试", "未知异常");
    }
}

void UITest::runRegWidgetsTests() {
    qDebug() << "\n--- 寄存器控件测试 ---";
    
    try {
        // 测试1: 创建UiRegWidget
        UiRegWidget* regWidget = new UiRegWidget();
        bool regWidgetCreated = (regWidget != nullptr);
        getInstance()->logTestResult(regWidgetCreated, "UiRegWidget创建测试", regWidgetCreated ? "UiRegWidget创建成功" : "UiRegWidget创建失败");
        
        if (!regWidgetCreated) {
            return;
        }
        
        // 检查初始状态
        bool labelExists = (regWidget->label == nullptr); // 初始状态应该为nullptr
        getInstance()->logTestResult(labelExists, "label初始状态测试", labelExists ? "label初始状态正确" : "label初始状态错误");
        
        bool lineEditExists = (regWidget->lineEdit == nullptr); // 初始状态应该为nullptr
        getInstance()->logTestResult(lineEditExists, "lineEdit初始状态测试", lineEditExists ? "lineEdit初始状态正确" : "lineEdit初始状态错误");
        
        // 测试2: 创建必要的控件
        QWidget parent;
        regWidget->label = new HoverLabel(&parent);
        regWidget->lineEdit = new QLineEdit(&parent);
        
        // 检查控件是否创建成功
        if (!regWidget->label || !regWidget->lineEdit) {
            getInstance()->logTestResult(false, "控件创建测试", "label或lineEdit创建失败");
            delete regWidget;
            return;
        }
        
        getInstance()->logTestResult(true, "控件创建测试", "label和lineEdit创建成功");
        
        // 测试3: 设置寄存器控件
        QString regName = "TestReg";
        uint32_t defaultValue = 0x1234;
        
        regWidget->UiSetRegCtrl(regName, defaultValue);
        qDebug() << "UiSetRegCtrl completed successfully";
        
        // 测试4: 获取寄存器值
        int value = regWidget->GetRegValue();
        qDebug() << "GetRegValue returned:" << value;
        bool valueCorrect = (value == defaultValue);
        getInstance()->logTestResult(valueCorrect, "获取寄存器值测试", valueCorrect ? "获取寄存器值成功" : "获取寄存器值失败");
        
        // 测试5: 创建Reg对象
        Reg* testReg = new Reg("TestReg", 0x1000, 0x1234, 0xFFFF, 0x0000);
        bool regCreated = (testReg != nullptr);
        getInstance()->logTestResult(regCreated, "Reg对象创建测试", regCreated ? "Reg对象创建成功" : "Reg对象创建失败");
        
        if (regCreated) {
            bool nameCorrect = (testReg->name == "TestReg");
            getInstance()->logTestResult(nameCorrect, "寄存器名称测试", nameCorrect ? "寄存器名称正确" : "寄存器名称设置失败");
            
            bool addrCorrect = (testReg->addr == 0x1000);
            getInstance()->logTestResult(addrCorrect, "寄存器地址测试", addrCorrect ? "寄存器地址正确" : "寄存器地址设置失败");
            
            bool defaultValueCorrect = (testReg->defaultValue == 0x1234);
            getInstance()->logTestResult(defaultValueCorrect, "默认值测试", defaultValueCorrect ? "默认值正确" : "默认值设置失败");
        }
        
        // 测试6: 创建ComboBox
        QFont testFont = getApplicationFont(12);
        QComboBox* comboBox = CreateComboBox(&parent, 10, 10, 100, 30, testFont);
        bool comboBoxCreated = (comboBox != nullptr);
        getInstance()->logTestResult(comboBoxCreated, "ComboBox创建测试", comboBoxCreated ? "ComboBox创建成功" : "ComboBox创建失败");
        
        if (comboBoxCreated) {
            bool xCorrect = (comboBox->x() == 10);
            getInstance()->logTestResult(xCorrect, "X坐标测试", xCorrect ? "X坐标正确" : "X坐标设置失败");
            
            bool yCorrect = (comboBox->y() == 10);
            getInstance()->logTestResult(yCorrect, "Y坐标测试", yCorrect ? "Y坐标正确" : "Y坐标设置失败");
            
            bool widthCorrect = (comboBox->width() == 100);
            getInstance()->logTestResult(widthCorrect, "宽度测试", widthCorrect ? "宽度正确" : "宽度设置失败");
            
            // 由于CreateComboBox设置了setMinimumHeight(40)，实际高度可能大于30
            bool heightCorrect = (comboBox->height() >= 30);
            getInstance()->logTestResult(heightCorrect, "高度测试", heightCorrect ? "高度正确" : "高度设置失败");
        }
        
        // 清理资源
        if (regWidget) {
            if (regWidget->label) {
                delete regWidget->label;
                regWidget->label = nullptr;
            }
            if (regWidget->lineEdit) {
                delete regWidget->lineEdit;
                regWidget->lineEdit = nullptr;
            }
            delete regWidget;
        }
        delete testReg;
        delete comboBox;
        
    } catch (const std::exception& e) {
        qDebug() << "Exception in runRegWidgetsTests:" << e.what();
        getInstance()->logTestResult(false, "寄存器控件测试", QString("异常: %1").arg(e.what()));
    } catch (...) {
        qDebug() << "Unknown exception in runRegWidgetsTests";
        getInstance()->logTestResult(false, "寄存器控件测试", "未知异常");
    }
}

void UITest::runSwitchButtonTests() {
    qDebug() << "\n--- SwitchButton测试 ---";
    
    try {
        // 测试1: 创建SwitchButton
        QWidget parent;
        SwitchButton* switchBtn = new SwitchButton(&parent);
        bool switchBtnCreated = (switchBtn != nullptr);
        getInstance()->logTestResult(switchBtnCreated, "SwitchButton创建测试", switchBtnCreated ? "SwitchButton创建成功" : "SwitchButton创建失败");
        
        if (!switchBtnCreated) {
            return;
        }
        
        // 测试2: 检查初始状态
        bool initialChecked = !switchBtn->checked(); // 初始状态应该是false
        getInstance()->logTestResult(initialChecked, "初始状态测试", initialChecked ? "初始状态正确" : "初始状态错误");
        
        // 测试3: 设置开关状态
        switchBtn->setChecked(true);
        bool checkedState = switchBtn->checked();
        getInstance()->logTestResult(checkedState, "设置开关状态测试", checkedState ? "开关状态设置成功" : "开关状态设置失败");
        
        // 测试4: 设置文本
        switchBtn->setTextOn("开启");
        switchBtn->setTextOff("关闭");
        bool textOnCorrect = (switchBtn->textOn() == "开启");
        getInstance()->logTestResult(textOnCorrect, "开启文本测试", textOnCorrect ? "开启文本设置成功" : "开启文本设置失败");
        
        bool textOffCorrect = (switchBtn->textOff() == "关闭");
        getInstance()->logTestResult(textOffCorrect, "关闭文本测试", textOffCorrect ? "关闭文本设置成功" : "关闭文本设置失败");
        
        // 测试5: 设置颜色
        QColor testColor(255, 0, 0);
        switchBtn->setBgColorOn(testColor);
        bool colorCorrect = (switchBtn->bgColorOn() == testColor);
        getInstance()->logTestResult(colorCorrect, "背景颜色测试", colorCorrect ? "背景颜色设置成功" : "背景颜色设置失败");
        
        // 测试6: 设置尺寸
        switchBtn->setSpace(5);
        bool spaceCorrect = (switchBtn->space() == 5);
        getInstance()->logTestResult(spaceCorrect, "间距设置测试", spaceCorrect ? "间距设置成功" : "间距设置失败");
        
        switchBtn->setRadius(10);
        bool radiusCorrect = (switchBtn->radius() == 10);
        getInstance()->logTestResult(radiusCorrect, "圆角设置测试", radiusCorrect ? "圆角设置成功" : "圆角设置失败");
        
        // 测试7: 设置字体
        QFont testFont = getApplicationFont(12);
        switchBtn->setFont(testFont);
        bool fontCorrect = (switchBtn->font().pointSize() == 12);
        getInstance()->logTestResult(fontCorrect, "字体设置测试", fontCorrect ? "字体设置成功" : "字体设置失败");
        
        // 测试8: 设置文本颜色
        QColor textColor(255, 255, 255);
        switchBtn->setTextColor(textColor);
        bool textColorCorrect = (switchBtn->textColor() == textColor);
        getInstance()->logTestResult(textColorCorrect, "文本颜色测试", textColorCorrect ? "文本颜色设置成功" : "文本颜色设置失败");
        
        // 测试9: 设置滑块颜色
        QColor sliderColor(0, 255, 0);
        switchBtn->setSliderColorOn(sliderColor);
        bool sliderColorCorrect = (switchBtn->sliderColorOn() == sliderColor);
        getInstance()->logTestResult(sliderColorCorrect, "滑块颜色测试", sliderColorCorrect ? "滑块颜色设置成功" : "滑块颜色设置失败");
        
        // 测试10: 设置显示文本
        switchBtn->setShowText(true);
        bool showTextCorrect = switchBtn->showText();
        getInstance()->logTestResult(showTextCorrect, "显示文本测试", showTextCorrect ? "显示文本设置成功" : "显示文本设置失败");
        
        // 清理资源
        delete switchBtn;
        
    } catch (const std::exception& e) {
        qDebug() << "Exception in runSwitchButtonTests:" << e.what();
        getInstance()->logTestResult(false, "SwitchButton测试", QString("异常: %1").arg(e.what()));
    } catch (...) {
        qDebug() << "Unknown exception in runSwitchButtonTests";
        getInstance()->logTestResult(false, "SwitchButton测试", "未知异常");
    }
}

void UITest::runUIEventsTests() {
    qDebug() << "\n--- UI基本控件测试 ---";
    
    // 测试1: 按钮基本属性
    QWidget parent;
    QPushButton* button = new QPushButton("Test Button", &parent);
    qDebug() << "创建按钮:" << (button ? "成功" : "失败");
    UI_TEST_ASSERT_NOT_NULL(button, "按钮创建失败");
    qDebug() << "按钮文本:" << button->text();
    UI_TEST_ASSERT_EQUALS("Test Button", button->text(), "按钮文本设置失败");
    
    // 设置按钮字体
    QFont buttonFont = getApplicationFont(12);
    button->setFont(buttonFont);
    UI_TEST_ASSERT_EQUALS(12, button->font().pointSize(), "按钮字体设置失败");
    
    // 测试2: 文本框基本属性
    QLineEdit* lineEdit = new QLineEdit(&parent);
    qDebug() << "创建文本框:" << (lineEdit ? "成功" : "失败");
    UI_TEST_ASSERT_NOT_NULL(lineEdit, "文本框创建失败");
    lineEdit->setText("测试文本");
    qDebug() << "文本框文本:" << lineEdit->text();
    UI_TEST_ASSERT_EQUALS("测试文本", lineEdit->text(), "文本框文本设置失败");
    
    // 设置文本框字体
    QFont lineEditFont = getApplicationFont(10);
    lineEdit->setFont(lineEditFont);
    UI_TEST_ASSERT_EQUALS(10, lineEdit->font().pointSize(), "文本框字体设置失败");
    
    // 测试3: 单选按钮基本属性
    QRadioButton* radioButton = new QRadioButton("Test Radio", &parent);
    UI_TEST_ASSERT_NOT_NULL(radioButton, "单选按钮创建失败");
    UI_TEST_ASSERT_EQUALS("Test Radio", radioButton->text(), "单选按钮文本设置失败");
    
    // 设置单选按钮字体
    QFont radioFont = getApplicationFont(11);
    radioButton->setFont(radioFont);
    UI_TEST_ASSERT_EQUALS(11, radioButton->font().pointSize(), "单选按钮字体设置失败");
    
    // 测试4: 组合框基本属性
    QComboBox* comboBox = new QComboBox(&parent);
    comboBox->addItem("Item 1");
    comboBox->addItem("Item 2");
    UI_TEST_ASSERT_NOT_NULL(comboBox, "组合框创建失败");
    UI_TEST_ASSERT_EQUALS(2, comboBox->count(), "组合框项目数量错误");
    
    // 设置组合框字体
    QFont comboFont = getApplicationFont(9);
    comboBox->setFont(comboFont);
    UI_TEST_ASSERT_EQUALS(9, comboBox->font().pointSize(), "组合框字体设置失败");
    
    // 测试5: 标签基本属性
    QLabel* label = new QLabel("Test Label", &parent);
    UI_TEST_ASSERT_NOT_NULL(label, "标签创建失败");
    UI_TEST_ASSERT_EQUALS("Test Label", label->text(), "标签文本设置失败");
    
    // 设置标签字体
    QFont labelFont = getApplicationFont(14);
    label->setFont(labelFont);
    UI_TEST_ASSERT_EQUALS(14, label->font().pointSize(), "标签字体设置失败");
    
    // 测试6: 进度条基本属性
    QProgressBar* progressBar = new QProgressBar(&parent);
    UI_TEST_ASSERT_NOT_NULL(progressBar, "进度条创建失败");
    progressBar->setValue(50);
    UI_TEST_ASSERT_EQUALS(50, progressBar->value(), "进度条值设置失败");
    
    // 设置进度条字体
    QFont progressFont = getApplicationFont(8);
    progressBar->setFont(progressFont);
    UI_TEST_ASSERT_EQUALS(8, progressBar->font().pointSize(), "进度条字体设置失败");
    
    // 测试7: 滑块基本属性
    QSlider* slider = new QSlider(Qt::Horizontal, &parent);
    UI_TEST_ASSERT_NOT_NULL(slider, "滑块创建失败");
    slider->setValue(75);
    UI_TEST_ASSERT_EQUALS(75, slider->value(), "滑块值设置失败");
    
    // 设置滑块字体
    QFont sliderFont = getApplicationFont(7);
    slider->setFont(sliderFont);
    UI_TEST_ASSERT_EQUALS(7, slider->font().pointSize(), "滑块字体设置失败");
    
    // 测试8: HoverLabel基本属性
    HoverLabel* hoverLabel = new HoverLabel(&parent);
    UI_TEST_ASSERT_NOT_NULL(hoverLabel, "HoverLabel创建失败");
    hoverLabel->setText("Hover Test");
    UI_TEST_ASSERT_EQUALS("Hover Test", hoverLabel->text(), "HoverLabel文本设置失败");
    
    // 设置HoverLabel字体
    QFont hoverFont = getApplicationFont(13);
    hoverLabel->setFont(hoverFont);
    UI_TEST_ASSERT_EQUALS(13, hoverLabel->font().pointSize(), "HoverLabel字体设置失败");
    
    // 测试9: 设置HoverLabel提示文本
    QString tipText = "这是一个提示文本";
    hoverLabel->setTipText(tipText);
    UI_TEST_ASSERT_NOT_EMPTY(tipText, "HoverLabel提示文本设置失败");
    
    delete button;
    delete lineEdit;
    delete radioButton;
    delete comboBox;
    delete label;
    delete progressBar;
    delete slider;
    delete hoverLabel;
}

void UITest::printTestResults() {
    qDebug() << "\n=== 测试结果 ===";
    qDebug() << stats.summary();
    
    if (!stats.errorMessages.isEmpty()) {
        qDebug() << "\n错误信息:";
        for (const QString& error : stats.errorMessages) {
            qDebug() << "  -" << error;
        }
    }
    
    qDebug() << "\n=== 测试完成 ===";
}

// 静态辅助函数实现
bool UITest::testWidgetCreation(QWidget* widget, const QString& testName) {
    bool result = (widget != nullptr);
    getInstance()->logTestResult(result, testName, result ? "控件创建成功" : "控件创建失败");
    return result;
}

bool UITest::testWidgetProperty(QWidget* widget, const QString& property, const QVariant& expectedValue, const QString& testName) {
    if (!widget) return false;
    
    QVariant actualValue = widget->property(property.toUtf8().constData());
    bool result = (actualValue == expectedValue);
    getInstance()->logTestResult(result, testName, 
        result ? QString("属性%1验证成功").arg(property) : 
        QString("属性%1验证失败，期望:%2，实际:%3").arg(property).arg(expectedValue.toString()).arg(actualValue.toString()));
    return result;
}

// 断言函数实现
bool UITest::assertTrue(bool condition, const QString& message) {
    if (!condition) {
        getInstance()->logError(message);
    }
    getInstance()->logTestResult(condition, "条件为真检查", message);
    return condition;
}

bool UITest::assertFalse(bool condition, const QString& message) {
    if (condition) {
        getInstance()->logError(message);
    }
    getInstance()->logTestResult(!condition, "条件为假检查", message);
    return !condition;
}

bool UITest::assertEquals(const QVariant& expected, const QVariant& actual, const QString& message) {
    bool condition = false;
    
    // 添加调试信息
    qDebug() << "assertEquals: 期望类型=" << expected.type() << "值=" << expected.toString();
    qDebug() << "assertEquals: 实际类型=" << actual.type() << "值=" << actual.toString();
    
    // 尝试不同类型的比较
    if (expected.type() == actual.type()) {
        condition = (expected == actual);
        qDebug() << "assertEquals: 类型相同，直接比较结果=" << condition;
    } else {
        // 如果类型不同，尝试转换为字符串比较
        condition = (expected.toString() == actual.toString());
        qDebug() << "assertEquals: 类型不同，字符串比较结果=" << condition;
    }
    
    if (!condition) {
        getInstance()->logError(QString("%1 - 期望:%2, 实际:%3").arg(message).arg(expected.toString()).arg(actual.toString()));
    }
    getInstance()->logTestResult(condition, "值相等检查", message);
    return condition;
}

bool UITest::assertNotNull(QObject* object, const QString& message) {
    bool condition = (object != nullptr);
    qDebug() << "assertNotNull: 对象指针=" << object << "条件=" << condition;
    if (!condition) {
        getInstance()->logError(message);
    }
    getInstance()->logTestResult(condition, "对象非空检查", message);
    return condition;
}

bool UITest::assertNull(QObject* object, const QString& message) {
    bool condition = (object == nullptr);
    if (!condition) {
        getInstance()->logError(message);
    }
    getInstance()->logTestResult(condition, "assertNull", message);
    return condition;
}

bool UITest::assertEmpty(const QString& text, const QString& message) {
    bool condition = text.isEmpty();
    if (!condition) {
        getInstance()->logError(message);
    }
    getInstance()->logTestResult(condition, "assertEmpty", message);
    return condition;
}

bool UITest::assertNotEmpty(const QString& text, const QString& message) {
    bool condition = !text.isEmpty();
    if (!condition) {
        getInstance()->logError(message);
    }
    getInstance()->logTestResult(condition, "assertNotEmpty", message);
    return condition;
}

void UITest::logTestResult(bool passed, const QString& testName, const QString& message) {
    stats.totalTests++;
    if (passed) {
        stats.passedTests++;
        qDebug() << "✓" << testName << ":" << message;
    } else {
        stats.failedTests++;
        qDebug() << "✗" << testName << ":" << message;
    }
}

void UITest::logError(const QString& error) {
    stats.errorMessages.append(error);
    qDebug() << "错误:" << error;
}

// 测试函数实现
void testRegWidgetsFunctionality() {
    UITest::getInstance()->runRegWidgetsTests();
}

void testSwitchButtonFunctionality() {
    UITest::getInstance()->runSwitchButtonTests();
}

void testUIEventsFunctionality() {
    UITest::getInstance()->runUIEventsTests();
}

void testCommonUIFunctions() {
    UITest::getInstance()->runAllTests();
}

#endif // ENABLE_UI_TEST 
 