#include "mmemorymodel.h"
#include "dbhelper.h"

MMemoryModel::MMemoryModel(MImageLoader *imgLoader, QObject *parent) :
    MImageListModel(imgLoader,parent)
{
    do_shorten = true;
    clear();
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

void MMemoryModel::clear()
{
    beginInsertRows(QModelIndex(),images.size(),images.size());
    images.clear();
    for (int i = 0; i < MAXMEMORYSLOTS; i++) {
        MImageListRecord rec = loader->loadFull(DBHelper::getMemorySlot(i));
        rec.valid = true; //make it valid anyway
        rec.fnshort = QString::asprintf("Slot %d",i+1);
        images.push_back(rec);
    }
    endInsertRows();
    DBHelper::eraseMemory();
}
