#include "dbhelper.h"
#include "mmatcher.h"

static int m_instance = 0;
static std::shared_ptr<DBCache> m_cache;

DBHelper::DBHelper() : QObject()
{
    if (m_instance++)
        cache = m_cache;
}

DBHelper::~DBHelper()
{
    if (--m_instance == 0) {
        cache.reset();
        m_cache.reset();
        QSqlDatabase::database().close();
        qDebug() << "[db] Database closed";
    }
}

bool DBHelper::initDatabase(ProgressCB progress_cb)
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");

    QString fn = QDir::homePath() + DB_FILEPATH;
    qDebug() << "[db] Storage filename: " << fn;
    db.setHostName("localhost");
    db.setDatabaseName(fn);
    db.setUserName("user");
    bool ok = db.open();
    qDebug() << "[db] open:  " << ok;
    if (!ok) return false;

    QSqlQuery q;
    ok = q.exec("SELECT version FROM meta") && q.next();
    qDebug() << "[db] Read meta table: " << ok;
    if (!ok) {
        q.clear();
        ok = q.exec("CREATE TABLE meta (" DBF_META ")");
        qDebug() << "[db] Create new meta table: " << ok;
        q.clear();
        q.prepare("INSERT INTO meta (version) VALUES (:ver)");
        q.bindValue(":ver",DB_VERSION);
        ok = q.exec();
        qDebug() << "[db] Inserting current DB version tag " << DB_VERSION << ": " << ok;
        if (!ok) return false;

    } else {
        qDebug() << "[db] version: " << q.value(0).toInt();
        if (DB_VERSION != q.value(0).toInt()) {
            qDebug() << "FATAL: DB version mismatch";
            return false;
        }
    }

    QList<std::pair<const char*,const char*>> tabs = DB_CONTENTS;
    for (auto &i : tabs) {
        if (!checkAndCreate(i.first,i.second)) return false;
    }

    cache.reset(new DBCache(progress_cb));
    m_cache = cache;

    return ok;
}

bool DBHelper::checkAndCreate(const char* tname, const char* format)
{
    QSqlQuery q;

    QString qr = QString::asprintf(DBF_TABLE_CHECK "'%s'",tname);
    bool ok = q.exec(qr) && q.next();
    qDebug() << "[db] Read" << tname << "table:" << ok;

    if (!ok) {
        q.clear();
        qr = QString::asprintf("CREATE TABLE %s (%s)",tname,format);
        ok = q.exec(qr);
        qDebug() << "[db] Create new" << tname << "table: " << ok;
    }

    return ok;
}

QString DBHelper::timePrinter(double sec)
{
    QString res;
    double ehr = floor(sec / 3600.f);
    if (ehr > 0) {
        res += QString::asprintf("%.0f hrs ",ehr);
        sec -= ehr * 3600.f;
    }
    double emn = floor(sec / 60.f);
    if (emn > 0) {
        res += QString::asprintf("%.0f min ",emn);
        sec -= emn * 60.f;
    }
    res += QString::asprintf("%.1f sec",sec);
    return res;
}

QString DBHelper::getCanonicalPath(QString const &fn)
{
    QFileInfo fi(fn);
    return fi.exists()? fi.canonicalFilePath() : QString();
}

QByteArray DBHelper::getSHA256(QString const &fn, qint64* size)
{
    if (fn.isEmpty()) return QByteArray();

    QCryptographicHash hash(QCryptographicHash::Sha256);
    QByteArray shasum;
    QFile mfile(fn);
    mfile.open(QIODevice::ReadOnly);
    if (hash.addData(&mfile))
        shasum = hash.result();
    else
        qDebug() << "ALERT: Unable to calculate SHA256 of file " << fn;
    if (size) *size = mfile.size();
    mfile.close();
    return shasum;
}

bool DBHelper::getThumbnail(MImageListRecord &rec)
{
    QSqlQuery q;
    q.prepare("SELECT thumb, mtime FROM thumbs WHERE file = (:fn)");
    q.bindValue(":fn",rec.filename);

    if (q.exec() && q.next() && q.value(0).canConvert(QVariant::ByteArray) && q.value(1).canConvert(QVariant::UInt)) {
        qDebug() << "[db] Loaded thumbnail for " << rec.filename;
        rec.thumb.loadFromData(q.value(0).toByteArray());
        rec.thumbOK = true;
        rec.filechanged = q.value(1).toUInt();
        return true;
    }
    return false;
}

bool DBHelper::updateThumbnail(MImageListRecord &rec, QByteArray const &png)
{
    QSqlQuery q;
    QString act;
    q.prepare("SELECT mtime FROM thumbs WHERE file = (:fn)");
    q.bindValue(":fn",rec.filename);
    bool mod = q.exec() && q.next();

    if (!mod) {
        q.prepare("INSERT INTO thumbs (file, mtime, thumb) VALUES (:fn, :mtm, :thm)");
        act = "Inserting";
    } else {
        q.prepare("UPDATE thumbs SET thumb = :thm, mtime = :mtm WHERE file = :fn");
        act = "Updating";
    }
    q.bindValue(":fn",rec.filename);
    q.bindValue(":mtm",(qlonglong)(rec.filechanged));
    q.bindValue(":thm",png);
    bool ok = q.exec();

    qDebug() << "[db] " << act << " record for " << rec.fnshort << ": " << ok;
    return ok;
}

bool DBHelper::isStatRecordExists(QString const &fn)
{
    if (fn.isEmpty()) return false;

    QSqlQuery q;
    q.prepare("SELECT views FROM stats WHERE file = (:fn)");
    q.bindValue(":fn",fn);

    return (q.exec() && q.next());
}

bool DBHelper::updateStatRecord(QString const &fn, MImageExtras &rec, bool update)
{
    if (fn.isEmpty()) return false;

    int fcn = 0;
    QString fcdat;
    for (auto &i : rec.rois)
        if (i.kind == MROI_FACE_FRONTAL) {
            fcn++;
            fcdat += QString::asprintf("%d,%d,%d,%d,",i.x,i.y,i.w,i.h);
        }

    QByteArray harr = qCompress(CVHelper::storeMat(rec.hist));
    qDebug() << "[db] Final harr length =" << harr.size();

    QSqlQuery q;
    if (update)
        q.prepare("UPDATE stats SET " DBF_STATS_UPDATE_SHORT " WHERE file = :fn");
    else
        q.prepare("INSERT INTO stats (" DBF_STATS_SHORT ") VALUES " DBF_STATS_INSERT_KEYS);

    q.bindValue(":fn",fn);
    q.bindValue(":tm",(uint)time(NULL));
    q.bindValue(":sx",rec.picsize.width());
    q.bindValue(":sy",rec.picsize.height());
    q.bindValue(":gry",rec.color? 0:1);
    q.bindValue(":fcn",fcn);
    q.bindValue(":fcr",fcdat);
    q.bindValue(":hst",harr);
    q.bindValue(":sha",rec.sha);
    q.bindValue(":len",rec.filelen);

    return q.exec();
}

