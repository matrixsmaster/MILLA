#ifndef DBHELPER_H
#define DBHELPER_H

#include <QString>
#include <QStringList>
#include <QCryptographicHash>
#include <QDir>
#include "db_format.h"
#include "mimpexpmodule.h"
#include "cvhelper.h"
#include "searchform.h"


class DBHelper
{
public:
    DBHelper();
    virtual ~DBHelper() {}

    bool initDatabase();

    static QString timePrinter(double sec);

    static QByteArray getSHA256(QString const &fn, qint64 *size);

    bool updateStatRecord(QString const &fn, MImageExtras &rec, bool update = true);

    MImageExtras getExtrasFromDB(QString const &fn);

    bool insertTag(QString const &ntag, unsigned &key);

    bool updateTags(QString const &tag, bool checked);

    bool updateFileTags(QString const &fn, MTagCache const &cache);

    int updateFileKudos(QString const &fn, int delta);

    bool updateFileNotes(QString const &fn, QString &notes);

    bool createLinkBetweenImages(QByteArray const &left, QByteArray const &right);

    QStringList getLinkedImages(QByteArray const &sha);

    QStringList tagSearch(MTagCache const &cache, QList<MImageListRecord>* within = nullptr);

    QStringList parametricSearch(SearchFormData flt, QList<MImageListRecord> const &from);

    void sanitizeLinks(progressCB progress_cb);

    void sanitizeTags(progressCB progress_cb);
};

#endif // DBHELPER_H
