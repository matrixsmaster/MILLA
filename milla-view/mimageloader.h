#ifndef MIMAGELOADER_H
#define MIMAGELOADER_H

#include <QObject>
#include <QPixmap>
#include <QBuffer>
#include <QStringList>
#include <QDirIterator>
#include <QFileInfo>
#include <QMessageBox>
#include "shared.h"
#include "pluginloader.h"

class MImageLoader : public QObject
{
    Q_OBJECT

public:
    explicit MImageLoader(MillaPluginLoader* pLoader, QObject *parent = 0);
    virtual ~MImageLoader() {}

    bool isLoadableFile(QString const &path, QString *canonicalPath);

    QStringList getSupported()  { return supported; }

    QStringList open(QString const &filename);
    QStringList append(QString const &filename, bool strict);

    QStringList getList()       { return accum; }
    void clearList()            { accum.clear(); }

    QPixmap load(QString const &filename);
    MImageListRecord loadFull(QString const &filename, bool fast = false);

private:
    MillaPluginLoader* plugins;
    QStringList supported = MILLA_DEFAULT_FORMATS;
    QStringList accum;
    int recursive = -1;

    void scanDirectory(QString const &dir, QStringList &addto, bool recursive);
    QStringList openDirByFile(QString const &fileName, bool recursive);
    QStringList openDirByList(QString const &fileName);

    QStringList _open(QString const &filename, bool strict);
    void thumb(MImageListRecord &rec, bool force = false, bool fast = false);
};

#endif // MIMAGELOADER_H
