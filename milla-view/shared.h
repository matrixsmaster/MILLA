#ifndef SHARED_H
#define SHARED_H

#include <QObject>
#include <QString>
#include <QPixmap>
#include <QVariant>
#include <map>
#include <functional>

typedef std::map<QString,std::pair<unsigned,Qt::CheckState>> MTagCache;

typedef std::function<bool(QString)> InitRecCB;
typedef std::function<bool(double)> ProgressCB;
typedef std::function<QVariant(QString,QVariant)> PlugConfCB;

struct MImageListRecord {
    QString filename, fnshort;
    QPixmap thumb, picture;
    time_t touched = 0, filechanged = 0;
    bool loaded = false;
    bool modified = true;
    bool valid = false;
};

Q_DECLARE_METATYPE(MImageListRecord)

#endif // SHARED_H
