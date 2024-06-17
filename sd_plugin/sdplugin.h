#ifndef SDPLUGIN_H
#define SDPLUGIN_H

#include <QObject>
#include "plugins.h"

class SDPlugin : public QObject, public MillaGenericPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID MILLA_PLUGIN_LID FILE "sd_plugin.json")
    Q_INTERFACES(MillaGenericPlugin)

public:
    SDPlugin();
    virtual ~SDPlugin();

    QString getPluginName()  { return "SDPlugin"; }
    QString getPluginDesc()  { return "Stable Diffusion plugin for all your image generation needs."; }

    bool isContinous() const { return false; }

    MillaPluginContentType inputContent() const  { return MILLA_CONTENT_NONE; }
    MillaPluginContentType outputContent() const { return MILLA_CONTENT_IMAGE; }

    bool init();
    bool finalize();

    void showUI();
    void setConfigCB(PlugConfCB)        {}
    void setProgressCB(ProgressCB)      {}

    QVariant getParam(QString key);
    bool setParam(QString key, QVariant val);

    QVariant action(QVariant in);

private:
    bool skip_gen = false;
};

#endif // SDPLUGIN_H
