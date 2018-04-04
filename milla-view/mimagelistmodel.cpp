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
    return ItemSizeInBytes(images.at(idx));
}

size_t MImageListModel::ItemSizeInBytes(MImageListRecord const &r)
{
    return (r.picture.depth() / 8) * r.picture.size().width() * r.picture.size().height();
}

QModelIndex MImageListModel::getRecordIndex(const QString &fn, bool allowPartialMatch)
{
    QString canon;
    bool path = fn.contains('/');
    if (path) canon = DBHelper::getCanonicalPath(fn);

    size_t idx = 0;
    bool ok = false;
    for (auto &i : images) {
        if (allowPartialMatch) {
            if ((path && (i.filename.contains(fn,Qt::CaseInsensitive) || i.filename.contains(canon,Qt::CaseInsensitive)))
                    || (!path && i.fnshort.contains(fn,Qt::CaseInsensitive))) ok = true;
        } else {
            if ((path && (!fn.compare(i.filename,Qt::CaseInsensitive) || !canon.compare(i.filename,Qt::CaseInsensitive)))
                    || (!path && !fn.compare(i.fnshort,Qt::CaseInsensitive))) ok = true;
        }
        if (ok) break;
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
