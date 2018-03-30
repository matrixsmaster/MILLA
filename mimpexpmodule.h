#ifndef MIMPEXPMODULE_H
#define MIMPEXPMODULE_H

#include <QDebug>
#include <QString>
#include <QTextStream>
#include <QSqlQuery>
#include <map>
#include <functional>
#include "db_format.h"
#include "mimagelistmodel.h"
#include "exportform.h"

typedef std::map<QString,std::pair<int,bool>> MTagCache;

//workaround bug with 'unrecognizable' std::function
typedef std::function<void(QString)> myFunVoidQStr;

class MImpExpModule
{
public:
    MImpExpModule(MTagCache* pCache, QList<MImageListRecord>* pList);
    virtual ~MImpExpModule() {}

    QString tagsLineConvert(QString in, bool encode);

    bool dataExport(ExportFormData const &s, QTextStream &f);

    bool dataImport(ExportFormData const &d, QTextStream &f, myFunVoidQStr init_rec_callback);

private:
    MTagCache* foreign_cache;
    QList<MImageListRecord>* foreign_list;
};

#endif // MIMPEXPMODULE_H
