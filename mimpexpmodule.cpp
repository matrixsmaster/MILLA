#include "mimpexpmodule.h"

MImpExpModule::MImpExpModule(MTagCache* pCache, QList<MImageListRecord>* pList) :
    foreign_cache(pCache),
    foreign_list(pList)
{
}

QString MImpExpModule::tagsLineConvert(QString in, bool encode)
{
    if (in.isEmpty() || foreign_cache->empty()) return in;

    if (encode && in.at(0) == '\"') in = in.mid(1,in.length()-2);

    QStringList lin = in.split(',',QString::SkipEmptyParts);
    QString out;
    QTextStream x(&out);
    for (auto &i : lin) {
        if (encode) {
            if (foreign_cache->count(i)) x << foreign_cache->at(i).first << ",";
            else qDebug() << "ALERT: unable to associate tag " << i;

        } else {
            bool ok;
            int n = i.toInt(&ok);
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

bool MImpExpModule::dataExport(ExportFormData const &s, QTextStream &f)
{
    QSqlQuery q;

    if (s.table == 1) {
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
            if (s.views) f << "Views count" << s.separator;
            if (s.rating) f << "Rating" << s.separator;
            if (s.likes) f << "Kudos" << s.separator;
            if (s.tags) f << "Tags" << s.separator;
            if (s.notes) f << "Notes" << s.separator;
            if (s.sha) f << "SHA-256" << s.separator;
            if (s.length) f << "File size" << s.separator;
            if (s.separator != '\n') f << '\n';
        }

        for (auto &i : c) {
            if (s.filename) f << i << s.separator;

            q.clear();
            q.prepare(DBF_EXPORT_RECORD "file = :fn");
            q.bindValue(":fn",i);
            if (q.exec() && q.next()) {
                if (s.views) f << q.value(0).toUInt() << s.separator;
                if (s.rating) f << q.value(1).toInt() << s.separator;
                if (s.likes) f << q.value(2).toInt() << s.separator;
                if (s.tags) f << "\"" << tagsLineConvert(q.value(3).toString(),false) << "\"" << s.separator;
                if (s.notes) f << "\"" << q.value(4).toString() << "\"" << s.separator;
                if (s.sha) f << q.value(5).toByteArray().toHex() << s.separator;
                if (s.length) f << q.value(6).toUInt() << s.separator;
            }

            if (s.separator != '\n') f << '\n';
        }

    } else {
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
    }

    return true;
}

bool MImpExpModule::dataImport(ExportFormData const &d, QTextStream &f, myFunVoidQStr init_rec_callback)
{
    QSqlQuery q;

    int m = (d.table == 1)?
                (d.filename+d.views+d.rating+d.likes+d.tags+d.notes+d.sha+d.length):
                (d.tagname+d.tagrate);

    std::map<QString,int> tmap;
    tmap["file"] =   0;
    tmap["views"] =  d.filename;
    tmap["rating"] = d.filename+d.views;
    tmap["likes"] =  d.filename+d.views+d.rating;
    tmap["tags"] =   d.filename+d.views+d.rating+d.likes;
    tmap["notes"] =  d.filename+d.views+d.rating+d.likes+d.tags;
    tmap["sha"] =    d.filename+d.views+d.rating+d.likes+d.tags+d.notes;
    tmap["length"] = d.filename+d.views+d.rating+d.likes+d.tags+d.notes+d.sha;

    bool once = false;
    for (QString s = f.readLine(); !s.isNull(); s = f.readLine()) {
        if (s.isEmpty()) continue;
        if (!once) {
            once = true;
            if (d.header) continue;
        }

        QString tmp(" ");
        while ((s.at(s.length()-1) != d.separator) && (s.count('\"') % 2) && !tmp.isNull()) {
            tmp = f.readLine();
            s += "\n" + tmp;
        }

        QStringList sl = s.split(d.separator,QString::SkipEmptyParts);
        if (sl.length() != m) {
            qDebug() << "[db] ALERT: Import error: fields count doesn't match!";
            return false;
        }

        if (d.table == 1) {
            std::map<QString,QVariant> tgs;

            q.clear();
            if (d.filename) {
                q.prepare(DBF_IMPORT_SELECT "file = :key");
                q.bindValue(":key",sl.front());

            } else if (d.sha) {
                q.prepare(DBF_IMPORT_SELECT "sha256 = :key");
                q.bindValue(":key",QByteArray::fromHex(sl.at(tmap["sha"]).toLatin1()));
            }

            bool ok = q.exec() && q.next();

            tgs["file"] =   ok? q.value(0).toString()       : QString();
            tgs["views"] =  ok? q.value(1).toUInt()         : uint(0);
            tgs["rating"] = ok? q.value(2).toInt()          : int(0);
            tgs["likes"] =  ok? q.value(3).toInt()          : int(0);
            tgs["tags"] =   ok? q.value(4).toString()       : QString();
            tgs["notes"] =  ok? q.value(5).toString()       : QString();
            tgs["sha"] =    ok? q.value(6).toByteArray()    : QByteArray();
            tgs["length"] = ok? q.value(7).toUInt()         : uint(0);

            if ((!d.imp_noover || tgs["file"].toString().isEmpty()) && d.filename)    tgs["file"] =  sl.front();
            if ((!d.imp_noover || tgs["views"] < 1) && d.views)                       tgs["views"] = sl.at(tmap["views"]).toUInt();
            if ((!d.imp_noover || tgs["rating"] < 1) && d.rating)                     tgs["rating"]= sl.at(tmap["rating"]).toInt();
            if ((!d.imp_noover || tgs["likes"] < 1) && d.likes)                       tgs["likes"] = sl.at(tmap["likes"]).toInt();
            if ((!d.imp_noover || tgs["tags"].toString().isEmpty()) && d.tags)        tgs["tags"] =  tagsLineConvert(sl.at(tmap["tags"]),true);
            if ((!d.imp_noover || tgs["notes"].toString().isEmpty()) && d.notes)      tgs["notes"] = sl.at(tmap["notes"]);
            if ((!d.imp_noover || tgs["sha"].toByteArray().isEmpty()) && d.sha)       tgs["sha"] =   QByteArray::fromHex(sl.at(tmap["sha"]).toLatin1());
            if ((!d.imp_noover || tgs["length"] < 1) && d.length)                     tgs["length"]= sl.at(tmap["length"]).toUInt();

            if (tgs["file"].toString().isEmpty()) {
                qDebug() << "[db] ALERT: Import data: file record without a name!";
                continue;
            }

            if (!ok) init_rec_callback(tgs["file"].toString());

            tmp = tgs["notes"].toString();
            if (!tmp.isEmpty() && tmp.at(0) == '\"') tmp = tmp.mid(1,tmp.length()-2);
            tgs["notes"] = tmp;

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

            qDebug() << "[db] Import data: " << ok;

        } else {
            //
        }
    }
    return true;
}
