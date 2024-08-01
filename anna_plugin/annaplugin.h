#ifndef ANNAPLUGIN_H
#define ANNAPLUGIN_H

#include "plugins.h"
#include "brain.h"
//#include "netclient.h"

#define AP_DEFAULT_CONTEXT 4096
#define AP_DEFAULT_BATCH 512
#define AP_DEFAULT_TEMP 0
#define AP_IMAGE_TEMP "/tmp/milla_anna.png"
#define AP_WAITCB_PERIOD 100ms
#define AP_WAITCB_INC 10

struct AnnaPluginExtra {
    std::string vision_file, ai_prefix, usr_prefix;
};

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

    bool isContinous()                            { return false; }

    MillaPluginContentType inputContent()         { return cin; }
    MillaPluginContentType outputContent()        { return cout; }

    bool init();
    bool finalize();

    void showUI();
    void setConfigCB(PlugConfCB cb);
    void setProgressCB(ProgressCB cb)             { progress_cb = cb; }

    QVariant getParam(QString key);
    bool setParam(QString key, QVariant val);

    QVariant action(QVariant in);

private:
    MillaPluginContentType cin = MILLA_CONTENT_IMAGE;
    MillaPluginContentType cout = MILLA_CONTENT_TEXT_NOTES;
    PlugConfCB config_cb = nullptr;
    ProgressCB progress_cb = nullptr;
    AnnaBrain* brain = nullptr;
    AnnaConfig config;
    AnnaPluginExtra cfg_extra;

    void DefaultConfig();
    bool Generate(bool no_sample);
};

#endif // ANNAPLUGIN_H
