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
    mmatcher.cpp

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
    shared.h

FORMS    += mviewer.ui \
    searchform.ui \
    exportform.ui

CONFIG   += c++14
QMAKE_CXXFLAGS  += -Wextra

LIBS     += -lopencv_core -lopencv_imgproc -lopencv_features2d -lopencv_flann -lopencv_objdetect -lopencv_highgui

RESOURCES+= milla-view.qrc
