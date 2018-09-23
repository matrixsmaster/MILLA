CV_PREFIX = /mnt/data/libs/opencv/local_static/lib
CV_3RDP_PREFIX = $$CV_PREFIX/opencv4/3rdparty

INCLUDEPATH += $$CV_PREFIX/../include/opencv4

QMAKE_LIBS += $$CV_PREFIX/libopencv_objdetect.a $$CV_PREFIX/libopencv_dnn.a $$CV_PREFIX/libopencv_highgui.a
QMAKE_LIBS += $$CV_PREFIX/libopencv_imgcodecs.a $$CV_PREFIX/libopencv_imgproc.a $$CV_PREFIX/libopencv_core.a

QMAKE_LFLAGS += -Wl,--gc-sections

LIBS += -L$$CV_3RDP_PREFIX -littnotify -llibprotobuf -llibwebp -lippiw -lippicv
#LIBS += -ltiff -ljpeg -lpng -lm -lz -ldl -lgtk-3 -lgdk-3 -lcairo -lgdk_pixbuf-2.0 -lgobject-2.0 -lglib-2.0 -lIlmImf
LIBS += -lgtk-3 -lgdk-3 -lcairo -lgdk_pixbuf-2.0 -lgobject-2.0 -lglib-2.0 -lpng -lz -ltiff -ljasper -ljpeg -lIlmImf -ldl -lm -lpthread -lrt
