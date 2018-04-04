#ifndef MIMAGELISTMODEL_H
#define MIMAGELISTMODEL_H

#include <ctime>
#include <initializer_list>
#include <QAbstractListModel>
#include <QPixmap>
#include <QFileInfo>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QDebug>
#include <QByteArray>
#include <QBuffer>
#include <QImageWriter>
#include <QDateTime>
#include "shared.h"

#define THUMBNAILSIZE 100
#define MAXSHORTLENGTH 24
#define MAXPICSBYTES 1024*1024*1024

class MImageListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles
    {
        LargePixmapRole = Qt::UserRole + 1,
        FullPathRole,
        FullDataRole
    };

    explicit MImageListModel(QObject *parent = 0);
    virtual ~MImageListModel();

    int columnCount(const QModelIndex &) const { return 1; }

    int rowCount(const QModelIndex &) const { return images.size(); }

    virtual QVariant data(const QModelIndex &index, int role) const;

    virtual QList<MImageListRecord>& GetAllImages() { return images; }

    virtual void setShortenFilenames(bool en) { do_shorten = en; }

    virtual QModelIndex getRecordIndex(int n) { return createIndex(n,0); }

    virtual QModelIndex getRecordIndex(const QString &fn, bool allowPartialMatch = false);

protected:
    QList<MImageListRecord> images;
    size_t ram_footprint = 0;
    bool do_shorten = false;

    virtual size_t ItemSizeInBytes(int idx);

    virtual size_t ItemSizeInBytes(MImageListRecord const &r);

    virtual void SaveThumbnail(MImageListRecord &rec) const;
};

#endif // MIMAGELISTMODEL_H
