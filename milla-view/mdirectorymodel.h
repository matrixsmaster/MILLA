#ifndef MDIRECTORYMODEL_H
#define MDIRECTORYMODEL_H

#include <QObject>
#include "mimagelistmodel.h"

class MDirectoryModel : public MImageListModel
{
    Q_OBJECT
public:
    explicit MDirectoryModel(MImageLoader* imgLoader, QObject *parent = nullptr);

    void update();

    void addDir(QString const &path);

    void delDir(QString const &path);

    void delAll();

    void updateDir(QString const &path, QString const &last);
};

#endif // MDIRECTORYMODEL_H
