#include "sresultmodel.h"
#include "dbhelper.h"

SResultModel::SResultModel(QList<MImageListRecord> items, QObject *parent)
    : MImageListModel(parent)
{
    images = items;

    for (auto &i : images) {
        if (i.loaded || !i.thumb.isNull() || i.filename.isEmpty()) continue;

        if (i.picture.isNull()) {
            qDebug() << "[SRModel] Loading pixmap for " << i.filename;
            i.picture = QPixmap(i.filename);
        }

        if (!DBHelper::getThumbnail(i)) {

            if (!i.picture.isNull()) {
                i.thumb = i.picture.scaled(THUMBNAILSIZE,THUMBNAILSIZE,Qt::KeepAspectRatio,Qt::SmoothTransformation);
                i.modified = true;
                qDebug() << "[SRModel] Created thumbnail for " << i.filename;
                SaveThumbnail(i);

            } else {
                i.thumb = QPixmap(THUMBNAILSIZE,THUMBNAILSIZE);
                i.thumb.fill(Qt::black);
            }
        }

        if (i.fnshort.isEmpty()) {
            QFileInfo fi(i.filename);
            i.fnshort = fi.fileName();
        }

        i.valid = true;
    }
}

SResultModel::~SResultModel()
{
    qDebug() << "Deleting SResultModel";
}