MImageExtras DBHelper::getExtrasFromDB(QString const &fn)
{
    MImageExtras res;
    if (fn.isEmpty()) return res;

    QSqlQuery q;
    q.prepare("SELECT sizex, sizey, grayscale, faces, facerects, hist, sha256, length FROM stats WHERE file = :fn");
    q.bindValue(":fn",fn);
    if (!q.exec() || !q.next()) return res;

    res.picsize = QSize(q.value(0).toUInt(), q.value(1).toUInt());
    res.color = !(q.value(2).toInt());
    QStringList flst = q.value(4).toString().split(',',QString::SkipEmptyParts);
    for (auto k = flst.begin(); k != flst.end();) {
        MROI r;
        r.kind = MROI_FACE_FRONTAL;
        r.x = k->toInt(); k++;
        r.y = k->toInt(); k++;
        r.w = k->toInt(); k++;
        r.h = k->toInt(); k++;
        res.rois.push_back(r);
    }
    if (q.value(5).canConvert(QVariant::ByteArray))
        res.hist = CVHelper::loadMat(qUncompress(q.value(5).toByteArray()));
    if (q.value(6).canConvert(QVariant::ByteArray))
        res.sha = q.value(6).toByteArray();
    if (q.value(7).canConvert(QVariant::UInt))
        res.filelen = q.value(7).toUInt();

    res.valid = true;
    return res;
}

time_t DBHelper::getLastViewTime(QString const &fn)
{
    if (fn.isEmpty()) return time(NULL);

    QSqlQuery q;
    q.prepare("SELECT lastview FROM stats WHERE file = :fn");
    q.bindValue(":fn",fn);
    if (q.exec() && q.next()) return q.value(0).toUInt();
    return time(NULL);
}

bool DBHelper::insertTag(QString const &ntag, unsigned &key)
{
    if (ntag.isEmpty()) return false;

    QSqlQuery q;
    q.prepare("SELECT * FROM tags WHERE tag = (:tg)");
    q.bindValue(":tg",ntag);
    if (q.exec() && q.next()) {
        qDebug() << "[db] tag is already defined";
        return false;
    }

    q.clear();
    if (!q.exec("SELECT MAX(key) FROM tags") || !q.next()) {
        qDebug() << "[db] ERROR: unable to get max key value from tags table";
        return false;
    }
    key = q.value(0).toUInt() + 1;

    q.clear();
    q.prepare("INSERT INTO tags (key, tag, rating) VALUES (:k, :tg, 0)");
    q.bindValue(":k",key);
    q.bindValue(":tg",ntag);
    bool ok = q.exec();

    qDebug() << "[db] Inserting tag " << ntag << ": " << ok;
    return ok;
}

int DBHelper::getFileRating(QString const &fn)
{
    int n = 5;
    if (fn.isEmpty()) return n;

    QSqlQuery q;
    q.prepare("SELECT rating FROM stats WHERE file = :fn");
    q.bindValue(":fn",fn);
    if (q.exec() && q.next()) n = q.value(0).toInt();
    else n = 0;
    return n;
}

bool DBHelper::updateFileRating(QString const &fn, int n)
{
    if (fn.isEmpty()) return false;
    if (n < 0) n = 0;
    if (n > 5) n = 5;

    QSqlQuery q;
    q.prepare("UPDATE stats SET rating = :r WHERE file = :fn");
    q.bindValue(":r",n);
    q.bindValue(":fn",fn);
    bool ok = q.exec();

    qDebug() << "[db] Rating update: " << ok;
    return ok;
}

unsigned DBHelper::getFileViews(QString const &fn, bool &ok)
{
    ok = false;
    if (fn.isEmpty()) return 0;

    QSqlQuery q;
    q.prepare("SELECT views FROM stats WHERE file = (:fn)");
    q.bindValue(":fn",fn);

    unsigned n = 0;
    ok = (q.exec() && q.next());
    if (ok) n = q.value(0).toUInt();

    return n;
}

bool DBHelper::updateFileViews(QString const &fn, unsigned n)
{
    if (fn.isEmpty()) return false;

    QSqlQuery q;
    q.prepare("UPDATE stats SET views = :v, lastview = :tm WHERE file = :fn");
    q.bindValue(":v",n);
    q.bindValue(":tm",(uint)time(NULL));
    q.bindValue(":fn",fn);
    bool ok = q.exec();

    qDebug() << "[db] Updating views: " << ok;
    return ok;
}

MTagsCheckList DBHelper::getFileTags(QString const &fn)
{
    MTagsCheckList out;
    QSqlQuery q,qq;
    QStringList tlst;

    bool ok = q.exec("SELECT tag, key FROM tags ORDER BY rating DESC");
    qDebug() << "[db] Read whole tags table: " << ok;
    if (!ok) return out;

    if (!fn.isEmpty()) {
        qq.prepare("SELECT tags FROM stats WHERE file = :fn");
        qq.bindValue(":fn",fn);
        if (qq.exec() && qq.next())
            tlst = qq.value(0).toString().split(',',QString::SkipEmptyParts);
    }

    bool c;
    while (q.next()) {
        c = (!tlst.empty() && tlst.contains(q.value(1).toString()));
        out.push_back(std::tuple<QString,unsigned,bool>(q.value(0).toString(),q.value(1).toUInt(),c));
    }

    return out;
}

QString DBHelper::getFileTagsString(QString const &fn)
{
    QString s;
    MTagsCheckList l = getFileTags(fn);
    for (auto &i : l) {
        if (std::get<2>(i) == false) continue;
        s += std::get<0>(i);
        s += ',';
    }
    return s;
}

