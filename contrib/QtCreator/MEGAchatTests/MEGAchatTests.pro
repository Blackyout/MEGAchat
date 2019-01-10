# DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x000000

debug_and_release {
    CONFIG -= debug_and_release
    CONFIG += debug_and_release
}
CONFIG(debug, debug|release) {
    CONFIG -= debug release
    CONFIG += debug
}
CONFIG(release, debug|release) {
    CONFIG -= debug release
    CONFIG += release
}

CONFIG -= qt
TARGET = megachat_tests
TEMPLATE = app

DEFINES += LOG_TO_LOGGER

CONFIG += USE_LIBUV
CONFIG += USE_MEGAAPI
CONFIG += USE_MEDIAINFO
CONFIG += USE_LIBWEBSOCKETS
CONFIG += USE_WEBRTC

macx {
    CONFIG += nofreeimage # there are symbols duplicated in libwebrtc.a. Discarded for the moment
}

include(../../../bindings/qt/megachat.pri)

DEPENDPATH += ../../../tests/sdk_test
INCLUDEPATH += ../../../tests/sdk_test

SOURCES +=  ../../../tests/sdk_test/sdk_test.cpp
HEADERS +=  ../../../tests/sdk_test/sdk_test.h

win32 {
    QMAKE_LFLAGS += /LARGEADDRESSAWARE
    QMAKE_LFLAGS_WINDOWS += /SUBSYSTEM:WINDOWS,5.01
    QMAKE_LFLAGS_CONSOLE += /SUBSYSTEM:CONSOLE,5.01
    DEFINES += PSAPI_VERSION=1
}

macx {
    QMAKE_CXXFLAGS += -DCRYPTOPP_DISABLE_ASM -D_DARWIN_C_SOURCE
    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.9 #the one used for webrtc
    QMAKE_CXXFLAGS += -fvisibility=hidden -fvisibility-inlines-hidden
    QMAKE_LFLAGS += -F /System/Library/Frameworks/Security.framework/
    DEFINES += WEBRTC_MAC
}

unix:debug|macx:debug{
    CONFIG += sanitizer sanitize_address
    CONFIG += QMAKE_COMMON_SANITIZE_CFLAGS
}
else {
    CONFIG -= sanitizer sanitize_address
    CONFIG -= QMAKE_COMMON_SANITIZE_CFLAGS
}

