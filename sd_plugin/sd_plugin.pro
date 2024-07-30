QT += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
include(../cfg/cfg_sd.pri)

TARGET = sd_plugin
TEMPLATE = lib
CONFIG += c++20 plugin
INCLUDEPATH += ../milla-view

QMAKE_CFLAGS += $$ARCH_CONFIG -fPIC
QMAKE_CXXFLAGS += $$ARCH_CONFIG -fPIC
QMAKE_CFLAGS_DEBUG += -O0 -g
QMAKE_CFLAGS_RELEASE += -DNDEBUG -O3
QMAKE_CXXFLAGS_DEBUG += -O0 -g
QMAKE_CXXFLAGS_RELEASE += -DNDEBUG -O3

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
    clip.hpp \
    common.hpp \
    control.hpp \
    denoiser.hpp \
    esrgan.hpp \
    ggml-alloc.h \
    ggml-backend-impl.h \
    ggml-backend.h \
    ggml-common.h \
    ggml-impl.h \
    ggml-quants.h \
    ggml.h \
    ggml_extend.hpp \
    json.hpp \
    lora.hpp \
    miniz.h \
    model.h \
    pmid.hpp \
    preprocessing.hpp \
    rng.hpp \
    rng_philox.hpp \
    sdcfgdialog.h \
    sdplugin.h \
    stable-diffusion.h \
    stb_image.h \
    stb_image_resize.h \
    tae.hpp \
    unet.hpp \
    util.h \
    vae.hpp \
    vocab.hpp \
    zip.h

FORMS += \
    sdcfgdialog.ui

DISTFILES += \
    sd_plugin.json

DEFINES += GGML_MAX_NAME=128

equals(USE_CUBLAS,1) {
    QMAKE_PRE_LINK = cd $$PWD && $(MAKE) -f cuda-make
    DEFINES += GGML_USE_CUDA GGML_USE_CUBLAS SD_USE_CUBLAS
    LIBS += -L$$PWD -lsdcuda $$CUBLAS_PATH -lcuda -lcublas -lculibos -lcudart -lcublasLt

    extraclean.commands = cd $$PWD && $(MAKE) -f cuda-make clean;
    clean.depends = extraclean
    QMAKE_EXTRA_TARGETS += clean extraclean
}
