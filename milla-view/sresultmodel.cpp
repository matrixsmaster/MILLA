#include "sresultmodel.h"
#include "dbhelper.h"

SResultModel::SResultModel(QList<MImageListRecord> items, QObject *parent)
    : MImageListModel(parent)
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

    for (int i = 0; i < MAXRESULTSBULK && curitem != images.end(); ++curitem,i++) {
        loadSingleFile(*curitem);
        if (ram_footprint > MAXPICSBYTES) {
            curitem = images.end();
            break;
        }
    }

    beginInsertRows(QModelIndex(),images.size(),images.size());
    endInsertRows();

    if (curitem != images.end()) timer.start(RESULTUPDATETIMEOUT);
}
