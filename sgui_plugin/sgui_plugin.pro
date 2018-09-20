#-------------------------------------------------
#
# Project created by QtCreator 2018-04-06T18:13:05
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
include(../cfg/cfg_sgui.pri)

TARGET = sgui_plugin

TEMPLATE = lib
CONFIG += c++14 plugin
INCLUDEPATH += ../milla-view

DEFINES += SGUI_HEADER_EXTERNAL_INCLUDE

SOURCES += main.cpp\
        dialog.cpp \
    sguiplugin.cpp \
    sguieventsink.cpp \
    vmouse.cpp

HEADERS  += dialog.h \
    sguiplugin.h \
    sguieventsink.h \
    vmouse.h

FORMS    += dialog.ui

DESTDIR = ../share/plugins

DISTFILES += \
    sguiplugin.json \
    cursors.json

RESOURCES += \
    sgui_plugin.qrc
