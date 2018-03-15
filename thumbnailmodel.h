#ifndef THUMBNAILMODEL_H
#define THUMBNAILMODEL_H

#include <list>
#include <initializer_list>
#include <QAbstractListModel>
#include <QPixmap>
#include <QFileInfo>
#include <QDebug>

struct PixmapPair
{
        QString _file;
        QString _short;
        QPixmap _small;
        QPixmap _large;
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
        LargePixmapRole = Qt::UserRole + 1
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
    QList<PixmapPair*> _data;
};

#endif // THUMBNAILMODEL_H
