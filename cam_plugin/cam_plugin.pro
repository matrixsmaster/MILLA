QT       += core gui sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
include(../cfg/cfg_opencv4.pri)

TEMPLATE = lib
CONFIG += c++14 plugin
INCLUDEPATH += ../milla-view

SOURCES += main.cpp\
        dialog.cpp \
    cameraplugin.cpp\
    ../milla-view/cvhelper.cpp

HEADERS  += dialog.h \
    cameraplugin.h

FORMS    += dialog.ui

DESTDIR = ../share/plugins

DISTFILES += \
    camplugin.json
