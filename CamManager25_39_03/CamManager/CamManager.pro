QT       += core gui serialport network
QT       += multimediawidgets
QT       += testlib
QT       += core gui widgets serialport
LIBS     += -lws2_32

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17
CONFIG += debug

TEMPLATE = app
TARGET = CamManager


# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# 测试控制 - 注释掉下面这行可以禁用测试
# DEFINES += ENABLE_CPPTEST_UI_TEST



SOURCES += \
    ../base_widgets/switch_button.cpp \
    ../base_widgets/customtabbar.cpp \
    ../base_widgets/customtabwidget.cpp \
    ../src/common.cpp \
    ../src/crc.cpp \
    ../src/hash.cpp \
    ../src/init_main_window.cpp \
    ../src/lz4.c \
    ../src/multi_threads.cpp \
    ../src/page_net_assist.cpp \
    ../src/page_network.cpp \
    ../src/page_reg_ctrl.cpp \
    ../src/page_update.cpp \
    ../src/page_usb.cpp \
    ../src/uart.cpp \
    ../src/update_increase.cpp \
    ../src/usb.cpp \
    ../QLogger/QLogger.cpp \
    ../QLogger/QLoggerManager.cpp \
    ../QLogger/QLoggerProtocol.cpp \
    ../QLogger/QLoggerWriter.cpp \
    ../src/fontmanager.cpp \
    main.cpp \
    mainwindow.cpp

# 条件编译：只有在启用测试时才包含测试相关文件
contains(DEFINES, ENABLE_CPPTEST_UI_TEST) {
    SOURCES += \
        ../test/collectoroutput.cpp \
        ../test/compileroutput.cpp \
        ../test/source.cpp \
        ../test/suite.cpp \
        ../test/time.cpp \
        ../test/textoutput.cpp \
        ../test/missing.cpp \
        ../test/utils.cpp \
        ../test/cpptest_ui_test.cpp
}

HEADERS += \
    ../base_widgets/switch_button.h \
    ../base_widgets/customtabbar.h \
    ../base_widgets/customtabwidget.h \
    ../src/common.h \
    ../src/crc.h \
    ../src/hash.h \
    ../src/init_main_window.h \
    ../src/lz4.h \
    ../src/multi_threads.h \
    ../src/page_net_assist.h \
    ../src/page_network.h \
    ../src/page_reg_ctrl.h \
    ../src/page_update.h \
    ../src/page_usb.h \
    ../src/prj_config.h \
    ../src/uart.h \
    ../src/update_increase.h \
    ../src/usb.h \
    ../QLogger/QLogger.h \
    ../QLogger/QLoggerManager.h \
    ../QLogger/QLoggerProtocol.h \
    ../QLogger/QLoggerLevel.h \
    ../QLogger/QLoggerWriter.h \
    ../src/winconfig.h \
    ../src/fontmanager.h \
    mainwindow.h

# 条件编译：只有在启用测试时才包含测试相关头文件
contains(DEFINES, ENABLE_CPPTEST_UI_TEST) {
    HEADERS += \
        ../test/cpptest.h \
        ../test/cpptest-assert.h \
        ../test/cpptest-collectoroutput.h \
        ../test/cpptest-compileroutput.h \
        ../test/cpptest-source.h \
        ../test/missing.h \
        ../test/utils.h \
        ../test/cpptest_ui_test.h
}

# LIBS += -L../../../../lib/libusb-1.0
# LIBS += D:/work/qt_prj/camera4002_upper/qt_upper/lib/libusb-1.0

FORMS += \
    mainwindow.ui

TRANSLATIONS += \
    CamManager_zh_CN.ts
CONFIG += lrelease
CONFIG += embed_translations

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    ../libusb/libusb-1.0.def


win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../lib/MinGW64/release/ -libusb-1.0
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../lib/MinGW64/debug/ -llibusb-1.0
else:unix: LIBS += -L$$PWD/../lib/MinGW64/ -llibusb-1.0

INCLUDEPATH += $$PWD/../lib/MinGW64/static
DEPENDPATH += $$PWD/../lib/MinGW64/static

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/../lib/MinGW64/release/libusb-1.0.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/../lib/MinGW64/debug/libusb-1.0.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/../lib/MinGW64/release/libusb-1.0.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/../lib/MinGW64/debug/libusb-1.0.lib
else:unix: PRE_TARGETDEPS += $$PWD/../lib/MinGW64/libusb-1.0.a

RESOURCES += \
    res.qrc

# 目标输出目录
DESTDIR = ../bin
OBJECTS_DIR = ../obj
MOC_DIR = ../moc
