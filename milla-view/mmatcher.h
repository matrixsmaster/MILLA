#ifndef MMATCHER_H
#define MMATCHER_H

#include <map>
#include <functional>
#include <QStringList>
#include "cvhelper.h"
#include "mimagelistmodel.h"
#include "mimpexpmodule.h"

typedef std::function<MImageExtras(QString)> CacheRetrieveCB;

class MMatcher
{
public:
    MMatcher(MImageExtras const &to, QString const &fn, int maxresults = 0);
    virtual ~MMatcher() {}

    QStringList LocalMatcher(QList<MImageListRecord> &known, CacheRetrieveCB cb);

    QStringList GlobalMatcher(ProgressCB cb);

private:
    MImageExtras const &original;
    QString filename;
    int results;
    double orig_area;
    std::map<double,MImageListRecord> targets;

    bool Comparator(MImageExtras const &cur, double &key);

    QStringList CreateList();
};

#endif // MMATCHER_H
