#ifndef THUMBNAILMODEL_H
#define THUMBNAILMODEL_H

#include <mimagelistmodel.h>

class ThumbnailModel : public MImageListModel
{
    Q_OBJECT

public:
    explicit ThumbnailModel(QList<QString> files, QObject *parent = 0);
    virtual ~ThumbnailModel();

    void LoadUp(int idx);

    void GC(int skip = -1);

    void touch(const QModelIndex &index);

protected:
    size_t ram_footprint = 0;
};

#endif // THUMBNAILMODEL_H
