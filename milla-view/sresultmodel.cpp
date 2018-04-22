#include "sresultmodel.h"
#include "dbhelper.h"

SResultModel::SResultModel(QList<MImageListRecord> items, MImageLoader *imgLoader, QObject *parent)
    : MImageListModel(imgLoader,parent)
{
    do_shorten = true;
    images = items;
    curitem = images.begin();
    connect(&timer,&QTimer::timeout,this,[this] { Loader(); });
    Loader();
}

SResultModel::~SResultModel()
{
    qDebug() << "Deleting SResultModel";
}

void SResultModel::Loader()
{
    timer.stop();
    beginInsertRows(QModelIndex(),images.size(),images.size());

    for (int i = 0; i < MAXRESULTSBULK && curitem != images.end(); ++curitem,i++) {
        *curitem = loader->loadFull(curitem->filename);
        ram_footprint += ItemSizeInBytes(*curitem);
        if (ram_footprint > MILLA_MAXPICSBYTES) {
            curitem = images.end();
            break;
        }
    }

    endInsertRows();
    if (curitem != images.end()) timer.start(RESULTUPDATETIMEOUT);
}
