#-------------------------------------------------
#
# Project created by QtCreator 2018-04-25T21:53:07
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = zip_plugin

TEMPLATE = lib
CONFIG += c++14 plugin
INCLUDEPATH += ../milla-view

SOURCES += main.cpp \
        zipcfgdialog.cpp \
        zip.cpp \
    zipplugin.cpp

LIBS     += -lz

HEADERS  += zipcfgdialog.h \
    zipreader.h \
    zipwriter.h \
    zipplugin.h

FORMS    += zipcfgdialog.ui

DISTFILES += \
    zipplugin.json

DESTDIR = ../share/plugins