bool DBHelper::updateTags(QString const &tag, bool checked)
{
    if (tag.isEmpty()) return false;

    QSqlQuery q;
    q.prepare("SELECT rating FROM tags WHERE tag = :tg");
    q.bindValue(":tg",tag);
    if (q.exec() && q.next()) {
        uint rat = q.value(0).toUInt();
        if (checked) rat++;
        else if (rat) rat--;

        q.clear();
        q.prepare("UPDATE tags SET rating = :rat WHERE tag = :tg");
        q.bindValue(":rat",rat);
        q.bindValue(":tg",tag);
        bool ok = q.exec();
        qDebug() << "[db] Updating rating to " << rat << " for tag " << tag << ":" << ok;

        return true;
    }

    qDebug() << "ALERT: partially known tag " << tag;
    return false;
}

bool DBHelper::updateFileTags(QString const &fn, MTagCache const &cache)
{
    if (fn.isEmpty()) return false;

    QString tgs;
    int ntg = 0;
    for (auto &i : cache)
        if (i.second.second) {
            tgs += QString::asprintf("%d,",i.second.first);
            ntg++;
        }

    QSqlQuery q;
    q.prepare("UPDATE stats SET ntags = :ntg, tags = :tgs WHERE file = :fn");
    q.bindValue(":ntg",ntg);
    q.bindValue(":tgs",tgs);
    q.bindValue(":fn",fn);
    bool ok = q.exec();

    qDebug() << "[db] Updating tags: " << ok;
    return ok;
}

int DBHelper::updateFileKudos(QString const &fn, int delta)
{
    if (fn.isEmpty()) return 0;

    int n = delta;
    QSqlQuery q;
    q.prepare("SELECT likes FROM stats WHERE file = :fn");
    q.bindValue(":fn",fn);

    if (!q.exec() || !q.next()) return 0;

    n += q.value(0).toInt();

    bool ok = true;
    if (delta) {
        q.clear();
        q.prepare("UPDATE stats SET likes = :lk WHERE file = :fn");
        q.bindValue(":lk",n);
        q.bindValue(":fn",fn);
        ok = q.exec();
        qDebug() << "[db] Updating likes: " << ok;
    }

    return n;
}

QString DBHelper::getFileNotes(QString const &fn)
{
    if (fn.isEmpty()) return QString();

    QSqlQuery q;
    q.prepare("SELECT notes FROM stats WHERE file = :fn");
    q.bindValue(":fn",fn);
    if (q.exec() && q.next()) return q.value(0).toString();
    else return QString();
}

bool DBHelper::updateFileNotes(QString const &fn, QString &notes)
{
    if (fn.isEmpty()) return false;

    QSqlQuery q;
    q.prepare("UPDATE stats SET notes = :nt WHERE file = :fn");
    q.bindValue(":nt",notes);
    q.bindValue(":fn",fn);
    bool ok = q.exec();
    qDebug() << "[db] Setting user notes: " << ok;
    return ok;
}

bool DBHelper::createLinkBetweenImages(QByteArray const &left, QByteArray const &right, bool force, uint stamp)
{
    QSqlQuery q;
    bool ok;

    q.prepare("SELECT created FROM links WHERE left = :sl AND right = :sr");
    q.bindValue(":sl",left);
    q.bindValue(":sr",right);
    if (q.exec() && q.next()) {
        if (!force) {
            qDebug() << "[db] Link already exists";
            return false;
        }
        removeLinkBetweenImages(left,right,true);
    }

    q.clear();
    q.prepare("INSERT INTO links (created, left, right) VALUES (:tm, :sl, :sr)");
    q.bindValue(":tm",stamp? stamp : (uint)time(NULL));
    q.bindValue(":sl",left);
    q.bindValue(":sr",right);
    ok = q.exec();

    qDebug() << "[db] Inserting link: " << ok;
    if (ok && cache) cache->addLink(left,right);
    return ok;
}

bool DBHelper::removeLinkBetweenImages(QByteArray const &left, QByteArray const &right, bool force)
{
    QSqlQuery q;
    bool ok;

    if (!force) {
        q.prepare("SELECT created FROM links WHERE left = :sl AND right = :sr");
        q.bindValue(":sl",left);
        q.bindValue(":sr",right);
        if (!q.exec() || !q.next()) {
            qDebug() << "[db] No link";
            return false;
        }
    }

    q.clear();
    q.prepare("DELETE FROM links WHERE left = :sl AND right = :sr");
    q.bindValue(":sl",left);
    q.bindValue(":sr",right);
    ok = q.exec();

    qDebug() << "[db] Removing link: " << ok;
    if (ok && cache) cache->removeLink(left,right);
    return ok;
}

QStringList DBHelper::getLinkedImages(QByteArray const &sha, bool reverse)
{
    QStringList out;

    if (!cache) {
        QSqlQuery q;

        if (reverse) q.prepare("SELECT left FROM links WHERE right = :sha");
        else q.prepare("SELECT right FROM links WHERE left = :sha");

        q.bindValue(":sha",sha);
        if (!q.exec()) return out;

        while (q.next()) {
            if (!q.value(0).canConvert(QVariant::ByteArray)) continue;

            QSqlQuery qq;
            qq.prepare("SELECT file FROM stats WHERE sha256 = :sha");
            qq.bindValue(":sha",q.value(0).toByteArray());
            if (qq.exec() && qq.next())
                out.push_back(qq.value(0).toString());
        }

    } else
        out = cache->getLinkedTo(sha,reverse);

    return out;
}

QStringList DBHelper::getLinkedImages(QString const &fn, bool reverse)
{
    if (!cache) return QStringList();
    return cache->getLinkedTo(fn,reverse);
}

