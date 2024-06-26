#CV_PREFIX = /usr/lib/x86_64-linux-gnu
#CV_3RDP_PREFIX = $$CV_PREFIX/opencv4/3rdparty

INCLUDEPATH += /usr/include/opencv4

#QMAKE_LIBS += $$CV_PREFIX/libopencv_video.a $$CV_PREFIX/libopencv_videoio.a
#QMAKE_LIBS += $$CV_PREFIX/libopencv_objdetect.a $$CV_PREFIX/libopencv_dnn.a $$CV_PREFIX/libopencv_highgui.a
#QMAKE_LIBS += $$CV_PREFIX/libopencv_imgcodecs.a $$CV_PREFIX/libopencv_imgproc.a $$CV_PREFIX/libopencv_core.a

#QMAKE_LIBS += $$CV_3RDP_PREFIX/liblibprotobuf.a $$CV_3RDP_PREFIX/libittnotify.a $$CV_3RDP_PREFIX/libIlmImf.a
#QMAKE_LIBS += $$CV_3RDP_PREFIX/libippiw.a $$CV_3RDP_PREFIX/libippicv.a

#QMAKE_LFLAGS += -Wl,--gc-sections

#LIBS += -L$$CV_3RDP_PREFIX -lIlmImf -llibjasper -lquirc
#LIBS += -lcblas -llapack -lwebp -lgtk-3 -lgdk-3 -lcairo -lgdk_pixbuf-2.0 -lgobject-2.0 -lglib-2.0 -lpng -lz -ltiff -ljpeg -ldl -lm -lpthread -lrt

LIBS += -lopencv_video -lopencv_videoio -lopencv_objdetect -lopencv_dnn -lopencv_highgui -lopencv_imgcodecs -lopencv_imgproc -lopencv_core
LIBS += -lpng -lz -ltiff -ljpeg -ldl -lm -lpthread -lrt
