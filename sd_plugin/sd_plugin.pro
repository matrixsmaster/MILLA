QT     += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = sd_plugin

TEMPLATE = lib

CONFIG += c++20 plugin
INCLUDEPATH += ../milla-view

DESTDIR = ../share/plugins

SOURCES += \
    main.cpp \
    sdcfgdialog.cpp \
    sdplugin.cpp

HEADERS += \
    sdcfgdialog.h \
    sdplugin.h

FORMS += \
    sdcfgdialog.ui

DISTFILES += \
    sd_plugin.json
