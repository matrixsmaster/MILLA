#include "mimagelistmodel.h"
#include "dbhelper.h"

MImageListModel::MImageListModel(MImageLoader *imgLoader, QObject *parent)
    : QAbstractListModel(parent),
      loader(imgLoader)
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
                return images.value(index.row()).fnshort.left(MILLA_MAXSHORTLENGTH);
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

QStringList MImageListModel::getAllFileNames()
{
    QStringList res;
    for (auto &i : images) {
        if (!i.filename.isEmpty() && i.valid) res.push_back(i.filename);
    }
    return res;
}

size_t MImageListModel::ItemSizeInBytes(int idx)
{
    return ItemSizeInBytes(images.at(idx));
}

size_t MImageListModel::ItemSizeInBytes(MImageListRecord const &r)
{
    return (r.picture.depth() / 8) * r.picture.size().width() * r.picture.size().height();
}

QModelIndex MImageListModel::getRecordIndex(const QString &fn, bool allowPartialMatch, size_t *pStartIdx)
{
    QString canon;
    bool path = fn.contains('/');
    if (path) canon = DBHelper::getCanonicalPath(fn);

    size_t idx = 0;
    bool ok = false;
    for (auto &i : images) {
        if (pStartIdx && idx < *pStartIdx) {
            idx++;
            continue;
        }
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

    if (pStartIdx) *pStartIdx = ok? idx+1:idx;
    return ok? createIndex(idx,0) : QModelIndex();
}

bool MImageListModel::deleteRecordByFullName(const QString &fn)
{
    for (auto i = images.begin(); i != images.end(); ++i) {
        if (i->filename == fn) {
            beginInsertRows(QModelIndex(),images.size(),images.size());
            images.erase(i);
            endInsertRows();
            return true;
        }
    }
    return false;
}
