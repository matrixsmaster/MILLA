#include "mmatcher.h"
#include "dbhelper.h"

using namespace cv;
using namespace std;

MMatcher::MMatcher(MImageExtras const &to, const QString &fn, int maxresults) :
    original(to), filename(fn), results(maxresults)
{
    orig_area = original.picsize.width() * original.picsize.height();
    normalized = false;
}

bool MMatcher::Comparator(MImageExtras const &cur, double &key)
{
    if (!cur.valid) return false;
    if (cur.color != original.color) return false; //don't mix color and grayscale

    double cur_area = cur.picsize.width() * cur.picsize.height();
    if (orig_area / cur_area > 2 || cur_area / orig_area > 2) return false; //too big or too small

    //normalize them
    Mat inb;
    if (!normalized) {
        normalize(original.hist,orig_hist_norm,0,1,NORM_MINMAX,-1,Mat());
        normalized = true;
    }
    normalize(cur.hist,inb,0,1,NORM_MINMAX,-1,Mat());

    key = compareHist(orig_hist_norm,inb,HISTCMP_CORREL);
    return (key >= MILLA_MIN_CORREL_MATCH);
}

QStringList MMatcher::LocalMatcher(QList<MImageListRecord> &known, CacheRetrieveCB ccb, ProgressCB pcb)
{
    double corr, prg = 0, dp = 100.f / (double)(known.size());
    targets.clear();

    for (auto &i : known) {
        prg += dp;
        if (pcb && !pcb(prg)) break;

        if (filename == i.filename) continue;

        if (!Comparator(ccb(i.filename),corr)) continue;

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
    QSet<QString> blackl = QSet<QString>::fromList(DBHelper::getExtraStringVal(DBF_EXTRA_EXCLUSION_LIST).split(';',QString::SkipEmptyParts));

    MImageExtras cur;
    MImageListRecord rec;
    double corr, prg = 0, dp = 100.f / (double)(all.size());

    for (auto &i : all) {
        prg += dp;
        if (cb && !cb(prg)) break;

        if (i == filename) continue; //don't match itself
        //check blacklisted directories
        bool f = false;
        for (auto &j : blackl)
            if (i.contains(j,Qt::CaseSensitive)) {
                f = true;
                break;
            }
        if (f) continue;

        cur = DBHelper::getExtrasFromDB(i);
        if (!Comparator(cur,corr)) continue;

        rec.filename = i;
        targets[corr] = rec;
        qDebug() << "[Match] Correlation with " << i << ":  " << corr;
    }

    qDebug() << "[Match] Global: " << targets.size() << " targets selected";
    return CreateList();
}

double MMatcher::OneTimeMatcher(cv::Mat const &a, cv::Mat const &b)
{
    Mat na,nb;
    normalize(a,na,0,1,NORM_MINMAX,-1,Mat());
    normalize(b,nb,0,1,NORM_MINMAX,-1,Mat());
    return compareHist(na,nb,HISTCMP_CORREL) * 100.f;
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
