#include "sresultmodel.h"

SResultModel::SResultModel(QList<SResultRecord> items, QObject *parent)
    : QAbstractListModel(parent),
    images(items)
{
}

SResultModel::~SResultModel()
{
    qDebug() << "Deleting SResultModel";
}

QVariant SResultModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid()) {
        switch (role) {
        case Qt::DecorationRole: return images.value(index.row()).thumb;
        case Qt::DisplayRole: return images.value(index.row()).shrt;
        case ThumbnailModel::FullPathRole: return images.value(index.row()).path;
        case ThumbnailModel::LargePixmapRole: return images.value(index.row()).large;
        }
    }
    return QVariant();
}
