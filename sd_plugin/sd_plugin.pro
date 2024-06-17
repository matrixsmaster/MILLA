QT     += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = sd_plugin

TEMPLATE = lib

CONFIG += c++20 plugin
INCLUDEPATH += ../milla-view

DESTDIR = ../share/plugins

SOURCES += \
    ggml-alloc.c \
    ggml-backend.c \
    ggml-quants.c \
    ggml.c \
    main.cpp \
    model.cpp \
    sdcfgdialog.cpp \
    sdplugin.cpp \
    stable-diffusion.cpp \
    upscaler.cpp \
    util.cpp \
    zip.c

HEADERS += \
    ggml-alloc.h \
    ggml-backend-impl.h \
    ggml-backend.h \
    ggml-common.h \
    ggml-impl.h \
    ggml-quants.h \
    ggml.h \
    json.hpp \
    miniz.h \
    model.h \
    sdcfgdialog.h \
    sdplugin.h \
    stable-diffusion.h \
    stb_image.h \
    stb_image_resize.h \
    util.h \
    zip.h

FORMS += \
    sdcfgdialog.ui

DISTFILES += \
    sd_plugin.json
