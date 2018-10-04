#ifndef MMATCHER_H
#define MMATCHER_H

#include <map>
#include <functional>
#include <QStringList>
#include "cvhelper.h"
#include "shared.h"

typedef std::function<MImageExtras(QString)> CacheRetrieveCB;

class MMatcher
{
public:
    MMatcher(MImageExtras const &to, QString const &fn, int maxresults = 0);
    virtual ~MMatcher() {}

    QStringList LocalMatcher(QList<MImageListRecord> &known, CacheRetrieveCB ccb, ProgressCB pcb);

    QStringList GlobalMatcher(ProgressCB cb);

    static double OneTimeMatcher(cv::Mat const &a, cv::Mat const &b);

private:
    MImageExtras const &original;
    cv::Mat orig_hist_norm;
    bool normalized;
    QString filename;
    int results;
    double orig_area;
    std::map<double,MImageListRecord> targets;

    bool Comparator(MImageExtras const &cur, double &key);

    QStringList CreateList();
};

#endif // MMATCHER_H
