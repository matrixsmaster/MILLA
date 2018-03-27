#include "mimagelistmodel.h"

MImageListModel::MImageListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

MImageListModel::~MImageListModel()
{
    images.clear();
    qDebug() << "Deleted MImageListModel";
}

QVariant MImageListModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid()) {
        switch (role) {
        case Qt::DecorationRole:
            return images.value(index.row()).thumb;

        case Qt::DisplayRole:
            return images.value(index.row()).fnshort;

        case LargePixmapRole:
            return images.value(index.row()).picture;

        case FullPathRole:
            return images.value(index.row()).filename;

        case FullDataRole:
        {
            QVariant _rec;
            _rec.setValue(images.at(index.row()));
            return _rec;
        }

        }
    }
    return QVariant();
}

size_t MImageListModel::ItemSizeInBytes(int idx)
{
    return (images.at(idx).picture.depth() / 8) * images.at(idx).picture.size().width() * images.at(idx).picture.size().height();
}
