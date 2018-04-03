#include "mimagelistmodel.h"
#include "dbhelper.h"

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
    if (index.isValid() && images.value(index.row()).valid) {
        switch (role) {
        case Qt::DecorationRole:
            return images.value(index.row()).thumb;

        case Qt::DisplayRole:
            if (do_shorten)
                return images.value(index.row()).fnshort.left(MAXSHORTLENGTH);
            else
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

QModelIndex MImageListModel::getRecordIndex(const QString &fn)
{
    bool path = fn.contains('/');

    size_t idx = 0;
    bool ok = false;
    for (auto &i : images) {
        if ((path && !fn.compare(i.filename,Qt::CaseInsensitive)) || (!path && !fn.compare(i.fnshort,Qt::CaseInsensitive))) {
            ok = true;
            break;
        }
        idx++;
    }

    return ok? createIndex(idx,0) : QModelIndex();
}

void MImageListModel::SaveThumbnail(MImageListRecord &rec) const
{
    QByteArray arr;
    QBuffer dat(&arr);
    dat.open(QBuffer::WriteOnly);
    if (rec.thumb.save(&dat,"png")) DBHelper::updateThumbnail(rec,arr);
}