QStringList DBHelper::tagSearch(MTagCache const &cache, QList<MImageListRecord>* within, int maxitems)
{
    QSqlQuery q;
    std::map<QString,std::pair<QList<int>,int>> targ;
    QList<int> goal,exclude;
    QList<std::pair<QString,int>> found;
    QStringList r_found;

    //collect
    for (auto &i : cache) {
        if (i.second.second == Qt::Unchecked) continue;

        q.clear();
        if (i.second.second == Qt::Checked) {
            goal.push_back(i.second.first);
            q.prepare("SELECT file,tags,rating FROM stats WHERE tags LIKE :t OR INSTR( tags, :i ) > 0");

        } else {
            exclude.push_back(i.second.first);
            q.prepare("SELECT file,tags,rating FROM stats WHERE LENGTH( tags ) > 0 AND tags NOT LIKE :t AND INSTR( tags, :i ) <= 0");
        }

        q.bindValue(":t",QString::asprintf("%d,%%",i.second.first));
        q.bindValue(":i",QString::asprintf(",%d,",i.second.first));
        if (!q.exec()) {
            qDebug() << "Select tag " << i.second.first << " failed";
            continue;
        }

        while (q.next()) {
            //qDebug() << "TAG " << i.second.first << " FOUND: " << q.value(0).toString();
            QList<int> l;
            QStringList _l = q.value(1).toString().split(",",QString::SkipEmptyParts);
            for (auto &j : _l) l.push_back(j.toInt());
            targ[q.value(0).toString()] = std::pair<QList<int>,int> (l,q.value(2).toInt());
        }
    }

    qDebug() << "Target list size: " << targ.size();

    //filter
    for (auto &i : targ) {
        bool k = true;
        //inclusions
        for (auto &j : goal)
            if (!i.second.first.contains(j)) {
                k = false;
                break;
            }
        if (!k) continue;
        //exclusions
        for (auto &j : exclude)
            if (i.second.first.contains(j)) {
                k = false;
                break;
            }
        if (!k) continue;

        //check limiting factor - currently loaded images
        if (within) {
            auto w = std::find_if(within->begin(),within->end(),[&] (const MImageListRecord& a) { return (a.filename == i.first); });
            if (w == within->end()) {
                qDebug() << "File " << i.first << " isn't among currently loaded ones";
                continue;
            }
        }

        found.push_back(std::pair<QString,int>(i.first,i.second.second));
    }
    qDebug() << "Found: " << found.size();

    //sort & convert
    std::sort(found.begin(),found.end(),[] (const auto &a, const auto &b) {
        return a.second > b.second;
    });
    for (auto &i : found) {
        r_found.push_back(i.first);
        if (maxitems > 0 && r_found.size() >= maxitems) break;
    }

    return r_found;
}

void DBHelper::initParametricSearch(QList<MImageListRecord> const &from)
{
    searchlist.clear();
    QSet<QString> blackl = QSet<QString>::fromList(getExtraStringVal(DBF_EXTRA_EXCLUSION_LIST).split(';',QString::SkipEmptyParts));

    MImageListRecord r;
    for (auto &i : from) {
        //check blacklisted directories
        bool f = false;
        for (auto &j : blackl)
            if (i.filename.contains(j,Qt::CaseSensitive)) {
                f = true;
                break;
            }
        if (f) continue;

        //save only fields needed for search
        r.filename = i.filename;
        r.filechanged = i.filechanged;

        searchlist.push_back(r);
    }
}

QStringList DBHelper::doParametricSearch(SearchFormData flt, ProgressCB pcb)
{
    QSqlQuery q;
    size_t area;
    double liked;
    std::multimap<uint,QString> tmap;
    std::set<QString> tlst;
    double prg = 0, dp = 100.f / static_cast<double>(searchlist.size());

    while (!searchlist.empty()) {
        MImageListRecord r = searchlist.front();
        searchlist.pop_front();

        prg += dp;
        if (pcb && !pcb(prg)) break;

        q.clear();
        q.prepare("SELECT rating, likes, views, faces, grayscale, sizex, sizey, ntags, notes, lastview FROM stats WHERE file = :fn");
        q.bindValue(":fn",r.filename);
        if (q.exec() && q.next()) {

            if (flt.rating > q.value(0).toInt()) continue;
            if (flt.kudos > q.value(1).toInt()) continue;
            if (flt.tags > q.value(7).toInt()) continue;
            if (q.value(2).toUInt() < flt.minviews || q.value(2).toUInt() >= flt.maxviews) continue;
            if (q.value(3).toInt() < flt.minface || q.value(3).toInt() >= flt.maxface) continue;
            if (flt.colors > -1 && flt.colors != (q.value(4).toInt()>0)) continue;
            if ((flt.minmtime && flt.maxmtime) && (r.filechanged < flt.minmtime || r.filechanged > flt.maxmtime)) continue;
            if ((flt.minstime && flt.maxstime) && (q.value(9).toUInt() < flt.minstime || q.value(9).toUInt() > flt.maxstime)) continue;
            if (flt.wo_tags && q.value(7).toInt()) continue;
            if (flt.w_notes && q.value(8).toString().isEmpty()) continue;
            if (!flt.text_notes.isEmpty() && !q.value(8).toString().contains(flt.text_notes,Qt::CaseInsensitive)) continue;

            liked = (q.value(1).toInt())? static_cast<double>(q.value(1).toInt()) / static_cast<double>(q.value(2).toInt()) * 100.f : 0;
            if (flt.liked > 0 && flt.liked > liked) continue;

            if (flt.orient >= 0) {
                if (flt.orient && q.value(5).toInt() <= q.value(6).toInt()) //landscape
                    continue;
                else if (!flt.orient && q.value(5).toInt() > q.value(6).toInt()) //portrait
                    continue;
            }

            area = q.value(5).toUInt() * q.value(6).toUInt();
            if (area < flt.minsize || area > flt.maxsize) continue;

            if (!flt.text_fn.isEmpty() || !flt.text_path.isEmpty()) {
                QFileInfo fi(r.filename);
                if (!flt.text_fn.isEmpty() && !fi.baseName().contains(flt.text_fn,Qt::CaseInsensitive)) continue;
                if (!flt.text_path.isEmpty() && !fi.path().contains(flt.text_path,Qt::CaseInsensitive)) continue;
                qDebug() << fi.baseName() << fi.filePath();
            }

            if (!flt.tags_inc.isEmpty() || !flt.tags_exc.isEmpty()) {
                QString stags = getFileTagsString(r.filename);
                qDebug() << stags;
                if (!flt.tags_inc.isEmpty()) {
                    if (!stags.contains(flt.tags_inc,Qt::CaseInsensitive)) continue;
                }
                if (!flt.tags_exc.isEmpty()) {
                    if (stags.contains(flt.tags_exc,Qt::CaseInsensitive)) continue;
                }
            }

            if (flt.similar > 0) {
                MImageExtras _ex = getExtrasFromDB(r.filename);
                if (_ex.valid && MMatcher::OneTimeMatcher(flt.similar_to,_ex.hist) < flt.similar)
                    continue;
#ifdef QT_DEBUG
                 qDebug() << "File " << r.filename << " have " << MMatcher::OneTimeMatcher(flt.similar_to,_ex.hist) << "% similarity";
#endif
            }

            switch (flt.sort) {
            case SRFRM_RATING: tmap.insert(std::pair<uint,QString>(q.value(0).toInt(), r.filename)); break;
            case SRFRM_KUDOS: tmap.insert(std::pair<uint,QString>(q.value(1).toInt(), r.filename)); break;
            case SRFRM_VIEWS: tmap.insert(std::pair<uint,QString>(q.value(2).toUInt(), r.filename)); break;
            case SRFRM_DATE: tmap.insert(std::pair<uint,QString>(r.filechanged, r.filename)); break;
            case SRFRM_FACES: tmap.insert(std::pair<uint,QString>(q.value(3).toInt(), r.filename)); break;
            case SRFRM_LASTSEEN: tmap.insert(std::pair<uint,QString>(q.value(9).toUInt(), r.filename)); break;
            default: tlst.insert(r.filename);
            }

        }

        if (tlst.size() + tmap.size() >= flt.maxresults) break;
    }

    QStringList out;

    qDebug() << "[search] Start of list";
    if (flt.sort == SRFRM_NAME) {
        for (auto &i : tlst) {
            qDebug() << i;
            out.push_back(i);
        }
    } else {
        for (auto i = tmap.rbegin(); i != tmap.rend(); ++i) {
            qDebug() << "key: " << i->first << "; val = " << i->second;
            out.push_back(i->second);
        }
    }
    qDebug() << "[search] End of list";

    return out;
}

