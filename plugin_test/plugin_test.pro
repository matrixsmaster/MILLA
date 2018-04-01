TEMPLATE        = lib
CONFIG         += c++14 plugin
QT             += core widgets
INCLUDEPATH    += ../milla-view
HEADERS         = testplugin.h
SOURCES         = testplugin.cpp
TARGET          = $$qtLibraryTarget(plugin_test)
DESTDIR         = ../plugins

DISTFILES += \
    testplugin.json

