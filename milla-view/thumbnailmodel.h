#ifndef THUMBNAILMODEL_H
#define THUMBNAILMODEL_H

#include <mimagelistmodel.h>

class ThumbnailModel : public MImageListModel
{
    Q_OBJECT

public:
    enum ThumbnailModelSort {
        NoSort,
        SortByNameAsc,
        SortByNameDesc,
        SortByTimeAsc,
        SortByTimeDesc
    };

    explicit ThumbnailModel(QStringList files, ProgressCB loading_cb, MImageLoader* imgLoader, QObject *parent = 0);
    virtual ~ThumbnailModel();

    void LoadUp(int idx, bool force_reload = false);

    void GC(int skip = -1);

    void touch(const QModelIndex &index);

    void clearCache();

    void sortBy(ThumbnailModelSort by);
};

#endif // THUMBNAILMODEL_H
