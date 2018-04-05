#-------------------------------------------------
#
# Project created by QtCreator 2018-04-05T13:41:50
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = life_generator

TEMPLATE = lib
CONFIG += c++14 plugin
INCLUDEPATH += ../milla-view

SOURCES += main.cpp\
        dialog.cpp

HEADERS  += dialog.h

FORMS    += dialog.ui

DESTDIR = ../share/plugins

DISTFILES += \
    lifegenerator.json
