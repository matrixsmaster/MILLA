#ifndef DB_FORMAT_H
#define DB_FORMAT_H

#include <QtSql/QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

#define DB_FILEPATH "/.milla/storage.db"

#define DB_VERSION 1

#define DBF_TABLE_CHECK "SELECT name FROM sqlite_master WHERE type='table' AND name="

#define DB_CORRECT_TABLE_CHECK(Q,TAB) (Q.exec(DBF_TABLE_CHECK TAB) && Q.next())

#define DBF_META "version INT"

#define DBF_THUMBS "file TEXT, mtime UNSIGNED INT, thumb BLOB"

#define DBF_STATS " file TEXT, \
                    views UNSIGNED BIGINT, \
                    lastview UNSIGNED INT, \
                    rating TINYINT, \
                    likes INT, \
                    ntags INT, \
                    tags TEXT, \
                    notes TEXT, \
                    sizex UNSIGNED INT, \
                    sizey UNSIGNED INT, \
                    grayscale TINYINT, \
                    faces INT, \
                    facerects TEXT, \
                    hist BLOB, \
                    sha256 BLOB, \
                    length UNSIGNED BIGINT"

#define DBF_STATS_SHORT "file, views, lastview, rating, likes, ntags, tags, notes, sizex, sizey, grayscale, faces, facerects, hist, sha256, length"

#define DBF_STATS_INSERT_KEYS "(:fn, 0, :tm, 0, 0, 0, \"\", \"\", :sx, :sy, :gry, :fcn, :fcr, :hst, :sha, :len)"

#define DBF_STATS_UPDATE_SHORT "sizex = :sx, sizey = :sy, grayscale = :gry, faces = :fcn, facerects = :fcr, hist = :hst, sha256 = :sha, length = :len"

#define DBF_TAGS "key UNSIGNED INT, tag TEXT, rating UNSIGNED BIGINT"

#define DBF_TAGS_SHORT "key, tag, rating"

#define DB_CORRECT_TAG_KEY_GET "SELECT key FROM tags ORDER BY key DESC LIMIT 1"

#define DBF_LINKS "left BLOB, right BLOB"

#define DBF_EXPORT_RECORD "SELECT views, rating, likes, tags, notes, sha256, length FROM stats WHERE "

#define DBF_IMPORT_SELECT "SELECT file, views, rating, likes, tags, notes, sha256, length FROM stats WHERE "

#define DBF_IMPORT_UPDATE "UPDATE stats SET file = :fn, views = :v, rating = :r, likes = :l, tags = :t, notes = :n, sha256 = :s, length = :len WHERE "

#endif // DB_FORMAT_H

