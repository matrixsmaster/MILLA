#include "thumbnailmodel.h"

ThumbnailModel::ThumbnailModel(std::list<QString> files, QObject *parent)
    : QAbstractListModel(parent)
{
    for (auto iter = files.begin(); iter != files.end(); ++iter)
    {
        QPixmap large(*iter);
        PixmapPair *pair = new PixmapPair();
        pair->_file = *iter;
        pair->_large = large;
        pair->_small = large.scaled(100, 100, Qt::KeepAspectRatio, Qt::SmoothTransformation);

        QFileInfo fi(*iter);
        pair->_short = fi.fileName();

        _data.append(pair);
    }
}

ThumbnailModel::~ThumbnailModel()
{
    qDebug() << "Deleting ThumbnailModel";
    qDeleteAll(_data);
}

int ThumbnailModel::rowCount(const QModelIndex &parent) const
{
    // This function should return the number of rows contained in the parent
    // parameter, the parent parameter is used for trees in order to retrieve the
    // number of rows contained in each node. Since we are doing a list each element
    // doesn't have child nodes so we return 0
    // By convention an invalid parent means the topmost level of a tree. In our case
    // we return the number of elements contained in our data store.
    if (parent.isValid())
        return 0;
    else
        return _data.count();
}

QVariant ThumbnailModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid())
    {
        switch (role)
        {
            case Qt::DecorationRole:
            {
                // DecorationRole = Icon show for a list
                return _data.value(index.row())->_small;
            }
            case Qt::DisplayRole:
            {
                // DisplayRole = Displayed text
                return _data.value(index.row())->_short;
            }
            case LargePixmapRole:
            {
                // This is a custom role, it will help us getting the pixmap more
                // easily later.
                return _data.value(index.row())->_large;
            }
        }
    }

    // returning a default constructed QVariant, will let Views knows we have nothing
    // to do and we let the default behavior of the view do work for us.
    return QVariant();
}
