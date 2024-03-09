QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = mov_plugin

TEMPLATE = lib
CONFIG += c++14 plugin
INCLUDEPATH += ../milla-view

SOURCES += main.cpp\
        movcfgdialog.cpp \
    movplugin.cpp

HEADERS  += movcfgdialog.h \
    movplugin.h

FORMS    += movcfgdialog.ui

DESTDIR = ../share/plugins

DISTFILES += \
    movplugin.json