QStringList DBHelper::getAllFiles()
{
    QSqlQuery q;
    QStringList out;

    if (!q.exec("SELECT file FROM stats")) return out;
    while (q.next()) out.push_back(q.value(0).toString());

    return out;
}

QString DBHelper::getFileBySHA(QByteArray const &sha)
{
    if (!cache) {
        QSqlQuery q;
        q.prepare("SELECT file FROM stats WHERE sha256 = :sha");
        q.bindValue(":sha",sha);
        if (!q.exec() || !q.next()) return QString();

        qDebug() << "[db] File found by SHA-256: " << q.value(0).toString();
        return q.value(0).toString();
    }

    return cache->getFilenameBySHA(sha);
}

QByteArray DBHelper::getSHAbyFile(QString const &fn)
{
    if (!cache) {
        QSqlQuery q;
        q.prepare("SELECT sha256 FROM stats WHERE file = :fn");
        q.bindValue(":fn",fn);
        if (!q.exec() || !q.next() || !q.value(0).canConvert<QByteArray>()) return QByteArray();

        return q.value(0).toByteArray();
    }

    return cache->getSHAbyFilename(fn);
}

bool DBHelper::removeFile(QString const &fn)
{
    QSqlQuery q;
    if (fn.isEmpty()) return false;

    //remove it from stats table...
    q.prepare("DELETE FROM stats WHERE file = :fn");
    q.bindValue(":fn",fn);
    bool ok1 = q.exec();
    qDebug() << "[db] Removing file " << fn << " from stats: " << ok1;

    //...and from thumbnails table too...
    q.clear();
    q.prepare("DELETE FROM thumbs WHERE file = :fn");
    q.bindValue(":fn",fn);
    bool ok2 = q.exec();
    qDebug() << "[db] Removing file " << fn << " from thumbs: " << ok2;

    //...and from cache
    if (cache) cache->removeFile(fn);

    return (ok1 || ok2);
}

void DBHelper::sanitizeFiles(ProgressCB progress_cb)
{
    QSqlQuery q;
    QStringList all = getAllFiles(); //get all known files listed
    double prg = 0, dp = 50.f / (double)(all.size());

    //add files with known thumbnails (whos metainfo was never saved)
    q.prepare("SELECT file FROM thumbs");
    if (q.exec()) {
        while (q.next()) {
            if (!all.contains(q.value(0).toString()))
                all.push_back(q.value(0).toString());
        }
    }

    //let's walk through all of them
    for (auto &i : all) {
        prg += dp;
        if (progress_cb && !progress_cb(prg)) return;

        QFileInfo fi(i);
        if (!fi.exists()) removeFile(i); //if a dead file is detected, remove it
    }

    //as last measure, let's remove all records about zero-length files and files without checksum
    q.clear();
    if (!q.exec("SELECT COUNT(file) FROM stats WHERE (length < 1) OR (sha256 IS NULL)") || !q.next()) {
        qDebug() << "[db] ALERT: error while searching for empty records";
        return;
    }
    if (!q.value(0).toUInt()) return; //nothing to do

    //update progress variables
    prg = 50;
    dp = 50.f / (double)(q.value(0).toUInt());

    //get actual list of empty entries
    q.clear();
    if (!q.exec("SELECT file FROM stats WHERE (length < 1) OR (sha256 IS NULL)")) return;
    while (q.next()) {
        prg += dp;
        if (progress_cb && !progress_cb(prg)) return;

        removeFile(q.value(0).toString());
    }
}

void DBHelper::sanitizeLinks(ProgressCB progress_cb)
{
    QSqlQuery q,qa;
    QSet<QByteArray> known_shas;
    QList<std::pair<QByteArray,QByteArray>> to_remove;

    if (!q.exec("SELECT COUNT(created) FROM links") || !qa.exec("SELECT left, right FROM links")) {
        qDebug() << "[db] Unable to load links table";
        return;
    }

    double prg = 0, dp = q.next()? 50.f / (double)(q.value(0).toInt()) : 0;

    while (qa.next()) {
        prg += dp;
        if (progress_cb && !progress_cb(prg)) return;

        if (qa.value(0).toByteArray() == qa.value(1).toByteArray())
            qDebug() << "[Sanitizer] File linked to itself detected";

        else {

            if (known_shas.contains(qa.value(0).toByteArray()) && known_shas.contains(qa.value(1).toByteArray()))
                continue;
            q.clear();
            q.prepare("SELECT file FROM stats WHERE sha256 = :sha1 OR sha256 = :sha2");
            q.bindValue(":sha1",qa.value(0).toByteArray());
            q.bindValue(":sha2",qa.value(1).toByteArray());
            if (q.exec() && q.next() && q.next()) { //there must be two consecutive records
                known_shas.insert(qa.value(0).toByteArray());
                known_shas.insert(qa.value(1).toByteArray());
                continue;
            }
            qDebug() << "[Sanitizer] Lost file with sha " << qa.value(0).toByteArray().toHex();
        }

        to_remove.push_back(std::pair<QByteArray,QByteArray>(qa.value(0).toByteArray(),qa.value(1).toByteArray()));
    }

    qDebug() << "[Sanitizer] " << to_remove.size() << " records scheduled for removal";

    prg = 50;
    dp = 50.f / (double)(to_remove.size());

    for (auto &i : to_remove) {
        q.clear();
        q.prepare("DELETE FROM links WHERE left = :sha1 AND right = :sha2");
        q.bindValue(":sha1",i.first);;
        q.bindValue(":sha2",i.second);;
        if (!q.exec()) {
            qDebug() << "[db] Failed to remove link record";
            break;
        }

        prg += dp;
        if (progress_cb && !progress_cb(prg)) return;
    }
}

