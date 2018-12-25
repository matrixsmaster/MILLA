#include <QDebug>
#include "dbcache.h"

DBCache::DBCache(ProgressCB progress_cb)
{
    QSqlQuery q,qa;
    double prg, dp;

    qDebug() << "[DBCache] Cache created";
    if (!(qa.exec("SELECT COUNT(file) FROM stats") && qa.next()) || !q.exec("SELECT file, sha256 FROM stats")) {
        qDebug() << "[DBCache] Unable to load stats table, giving up";
        return;
    }

    prg = 0;
    dp = 100.f / (double)(qa.value(0).toUInt());
    size_t n = 0; //FIXME: debug only
    qDebug() << "[DBCache] Starting cache filling process...";

    while (q.next()) {
        qDebug() << "[DBCache] " << n++; //FIXME: debug only
        prg += dp;
        if (progress_cb && !progress_cb(prg)) break;

        QString fn = q.value(0).toString();
        QString sha = q.value(1).toByteArray().toHex();
        cache_names_forward[fn] = sha;
        cache_names_reverse[sha] = fn;
    }

    qDebug() << "[DBCache] names cache populated, " << cache_names_forward.size()*2 << " entries";

    q.clear();
    if (!q.exec("SELECT left, right FROM links")) {
        qDebug() << "[DBCache] Unable to load links table, giving up";
        return;
    }

    while (q.next()) {
        QString l = q.value(0).toByteArray().toHex();
        QString r = q.value(1).toByteArray().toHex();
        cache_links_forward.insert(l,r);
        cache_links_reverse.insert(r,l);
    }

    qDebug() << "[DBCache] links cache populated, " << cache_links_forward.size()*2 << " entries";
}

DBCache::~DBCache()
{
    qDebug() << "[DBCache] Destroyed";
}

void DBCache::addFile(QString const &fn, QByteArray const &sha)
{
    QString hex = sha.toHex();
    cache_names_forward[fn] = hex;
    cache_names_reverse[hex] = fn;

    qDebug() << "[DBCache] File record added for " << fn << " [" << hex << "]";
}

void DBCache::removeFile(QString const &fn)
{
    auto it = cache_names_forward.find(fn);
    if (it == cache_names_forward.end()) {
        qDebug() << "[DBCache] Unable to remove file " << fn;
        return;
    }

    QString hex = it.value();
    cache_names_forward.erase(it);

    it = cache_names_reverse.find(hex);
    if (it != cache_names_reverse.end())
        cache_names_reverse.erase(it);

    qDebug() << "[DBCache] File record removed for " << fn << " [" << hex << "]";
}

QString DBCache::getFilenameBySHA(QByteArray const &sha)
{
    return cache_names_reverse[sha.toHex()];
}

QByteArray DBCache::getSHAbyFilename(QString const &fn)
{
    return QByteArray::fromHex(cache_names_forward[fn].toLatin1());
}

void DBCache::addLink(QByteArray const &left, QByteArray const &right)
{
    QString l = left.toHex();
    QString r = right.toHex();
    cache_links_forward.insert(l,r);
    cache_links_reverse.insert(r,l);

    qDebug() << "[DBCache] Link record added: " << l << " -> " << r;
}

void DBCache::removeLink(QByteArray const &left, QByteArray const &right)
{
    QString l = left.toHex();
    QString r = right.toHex();
    int n = cache_links_forward.remove(l,r);
    int m = cache_links_reverse.remove(r,l);

    qDebug() << "[DBCache] Link records removed: " << n << ", " << m;
}

QStringList DBCache::getLinkedTo(QByteArray const &sha, bool reverse)
{
    QString hex = sha.toHex();
    if (hex.isEmpty()) return QStringList();

    QStringList h = reverse ? cache_links_reverse.values(hex) : cache_links_forward.values(hex);
    QStringList out;
    for (auto &i : h) {
        auto it = cache_names_reverse.find(i);
        if (it != cache_names_reverse.end())
            out.push_back(*it);
    }

    return out;
}

QStringList DBCache::getLinkedTo(QString const &fn, bool reverse)
{
    return getLinkedTo(QByteArray::fromHex(cache_names_forward[fn].toLatin1()),reverse);
}
