#-------------------------------------------------
#
# Project created by QtCreator 2018-03-14T12:17:54
#
#-------------------------------------------------

QT       += core gui sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = milla-view
TEMPLATE = app


SOURCES += main.cpp\
        mviewer.cpp \
    thumbnailmodel.cpp

HEADERS  += mviewer.h \
    thumbnailmodel.h

FORMS    += mviewer.ui

CONFIG   += c++11

LIBS     += -lopencv_core -lopencv_features2d -lopencv_flann -lopencv_nonfree -lopencv_highgui
