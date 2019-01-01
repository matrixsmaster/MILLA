#include "mimpexpmodule.h"
#include "dbhelper.h"

MImpExpModule::MImpExpModule(MTagCache* pCache, QList<MImageListRecord>* pList) :
    foreign_cache(pCache),
    foreign_list(pList)
{
}

QString MImpExpModule::tagsLineConvert(QString in, bool encode)
{
    if (in.isEmpty() || foreign_cache->empty()) return in;

    QStringList lin = in.split(',',QString::SkipEmptyParts);
    QString out;
    QTextStream x(&out);
    for (auto &i : lin) {
        i = i.trimmed();

        if (encode) {
            if (foreign_cache->count(i)) x << foreign_cache->at(i).first << ",";
            else qDebug() << "ALERT: unable to associate tag " << i;

        } else {
            bool ok;
            unsigned n = i.toUInt(&ok);
            if (ok) {
                auto j = std::find_if(foreign_cache->begin(),foreign_cache->end(),[n] (auto &it) {
                    return (it.second.first == n); });
                if (j != foreign_cache->end())
                    x << j->first << ",";
                else
                    qDebug() << "ALERT: unable to associate tag #" << n;
            } else {
                qDebug() << "ALERT: malformed tags string " << in;
                break;
            }
        }
    }

    x.flush();
    return out;
}

QString MImpExpModule::removeQuotes(QString const &in) const
{
    QString tmp(in);
    if (!tmp.isEmpty() && tmp.at(0) == '\"') tmp = tmp.mid(1,tmp.length()-2);
    return tmp;
}

bool MImpExpModule::exportStats(ExportFormData const &s, QTextStream &f)
{
    QSqlQuery q;
    QSet<QString> a,b,c;
    if (s.loaded_only) {
        if (!foreign_list) return false;
        for (auto &i : *foreign_list) a.insert(i.filename);
    }

    if (!q.exec("SELECT file FROM stats")) return false;
    while (q.next()) b.insert(q.value(0).toString());

    if (s.loaded_only) c = b.intersect(a);
    else c = b;

    if (s.header) {
        if (s.filename) f << "File name" << s.separator;
        if (s.views)    f << "Views count" << s.separator;
        if (s.rating)   f << "Rating" << s.separator;
        if (s.likes)    f << "Kudos" << s.separator;
        if (s.tags)     f << "Tags" << s.separator;
        if (s.tagids)   f << "Tag IDs" << s.separator;
        if (s.notes)    f << "Notes" << s.separator;
        if (s.sha)      f << "SHA-256" << s.separator;
        if (s.length)   f << "File size" << s.separator;
        if (s.separator != '\n') f << '\n';
    }

    setProgress(c.size());

    for (auto &i : c) {
        if (s.filename) f << i << s.separator;

        q.clear();
        q.prepare(DBF_EXPORT_RECORD "file = :fn");
        q.bindValue(":fn",i);
        if (q.exec() && q.next()) {
            if (s.views)    f << q.value(0).toUInt() << s.separator;
            if (s.rating)   f << q.value(1).toInt() << s.separator;
            if (s.likes)    f << q.value(2).toInt() << s.separator;
            if (s.tags)     f << "\"" << tagsLineConvert(q.value(3).toString(),false) << "\"" << s.separator;
            if (s.tagids)   f << "\"" << q.value(3).toString() << "\"" << s.separator;
            if (s.notes)    f << "\"" << q.value(4).toString() << "\"" << s.separator;
            if (s.sha)      f << q.value(5).toByteArray().toHex() << s.separator;
            if (s.length)   f << q.value(6).toUInt() << s.separator;
        }

        if (s.separator != '\n') f << '\n';
        if (!incProgress()) break;
    }

    return true;
}

bool MImpExpModule::exportTags(ExportFormData const &s, QTextStream &f)
{
    QSqlQuery q;
    if (s.header) {
        if (s.tagname) f << "Tag" << s.separator;
        if (s.tagrate) f << "Rating" << s.separator;
        if (s.separator != '\n') f << '\n';
    }

    if (!q.exec("SELECT tag, rating FROM tags")) return false;
    while (q.next()) {
        if (s.tagname) f << q.value(0).toString() << s.separator;
        if (s.tagrate) f << q.value(1).toUInt() << s.separator;
        if (s.separator != '\n') f << '\n';
    }

    return true;
}

