#-------------------------------------------------
#
# Project created by QtCreator 2018-03-14T12:17:54
#
#-------------------------------------------------

QT       += core gui sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
include(../cfg/cfg_opencv4.pri)

TARGET = milla-view
TEMPLATE = app


SOURCES += main.cpp\
        mviewer.cpp \
    thumbnailmodel.cpp \
    sresultmodel.cpp \
    searchform.cpp \
    mimagelistmodel.cpp \
    starlabel.cpp \
    exportform.cpp \
    mimpexpmodule.cpp \
    facedetector.cpp \
    cvhelper.cpp \
    pluginloader.cpp \
    dbhelper.cpp \
    mmatcher.cpp \
    mmemorymodel.cpp \
    mimageops.cpp \
    storyselector.cpp \
    mimageloader.cpp \
    listeditor.cpp \
    mdnnbase.cpp \
    mcolornet.cpp

HEADERS  += mviewer.h \
    thumbnailmodel.h \
    sresultmodel.h \
    searchform.h \
    mimagelistmodel.h \
    db_format.h \
    starlabel.h \
    exportform.h \
    mimpexpmodule.h \
    plugins.h \
    facedetector.h \
    cvhelper.h \
    pluginloader.h \
    dbhelper.h \
    mmatcher.h \
    shared.h \
    mmemorymodel.h \
    mimageops.h \
    storyselector.h \
    mimageloader.h \
    listeditor.h \
    mdnnbase.h \
    mcolornet.h

FORMS    += mviewer.ui \
    searchform.ui \
    exportform.ui \
    storyselector.ui \
    listeditor.ui

CONFIG   += c++14
QMAKE_CXXFLAGS  += -Wextra

RESOURCES += milla-view.qrc

DESTDIR = ../bin
