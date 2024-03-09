QT     += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = anna_plugin

TEMPLATE = lib
CONFIG += c++14 plugin
INCLUDEPATH += ../milla-view

DESTDIR = ../share/plugins

SOURCES += \
    annacfgdialog.cpp \
    annaplugin.cpp \
    main.cpp

FORMS += \
    annacfgdialog.ui

HEADERS += \
    annacfgdialog.h \
    annaplugin.h

DISTFILES += \
    anna_plugin.json
