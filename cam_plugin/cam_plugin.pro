#-------------------------------------------------
#
# Project created by QtCreator 2018-04-11T16:07:04
#
#-------------------------------------------------

QT       += core gui sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

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

LIBS     += -lopencv_core -lopencv_imgproc -lopencv_highgui

DESTDIR = ../share/plugins

DISTFILES += \
    camplugin.json
