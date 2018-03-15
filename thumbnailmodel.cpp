#include "thumbnailmodel.h"

ThumbnailModel::ThumbnailModel(std::list<QString> files, QObject *parent)
    : QAbstractListModel(parent)
{
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery q;
    bool ok = q.exec("SELECT * FROM thumbs");
    qDebug() << "INITIAL OPEN: " << ok;
    if (!ok) {
        QSqlQuery qq;
        ok = qq.exec("CREATE TABLE thumbs (file TEXT, thumb BLOB)");
        qDebug() << "CREATE: " << ok;
    }

    for (auto &i : files) {
        PixmapPair *pair = new PixmapPair();
        pair->filename = i;
        pair->loaded = false;
        pair->modified = true;

        if (ok) {
            QSqlQuery qq;
            qq.prepare("SELECT thumb FROM thumbs WHERE file = (:fn)");
            qq.bindValue(":fn",i);

            if (qq.exec() && qq.next() && qq.value(0).canConvert(QVariant::ByteArray)) {
                qDebug() << "Loaded thumbnail for " << i;
                pair->thumb.loadFromData(qq.value(0).toByteArray());
                pair->modified = false;
            }
        }
        if (pair->modified) pair->thumb = QPixmap(THUMBNAILSIZE,THUMBNAILSIZE);

        QFileInfo fi(i);
        pair->fnshort = fi.fileName();

        images.append(pair);
    }
}

ThumbnailModel::~ThumbnailModel()
{
    qDebug() << "Deleting ThumbnailModel";
    QSqlDatabase db = QSqlDatabase::database();
    for (auto &i : images) {
        if (!i->loaded) continue;
        if (!i->modified) continue;

        QByteArray arr;
        QBuffer dat(&arr);
        dat.open(QBuffer::WriteOnly);
        bool ok = i->thumb.save(&dat,"png");
        qDebug() << "Compressing " << i->fnshort << ok;

        QSqlQuery q;
        q.prepare("INSERT INTO thumbs (file, thumb) VALUES (:fn, :thm)");
        q.bindValue(":fn",i->filename);
        q.bindValue(":thm",arr);
        qDebug() << i->fnshort << " --> " << q.exec();
    }
    qDeleteAll(images);
}

int ThumbnailModel::rowCount(const QModelIndex &parent) const
{
    // This function should return the number of rows contained in the parent
    // parameter, the parent parameter is used for trees in order to retrieve the
    // number of rows contained in each node. Since we are doing a list each element
    // doesn't have child nodes so we return 0
    // By convention an invalid parent means the topmost level of a tree. In our case
    // we return the number of elements contained in our images store.
    return parent.isValid()? 0 : images.count();
}

QVariant ThumbnailModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid()) {
        switch (role) {
        case Qt::DecorationRole:
            // DecorationRole = Icon show for a list
            return images.value(index.row())->thumb;

        case Qt::DisplayRole:
            // DisplayRole = Displayed text
            return images.value(index.row())->fnshort;

        case LargePixmapRole:
            // This is a custom role, it will help us getting the pixmap more
            // easily later.
            LoadUp(index.row());
            return images.value(index.row())->picture;

        case FullPathRole:
            return images.value(index.row())->filename;

        }
    }

    // returning a default constructed QVariant, will let Views knows we have nothing
    // to do and we let the default behavior of the view do work for us.
    return QVariant();
}

void ThumbnailModel::LoadUp(int idx) const
{
    if (images.at(idx)->loaded) return;
    images[idx]->picture = QPixmap(images[idx]->filename);
    images[idx]->thumb = images[idx]->picture.scaled(THUMBNAILSIZE, THUMBNAILSIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    images[idx]->loaded = true;
    images[idx]->modified = true;
}
