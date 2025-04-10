QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = life_generator

TEMPLATE = lib
CONFIG += c++14 plugin
INCLUDEPATH += ../milla-view

SOURCES += main.cpp\
        dialog.cpp \
    lifegendlg.cpp \
    lifegenplugin.cpp

HEADERS  += dialog.h \
    lifegendlg.h \
    lifegenplugin.h

FORMS    += dialog.ui \
    lifegendlg.ui

DESTDIR = ../share/plugins

DISTFILES += \
    lifegenerator.json

QMAKE_LFLAGS += -Wl,--export-dynamic
