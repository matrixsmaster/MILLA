#include "thumbnailmodel.h"

ThumbnailModel::ThumbnailModel(std::list<QString> files, QObject *parent)
    : QAbstractListModel(parent)
{
    ram_footprint = 0;
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery q;
    bool ok = q.exec("SELECT * FROM thumbs");
    qDebug() << "[db] Read thumbs table: " << ok;
    if (!ok) {
        QSqlQuery qq;
        ok = qq.exec("CREATE TABLE thumbs (file TEXT, mtime UNSIGNED INT, thumb BLOB)");
        qDebug() << "[db] Create new thumbs table: " << ok;
    }

    for (auto &i : files) {
        ThumbnailRec *pair = new ThumbnailRec();
        pair->filename = i;
        pair->loaded = false;
        pair->modified = true;
        pair->filechanged = 0;

        if (ok) {
            QSqlQuery qq;
            qq.prepare("SELECT thumb, mtime FROM thumbs WHERE file = (:fn)");
            qq.bindValue(":fn",i);

            if (qq.exec() && qq.next() && qq.value(0).canConvert(QVariant::ByteArray) && qq.value(1).canConvert(QVariant::UInt)) {
                qDebug() << "Loaded thumbnail for " << i;
                pair->thumb.loadFromData(qq.value(0).toByteArray());
                pair->filechanged = qq.value(1).toUInt();
                pair->modified = false;
            }
        }

        if (pair->modified) {
            pair->thumb = QPixmap(THUMBNAILSIZE,THUMBNAILSIZE);
            pair->thumb.fill(Qt::black);
        }

        QFileInfo fi(i);
        pair->fnshort = fi.fileName();

        images.append(pair);
    }
}

ThumbnailModel::~ThumbnailModel()
{
    qDebug() << "Deleting ThumbnailModel";
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
            //LoadUp(index.row());
            images[index.row()]->touched = time(NULL); //we're inside a const method, wtf?!
            return images.value(index.row())->picture;

        case FullPathRole:
            return images.value(index.row())->filename;

        }
    }

    // returning a default constructed QVariant, will let Views knows we have nothing
    // to do and we let the default behavior of the view do work for us.
    return QVariant();
}

void ThumbnailModel::LoadUp(int idx) //const
{
    if (images.at(idx)->loaded) return;
    images[idx]->picture = QPixmap(images[idx]->filename);
    if (images.at(idx)->picture.isNull()) return;
    images[idx]->loaded = true;

    //qDebug() << "depth = " << images.at(idx)->picture.depth();
    ram_footprint += ItemSizeInBytes(idx);
    GC(idx);

    QFileInfo fi(images.at(idx)->filename);
    if (fi.lastModified().toTime_t() == images.at(idx)->filechanged) return;

    images[idx]->thumb = images[idx]->picture.scaled(THUMBNAILSIZE,THUMBNAILSIZE,Qt::KeepAspectRatio,Qt::SmoothTransformation);
    images[idx]->modified = true;
    images[idx]->touched = time(NULL);
    images[idx]->filechanged = fi.lastModified().toTime_t();

    QByteArray arr;
    QBuffer dat(&arr);
    dat.open(QBuffer::WriteOnly);
    bool ok = images.at(idx)->thumb.save(&dat,"png");

    QSqlQuery qq;
    qq.prepare("SELECT thumb FROM thumbs WHERE file = (:fn)");
    qq.bindValue(":fn",images.at(idx)->filename);
    bool mod = qq.exec() && qq.next();

    if (ok) {
        QSqlQuery q;
        QString act;
        ok = false;

        if (!mod) {
            q.prepare("INSERT INTO thumbs (file, mtime, thumb) VALUES (:fn, :mtm, :thm)");
            act = "Inserting";
        } else {
            q.prepare("UPDATE thumbs SET thumb = :thm, mtime = :mtm WHERE file = :fn");
            act = "Updating";
        }
        q.bindValue(":fn",images.at(idx)->filename);
        q.bindValue(":mtm",fi.lastModified().toTime_t());
        q.bindValue(":thm",arr);
        ok = q.exec();

        qDebug() << "[db] " << act << " record for " << images.at(idx)->fnshort << ": " << ok;
        if (ok) images[idx]->modified = false;
        else qDebug() << q.lastError();
    }
}

size_t ThumbnailModel::ItemSizeInBytes(int idx)
{
    return (images.at(idx)->picture.depth() / 8) * images.at(idx)->picture.size().width() * images.at(idx)->picture.size().height();
}

void ThumbnailModel::GC(int skip)
{
    qDebug() << "[GC] Current RAM footprint: " << (ram_footprint/1024/1024) << " MiB";
    if (ram_footprint < MAXPICSBYTES) return;

    std::map<time_t,int> allocated;
    for (int i = 0; i < images.size(); i++) {
        if ((i == skip) || !images.at(i)->loaded) continue;
        allocated[images.at(i)->touched] = i;
    }
    qDebug() << "[GC] Time map populated: " << allocated.size() << " entries";
    if (allocated.empty()) return;

    for (auto it = allocated.begin(); it != allocated.end(); ++it) {
        size_t csz = ItemSizeInBytes(it->second);
        qDebug() << "[GC] Deleting " << images.at(it->second)->fnshort << " Timestamp " << it->first << "; size = " << csz;
        images[it->second]->picture = QPixmap();
        images[it->second]->loaded = false;
        ram_footprint -= csz;

        if (ram_footprint < MAXPICSBYTES) {
            qDebug() << "[GC] Goal reached, ram footprint now is " << ram_footprint;
            break;
        }
    }
}
