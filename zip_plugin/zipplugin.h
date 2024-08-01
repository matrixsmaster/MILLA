#ifndef ZIPPLUGIN_H
#define ZIPPLUGIN_H

#include <QObject>
#include <QFileInfo>
#include "plugins.h"

class ZipPlugin : public QObject, public MillaGenericPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID MILLA_PLUGIN_LID FILE "zipplugin.json")
    Q_INTERFACES(MillaGenericPlugin)

public:
    ZipPlugin(QObject *parent = 0);
    virtual ~ZipPlugin();

    QString getPluginName() { return "ZipReader"; }
    QString getPluginDesc() { return "A ZIP files indexing plugin."; }
#error "Update plugin interface!"
    bool isFilter()         { return false; }
    bool isContinous()      { return false; }
    bool isFileFormat()     { return true; }
    bool isContainer()      { return true; }

    bool init();
    bool finalize();

    void showUI();
    void setConfigCB(PlugConfCB)        {}
    void setProgressCB(ProgressCB)      {}

    QVariant getParam(QString key);
    bool setParam(QString key, QVariant val);

    QVariant action(QVariant in);

private:
    QFileInfo zipfile;
};

#endif // ZIPPLUGIN_H