void DBHelper::sanitizeTags(ProgressCB progress_cb)
{
    QSqlQuery q,qa,qz;
    if (!q.exec("SELECT COUNT(tags) FROM stats") || !qa.exec("SELECT tags, ntags, file FROM stats")) {
        qDebug() << "[db] Unable to query stats table for tags listing";
        return;
    }
    if (!qz.exec("SELECT key FROM tags")) {
        qDebug() << "[db] Unable to query tags table for tags listing";
        return;
    }

    QSet<unsigned> all;
    while (qz.next()) all.insert(qz.value(0).toUInt());

    double prg = 0, dp = q.next()? 50.f / (double)(q.value(0).toInt()) : 0;

    std::map<unsigned,int> used_tags;
    while (qa.next()) {
        prg += dp;
        if (progress_cb && !progress_cb(prg)) return;

        if (qa.value(0).toString().isEmpty()) continue;
        QStringList ls = qa.value(0).toString().split(',',QString::SkipEmptyParts);
        if (ls.isEmpty()) continue;

        bool mod = false;
        for (auto j = ls.begin(); j != ls.end();) {
            unsigned ji = j->toUInt();

            if (!all.contains(ji)) {
                mod = true;
                j = ls.erase(j);
                qDebug() << "[Sanitizer] Removing unknown tag index" << ji;
                continue;
            }

            if (used_tags.count(ji)) used_tags[ji]++;
            else used_tags[ji] = 1;
            ++j;
        }

        if (qa.value(1).toInt() != ls.size()) mod = true;

        if (mod) {
            QSqlQuery wq;
            QString ln2 = ls.empty()? "" : ln2 = ls.join(',') + ",";
            wq.prepare("UPDATE stats SET tags = :tg, ntags = :nt WHERE file = :fn");
            wq.bindValue(":tg",ln2);
            wq.bindValue(":nt",ls.size());
            wq.bindValue(":fn",qa.value(2).toString());
            wq.exec();
        }
    }

    qDebug() << "[Sanitizer] " << used_tags.size() << " tags currently in use";

    prg = 50;
    dp = all.empty()? 0 : 50.f / (double)(all.size());

    for (auto &i : used_tags) {
        int n;
        unsigned k = i.first;
        if (!used_tags.count(k)) n = 0;
        else n = used_tags.at(k);

        q.clear();
        q.prepare("UPDATE tags SET rating = :n WHERE key = :k");
        q.bindValue(":n",n);
        q.bindValue(":k",k);
        if (!q.exec()) {
            qDebug() << "[db] Failed to update tag record";
            break;
        }

        prg += dp;
        if (progress_cb && !progress_cb(prg)) return;
    }
}

void DBHelper::sanitizeDBMeta()
{
    QSqlQuery q;
    qDebug() << "[db] Running vacuum...";
    bool ok = q.exec("VACUUM");
    qDebug() << "[db] Vacuum: " << ok;
}

QString DBHelper::detectExactCopies(ProgressCB progress_cb)
{
    QString res;
    QSqlQuery q;
    double prg, dp;

    if (q.exec("SELECT COUNT(file) FROM stats") && q.next()) {
        prg = 0;
        dp = 100.f / (double)(q.value(0).toUInt());
    } else {
        qDebug() << "[db] ALERT: unable to get stats table size";
        return res;
    }

    q.clear();
    if (!q.exec("SELECT file, sha256, length FROM stats")) {
        qDebug() << "[db] ALERT: unable to query stats table";
        return res;
    }

    QTextStream out(&res);
    std::multimap<size_t,std::pair<QByteArray,QString>> kmap;
    while (q.next()) {
        prg += dp;
        if (progress_cb && !progress_cb(prg)) break;

        QString fn = q.value(0).toString();
        QByteArray sha = q.value(1).toByteArray();
        size_t len = q.value(2).toUInt();

        bool fnd = false;
        if (kmap.count(len)) {
            auto pk = kmap.equal_range(len);
            for (auto ik = pk.first; ik != pk.second; ++ik)
                if (sha == ik->second.first) {
                    out << "Duplicates: \"" << ik->second.second;
                    out << "\" and \"" << fn << "\"\n";
                    fnd = true;
                    break;
                }
        }

        if (!fnd)
            kmap.insert(std::pair<size_t,std::pair<QByteArray,QString>>(len,std::pair<QByteArray,QString>(sha,fn)));
    }

    return res;
}

void DBHelper::setWinTableName(QString const &nm)
{
    wintable = nm;
}

QByteArray DBHelper::getWindowGeometryOrState(bool geom)
{
    QSqlQuery q;
    q.prepare(QString("SELECT %1 FROM %2 WHERE name = 'MainWindow'").arg((geom?"geometry":"state"),wintable));
    if (!q.exec() || !q.next()) return QByteArray();
    return q.value(0).toByteArray();
}

bool DBHelper::updateWindowGeometryAndState(QByteArray const &geom, QByteArray const &state)
{
    QSqlQuery q;
    q.prepare(QString("SELECT COUNT(geometry) FROM %1 WHERE name = 'MainWindow'").arg(wintable));
    if (!q.exec() || !q.next()) return false;

    if (!q.value(0).toInt())
        q.prepare(QString("INSERT INTO %1 (name, geometry, state) VALUES ('MainWindow', :g, :s)").arg(wintable));
    else
        q.prepare(QString("UPDATE %1 SET geometry = :g, state = :s WHERE name = 'MainWindow'").arg(wintable));
    q.bindValue(":g",geom);
    q.bindValue(":s",state);

    bool ok = q.exec();
    qDebug() << "[db] Saving window geometry and state to" << wintable << ":" << ok;
    return ok;
}

