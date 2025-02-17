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
#include <memory>
#include "db_format.h"
#include "shared.h"
#include "cvhelper.h"
#include "searchform.h"
#include "mimageops.h"
#include "dbcache.h"

typedef QList<std::tuple<QString,unsigned,bool>> MTagsCheckList;

class DBHelper : public QObject
{
    Q_OBJECT

public:
    DBHelper();
    virtual ~DBHelper();

    bool initDatabase(ProgressCB progress_cb, UserActionCB uaction_cb);

    bool updateDatabase(int ver);

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

    static QString getFileTagsString(QString const &fn);

    static bool updateTags(QString const &tag, bool checked);

    static bool updateFileTags(QString const &fn, MTagCache const &cache);

    static int updateFileKudos(QString const &fn, int delta);

    static QString getFileNotes(QString const &fn);

    static bool updateFileNotes(QString const &fn, QString &notes);

    bool createLinkBetweenImages(QByteArray const &left, QByteArray const &right, bool force = false, uint stamp = 0);

    bool removeLinkBetweenImages(QByteArray const &left, QByteArray const &right, bool force = false);

    QStringList getLinkedImages(QByteArray const &sha, bool reverse);

    QStringList getLinkedImages(QString const &fn, bool reverse);

    static QStringList tagSearch(MTagCache const &cache, QList<MImageListRecord>* within = nullptr, int maxitems = 0);

    void initParametricSearch(QList<MImageListRecord> const &from);

    QStringList doParametricSearch(SearchFormData flt, ProgressCB pcb);

    static QStringList getAllFiles();

    QString getFileBySHA(QByteArray const &sha);

    QByteArray getSHAbyFile(QString const &fn);

    bool removeFile(QString const &fn);

    void sanitizeFiles(ProgressCB progress_cb);

    void sanitizeLinks(ProgressCB progress_cb);

    void sanitizeTags(ProgressCB progress_cb);

    static void sanitizeDBMeta();

    static QString detectExactCopies(ProgressCB progress_cb);

    void setWinTableName(QString const &nm);

    QByteArray getWindowGeometryOrState(bool geom);

    bool updateWindowGeometryAndState(QByteArray const &geom, QByteArray const &state);

    bool restoreViewerState(QObjectList const &lst);

    bool updateViewerState(QObjectList const &lst);

    bool readRecentDirs(QMenu* add_to, int maxcount, LoadFileCB cb);

    bool clearRecentDirs(bool total = false);

    static bool addRecentDir(QString const &path, bool dir);

    static QString getMostRecentDir();

    static QString getMemorySlot(int n);

    static bool updateMemorySlot(int n, QString const &fn);

    static bool eraseMemorySlot(int n);

    static bool eraseMemory();

    static bool updateDirPath(QString const &path, const QString &last);

    static bool delDirPath(QString const &path);

    static bool eraseDirs();

    static QStringList getDirsList();

    static QString getDirLastFile(QString const &path);

    static QStringList getStoriesList();

    static bool updateStory(QString const &title, MImageOps* macro, uint stamp = 0);

    static bool updateStory(QString const &title, QString const &macro, uint stamp = 0);

    static bool loadStory(QString const &title, MImageOps* macro);

    static QString getExtraStringVal(QString const &key);

    static int getExtraInt(QString const &key);

    static bool setExtraStringVal(QString const &key, QString const &val);

    static bool setExtraInt(QString const &key, int val);

    static QString getDBInfoString();

    void invalidateCache();

private:
    std::map<time_t,QAction*> recents;
    QList<MImageListRecord> searchlist;
    QString wintable;
    std::shared_ptr<DBCache> cache;

    bool checkAndCreate(const char *tname, const char *format);
};

#endif // DBHELPER_H
