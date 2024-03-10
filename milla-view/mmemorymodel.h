#ifndef MMEMORYMODEL_H
#define MMEMORYMODEL_H

#include "mimagelistmodel.h"

#define MAXMEMORYSLOTS 10

class MMemoryModel : public MImageListModel
{
    Q_OBJECT

public:
    explicit MMemoryModel(MImageLoader* imgLoader, QObject *parent = 0);
    virtual ~MMemoryModel() {}

    void setSlot(int n, MImageListRecord const &rec);

    void eraseSlot(int n);

    void clear(bool full = false);

    void assignFirst(QStringList const &lst);
};

#endif // MMEMORYMODEL_H
