QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = life_generator

TEMPLATE = lib
CONFIG += c++14 plugin
INCLUDEPATH += ../milla-view

SOURCES +=\
    lifegendlg.cpp \
    lifegenplugin.cpp

HEADERS  += \
    lifegendlg.h \
    lifegenplugin.h

FORMS    += \
    lifegendlg.ui

DESTDIR = ../share/plugins

DISTFILES += \
    lifegenerator.json
