#include "mdirectorymodel.h"
#include "dbhelper.h"

MDirectoryModel::MDirectoryModel(MImageLoader *imgLoader, QObject *parent) :
    MImageListModel(imgLoader,parent)
{
    update();
}

void MDirectoryModel::update()
{
    QStringList lst = DBHelper::getDirsList();

    beginInsertRows(QModelIndex(),images.size(),images.size());
    images.clear();
    for (auto &i : lst) {
        MImageListRecord rec = loader->loadFull(DBHelper::getDirLastFile(i));
        rec.fnshort = i;
        images.push_back(rec);
    }
    endInsertRows();
}

void MDirectoryModel::addDir(const QString &path)
{
    if (path.isEmpty()) return;
    for (auto &i : images) {
        if (i.fnshort == path) return; // dir exists
    }
    DBHelper::updateDirPath(path,"");
    update();
}

void MDirectoryModel::delDir(const QString &path)
{
    if (path.isEmpty()) return;
    if (DBHelper::delDirPath(path)) update();
}

void MDirectoryModel::delAll()
{
    DBHelper::eraseDirs();
    update();
}

void MDirectoryModel::updateDir(const QString &path, const QString &last)
{
    if (path.isEmpty() || last.isEmpty()) return;
    if (DBHelper::updateDirPath(path,last)) update();
}
