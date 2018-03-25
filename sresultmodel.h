#ifndef SRESULTMODEL_H
#define SRESULTMODEL_H

#include "thumbnailmodel.h"

struct SResultRecord {
    QString path,shrt;
    QPixmap thumb,large;
};

class SResultModel : public QAbstractListModel
{
    Q_OBJECT
public:

    explicit SResultModel(QList<SResultRecord> items, QObject *parent = 0);
    virtual ~SResultModel();

    int columnCount(const QModelIndex &) const { return 1; }
    int rowCount(const QModelIndex &) const { return images.size(); }
    QVariant data(const QModelIndex &index, int role) const;

private:
    QList<SResultRecord> images;
};

#endif // SRESULTMODEL_H