bool DBHelper::restoreViewerState(QObjectList const &lst)
{
    for (auto &i : lst) {
        if (!restoreViewerState(i->children())) return false;
        if (i->objectName().isEmpty()) continue;

        if (    QString(i->metaObject()->className()) != "QSplitter" &&
                QString(i->metaObject()->className()) != "QAction")     continue;

        QSqlQuery q;
        q.prepare(QString("SELECT state FROM %1 WHERE name = :n").arg(wintable));
        q.bindValue(":n",i->objectName());
        if (!q.exec() || !q.next()) continue;

        if (QString(i->metaObject()->className()) == "QSplitter") {
            QSplitter* ptr = dynamic_cast<QSplitter*>(i);
            if (ptr) ptr->restoreState(q.value(0).toByteArray());

        } else {
            QAction* ptr = dynamic_cast<QAction*>(i);
            if (!ptr || !ptr->isCheckable()) continue;
            bool b = q.value(0).toByteArray().at(0);
            if (ptr->isChecked() != b) ptr->trigger();
        }
    }
    return true;
}

bool DBHelper::updateViewerState(QObjectList const &lst)
{
    for (auto &i : lst) {
        if (!updateViewerState(i->children())) return false;
        if (i->objectName().isEmpty()) continue;

        QSplitter* spl = nullptr;
        QAction* act = nullptr;

        if (QString(i->metaObject()->className()) == "QSplitter") spl = dynamic_cast<QSplitter*>(i);
        if (QString(i->metaObject()->className()) == "QAction") act = dynamic_cast<QAction*>(i);

        if (!spl && !act) continue;
        if (act && !act->isCheckable()) continue;

        QByteArray tmp;
        if (spl) tmp = spl->saveState();
        else tmp.append(act->isChecked());

        QSqlQuery q;
        q.prepare(QString("SELECT COUNT(state) FROM %1 WHERE name = :n").arg(wintable));
        q.bindValue(":n",i->objectName());
        if (!q.exec() || !q.next()) {
            qDebug() << "ALERT: Unable to qeury for state existence of" << i->objectName() << "in" << wintable;
            return false;
        }

        if (!q.value(0).toInt())
            q.prepare(QString("INSERT INTO %1 (name, state) VALUES (:n, :s)").arg(wintable));
        else
            q.prepare(QString("UPDATE %1 SET state = :s WHERE name = :n").arg(wintable));
        q.bindValue(":n",i->objectName());
        q.bindValue(":s",tmp);
        bool ok = q.exec();

        qDebug() << "[db] Object" << i->objectName() << "saved to" << wintable << ":" << ok;
        if (!ok) return false;
    }
    return true;
}

bool DBHelper::readRecentDirs(QMenu* add_to, int maxcount, LoadFileCB cb)
{
    if (!add_to) return false;
    if (!clearRecentDirs()) return false;

    QSqlQuery q;
    if (!q.exec("SELECT lastaccess, path FROM recent ORDER BY lastaccess DESC")) return false;

    for (int n = 0; n < maxcount && q.next(); n++) {
        QString s = q.value(1).toString();
        QAction* a = add_to->addAction(s);
        recents[q.value(0).toUInt()] = a;
        connect(a,&QAction::triggered,this,[cb,s] { cb(s); });
    }

    return true;
}

bool DBHelper::clearRecentDirs(bool total)
{
    for (auto &i : recents)
        if (i.second) {
            delete i.second;
            i.second = nullptr;
        }

    recents.clear();
    if (!total) return true;

    QSqlQuery q;
    return q.exec("DELETE FROM recent");
}

bool DBHelper::addRecentDir(QString const &path, bool dir)
{
    QFileInfo fi(path);
    QString right;
    if (fi.isDir()) {
        QDir dr(path);
        right = dr.canonicalPath();
    } else
        right = dir? fi.canonicalPath() : fi.canonicalFilePath();

    if (right.isEmpty()) return false;
    time_t stamp = time(NULL);

    QSqlQuery q;
    q.prepare("SELECT COUNT(lastaccess) FROM recent WHERE path = :p");
    q.bindValue(":p",right);
    if (!q.exec() || !q.next()) return false;

    if (!q.value(0).toInt())
        q.prepare("INSERT INTO recent (lastaccess, path) VALUES (:tm, :p)");
    else
        q.prepare("UPDATE recent SET lastaccess = :tm WHERE path = :p");
    q.bindValue(":tm",(uint)stamp);
    q.bindValue(":p",right);
    bool ok = q.exec();

    qDebug() << "[db] Saving recent dir (" << right << "):" << ok;
    return ok;
}

QString DBHelper::getMostRecentDir()
{
    QSqlQuery q;
    if (q.exec("SELECT path FROM recent ORDER BY lastaccess DESC LIMIT 1") && q.next())
        return q.value(0).toString();
    return QString();
}

QString DBHelper::getMemorySlot(int n)
{
    QSqlQuery q;
    q.prepare("SELECT file FROM memory WHERE slot = :n");
    q.bindValue(":n",n);
    if (q.exec() && q.next()) return q.value(0).toString();
    return QString();
}

bool DBHelper::updateMemorySlot(int n, QString const &fn)
{
    QSqlQuery q;
    q.prepare("SELECT COUNT(file) FROM memory WHERE slot = :n");
    q.bindValue(":n",n);
    if (!q.exec() || !q.next()) return false;

    if (!q.value(0).toInt())
        q.prepare("INSERT INTO memory (slot, file) VALUES (:s, :fn)");
    else
        q.prepare("UPDATE memory SET file = :fn WHERE slot = :s");
    q.bindValue(":s",n);
    q.bindValue(":fn",fn);
    bool ok = q.exec();

    qDebug() << "[db] Updating memory slot" << n << ":" << ok;
    return ok;
}

bool DBHelper::eraseMemorySlot(int n)
{
    QSqlQuery q;
    q.prepare("DELETE FROM memory WHERE slot = :n");
    q.bindValue(":n",n);
    bool ok = q.exec();

    qDebug() << "[db] Erasing memory slot " << n << ": " << ok;
    return ok;
}

bool DBHelper::eraseMemory()
{
    QSqlQuery q;
    return q.exec("DELETE FROM memory");
}

