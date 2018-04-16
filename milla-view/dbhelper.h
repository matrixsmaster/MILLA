#ifndef DBHELPER_H
#define DBHELPER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QCryptographicHash>
#include <QFileInfo>
#include <QDir>
#include <QSplitter>
#include <QMenu>
#include <ctime>
#include "db_format.h"
#include "shared.h"
#include "cvhelper.h"
#include "searchform.h"

typedef QList<std::tuple<QString,unsigned,bool>> MTagsCheckList;

class DBHelper : public QObject
{
    Q_OBJECT

public:
    DBHelper();
    virtual ~DBHelper();

    bool initDatabase();

    static QString timePrinter(double sec);

    static QString getCanonicalPath(QString const &fn);

    static QByteArray getSHA256(QString const &fn, qint64 *size);

    static bool getThumbnail(MImageListRecord &rec);

    static bool updateThumbnail(MImageListRecord &rec, QByteArray const &png);

    static bool isStatRecordExists(QString const &fn);

    static bool updateStatRecord(QString const &fn, MImageExtras &rec, bool update = true);

    static MImageExtras getExtrasFromDB(QString const &fn);

    static time_t getLastViewTime(QString const &fn);

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

    static QStringList getLinkedImages(QByteArray const &sha, bool reverse);

    static QStringList tagSearch(MTagCache const &cache, QList<MImageListRecord>* within = nullptr, int maxitems = 0);

    static QStringList parametricSearch(SearchFormData flt, QList<MImageListRecord> const &from);

    static QStringList getAllFiles();

    static bool removeFile(QString const &fn);

    static void sanitizeFiles(ProgressCB progress_cb);

    static void sanitizeLinks(ProgressCB progress_cb);

    static void sanitizeTags(ProgressCB progress_cb);

    static QString detectExactCopies(ProgressCB progress_cb);

    static QByteArray getWindowGeometryOrState(bool geom);

    static bool updateWindowGeometryAndState(QByteArray const &geom, QByteArray const &state);

    static bool restoreViewerState(QObjectList const &lst);

    static bool updateViewerState(QObjectList const &lst);

    bool readRecentDirs(QMenu* add_to, int maxcount, LoadFileCB cb);

    bool clearRecentDirs(bool total = false);

    static bool addRecentDir(QString const &path, bool dir);

    static QString getMostRecentDir();

    static QString getMemorySlot(int n);

    static bool updateMemorySlot(int n, QString const &fn);

    static bool eraseMemory();

private:
    std::map<time_t,QAction*> recents;

    bool checkAndCreate(const char *tname, const char *format);
};

#endif // DBHELPER_H