bool MImpExpModule::exportStories(ExportFormData const &s, QTextStream &f)
{
    QSqlQuery q;
    if (s.header) {
        if (s.story_update)   f << "Updated" << s.separator;
        if (s.story_title)    f << "Title" << s.separator;
        if (s.story_actions)  f << "Actions" << s.separator;
        if (s.separator != '\n') f << '\n';
    }

    if (!q.exec("SELECT updated, title, actions FROM stories")) return false;
    while (q.next()) {
        if (s.story_update)   f << q.value(0).toUInt() << s.separator;
        if (s.story_title)    f << q.value(1).toString() << s.separator;
        if (s.story_actions)  f << '\"' << q.value(2).toString() << '\"' << s.separator;
        if (s.separator != '\n') f << '\n';
    }

    return true;
}

bool MImpExpModule::exportLinks(ExportFormData const &s, QTextStream &f)
{
    QSqlQuery q;
    if (s.header) {
        if (s.link_created) f << "Created" << s.separator;
        if (s.link_left)    f << "Left_side" << s.separator;
        if (s.link_right)   f << "Right_side" << s.separator;
        if (s.separator != '\n') f << '\n';
    }

    if (!q.exec("SELECT created, left, right FROM links")) return false;
    while (q.next()) {
        if (s.link_created) f << q.value(0).toUInt() << s.separator;
        if (s.link_left) f << q.value(1).toByteArray().toHex() << s.separator;
        if (s.link_right) f << q.value(2).toByteArray().toHex() << s.separator;
        if (s.separator != '\n') f << '\n';
    }

    return true;
}

bool MImpExpModule::dataExport(ExportFormData const &s, QTextStream &f)
{
    switch (s.table) {
    case IMPEXP_STATS:      return exportStats(s,f);
    case IMPEXP_TAGS:       return exportTags(s,f);
    case IMPEXP_STORIES:    return exportStories(s,f);
    case IMPEXP_LINKS:      return exportLinks(s,f);
    default: break;
    }
    return false;
}

