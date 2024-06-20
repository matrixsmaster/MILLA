#ifndef SDPLUGIN_H
#define SDPLUGIN_H

#include <QObject>
#include "plugins.h"

#define SDPLUGIN_IMGSIZE 512

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
    void setConfigCB(PlugConfCB cb);
    void setProgressCB(ProgressCB cb)            { progress_cb = cb; }

    QVariant getParam(QString key);
    bool setParam(QString key, QVariant val);

    QVariant action(QVariant in);

    bool progress(double val);

protected:
    bool eventFilter(QObject *obj, QEvent *event);

private:
    PlugConfCB config_cb = nullptr;
    ProgressCB progress_cb = nullptr;
    bool load_once = false;
    bool skip_gen = false;
    std::string model, vaemodel, cnmodel, prompt, nprompt; // TODO: make use of negprompt and add controlnet image input
    float cfg_scale = 1;
    float style_ratio = 1;
    int steps = 2;
    int batch = 1;
    int seed = -1;
    int curout = 0;
    QList<QPixmap> outputs;

    bool GenerateBatch();
};

#endif // SDPLUGIN_H
