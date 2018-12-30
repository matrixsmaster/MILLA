#ifndef SHARED_H
#define SHARED_H

#include <QObject>
#include <QAction>
#include <QString>
#include <QPixmap>
#include <QVariant>
#include <map>
#include <functional>


#define MILLA_VERSION           "0.5.3.4"
#define MILLA_CLI_BANNER        "MILLA:  Qt5-based, AI-enhanced image viewer ver."
#define MILLA_CLI_COPYRIGHT     "(C) Dmitry 'MatrixS_Master' Solovyev, 2018-2019. All rights reserved."
#define MILLA_CONFIG_PATH       "/.milla/"
#define MILLA_SITE              "http://github.com/matrixsmaster/MILLA"

#define MILLA_EXTRA_CACHE_SIZE  900
#define MILLA_MAXMATCH_RESULTS  10
#define MILLA_MAXTAG_RESULTS    300
#define MILLA_MAX_RECENT_DIRS   10
#define MILLA_SCALE_UP          1.1
#define MILLA_SCALE_DOWN        0.9
#define MILLA_VIEW_TIMER        15
#define MILLA_MIN_CORREL_MATCH  0.05

#define MILLA_THUMBNAIL_SIZE    100
#define MILLA_MAXSHORTLENGTH    24
#define MILLA_MAXPICSBYTES      (1024*1024*1024)

#define MILLA_DEFAULT_FORMATS   { "png", "jpg", "jpeg", "bmp" }
#define MILLA_LIST_FORMATS      { "txt", "lst" }
#define MILLA_OPEN_FILE         "All supported files"
#define MILLA_OPEN_LIST         "Text list files"
#define MILLA_SAVE_FILE         "Supported formats (*.png, *.jpg, *.bmp)"

#define FACE_CASCADE_FILE       "/../share/face_cascade.xml"
#define COLORIZATION_NET_FILE   "/../share/colors.prototxt"
#define COLORIZATION_NET_WEIGHT "/../share/colors.caffemodel"
#define POSDETECTOR_NET_FILE    "/../share/poses.prototxt"
#define POSDETECTOR_NET_WEIGHT  "/../share/poses.caffemodel"


typedef std::map<QString,std::pair<unsigned,Qt::CheckState>> MTagCache;

typedef std::function<bool(QString)> InitRecCB;
typedef std::function<bool(double)> ProgressCB;
typedef std::function<QVariant(QString,QVariant)> PlugConfCB;
typedef std::function<void(QString)> LoadFileCB;

struct MImageListRecord {
    QString filename, fnshort;
    QPixmap thumb, picture;
    time_t touched = 0, filechanged = 0;
    bool loaded = false;
    bool thumbOK = false;
    bool generated = false;
    bool valid = false;
};

Q_DECLARE_METATYPE(MImageListRecord)

typedef QObject* QObjectPtr;

Q_DECLARE_METATYPE(QObjectPtr)

#endif // SHARED_H

