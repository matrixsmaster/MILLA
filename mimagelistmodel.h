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

#define THUMBNAILSIZE 100
#define MAXPICSBYTES 1024*1024*1024

struct MImageListRecord {
    QString filename, fnshort;
    QPixmap thumb, picture;
    time_t touched = 0, filechanged = 0;
    bool loaded = false;
    bool modified = true;
    bool valid = false;
};

Q_DECLARE_METATYPE(MImageListRecord)

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

protected:
    QList<MImageListRecord> images;

    virtual size_t ItemSizeInBytes(int idx);
};

#endif // MIMAGELISTMODEL_H