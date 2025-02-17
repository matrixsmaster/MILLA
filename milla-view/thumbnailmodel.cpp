#include "thumbnailmodel.h"
#include "dbhelper.h"

ThumbnailModel::ThumbnailModel(QStringList files, ProgressCB loading_cb, MImageLoader *imgLoader, QObject *parent)
    : MImageListModel(imgLoader,parent)
{
    double prg = 0, dp = 100.f / (double)(files.size());

    for (auto &i : files) {
        if (i.isEmpty()) continue;

        images.append(loader->loadFull(i,true));

        prg += dp;
        if (loading_cb) {
            if (!loading_cb(prg)) {
                qDebug() << "[THUMBMODEL] Aborted by callback";
                break;
            }
        }
    }
}

ThumbnailModel::~ThumbnailModel()
{
    qDebug() << "Deleting ThumbnailModel";
}

void ThumbnailModel::LoadUp(int idx, bool force_reload)
{
    if (idx < 0 || idx >= images.size()) return;
    if (!force_reload && images.at(idx).loaded) return;

    images[idx] = loader->loadFull(images.at(idx).filename);
    if (!images.at(idx).loaded) return;

    ram_footprint += ItemSizeInBytes(idx);
    GC(idx);
}

void ThumbnailModel::GC(int skip)
{
    qDebug() << "[GC] Current RAM footprint: " << (ram_footprint/1024/1024) << " MiB";
    if (ram_footprint < MILLA_MAXPICSBYTES) return;

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

        if (ram_footprint < MILLA_MAXPICSBYTES) {
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

    beginInsertRows(QModelIndex(),images.size(),images.size());

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

    endInsertRows();
}
