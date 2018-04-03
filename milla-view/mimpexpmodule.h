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

typedef std::map<QString,std::pair<unsigned,Qt::CheckState>> MTagCache;

//workaround bug with 'unrecognizable' std::function
typedef std::function<bool(QString)> InitRecCB;
typedef std::function<bool(double)> ProgressCB;

class MImpExpModule
{
public:
    MImpExpModule(MTagCache* pCache, QList<MImageListRecord>* pList);
    virtual ~MImpExpModule() {}

    QString tagsLineConvert(QString in, bool encode);

    QString removeQuotes(QString const &in) const;

    bool dataExport(ExportFormData const &s, QTextStream &f);

    bool dataImport(ExportFormData const &d, QTextStream &f, InitRecCB init_rec_callback);

    void setProgressBar(ProgressCB fun) { pbar_fun = fun; }

private:
    MTagCache* foreign_cache;
    QList<MImageListRecord>* foreign_list;
    ProgressCB pbar_fun = 0;
    double maxprogval, curprogval;

    int getNumOfLines(QTextStream &f);
    void setProgress(int total);
    bool incProgress();
};

#endif // MIMPEXPMODULE_H
