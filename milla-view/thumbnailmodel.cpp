#include "thumbnailmodel.h"
#include "dbhelper.h"

ThumbnailModel::ThumbnailModel(QStringList files, QObject *parent)
    : MImageListModel(parent)
{
    for (auto &i : files) {
        MImageListRecord rec;
        rec.filename = i;
        rec.valid = true;
        DBHelper::getThumbnail(rec);

        if (rec.modified) {
            rec.thumb = QPixmap(THUMBNAILSIZE,THUMBNAILSIZE);
            rec.thumb.fill(Qt::black);
        }

        QFileInfo fi(i);
        rec.fnshort = fi.fileName();

        images.append(rec);
    }
}

ThumbnailModel::~ThumbnailModel()
{
    qDebug() << "Deleting ThumbnailModel";
}

void ThumbnailModel::LoadUp(int idx, bool force_reload)
{
    if (!force_reload && images.at(idx).loaded) return;

    images[idx].picture = QPixmap(images[idx].filename);
    if (images.at(idx).picture.isNull()) return;
    images[idx].loaded = true;

    ram_footprint += ItemSizeInBytes(idx);
    GC(idx);

    QFileInfo fi(images.at(idx).filename);
    if (fi.lastModified().toTime_t() == images.at(idx).filechanged) return;

    images[idx].thumb = images[idx].picture.scaled(THUMBNAILSIZE,THUMBNAILSIZE,Qt::KeepAspectRatio,Qt::SmoothTransformation);
    images[idx].modified = true;
    images[idx].touched = time(NULL);
    images[idx].filechanged = fi.lastModified().toTime_t();

    SaveThumbnail(images[idx]);
}

void ThumbnailModel::GC(int skip)
{
    qDebug() << "[GC] Current RAM footprint: " << (ram_footprint/1024/1024) << " MiB";
    if (ram_footprint < MAXPICSBYTES) return;

    std::map<time_t,int> allocated;
    for (int i = 0; i < images.size(); i++) {
        if ((i == skip) || !images.at(i).loaded) continue;
        allocated[images.at(i).touched] = i;
    }
    qDebug() << "[GC] Time map populated: " << allocated.size() << " entries";
    if (allocated.empty()) return;

    for (auto it = allocated.begin(); it != allocated.end(); ++it) {
        size_t csz = ItemSizeInBytes(it->second);
        qDebug() << "[GC] Deleting " << images.at(it->second).fnshort << " Timestamp " << it->first << "; size = " << csz;
        images[it->second].picture = QPixmap();
        images[it->second].loaded = false;
        ram_footprint -= csz;

        if (ram_footprint < MAXPICSBYTES) {
            qDebug() << "[GC] Goal reached, ram footprint now is " << ram_footprint;
            break;
        }
    }
}

void ThumbnailModel::touch(const QModelIndex &index)
{
    images[index.row()].touched = time(NULL);
}

void ThumbnailModel::clearCache()
{
    for (auto &i : images)
        if (i.loaded) {
            i.loaded = false;
            i.picture = QPixmap();
        }
    ram_footprint = 0;
}

void ThumbnailModel::sortBy(ThumbnailModelSort by)
{
    if (by == NoSort) return;

    std::sort(images.begin(),images.end(),[by] (const auto &a, const auto &b) {
        switch (by) {
        case SortByNameAsc:
            return (a.fnshort.compare(b.fnshort,Qt::CaseInsensitive) < 0)? true : false;

        case SortByNameDesc:
            return (a.fnshort.compare(b.fnshort,Qt::CaseInsensitive) < 0)? false : true;

        case SortByTimeAsc:
            return a.filechanged < b.filechanged;

        case SortByTimeDesc:
            return a.filechanged > b.filechanged;

        default: return false; //basically to make compiler happy
        }
    });
}
