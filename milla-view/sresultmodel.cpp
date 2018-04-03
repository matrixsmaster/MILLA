#include "sresultmodel.h"
#include "dbhelper.h"

SResultModel::SResultModel(QList<MImageListRecord> items, QObject *parent)
    : MImageListModel(parent)
{
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
        MImageListRecord &j = *curitem;

        if (j.loaded || !j.thumb.isNull() || j.filename.isEmpty()) continue;

        if (j.picture.isNull()) {
            qDebug() << "[SRModel] Loading pixmap for " << j.filename;
            j.picture = QPixmap(j.filename);
        } else
            j.loaded = true;

        if (!DBHelper::getThumbnail(j)) {

            if (!j.picture.isNull()) {
                j.thumb = j.picture.scaled(THUMBNAILSIZE,THUMBNAILSIZE,Qt::KeepAspectRatio,Qt::SmoothTransformation);
                j.modified = true;
                qDebug() << "[SRModel] Created thumbnail for " << j.filename;
                SaveThumbnail(j);

            } else {
                j.thumb = QPixmap(THUMBNAILSIZE,THUMBNAILSIZE);
                j.thumb.fill(Qt::black);
            }
        }

        if (j.fnshort.isEmpty()) {
            QFileInfo fi(j.filename);
            j.fnshort = fi.fileName();
        }

        j.valid = true;
    }

    beginInsertRows(QModelIndex(),0,images.size());
    endInsertRows();

    if (curitem != images.end()) timer.start(RESULTUPDATETIMEOUT);
}
