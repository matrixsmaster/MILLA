#include "mimageloader.h"
#include "dbhelper.h"

MImageLoader::MImageLoader(MillaPluginLoader *pLoader, QObject *parent) :
    QObject(parent),
    plugins(pLoader)
{
    plugins->updateSupportedFileFormats(supported);
}

bool MImageLoader::isLoadableFile(QString const &path, QString *canonicalPath)
{
    QFileInfo cfn(path);
    QString ext(cfn.suffix().toLower());
    if (supported.contains(ext)) {
        if (canonicalPath) *canonicalPath = cfn.canonicalFilePath();
        return true;
    }
    return false;
}

void MImageLoader::scanDirectory(QString const &dir, QStringList &addto, bool recursive)
{
    QDirIterator::IteratorFlags flags = QDirIterator::FollowSymlinks;
    if (recursive) flags |= QDirIterator::Subdirectories;
    QDirIterator it(dir, QDir::Files | QDir::NoDotAndDotDot, flags);
    QString cpath;
    while (it.hasNext()) {
        if (isLoadableFile(it.next(),&cpath)) addto.push_back(cpath);
    }
}

QStringList MImageLoader::openDirByFile(QString const &fileName, bool recursive)
{
    QStringList lst;
    if (fileName.isEmpty()) return lst;

    QFileInfo bpf(fileName);
    QString bpath;
    if (bpf.isDir()) {
        QDir dr(fileName);
        bpath = dr.canonicalPath();
    } else
        bpath = bpf.canonicalPath();
    if (bpath.isEmpty()) return lst;

    scanDirectory(bpath,lst,recursive);
    DBHelper::addRecentDir(fileName,true);

    return lst;
}

QStringList MImageLoader::openDirByList(QString const &fileName)
{
    QStringList lst;
    if (fileName.isEmpty()) return lst;

    QFile ilist(fileName);
    if (!ilist.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "ALERT: unable to read file " << fileName;
        return lst;
    }
    QString dat = ilist.readAll();
    ilist.close();

    QStringList ldat = dat.split('\n',QString::SkipEmptyParts);
    dat.clear();
    if (ldat.empty()) return lst;

    QString cpath;
    for (auto &i : ldat) {
        if (i.isEmpty()) continue;
        if (i.at(i.size()-1) == '/') scanDirectory(i,lst,false);
        else if (i.right(2) == "/*") scanDirectory(i.left(i.size()-1),lst,true);
        else if (isLoadableFile(i,&cpath)) lst.push_back(cpath);
    }

    DBHelper::addRecentDir(fileName,false);

    return lst;
}

QStringList MImageLoader::_open(QString const &filename, bool strict)
{
    if (isLoadableFile(filename,nullptr)) {
        if (strict) {
            accum.push_back(filename);

        } else {
            if (recursive < 0) {
                recursive = (QMessageBox::question(nullptr, tr("Type of scan"), tr("Do recursive scan?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes);
            }
            accum += openDirByFile(filename,(recursive > 0));
        }

    } else {
        QFileInfo cfn(filename);
        if (cfn.isDir()) {
            accum += openDirByFile(filename,false);

        } else {
            QString ext(cfn.suffix().toLower());
            QStringList _l = MILLA_LIST_FORMATS;
            if (_l.contains(ext)) accum += openDirByList(filename);
        }
    }

    return accum;
}

QStringList MImageLoader::open(QString const &filename)
{
    clearList();
    QStringList _l = _open(filename,false);
    clearList();
    return _l;
}

QStringList MImageLoader::append(QString const &filename, bool strict)
{
    return _open(filename,strict);
}

QPixmap MImageLoader::load(QString const &filename)
{
    //now it's that simple, but sooner or later that would become something completely different
    return QPixmap(filename);
}

void MImageLoader::thumb(MImageListRecord &rec, bool force, bool fast)
{
    if (rec.filename.isEmpty()) return;

    DBHelper::getThumbnail(rec);
    QFileInfo fi(rec.filename);
    if (!force && fi.lastModified().toTime_t() == rec.filechanged && rec.thumbOK) return;

    rec.thumbOK = false;
    rec.loaded = !rec.picture.isNull();

    if (!fast) {
        if (force || !rec.loaded) {
            rec.picture = load(rec.filename);
        }
        if (rec.picture.isNull()) {
            qDebug() << "[ImgLoader] Unable to load pixmap for " << rec.filename;
            return;
        }
        rec.loaded = true;
    }

    if (rec.loaded) {
        rec.thumb = rec.picture.scaled(MILLA_THUMBNAIL_SIZE,MILLA_THUMBNAIL_SIZE,Qt::KeepAspectRatio,Qt::SmoothTransformation);
        rec.thumbOK = !rec.thumb.isNull();
    }

    if (!rec.thumbOK) {
        rec.thumb = QPixmap(MILLA_THUMBNAIL_SIZE,MILLA_THUMBNAIL_SIZE);
        rec.thumb.fill(Qt::black);

    } else {
        QByteArray arr;
        QBuffer dat(&arr);
        dat.open(QBuffer::WriteOnly);
        if (rec.thumb.save(&dat,"png")) DBHelper::updateThumbnail(rec,arr);
    }
}

MImageListRecord MImageLoader::loadFull(QString const &filename, bool fast)
{
    MImageListRecord r;
    if (filename.isEmpty()) return r;

    r.filename = filename;
    if (!fast) r.picture = load(filename);
    r.loaded = !r.picture.isNull();
    r.touched = DBHelper::getLastViewTime(filename);
    thumb(r,false,fast);

    QFileInfo fi(filename);
    r.fnshort = fi.fileName();
    r.filechanged = fi.lastModified().toTime_t();

    r.valid = true;

    return r;
}
