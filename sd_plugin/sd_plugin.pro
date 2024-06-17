QT     += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = sd_plugin

TEMPLATE = lib

CONFIG += c++20 plugin
INCLUDEPATH += ../milla-view

DESTDIR = ../share/plugins

SOURCES += \
    ggml-alloc.c \
    ggml-backend.c \
    ggml-quants.c \
    ggml.c \
    main.cpp \
    sdcfgdialog.cpp \
    sdplugin.cpp

HEADERS += \
    ggml-alloc.h \
    ggml-backend-impl.h \
    ggml-backend.h \
    ggml-common.h \
    ggml-impl.h \
    ggml-quants.h \
    ggml.h \
    sdcfgdialog.h \
    sdplugin.h

FORMS += \
    sdcfgdialog.ui

DISTFILES += \
    sd_plugin.json
