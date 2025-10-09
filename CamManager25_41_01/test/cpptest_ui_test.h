#ifndef CPPTEST_UI_TEST_H
#define CPPTEST_UI_TEST_H

#include "cpptest.h"
#include <QWidget>
#include <QApplication>
#include <QDebug>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QRadioButton>
#include <QLabel>
#include <QProgressBar>
#include <QSlider>
#include <QFont>
#include <QColor>
#include <QSize>
#include <QPoint>
#include <QString>
#include <QVariant>
#include <QTimer>
#include <QToolTip>
#include "test_config.h"

// 测试开关宏
#define ENABLE_CPPTEST_UI_TEST ENABLE_UI_TESTING

#if ENABLE_CPPTEST_UI_TEST

// CppTest UI测试套件
class CppTestUITest : public Test::Suite {
public:
    CppTestUITest();
    
private:
    // 字体管理器测试
    void testFontManagerSingleton();
    void testFontManagerLoadFonts();
    void testFontManagerSetFont();
    void testGetApplicationFont();
    
    // 寄存器控件测试
    void testUiRegWidgetCreation();
    void testUiRegWidgetUiSetRegCtrl();
    void testUiRegWidgetGetRegValue();
    void testRegObjectCreation();
    void testRegObjectProperties();
    void testCreateComboBox();
    void testComboBoxProperties();
    
    // SwitchButton测试
    void testSwitchButtonCreation();
    void testSwitchButtonProperties();
    void testSwitchButtonState();
    void testSwitchButtonText();
    void testSwitchButtonColors();
    
    // UI基本控件测试
    void testButtonCreation();
    void testLineEditCreation();
    void testRadioButtonCreation();
    void testComboBoxCreation();
    void testLabelCreation();
    void testProgressBarCreation();
    void testSliderCreation();
    void testHoverLabelCreation();
    
    // 控件属性测试
    void testWidgetProperties();
    void testWidgetGeometry();
    void testWidgetVisibility();
    
    // 辅助函数
    void setupTestEnvironment();
    void cleanupTestEnvironment();
};

// 测试函数声明
void runCppTestUITests();

#endif // ENABLE_CPPTEST_UI_TEST

#endif // CPPTEST_UI_TEST_H 
