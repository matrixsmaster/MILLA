QT     += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
include(../cfg/cfg_anna.pri)

TARGET = anna_plugin

TEMPLATE = lib
CONFIG += c++20 plugin
INCLUDEPATH += ../milla-view

DESTDIR = ../share/plugins

SOURCES += \
    $$ANNA_PATH/netclient.cpp \
    annacfgdialog.cpp \
    annaplugin.cpp \
    main.cpp

FORMS += \
    annacfgdialog.ui

HEADERS += \
    $$ANNA_PATH/lfs.h \
    $$ANNA_PATH/md5calc.h \
    $$ANNA_PATH/netclient.h \
    $$ANNA_PATH/server/base64m.h \
    $$ANNA_PATH/server/codec.h \
    $$ANNA_PATH/vecstore.h \
    annacfgdialog.h \
    annaplugin.h

DISTFILES += \
    anna_plugin.json

DEFINES += _XOPEN_SOURCE=600

QMAKE_CFLAGS += -fPIC
QMAKE_CXXFLAGS += -fPIC

QMAKE_CFLAGS_DEBUG += -O0 -g
QMAKE_CFLAGS_RELEASE += -DNDEBUG -Ofast
QMAKE_CXXFLAGS_DEBUG += -O0 -g
QMAKE_CXXFLAGS_RELEASE += -DNDEBUG -Ofast

INCLUDEPATH += $$ANNA_PATH $$ANNA_PATH/server
DEPENDPATH += $$ANNA_PATH $$ANNA_PATH/server

LIBS += -L$$ANNA_PATH -lanna

equals(USE_CUBLAS,1) {
    DEFINES += GGML_USE_CUBLAS
    LIBS += $$CUBLAS_PATH -lcuda -lcublas -lculibos -lcudart -lcublasLt
}