bool MImpExpModule::dataImport(ExportFormData const &d, QTextStream &f, InitRecCB init_rec_callback)
{
    QSqlQuery q;
    bool ok;
    DBHelper dbh;

    //check if we able to filter known records
    QSet<QString> filterlist;
    if (d.loaded_only) {
        if (!foreign_list) return false;
        for (auto &i : *foreign_list) filterlist.insert(i.filename);
    }

    //calculate number of fileds we expect in the input file
    int m = 0;
    switch (d.table) {
    case IMPEXP_STATS:      m = d.filename+d.views+d.rating+d.likes+d.tags+d.tagids+d.notes+d.sha+d.length; break;
    case IMPEXP_TAGS:       m = d.tagname+d.tagrate; break;
    case IMPEXP_STORIES:    m = d.story_update+d.story_title+d.story_actions; break;
    case IMPEXP_LINKS:      m = d.link_created+d.link_left+d.link_right; break;
    default: return false;
    }

    //create tagged map of field indices
    std::map<QString,int> tmap;
    tmap["file"] =   0;
    tmap["views"] =  d.filename;
    tmap["rating"] = d.filename+d.views;
    tmap["likes"] =  d.filename+d.views+d.rating;
    tmap["tags"] =   d.filename+d.views+d.rating+d.likes;
    tmap["tagids"] = d.filename+d.views+d.rating+d.likes+d.tags;
    tmap["notes"] =  d.filename+d.views+d.rating+d.likes+d.tags+d.tagids;
    tmap["sha"] =    d.filename+d.views+d.rating+d.likes+d.tags+d.tagids+d.notes;
    tmap["length"] = d.filename+d.views+d.rating+d.likes+d.tags+d.tagids+d.notes+d.sha;

    //prepare data if we're going to load tags
    QStringList otgs;
    std::vector<std::pair<QString,int>> nwtgs;
    if (d.table == IMPEXP_TAGS) {
        //collect all known tags first
        if (q.exec("SELECT tag, rating FROM tags")) {
            while (q.next()) otgs.push_back(q.value(0).toString());
        }
    }

    //prepare for stories
    QStringList ostories;
    if (d.table == IMPEXP_STORIES) ostories = DBHelper::getStoriesList();

    //read file line-by-line
    unsigned counter = 0;
    bool once = false;
    setProgress(getNumOfLines(f));
    for (QString s = f.readLine(); !s.isNull(); s = f.readLine()) {

        if (!incProgress()) break;  //break operation if user had cancelled it

        if (s.isEmpty()) continue;  //skip empty lines

        if (!once) {
            once = true;
            if (d.header) continue; //skip header line
        }

        //since 'notes' & 'actions' fields can contain multiple lines, we need to collect'em all
        if (d.table == IMPEXP_STATS || d.table == IMPEXP_STORIES) {
            QString _tmp(" ");
            while ((s.at(s.length()-1) != d.separator || s.count('\"') % 2) && !_tmp.isNull()) {
                _tmp = f.readLine();
                s += "\n" + _tmp;
                incProgress(); //finish current record even if operation was cancelled
            }
        }

        //now actually extract fields - split the line by separators
        QStringList sl = s.split(d.separator,QString::SkipEmptyParts);
        checkBalance(sl,'\"');
        if (sl.length() != m) {
            qDebug() << "[IMPEXP] ALERT: Import error: fields count doesn't match!";
            return false;
        }

        if (d.table == IMPEXP_STATS) { //importing into stats table
            std::map<QString,QVariant> tgs;

            //try to look into our current DB to get something we already knew
            q.clear();
            if (d.filename) { //we can search by filename
                q.prepare(DBF_IMPORT_SELECT "file = :key");
                q.bindValue(":key",sl.front());

            } else if (d.sha) { //or by SHA-256 sum, if the filename isn't listed in input file
                q.prepare(DBF_IMPORT_SELECT "sha256 = :key");
                q.bindValue(":key",QByteArray::fromHex(sl.at(tmap["sha"]).toLatin1()));
            }

            ok = q.exec() && q.next(); //are you feeling lucky?
            qDebug() << "[IMPEXP] File entry first match: " << ok;

            //are we importing data for currently loaded files only?
            if (ok && d.loaded_only) {
                if (!filterlist.contains(q.value(0).toString())) {
                    qDebug() << "[IMPEXP] A record for " << q.value(0).toString() << " filtered out";
                    continue;
                }
            }

            //second take (what if file IS here, but was moved)
            if (!ok && d.filename && d.sha) {
                q.clear();
                q.prepare(DBF_IMPORT_SELECT "sha256 = :key");
                q.bindValue(":key",QByteArray::fromHex(sl.at(tmap["sha"]).toLatin1()));

                ok = q.exec() && q.next(); //hopefully this way it would be found
                qDebug() << "[IMPEXP] File entry second match: " << ok;
            }

            //let's update temporary fields either by empty data or by actual data from db
            tgs["file"] =   ok? q.value(0).toString()       : QString();
            tgs["views"] =  ok? q.value(1).toUInt()         : uint(0);
            tgs["rating"] = ok? q.value(2).toInt()          : int(0);
            tgs["likes"] =  ok? q.value(3).toInt()          : int(0);
            tgs["tags"] =   ok? q.value(4).toString()       : QString();
            tgs["notes"] =  ok? q.value(5).toString()       : QString();
            tgs["sha"] =    ok? q.value(6).toByteArray()    : QByteArray();
            tgs["length"] = ok? q.value(7).toUInt()         : uint(0);

            //now the UPDATER himself - let's decide how we overwrite each field individually
            if ((!d.imp_noover || tgs["file"].toString().isEmpty()) && d.filename)    tgs["file"] =  sl.front();
            if ((!d.imp_noover || tgs["views"] < 1) && d.views)                       tgs["views"] = sl.at(tmap["views"]).toUInt();
            if ((!d.imp_noover || tgs["rating"] < 1) && d.rating)                     tgs["rating"]= sl.at(tmap["rating"]).toInt();
            if ((!d.imp_noover || tgs["likes"] < 1) && d.likes)                       tgs["likes"] = sl.at(tmap["likes"]).toInt();
            if ((!d.imp_noover || tgs["tags"].toString().isEmpty()) && d.tags)        tgs["tags"] =  tagsLineConvert(removeQuotes(sl.at(tmap["tags"])),true);
            if ((!d.imp_noover || tgs["tags"].toString().isEmpty()) && d.tagids)      tgs["tags"] =  removeQuotes(sl.at(tmap["tagids"]));
            if ((!d.imp_noover || tgs["notes"].toString().isEmpty()) && d.notes)      tgs["notes"] = removeQuotes(sl.at(tmap["notes"]));
            if ((!d.imp_noover || tgs["sha"].toByteArray().isEmpty()) && d.sha)       tgs["sha"] =   QByteArray::fromHex(sl.at(tmap["sha"]).toLatin1());
            if ((!d.imp_noover || tgs["length"] < 1) && d.length)                     tgs["length"]= sl.at(tmap["length"]).toUInt();

            //on this stage, we SHOULD know a file's name. If not - we failed.
            if (tgs["file"].toString().isEmpty()) {
                qDebug() << "[IMPEXP] ALERT: Import data: file record without a name!";
                continue;
            }

            //if current file is completely new to us - let's initialize its own DB record
            if (!ok) {
                if (!init_rec_callback(tgs["file"].toString())) continue; //don't "update" invalid records
            }

            //the last thing to do - send a SQL message to DB
            q.clear();
            q.prepare(DBF_IMPORT_UPDATE "file = :fn");
            q.bindValue(":fn",tgs["file"].toString());
            q.bindValue(":v",tgs["views"].toUInt());
            q.bindValue(":r",tgs["rating"].toInt());
            q.bindValue(":l",tgs["likes"].toInt());
            q.bindValue(":t",tgs["tags"].toString());
            q.bindValue(":n",tgs["notes"].toString());
            q.bindValue(":s",tgs["sha"].toByteArray());
            q.bindValue(":len",tgs["length"].toUInt());
            ok = q.exec();

            qDebug() << "[IMPEXP] Import stat data: " << ok;

        } else if (d.table == IMPEXP_TAGS) { //importing into tags table
            if (!d.tagname) continue; //nothing to import
            if (otgs.contains(sl.front(),Qt::CaseInsensitive)) continue; //tag already known
            nwtgs.push_back(std::pair<QString,int>(sl.front(),(d.tagrate? sl.at(1).toInt():0)));

            qDebug() << "[IMPEXP] Import tag data: OK";

        } else if (d.table == IMPEXP_STORIES) { //importing into stories table
            int ix = d.story_update? 1:0;
            QString stit = d.story_title? sl.at(ix).toLatin1() : QString::asprintf("Story %u",++counter);
            if (ostories.contains(stit) && d.imp_noover) continue; //don't overwrite existing story
            ix = d.story_update? 2:1;
            QString act = removeQuotes(sl.at(ix));
            ok = dbh.updateStory(stit,act,d.story_update? sl.at(0).toUInt():0);

            qDebug() << "[IMPEXP] Import story data: " << ok;

        } else if (d.table == IMPEXP_LINKS) { //importing into links table
            if (!d.link_left || !d.link_right) continue; //nothing to do
            int ix = d.link_created? 1:0;

            QByteArray lft = QByteArray::fromHex(sl.at(ix).toLatin1());
            QByteArray rgh = QByteArray::fromHex(sl.at(++ix).toLatin1());
            ok = dbh.createLinkBetweenImages(lft,rgh,!d.imp_noover,d.link_created? sl.at(0).toUInt():0);

            qDebug() << "[IMPEXP] Import link data: " << ok;
        }
    }

    //if tags table loaded, let's update the DB
    if (d.table == IMPEXP_TAGS) {
        setProgress(nwtgs.size());

        //determine max key
        int mxkey = 0;
        q.clear();
        if (q.exec(DB_CORRECT_TAG_KEY_GET) && q.next()) mxkey = q.value(0).toUInt();
        qDebug() << "[IMPEXP] Max key known: " << mxkey;

        //insert new tags
        for (auto &i : nwtgs) {
            q.clear();
            q.prepare("INSERT INTO tags (" DBF_TAGS_SHORT ") VALUES (:key, :tg, :rt)");
            q.bindValue(":key",++mxkey);
            q.bindValue(":tg",i.first);
            q.bindValue(":rt",i.second);
            if (!q.exec()) break;
            qDebug() << "[IMPEXP] Tag " << i.first << "inserted";
            if (!incProgress()) break;
        }
    }

    return true;
}

int MImpExpModule::getNumOfLines(QTextStream &f)
{
    char c;
    int cnt = 0;

    f.seek(0);
    while (!f.atEnd()) {
        f >> c;
        if (c == '\n') cnt++;
    }
    f.seek(0);

    return cnt;
}

void MImpExpModule::setProgress(int total)
{
    if (pbar_fun) {
        maxprogval = total;
        pbar_fun(0);
    } else
        maxprogval = 1;

    curprogval = 0;
}

bool MImpExpModule::incProgress()
{
    if (!pbar_fun) return true;

    curprogval += 1;
    double v = (curprogval / maxprogval) * 100.f;

    return pbar_fun(v);
}

void MImpExpModule::checkBalance(QStringList &l, QChar toBalance)
{
    for (int i = 0; i < l.count();) {
        if (l.at(i).count(toBalance) % 2) {
            l[i] += l.at(i+1);
            l.removeAt(i+1);
        } else
            i++;
    }
}
