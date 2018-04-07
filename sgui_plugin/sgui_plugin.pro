#-------------------------------------------------
#
# Project created by QtCreator 2018-04-06T18:13:05
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = sgui_plugin

TEMPLATE = lib
CONFIG += c++14 plugin
INCLUDEPATH += ../milla-view ../libsgui ../libsgui/include/mlua

DEFINES += SGUI_HEADER_EXTERNAL_INCLUDE

SOURCES += main.cpp\
        dialog.cpp \
    sguiplugin.cpp \
    sguieventsink.cpp

HEADERS  += dialog.h \
    sguiplugin.h \
    sguieventsink.h

FORMS    += dialog.ui

LIBS    += -L../share/plugins -lsgui

DESTDIR = ../share/plugins

DISTFILES += \
    sguiplugin.json \
    cursors.json

RESOURCES += \
    sgui_plugin.qrc
