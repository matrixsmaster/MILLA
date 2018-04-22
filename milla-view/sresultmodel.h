#ifndef SRESULTMODEL_H
#define SRESULTMODEL_H

#include <QTimer>
#include <QListView>
#include <mimagelistmodel.h>

#define MAXRESULTSBULK 20
#define RESULTUPDATETIMEOUT 2000

class SResultModel : public MImageListModel
{
    Q_OBJECT

public:
    explicit SResultModel(QList<MImageListRecord> items, MImageLoader* imgLoader, QObject *parent = 0);
    virtual ~SResultModel();

private:
    QTimer timer;
    QList<MImageListRecord>::iterator curitem;

    void Loader();
};

#endif // SRESULTMODEL_H
