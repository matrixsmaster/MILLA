#ifndef THUMBNAILMODEL_H
#define THUMBNAILMODEL_H

#include <list>
#include <initializer_list>
#include <QAbstractListModel>
#include <QPixmap>
#include <QFileInfo>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QDebug>
#include <QByteArray>
#include <QBuffer>
#include <QImageWriter>

#define THUMBNAILSIZE 100

struct PixmapPair
{
    QString filename;
    QString fnshort;
    QPixmap thumb;
    QPixmap picture;
    bool loaded;
    bool modified;
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
    int columnCount(const QModelIndex /*&parent*/) const { return 1; }
    int rowCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    // ========================================================

private:
    QList<PixmapPair*> images;

    void LoadUp(int idx) const;
};

#endif // THUMBNAILMODEL_H
