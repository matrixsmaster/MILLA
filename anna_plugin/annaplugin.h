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
    virtual ~AnnaPlugin();

    QString getPluginName()  { return "ANNA"; }
    QString getPluginDesc()  { return "Integration plugin for ANNA project. Run LLMs and VLMs inside MILLA."; }

    bool isContinous() const                      { return false; }

    MillaPluginContentType inputContent() const   { return cin; }
    MillaPluginContentType outputContent() const  { return cout; }

    bool init();
    bool finalize();

    void showUI();
    void setConfigCB(PlugConfCB cb)               {}
    void setProgressCB(ProgressCB cb)             { progress_cb = cb; }

    QVariant getParam(QString key);
    bool setParam(QString key, QVariant val);

    QVariant action(QVariant in);

private:
    MillaPluginContentType cin = MILLA_CONTENT_IMAGE;
    MillaPluginContentType cout = MILLA_CONTENT_TEXT_NOTES;
    ProgressCB progress_cb = nullptr;
};

#endif // ANNAPLUGIN_H
