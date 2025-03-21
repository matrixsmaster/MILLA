QT += core gui sql
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
include(../cfg/cfg_sd.pri)

TARGET = sd_plugin
TEMPLATE = lib
CONFIG += c++20 plugin
INCLUDEPATH += ../milla-view ggml/include

QMAKE_CFLAGS += $$ARCH_CONFIG -fPIC
QMAKE_CXXFLAGS += $$ARCH_CONFIG -fPIC
QMAKE_CFLAGS_DEBUG += -O0 -g
QMAKE_CFLAGS_RELEASE += -DNDEBUG -O3
QMAKE_CXXFLAGS_DEBUG += -O0 -g
QMAKE_CXXFLAGS_RELEASE += -DNDEBUG -O3

DESTDIR = ../share/plugins

SOURCES += \
    main.cpp \
    model.cpp \
    sdcfgdialog.cpp \
    sdplugin.cpp \
    stable-diffusion.cpp \
    upscaler.cpp \
    util.cpp \
    zip.c

HEADERS += \
    ggml_extend.hpp \
    clip.hpp \
    common.hpp \
    control.hpp \
    denoiser.hpp \
    esrgan.hpp \
    conditioner.hpp \
    darts.h \
    json.hpp \
    lora.hpp \
    miniz.h \
    mmdit.hpp \
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

#QMAKE_PRE_LINK = cd $$PWD && $(MAKE) -f ggml-make

DEFINES += GGML_MAX_NAME=128 GGML_USE_CPU
LIBS += -L$$PWD/ggml/build/src -lggml -lggml-base -lggml-cpu

equals(USE_CUDA,1) {
    DEFINES += GGML_USE_CUDA SD_USE_CUDA
    LIBS += -L$$PWD/ggml/build/src/ggml-cuda -lggml-cuda $$CUDA_PATH -lcuda -lcublas -lculibos -lcudart -lcublasLt
}

extraclean.commands = cd $$PWD && $(MAKE) -f ggml-make clean;
clean.depends = extraclean
#QMAKE_EXTRA_TARGETS += clean extraclean
