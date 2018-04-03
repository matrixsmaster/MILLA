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

typedef QList<std::tuple<QString,unsigned,bool>> MTagsCheckList;

class DBHelper
{
public:
    DBHelper() {}
    virtual ~DBHelper() {}

    bool initDatabase();

    static QString timePrinter(double sec);

    static QByteArray getSHA256(QString const &fn, qint64 *size);

    static bool getThumbnail(MImageListRecord &rec);

    static bool updateThumbnail(MImageListRecord &rec, QByteArray const &png);

    static bool isStatRecordExists(QString const &fn);

    static bool updateStatRecord(QString const &fn, MImageExtras &rec, bool update = true);

    static MImageExtras getExtrasFromDB(QString const &fn);

    static bool insertTag(QString const &ntag, unsigned &key);

    static int getFileRating(QString const &fn);

    static bool updateFileRating(QString const &fn, int n);

    static unsigned getFileViews(QString const &fn, bool &ok);

    static bool updateFileViews(QString const &fn, unsigned n);

    static MTagsCheckList getFileTags(QString const &fn);

    static bool updateTags(QString const &tag, bool checked);

    static bool updateFileTags(QString const &fn, MTagCache const &cache);

    static int updateFileKudos(QString const &fn, int delta);

    static QString getFileNotes(QString const &fn);

    static bool updateFileNotes(QString const &fn, QString &notes);

    static bool createLinkBetweenImages(QByteArray const &left, QByteArray const &right);

    static QStringList getLinkedImages(QByteArray const &sha);

    static QStringList tagSearch(MTagCache const &cache, QList<MImageListRecord>* within = nullptr, int maxitems = 0);

    static QStringList parametricSearch(SearchFormData flt, QList<MImageListRecord> const &from);

    static void sanitizeLinks(progressCB progress_cb);

    static void sanitizeTags(progressCB progress_cb);
};

#endif // DBHELPER_H