bool DBHelper::updateDirPath(const QString &path, const QString &last)
{
    QSqlQuery q;
    q.prepare("SELECT COUNT(last) FROM dirs WHERE path = :p");
    q.bindValue(":p",path);
    if (!q.exec() || !q.next()) return false;

    if (!q.value(0).toInt())
        q.prepare("INSERT INTO dirs (path, last) VALUES (:p, :l)");
    else
        q.prepare("UPDATE dirs SET last = :l WHERE path = :p");
    q.bindValue(":p",path);
    q.bindValue(":l",last);
    bool ok = q.exec();

    qDebug() << "[db] Updating user directory " << path << ":" << ok;
    return ok;
}

bool DBHelper::delDirPath(const QString &path)
{
    QSqlQuery q;
    q.prepare("DELETE FROM dirs WHERE path = :p");
    q.bindValue(":p",path);
    bool ok = q.exec();

    qDebug() << "[db] Erasing user directory " << path << ": " << ok;
    return ok;
}

bool DBHelper::eraseDirs()
{
    QSqlQuery q;
    return q.exec("DELETE FROM dirs");
}

QStringList DBHelper::getDirsList()
{
    QStringList res;
    QSqlQuery q;
    if (!q.exec("SELECT path FROM dirs")) return res;

    while (q.next()) res.push_back(q.value(0).toString());
    return res;
}

QString DBHelper::getDirLastFile(const QString &path)
{
    QSqlQuery q;
    q.prepare("SELECT last FROM dirs WHERE path = :p");
    q.bindValue(":p",path);
    if (q.exec() && q.next()) return q.value(0).toString();
    return QString();
}

QStringList DBHelper::getStoriesList()
{
    QStringList res;
    QSqlQuery q;
    if (!q.exec("SELECT title FROM stories")) return res;

    while (q.next()) res.push_back(q.value(0).toString());
    return res;
}

bool DBHelper::updateStory(QString const &title, MImageOps* macro, uint stamp)
{
    if (!macro) return false;
    return updateStory(title,macro->serialize(),stamp);
}

bool DBHelper::updateStory(QString const &title, QString const &macro, uint stamp)
{
    if (title.isEmpty()) return false;

    QSqlQuery q;
    q.prepare("SELECT COUNT(title) FROM stories WHERE title = :t");
    q.bindValue(":t",title);
    if (!q.exec() || !q.next()) return false;

    if (!q.value(0).toInt())
        q.prepare("INSERT INTO stories (updated, title, actions) VALUES (:tm, :tit, :act)");
    else
        q.prepare("UPDATE stories SET updated = :tm, actions = :act WHERE title = :tit");
    q.bindValue(":tm",stamp? stamp : (uint)time(NULL));
    q.bindValue(":tit",title);
    q.bindValue(":act",macro);
    bool ok = q.exec();

    qDebug() << "[db] Updating story" << title << ":" << ok;
    return ok;
}

bool DBHelper::loadStory(QString const &title, MImageOps* macro)
{
    if (title.isEmpty() || !macro) return false;

    QSqlQuery q;
    q.prepare("SELECT actions FROM stories WHERE title = :t");
    q.bindValue(":t",title);
    if (!q.exec() || !q.next()) return false;

    return macro->deserialize(q.value(0).toString());
}

QString DBHelper::getExtraStringVal(QString const &key)
{
    QSqlQuery q;
    q.prepare("SELECT val FROM extras WHERE key = :k");
    q.bindValue(":k",key);
    if (q.exec() && q.next()) return q.value(0).toString();
    return QString();
}

int DBHelper::getExtraInt(QString const &key)
{
    QSqlQuery q;
    q.prepare("SELECT bin FROM extras WHERE key = :k");
    q.bindValue(":k",key);
    if (!q.exec() || !q.next()) return 0;
    QByteArray ba = q.value(0).toByteArray();
    if (ba.length() != sizeof(int)) return 0;
    int* val = reinterpret_cast<int*>(ba.data());
    return *val;
}

bool DBHelper::setExtraStringVal(QString const &key, QString const &val)
{
    QSqlQuery q;
    q.prepare("SELECT COUNT(key) FROM extras WHERE key = :k");
    q.bindValue(":k",key);
    if (!q.exec() || !q.next()) return false;

    QByteArray empty;
    if (!q.value(0).toInt())
        q.prepare("INSERT INTO extras (key, val, bin) VALUES (:k, :v, :b)");
    else
        q.prepare("UPDATE extras SET val = :v WHERE key = :k");
    q.bindValue(":k",key);
    q.bindValue(":v",val);
    q.bindValue(":b",empty);
    bool ok = q.exec();

    qDebug() << "[db] Updating key/val" << key << "/" << val << ":" << ok;
    return ok;
}

bool DBHelper::setExtraInt(QString const &key, int val)
{
    QSqlQuery q;
    q.prepare("SELECT COUNT(key) FROM extras WHERE key = :k");
    q.bindValue(":k",key);
    if (!q.exec() || !q.next()) return false;

    QString empty;
    QByteArray buf(sizeof(int),0);
    int* ptr = reinterpret_cast<int*>(buf.data());
    *ptr = val;

    if (!q.value(0).toInt())
        q.prepare("INSERT INTO extras (key, val, bin) VALUES (:k, :v, :b)");
    else
        q.prepare("UPDATE extras SET bin = :b WHERE key = :k");
    q.bindValue(":k",key);
    q.bindValue(":v",empty);
    q.bindValue(":b",buf);
    bool ok = q.exec();

    qDebug() << "[db] Updating key/bin" << key << "/" << val << ":" << ok;
    return ok;
}

QString DBHelper::getDBInfoString()
{
    QSqlQuery q;
    QFileInfo fi(QDir::homePath() + DB_FILEPATH);
    if (!q.exec(DBF_GET_METAINFO) || !q.next()) return QString();

    qint64 sz = fi.size() / 1024;
    char prf = 'K';
    if (sz > 1024) {
        sz /= 1024;
        prf = 'M';
    }
    if (sz > 1024) {
        sz /= 1024;
        prf = 'G';
    }

    unsigned stat = q.value(0).toUInt();
    q.next();
    unsigned links = q.value(0).toUInt();

    return QString::asprintf("DB: %u stat records; %u links; %lli %ciB",stat,links,sz,prf);
}

void DBHelper::invalidateCache()
{
    cache.reset(new DBCache(nullptr));
    m_cache = cache;
    qDebug() << "[db] Cache invalidated";
}
