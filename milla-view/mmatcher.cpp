#include "mmatcher.h"
#include "dbhelper.h"

using namespace cv;
using namespace std;

MMatcher::MMatcher(MImageExtras const &to, const QString &fn, int maxresults) :
    original(to), filename(fn), results(maxresults)
{
    orig_area = original.picsize.width() * original.picsize.height();
}

bool MMatcher::Comparator(MImageExtras const &cur, double &key)
{
    if (!cur.valid) return false;
    if (cur.color != original.color) return false; //don't mix color and grayscale

    double cur_area = cur.picsize.width() * cur.picsize.height();
    if (orig_area / cur_area > 2 || cur_area / orig_area > 2) return false; //too big or too small

    key = compareHist(original.hist,cur.hist,CV_COMP_CORREL);
    return (key > 0);
}

QStringList MMatcher::LocalMatcher(QList<MImageListRecord> &known, CacheRetrieveCB cb)
{
    targets.clear();

    double corr;
    for (auto &i : known) {
        if (filename == i.filename) continue;

        if (!Comparator(cb(i.filename),corr)) continue;

        targets[corr] = i;
        qDebug() << "[Match] Correlation with " << i.filename << ":  " << corr;
    }

    qDebug() << "[Match] Local: " << targets.size() << " targets selected";
    return CreateList();
}

QStringList MMatcher::GlobalMatcher(ProgressCB cb)
{
    targets.clear();

    QStringList all = DBHelper::getAllFiles();
    MImageExtras cur;
    MImageListRecord rec;
    double corr, prg = 0, dp = 100.f / (double)(all.size());

    for (auto &i : all) {
        prg += dp;
        if (cb && !cb(prg)) break;

        if (i == filename) continue;

        cur = DBHelper::getExtrasFromDB(i);
        if (!Comparator(cur,corr)) continue;

        rec.filename = i;
        targets[corr] = rec;
        qDebug() << "[Match] Correlation with " << i << ":  " << corr;
    }

    qDebug() << "[Match] Global: " << targets.size() << " targets selected";
    return CreateList();
}

QStringList MMatcher::CreateList()
{
    QStringList lst;
    int k = 0;
    for (auto i = targets.rbegin(); i != targets.rend() && k < results; ++i,k++) {
        lst.push_back(i->second.filename);
    }
    return lst;
}