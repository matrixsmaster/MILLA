#ifndef DB_FORMAT_H
#define DB_FORMAT_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include "shared.h"

#ifdef QT_DEBUG
#define DB_FILEPATH MILLA_CONFIG_PATH "storage_debug.db"
#else
#define DB_FILEPATH MILLA_CONFIG_PATH "storage.db"
#endif

#define DB_VERSION 5

#define DB_VER_SUPPORTED { 3, 4 }

#define DB_CONTENTS {   {"stats",   DBF_STATS}, \
                        {"tags",    DBF_TAGS}, \
                        {"links",   DBF_LINKS}, \
                        {"thumbs",  DBF_THUMBS}, \
                        {"memory",  DBF_MEMORY}, \
                        {"dirs",    DBF_DIRS}, \
                        {"stories", DBF_STORIES}, \
                        {"recent",  DBF_RECENT}, \
                        {"extras",  DBF_EXTRAS}, \
                        {"window",  DBF_WINDOW}, \
                        {"layout1", DBF_WINDOW}, \
                        {"layout2", DBF_WINDOW}, \
                        {"layout3", DBF_WINDOW}, }

#define DBF_TABLE_CHECK "SELECT name FROM sqlite_master WHERE type='table' AND name="

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

#define DBF_MEMORY "slot INT, file TEXT"

#define DBF_DIRS "path TEXT, last TEXT"

#define DBF_STORIES "updated UNSIGNED INT, title TEXT, actions TEXT"

#define DBF_RECENT "lastaccess UNSIGNED INT, path TEXT"

#define DBF_EXTRAS "key TEXT, val TEXT, bin BLOB"

#define DBF_WINDOW "name TEXT, geometry BLOB, state BLOB"

#define DB_CORRECT_TAG_KEY_GET "SELECT key FROM tags ORDER BY key DESC LIMIT 1"

#define DBF_LINKS "created UNSIGNED INT, left BLOB, right BLOB"

#define DBF_EXPORT_RECORD "SELECT views, rating, likes, tags, notes, sha256, length FROM stats WHERE "

#define DBF_IMPORT_SELECT "SELECT file, views, rating, likes, tags, notes, sha256, length FROM stats WHERE "

#define DBF_IMPORT_UPDATE "UPDATE stats SET file = :fn, views = :v, rating = :r, likes = :l, tags = :t, notes = :n, sha256 = :s, length = :len WHERE "

#define DBF_EXTRA_EXTERNAL_EDITOR "external_editor"

#define DBF_EXTRA_EXCLUSION_LIST "global_exclusion"

#define DBF_GET_METAINFO "SELECT COUNT(file) FROM stats UNION ALL SELECT COUNT(created) FROM links"

#endif // DB_FORMAT_H

