#include "sresultmodel.h"

SResultModel::SResultModel(QList<MImageListRecord> items, QObject *parent)
    : MImageListModel(parent)
{
    images = items;
}

SResultModel::~SResultModel()
{
    qDebug() << "Deleting SResultModel";
}
