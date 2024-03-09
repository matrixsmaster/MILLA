#ifndef ANNAPLUGIN_H
#define ANNAPLUGIN_H

#include "plugins.h"

class AnnaPlugin : public QObject, public MillaGenericPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID MILLA_PLUGIN_LID FILE "anna_plugin.json")
    Q_INTERFACES(MillaGenericPlugin)

public:
    AnnaPlugin();
    virtual ~AnnaPlugin()   {}

    QString getPluginName() { return "ANNA"; }
    QString getPluginDesc() { return "Integration plugin for ANNA project. Run LLMs and VLMs inside MILLA."; }

    bool isFilter()         { return true; }
    bool isContinous()      { return false; }
    bool isFileFormat()     { return false; }
    bool isContainer()      { return false; }

    bool init();
    bool finalize();

    void showUI();
    void setConfigCB(PlugConfCB cb)     {}
    void setProgressCB(ProgressCB cb)   {}

    QVariant getParam(QString key);
    bool setParam(QString key, QVariant val);

    QVariant action(QVariant in);

};

#endif // ANNAPLUGIN_H
