#ifndef MIMPEXPMODULE_H
#define MIMPEXPMODULE_H

#include <QDebug>
#include <QTextStream>
#include <QSqlQuery>
#include "db_format.h"
#include "shared.h"
#include "mimagelistmodel.h"
#include "exportform.h"

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

    void checkBalance(QStringList &l, QChar toBalance);

    bool exportStats(ExportFormData const &s, QTextStream &f);
    bool exportTags(ExportFormData const &s, QTextStream &f);
    bool exportStories(ExportFormData const &s, QTextStream &f);
    bool exportLinks(ExportFormData const &s, QTextStream &f);
};

#endif // MIMPEXPMODULE_H
