#ifndef MMEMORYMODEL_H
#define MMEMORYMODEL_H

#include "mimagelistmodel.h"

#define MAXMEMORYSLOTS 10

class MMemoryModel : public MImageListModel
{
    Q_OBJECT

public:
    MMemoryModel(QObject *parent = 0);
    virtual ~MMemoryModel() {}

    void setSlot(int n, MImageListRecord const &rec);
};

#endif // MMEMORYMODEL_H
