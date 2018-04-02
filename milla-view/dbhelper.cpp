#include "dbhelper.h"

bool DBHelper::initDatabase()
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
        QSqlQuery qq;
        ok = qq.exec("CREATE TABLE meta (" DBF_META ")");
        qDebug() << "[db] Create new meta table: " << ok;
        qq.clear();
        qq.prepare("INSERT INTO meta (version) VALUES (:ver)");
        qq.bindValue(":ver",DB_VERSION);
        ok = qq.exec();
        qDebug() << "[db] Inserting current DB version tag " << DB_VERSION << ": " << ok;
        if (!ok) return false;
    } else {
        qDebug() << "[db] version: " << q.value(0).toInt();
        if (DB_VERSION != q.value(0).toInt()) {
            qDebug() << "FATAL: DB version mismatch";
            return false;
        }
    }

    q.clear();
    ok = DB_CORRECT_TABLE_CHECK(q,"'stats'");
    qDebug() << "[db] Read stats table: " << ok;
    if (!ok) {
        QSqlQuery qq;
        ok = qq.exec("CREATE TABLE stats (" DBF_STATS ")");
        qDebug() << "[db] Create new stats table: " << ok;
        if (!ok) return false;
    }

    q.clear();
    ok = DB_CORRECT_TABLE_CHECK(q,"'tags'");
    qDebug() << "[db] Read tags table: " << ok;
    if (!ok) {
        QSqlQuery qq;
        ok = qq.exec("CREATE TABLE tags (" DBF_TAGS ")");
        qDebug() << "[db] Create new tags table: " << ok;
        if (!ok) return false;
    }

    q.clear();
    ok = DB_CORRECT_TABLE_CHECK(q,"'links'");
    qDebug() << "[db] Read links table: " << ok;
    if (!ok) {
        QSqlQuery qq;
        ok = qq.exec("CREATE TABLE links (" DBF_LINKS ")");
        qDebug() << "[db] Create new links table: " << ok;
        if (!ok) return false;
    }

    q.clear();
    ok = DB_CORRECT_TABLE_CHECK(q,"'thumbs'");
    qDebug() << "[db] Read thumbs table: " << ok;
    if (!ok) {
        QSqlQuery qq;
        ok = qq.exec("CREATE TABLE thumbs (" DBF_THUMBS ")");
        qDebug() << "[db] Create new thumbs table: " << ok;
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

QByteArray DBHelper::getSHA256(QString const &fn, qint64* size)
{
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
        rec.filechanged = q.value(1).toUInt();
        rec.modified = false;
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
    if (ok) rec.modified = false;
    else qDebug() << q.lastError();

    return ok;
}

bool DBHelper::isStatRecordExists(QString const &fn)
{
    QSqlQuery q;
    q.prepare("SELECT views FROM stats WHERE file = (:fn)");
    q.bindValue(":fn",fn);
    return (q.exec() && q.next());
}

bool DBHelper::updateStatRecord(QString const &fn, MImageExtras &rec, bool update)
{
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
    QSqlQuery q;
    q.prepare("SELECT sizex, sizey, grayscale, faces, facerects, hist, sha256, length FROM stats WHERE file = (:fn)");
    q.bindValue(":fn",fn);;
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

bool DBHelper::insertTag(QString const &ntag, unsigned &key)
{
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

bool DBHelper::updateTags(QString const &tag, bool checked)
{
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
    QSqlQuery q;
    q.prepare("SELECT notes FROM stats WHERE file = :fn");
    q.bindValue(":fn",fn);
    if (q.exec() && q.next()) return q.value(0).toString();
    else return QString();
}

bool DBHelper::updateFileNotes(QString const &fn, QString &notes)
{
    QSqlQuery q;
    q.prepare("UPDATE stats SET notes = :nt WHERE file = :fn");
    q.bindValue(":nt",notes);
    q.bindValue(":fn",fn);
    bool ok = q.exec();
    qDebug() << "[db] Setting user notes: " << ok;
    return ok;
}

bool DBHelper::createLinkBetweenImages(QByteArray const &left, QByteArray const &right)
{
    QSqlQuery q;
    q.prepare("SELECT * FROM links WHERE left = :sl AND right = :sr");
    q.bindValue(":sl",left);
    q.bindValue(":sr",right);
    if (q.exec() && q.next()) return false;

    q.clear();
    q.prepare("INSERT INTO links (left, right) VALUES (:sl, :sr)");
    q.bindValue(":sl",left);
    q.bindValue(":sr",right);
    bool ok = q.exec();

    qDebug() << "[db] Inserting link: " << ok;
    return ok;
}

QStringList DBHelper::getLinkedImages(QByteArray const &sha)
{
    QStringList out;
    QSqlQuery q;

    q.prepare("SELECT right FROM links WHERE left = :sha");
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

    return out;
}

QStringList DBHelper::tagSearch(MTagCache const &cache, QList<MImageListRecord>* within)
{
    QSqlQuery q;
    std::map<QString,QList<int>> targ;
    QList<int> goal;
    QStringList found;

    for (auto &i : cache) {
        if (!i.second.second) continue;
        goal.push_back(i.second.first);

        q.clear();
        q.prepare("SELECT file,tags FROM stats WHERE tags LIKE :t OR INSTR( tags, :i ) > 0");
        q.bindValue(":t",QString::asprintf("%d,%%",i.second.first));
        q.bindValue(":i",QString::asprintf(",%d,",i.second.first));
        if (!q.exec()) {
            qDebug() << "Select tag " << i.second.first << " failed";
            continue;
        }
        //qDebug() << q.lastQuery();

        while (q.next()) {
            qDebug() << "TAG " << i.second.first << " FOUND: " << q.value(0).toString();
            QList<int> l;
            QStringList _l = q.value(1).toString().split(",",QString::SkipEmptyParts);
            for (auto &j : _l) l.push_back(j.toInt());
            targ[q.value(0).toString()] = l;
        }
    }

    qDebug() << "Target list size: " << targ.size();

    for (auto &i : targ) {
        bool k = true;
        for (auto &j : goal)
            if (!i.second.contains(j)) {
                k = false;
                break;
            }
        if (!k) continue;

        if (within) {
            auto w = std::find_if(within->begin(),within->end(),[&] (const MImageListRecord& a) { return (a.filename == i.first); });
            if (w == within->end()) {
                qDebug() << "File " << i.first << " isn't among currently loaded ones";
                continue;
            }
        }

        found.push_back(i.first);
    }

    return found;
}

QStringList DBHelper::parametricSearch(SearchFormData flt, QList<MImageListRecord> const &from)
{
    QSqlQuery q;
    size_t area;
    std::multimap<int,QString> tmap;
    std::set<QString> tlst;

    for (auto &i : from) {
        q.clear();
        q.prepare("SELECT rating, likes, views, faces, grayscale, sizex, sizey, ntags, notes FROM stats WHERE file = :fn");
        q.bindValue(":fn",i.filename);
        if (q.exec() && q.next()) {

            if (flt.rating > q.value(0).toInt()) continue;
            if (flt.kudos > q.value(1).toInt()) continue;
            if (q.value(2).toUInt() < flt.minviews || q.value(2).toUInt() > flt.maxviews) continue;
            if (q.value(3).toInt() < flt.minface || q.value(3).toInt() > flt.maxface) continue;
            if (flt.colors > -1 && flt.colors != (q.value(4).toInt()>0)) continue;
            if (i.filechanged < flt.minmtime || i.filechanged > flt.maxmtime) continue;
            if (flt.wo_tags && q.value(7).toInt()) continue;
            if (flt.w_notes && q.value(8).toString().isEmpty()) continue;

            area = q.value(5).toUInt() * q.value(6).toUInt();
            if (area < flt.minsize || area > flt.maxsize) continue;

            switch (flt.sort) {
            case SRFRM_RATING: tmap.insert(std::pair<int,QString>(q.value(0).toInt(), i.filename)); break;
            case SRFRM_KUDOS: tmap.insert(std::pair<int,QString>(q.value(1).toInt(), i.filename)); break;
            case SRFRM_VIEWS: tmap.insert(std::pair<int,QString>(q.value(2).toUInt(), i.filename)); break;
            case SRFRM_DATE: tmap.insert(std::pair<int,QString>(i.filechanged, i.filename)); break;
            case SRFRM_FACES: tmap.insert(std::pair<int,QString>(q.value(3).toInt(), i.filename)); break;
            default: tlst.insert(i.filename);
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

void DBHelper::sanitizeLinks(progressCB progress_cb)
{
    QSqlQuery q,qa;
    QSet<QByteArray> known_shas;
    QList<std::pair<QByteArray,QByteArray>> to_remove;

    if (!q.exec("SELECT COUNT(left) FROM links") || !qa.exec("SELECT left, right FROM links")) {
        qDebug() << "[db] Unable to load links table";
        return;
    }

    double prg = 0, dp = q.next()? 50.f / (double)(q.value(0).toInt()) : 0;

    while (qa.next()) {
        prg += dp;
        if (progress_cb && !progress_cb(prg)) return;

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

void DBHelper::sanitizeTags(progressCB progress_cb)
{
    QSqlQuery q,qa,qw;
    if (!q.exec("SELECT COUNT(tags) FROM stats") || !qa.exec("SELECT tags FROM stats")) {
        qDebug() << "[db] Unable to query stats table for tags listing";
        return;
    }
    if (!qw.exec("SELECT COUNT(key) FROM tags")) {
        qDebug() << "[db] Unable to query tags table";
        return;
    }

    double prg = 0, dp = q.next()? 50.f / (double)(q.value(0).toInt()) : 0;

    std::map<unsigned,int> used_tags;
    while (qa.next()) {
        prg += dp;
        if (progress_cb && !progress_cb(prg)) return;

        if (qa.value(0).toString().isEmpty()) continue;
        QStringList ls = qa.value(0).toString().split(',',QString::SkipEmptyParts);
        if (ls.isEmpty()) continue;

        for (auto &j : ls) {
            unsigned ji = j.toUInt();
            if (used_tags.count(ji)) used_tags[ji]++;
            else used_tags[ji] = 1;
        }
    }

    qDebug() << "[Sanitizer] " << used_tags.size() << " tags currently in use";

    prg = 50;
    dp = qw.next()? 50.f / (double)(qw.value(0).toInt()) : 0;

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
