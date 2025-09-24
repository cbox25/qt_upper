#ifndef TEST_CONFIG_H
#define TEST_CONFIG_H

// 测试配置开关
#define ENABLE_UI_TESTING 1        // 启用UI测试
#define ENABLE_FONT_MANAGER_TEST 1 // 启用字体管理器测试
#define ENABLE_SWITCH_BUTTON_TEST 1 // 启用SwitchButton测试
#define ENABLE_HOVER_LABEL_TEST 1   // 启用HoverLabel测试
#define ENABLE_REG_WIDGETS_TEST 1   // 启用寄存器控件测试
#define ENABLE_UI_EVENTS_TEST 1     // 启用UI事件测试

// 测试详细程度
#define UI_TEST_VERBOSE 1           // 详细输出
#define UI_TEST_SHOW_PROGRESS 1     // 显示进度

// 测试超时设置
#define UI_TEST_TIMEOUT_MS 5000     // 测试超时时间(毫秒)
#define UI_TEST_EVENT_WAIT_MS 100   // 事件等待时间(毫秒)
#define UI_TEST_ANIMATION_WAIT_MS 500 // 动画等待时间(毫秒)

// 测试环境设置
#define UI_TEST_CREATE_WIDGETS 1    // 创建测试控件
#define UI_TEST_SIMULATE_EVENTS 1   // 模拟事件
#define UI_TEST_CHECK_SIGNALS 1     // 检查信号

// 调试设置
#define UI_TEST_DEBUG 1             // 启用调试输出
#define UI_TEST_LOG_TO_FILE 0       // 日志输出到文件

// 测试数据
#define UI_TEST_SAMPLE_TEXT "测试文本"
#define UI_TEST_SAMPLE_VALUE 12345
#define UI_TEST_SAMPLE_COLOR QColor(255, 0, 0)

// 测试开关组合
#define ENABLE_ALL_TESTS (ENABLE_UI_TESTING && ENABLE_FONT_MANAGER_TEST && ENABLE_SWITCH_BUTTON_TEST && ENABLE_HOVER_LABEL_TEST && ENABLE_REG_WIDGETS_TEST && ENABLE_UI_EVENTS_TEST)

#endif // TEST_CONFIG_H 