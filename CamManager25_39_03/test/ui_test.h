#ifndef UI_TEST_H
#define UI_TEST_H

#include <QWidget>
#include <QApplication>
#include <QDebug>
#include <QTest>
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
#define ENABLE_UI_TEST ENABLE_UI_TESTING

#if ENABLE_UI_TEST

// 测试结果枚举
enum TestResult {
    TEST_PASS = 0,
    TEST_FAIL = 1,
    TEST_ERROR = 2
};

// 测试统计结构
struct TestStats {
    int totalTests;
    int passedTests;
    int failedTests;
    int errorTests;
    QStringList errorMessages;
    
    TestStats() : totalTests(0), passedTests(0), failedTests(0), errorTests(0) {}
    
    void reset() {
        totalTests = passedTests = failedTests = errorTests = 0;
        errorMessages.clear();
    }
    
    QString summary() const {
        return QString("测试统计: 总计=%1, 通过=%2, 失败=%3, 错误=%4")
                .arg(totalTests).arg(passedTests).arg(failedTests).arg(errorTests);
    }
};

// UI测试类
class UITest {
public:
    static UITest* getInstance();
    void runAllTests();
    void runFontManagerTests();
    void runRegWidgetsTests();
    void runSwitchButtonTests();
    void runUIEventsTests();
    void printTestResults();
    
    // 测试辅助函数
    static bool testWidgetCreation(QWidget* widget, const QString& testName);
    static bool testWidgetProperty(QWidget* widget, const QString& property, const QVariant& expectedValue, const QString& testName);
    
    // 断言函数
    static bool assertTrue(bool condition, const QString& message);
    static bool assertFalse(bool condition, const QString& message);
    static bool assertEquals(const QVariant& expected, const QVariant& actual, const QString& message);
    static bool assertNotNull(QObject* object, const QString& message);
    template<typename T>
    static bool assertNotNull(T* object, const QString& message) {
        bool result = (object != nullptr);
        getInstance()->logTestResult(result, "对象非空检查", message);
        return result;
    }
    static bool assertNull(QObject* object, const QString& message);
    static bool assertEmpty(const QString& text, const QString& message);
    static bool assertNotEmpty(const QString& text, const QString& message);
    
private:
    UITest() {}
    ~UITest() {}
    UITest(const UITest&) = delete;
    UITest& operator=(const UITest&) = delete;
    
    static UITest* instance;
    TestStats stats;
    
    void logTestResult(bool passed, const QString& testName, const QString& message = "");
    void logError(const QString& error);
};

// 测试宏
#define UI_TEST_ASSERT_TRUE(condition, message) \
    UITest::assertTrue(condition, QString("%1: %2").arg(__FUNCTION__).arg(message))

#define UI_TEST_ASSERT_FALSE(condition, message) \
    UITest::assertFalse(condition, QString("%1: %2").arg(__FUNCTION__).arg(message))

#define UI_TEST_ASSERT_EQUALS(expected, actual, message) \
    UITest::assertEquals(QVariant(expected), QVariant(actual), QString("%1: %2").arg(__FUNCTION__).arg(message))

#define UI_TEST_ASSERT_NOT_NULL(object, message) \
    UITest::assertNotNull(static_cast<QObject*>(object), QString("%1: %2").arg(__FUNCTION__).arg(message))

#define UI_TEST_ASSERT_NULL(object, message) \
    UITest::assertNull(object, QString("%1: %2").arg(__FUNCTION__).arg(message))

#define UI_TEST_ASSERT_EMPTY(text, message) \
    UITest::assertEmpty(text, QString("%1: %2").arg(__FUNCTION__).arg(message))

#define UI_TEST_ASSERT_NOT_EMPTY(text, message) \
    UITest::assertNotEmpty(text, QString("%1: %2").arg(__FUNCTION__).arg(message))

// 测试函数声明
void testRegWidgetsFunctionality();
void testSwitchButtonFunctionality();
void testUIEventsFunctionality();
void testCommonUIFunctions();

#endif // ENABLE_UI_TEST

#endif // UI_TEST_H 