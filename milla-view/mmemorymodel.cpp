#include "mmemorymodel.h"

MMemoryModel::MMemoryModel(QObject *parent) :
    MImageListModel(parent)
{
    do_shorten = true;

    for (int i = 0; i < MAXMEMORYSLOTS; i++) {
        MImageListRecord rec;
        rec.valid = true;
        rec.thumb = QPixmap(THUMBNAILSIZE,THUMBNAILSIZE);
        rec.thumb.fill(Qt::black);
        rec.fnshort = QString::asprintf("Slot %d",i+1);
        images.push_back(rec);
    }
}

void MMemoryModel::setSlot(int n, MImageListRecord const &rec)
{
    if (n < 0 || n >= MAXMEMORYSLOTS) return;
    images[n] = rec;
    beginInsertRows(QModelIndex(),images.size(),images.size());
    endInsertRows();
}
