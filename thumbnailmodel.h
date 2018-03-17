#ifndef THUMBNAILMODEL_H
#define THUMBNAILMODEL_H

#include <ctime>
#include <list>
#include <map>
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

struct ThumbnailRec {
    QString filename, fnshort;
    QPixmap thumb, picture;
    bool loaded, modified;
    time_t touched, filechanged;
};

class ThumbnailModel : public QAbstractListModel
{
    Q_OBJECT
public:
    // QAbstractItemModel retrieves various information (like text, color, ...)
    // from the same index using roles. We can define custom ones, however to
    // avoid a clash with predefined roles, ours must start at Qt::UserRole.
    // All numbers below this one are reserved for Qt internals.
    enum Roles
    {
        LargePixmapRole = Qt::UserRole + 1,
        FullPathRole
    };

    explicit ThumbnailModel(std::list<QString> files, QObject *parent = 0);
    virtual ~ThumbnailModel();

    // QAbstractItemModel interface ===========================
public:
    int columnCount(const QModelIndex &) const { return 1; }
    int rowCount(const QModelIndex &) const { return images.size(); }
    QVariant data(const QModelIndex &index, int role) const;
    // ========================================================

    void LoadUp(int idx);
    void GC(int skip = -1);
    QList<ThumbnailRec*>& GetAllImages() { return images; }

private:
    QList<ThumbnailRec*> images;
    size_t ram_footprint;

    size_t ItemSizeInBytes(int idx);
};

#endif // THUMBNAILMODEL_H
