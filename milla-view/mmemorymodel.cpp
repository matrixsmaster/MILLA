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
    beginInsertRows(QModelIndex(),images.size(),images.size());
    images[n] = rec;
    images[n].fnshort = QString::asprintf("Slot %d",n+1);
    endInsertRows();
    DBHelper::updateMemorySlot(n,rec.filename);
}

void MMemoryModel::eraseSlot(int n)
{
    if (n < 0 || n >= MAXMEMORYSLOTS) return;
    if (DBHelper::eraseMemorySlot(n)) clear();
}

void MMemoryModel::clear(bool full)
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

    if (full) DBHelper::eraseMemory();
}

void MMemoryModel::assignFirst(const QStringList &lst)
{
    for (int i = 0; i < MAXMEMORYSLOTS && i < lst.size(); i++)
        DBHelper::updateMemorySlot(i,lst.at(i));
    clear();
}
