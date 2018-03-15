#include "thumbnailmodel.h"

ThumbnailModel::ThumbnailModel(std::list<QString> files, QObject *parent)
    : QAbstractListModel(parent)
{
    for (auto iter = files.begin(); iter != files.end(); ++iter)
    {
        //QPixmap large(*iter);
        PixmapPair *pair = new PixmapPair();
        pair->filename = *iter;
        //pair->_large = large;
        //pair->_small = large.scaled(100, 100, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        pair->thumb = QPixmap(100,100);
        pair->loaded = false;

        QFileInfo fi(*iter);
        pair->fnshort = fi.fileName();

        images.append(pair);
    }
}

ThumbnailModel::~ThumbnailModel()
{
    qDebug() << "Deleting ThumbnailModel";
    qDeleteAll(images);
}

int ThumbnailModel::rowCount(const QModelIndex &parent) const
{
    // This function should return the number of rows contained in the parent
    // parameter, the parent parameter is used for trees in order to retrieve the
    // number of rows contained in each node. Since we are doing a list each element
    // doesn't have child nodes so we return 0
    // By convention an invalid parent means the topmost level of a tree. In our case
    // we return the number of elements contained in our images store.
    if (parent.isValid())
        return 0;
    else
        return images.count();
}

QVariant ThumbnailModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid())
    {
        switch (role)
        {
        case Qt::DecorationRole:
            // DecorationRole = Icon show for a list
            return images.value(index.row())->thumb;

        case Qt::DisplayRole:
            // DisplayRole = Displayed text
            return images.value(index.row())->fnshort;

        case LargePixmapRole:
            // This is a custom role, it will help us getting the pixmap more
            // easily later.
            LoadUp(index.row());
            return images.value(index.row())->picture;

        case FullPathRole:
            return images.value(index.row())->filename;

        }
    }

    // returning a default constructed QVariant, will let Views knows we have nothing
    // to do and we let the default behavior of the view do work for us.
    return QVariant();
}

void ThumbnailModel::LoadUp(int idx) const
{
    if (images.at(idx)->loaded) return;
    images[idx]->picture = QPixmap(images[idx]->filename);
    images[idx]->thumb = images[idx]->picture.scaled(100, 100, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    images[idx]->loaded = true;
}
