#include "dbhelper.h"

static int instance = 0;

DBHelper::DBHelper()
{
    instance++;
}

DBHelper::~DBHelper()
{
    if (--instance == 0) {
        QSqlDatabase::database().close();
        qDebug() << "[db] Database closed";
    }
}

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

QStringList DBHelper::getLinkedImages(QByteArray const &sha, bool reverse)
{
    QStringList out;
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

    return out;
}

QStringList DBHelper::tagSearch(MTagCache const &cache, QList<MImageListRecord>* within, int maxitems)
{
    QSqlQuery q;
    std::map<QString,QList<int>> targ;
    QList<int> goal,exclude;
    QStringList found;

    for (auto &i : cache) {
        if (i.second.second == Qt::Unchecked) continue;

        q.clear();
        if (i.second.second == Qt::Checked) {
            goal.push_back(i.second.first);
            q.prepare("SELECT file,tags FROM stats WHERE tags LIKE :t OR INSTR( tags, :i ) > 0");

        } else {
            exclude.push_back(i.second.first);
            q.prepare("SELECT file,tags FROM stats WHERE LENGTH( tags ) > 0 AND tags NOT LIKE :t AND INSTR( tags, :i ) <= 0");
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
            targ[q.value(0).toString()] = l;
        }
    }

    qDebug() << "Target list size: " << targ.size();

    for (auto &i : targ) {
        bool k = true;
        //inclusions
        for (auto &j : goal)
            if (!i.second.contains(j)) {
                k = false;
                break;
            }
        if (!k) continue;
        //exclusions
        for (auto &j : exclude)
            if (i.second.contains(j)) {
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

        found.push_back(i.first);
        if (maxitems > 0 && found.size() >= maxitems) break;
    }
    qDebug() << "Found: " << found.size();

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
            if (q.value(2).toUInt() < flt.minviews || q.value(2).toUInt() >= flt.maxviews) continue;
            if (q.value(3).toInt() < flt.minface || q.value(3).toInt() >= flt.maxface) continue;
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

QStringList DBHelper::getAllFiles()
{
    QSqlQuery q;
    QStringList out;

    if (!q.exec("SELECT file FROM stats")) return out;
    while (q.next()) out.push_back(q.value(0).toString());

    return out;
}

bool DBHelper::removeFile(QString const &fn)
{
    QSqlQuery q;

    //remove it from stats table...
    q.prepare("DELETE FROM stats WHERE file = :fn");
    q.bindValue(":fn",fn);
    bool ok1 = q.exec();
    qDebug() << "[db] Removing file " << fn << " from stats: " << ok1;

    //...and from thumbnails table too
    q.clear();
    q.prepare("DELETE FROM thumbs WHERE file = :fn");
    q.bindValue(":fn",fn);
    bool ok2 = q.exec();
    qDebug() << "[db] Removing file " << fn << " from thumbs: " << ok2;

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

void DBHelper::sanitizeTags(ProgressCB progress_cb)
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

QByteArray DBHelper::getWindowGeometryOrState(bool geom)
{
    QSqlQuery q;
    if (geom)
        q.exec("SELECT geometry FROM window WHERE name = 'MainWindow'");
    else
        q.exec("SELECT state FROM window WHERE name = 'MainWindow'");
    if (!q.exec() || !q.next()) return QByteArray();
    return q.value(0).toByteArray();
}

bool DBHelper::updateWindowGeometryAndState(QByteArray const &geom, QByteArray const &state)
{
    QSqlQuery q;
    q.exec("SELECT COUNT(geometry) FROM window WHERE name = 'MainWindow'");
    if (!q.exec() || !q.next()) return false;

    if (!q.value(0).toInt())
        q.prepare("INSERT INTO window (name, geometry, state) VALUES ('MainWindow', :g, :s)");
    else
        q.prepare("UPDATE window SET geometry = :g, state = :s WHERE name = 'MainWindow'");
    q.bindValue(":g",geom);
    q.bindValue(":s",state);

    bool ok = q.exec();
    qDebug() << "[db] Saving window geometry and state: " << ok;
    return ok;
}

bool DBHelper::restoreSplittersState(QObjectList const &lst)
{
    for (auto &i : lst) {
        if (!restoreSplittersState(i->children())) return false;

        if (QString(i->metaObject()->className()) != "QSplitter") continue;
        QSplitter* ptr = dynamic_cast<QSplitter*>(i);
        if (!ptr) continue;

        QSqlQuery q;
        q.prepare("SELECT state FROM window WHERE name = :n");
        q.bindValue(":n",ptr->objectName());
        if (!q.exec() || !q.next()) {
            qDebug() << "[db] Failed to get state of" << ptr->objectName();
            return false;
        }

        ptr->restoreState(q.value(0).toByteArray());
    }
    return true;
}

bool DBHelper::updateSplittersState(QObjectList const &lst)
{
    for (auto &i : lst) {
        if (!updateSplittersState(i->children())) return false;

        if (QString(i->metaObject()->className()) != "QSplitter") continue;
        QSplitter* ptr = dynamic_cast<QSplitter*>(i);
        if (!ptr) continue;

        QSqlQuery q;
        q.prepare("SELECT COUNT(state) FROM window WHERE name = :n");
        q.bindValue(":n",ptr->objectName());
        if (!q.exec() || !q.next()) {
            qDebug() << "ALERT: Unable to qeury for state existence of" << ptr->objectName();
            return false;
        }

        if (!q.value(0).toInt())
            q.prepare("INSERT INTO window (name, state) VALUES (:n, :s)");
        else
            q.prepare("UPDATE window SET state = :s WHERE name = :n");
        q.bindValue(":n",ptr->objectName());
        q.bindValue(":s",ptr->saveState());
        bool ok = q.exec();

        qDebug() << "[db] Splitter" << ptr->objectName() << "saved:" << ok;
        if (!ok) return false;
    }
    return true;
}
