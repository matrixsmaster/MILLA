#-------------------------------------------------
#
# Project created by QtCreator 2018-04-05T13:20:54
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = pixel_mixer

TEMPLATE = lib
CONFIG += c++14 plugin
INCLUDEPATH += ../milla-view

SOURCES += main.cpp\
        dialog.cpp \
    pixelmixerplugin.cpp

HEADERS  += dialog.h \
    pixelmixerplugin.h

FORMS    += dialog.ui

DISTFILES += \
    pixelmixer.json

DESTDIR         = ../share/plugins
