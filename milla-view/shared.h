#ifndef SHARED_H
#define SHARED_H

#include <QObject>
#include <QAction>
#include <QString>
#include <QPixmap>
#include <QVariant>
#include <map>
#include <functional>

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
    bool modified = true;
    bool valid = false;
};

Q_DECLARE_METATYPE(MImageListRecord)

typedef QObject* QObjectPtr;

Q_DECLARE_METATYPE(QObjectPtr)

struct MViewSettings {
    enum EFlags {
        FitToViewport = 0x1,
        ShowFaces = 0x2,
        CenterOnFace = 0x4,
        ShowLinked = 0x8,
        ShowReverseLinks = 0x10,
        ThumbCloud = 0x20,
        HotkeysEnabled = 0x40
    } flags;
    float scale;
};

#endif // SHARED_H

