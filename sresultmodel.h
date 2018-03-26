#ifndef SRESULTMODEL_H
#define SRESULTMODEL_H

#include <mimagelistmodel.h>

class SResultModel : public MImageListModel
{
    Q_OBJECT

public:
    explicit SResultModel(QList<MImageListRecord> items, QObject *parent = 0);
    virtual ~SResultModel();
};

#endif // SRESULTMODEL_H
