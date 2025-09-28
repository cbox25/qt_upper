QT       += core gui serialport network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# CppTest 配置
DEFINES += ENABLE_CPPTEST_UI_TEST

SOURCES += \
    ../base_widgets/switch_button.cpp \
    ../src/InitMainWindow.cpp \
    ../src/common.cpp \
    ../src/crc.cpp \
    ../src/lz4.c \
    ../src/page_net_assist.cpp \
    ../src/page_network.cpp \
    ../src/update_increase.cpp \
    main.cpp \
    mainwindow.cpp \
    ../src/page_reg_ctrl.cpp \
    ../src/page_update.cpp \
    ../src/uart.cpp \
    ../src/suite.cpp \
    ../src/source.cpp \
    ../src/time.cpp \
    ../src/compileroutput.cpp \
    ../src/htmloutput.cpp \
    ../src/textoutput.cpp \
    ../src/cpptest_ui_test.cpp

HEADERS += \
    ../base_widgets/switch_button.h \
    ../src/InitMainWindow.h \
    ../src/common.h \
    ../src/config.h \
    ../src/crc.h \
    ../src/lz4.h \
    ../src/page_net_assist.h \
    ../src/page_network.h \
    ../src/update_increase.h \
    mainwindow.h \
    ../src/page_reg_ctrl.h \
    ../src/page_update.h \
    ../src/uart.h \
    ../src/cpptest.h \
    ../src/cpptest-suite.h \
    ../src/cpptest-source.h \
    ../src/cpptest-time.h \
    ../src/cpptest-assert.h \
    ../src/cpptest-compileroutput.h \
    ../src/cpptest-htmloutput.h \
    ../src/cpptest-textoutput.h \
    ../src/cpptest_ui_test.h

FORMS += \
    mainwindow.ui

TRANSLATIONS += \
    pv4002_arm64_zh_CN.ts

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
