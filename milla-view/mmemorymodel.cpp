#include "mmemorymodel.h"
#include "dbhelper.h"

MMemoryModel::MMemoryModel(QObject *parent) :
    MImageListModel(parent)
{
    do_shorten = true;

    for (int i = 0; i < MAXMEMORYSLOTS; i++) {
        MImageListRecord rec;
        rec.filename = DBHelper::getMemorySlot(i);
        loadSingleFile(rec);

        if (!rec.valid) {
            rec.thumb = QPixmap(THUMBNAILSIZE,THUMBNAILSIZE);
            rec.thumb.fill(Qt::black);
            rec.valid = true;
        }

        rec.fnshort = QString::asprintf("Slot %d",i+1);
        images.push_back(rec);
    }
}

void MMemoryModel::setSlot(int n, MImageListRecord const &rec)
{
    if (n < 0 || n >= MAXMEMORYSLOTS) return;
    images[n] = rec;
    images[n].fnshort = QString::asprintf("Slot %d",n+1);
    beginInsertRows(QModelIndex(),images.size(),images.size());
    endInsertRows();
    DBHelper::updateMemorySlot(n,rec.filename);
}
