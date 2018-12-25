#ifndef DBCACHE_H
#define DBCACHE_H

#include <QString>
#include <QByteArray>
#include <QMap>
#include <QMultiMap>
#include <QStringList>
#include "db_format.h"
#include "shared.h"

class DBCache
{
public:
    DBCache(ProgressCB progress_cb);
    virtual ~DBCache();

    void addFile(QString const &fn, QByteArray const &sha);
    void removeFile(QString const &fn);

    QString getFilenameBySHA(QByteArray const &sha);
    QByteArray getSHAbyFilename(QString const &fn);

    void addLink(QByteArray const &left, QByteArray const &right);
    void removeLink(QByteArray const &left, QByteArray const &right);

    QStringList getLinkedTo(QByteArray const &sha, bool reverse);
    QStringList getLinkedTo(QString const &fn, bool reverse);

private:
    QMap<QString,QString> cache_names_forward;
    QMap<QString,QString> cache_names_reverse;
    QMultiMap<QString,QString> cache_links_forward;
    QMultiMap<QString,QString> cache_links_reverse;
};

#endif // DBCACHE_H
