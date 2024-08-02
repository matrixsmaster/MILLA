#ifndef SDPLUGIN_H
#define SDPLUGIN_H

#include <QObject>
#include <thread>
#include <mutex>
#include "plugins.h"
#include "stable-diffusion.h"

#define SDPLUGIN_IMGSIZE 512
#define SDPLUGIN_DEF_DELAY 50
#define SDPLUGIN_TILE_SIZE 32

typedef enum {
    SDP_ACT_GEN_ONLY = 0x01,
    SDP_ACT_SCALE_ONLY = 0x02,
    SDP_ACT_GEN_SCALE = 0x03,
} sdplug_action_t;

class SDPlugin : public QObject, public MillaGenericPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID MILLA_PLUGIN_LID FILE "sd_plugin.json")
    Q_INTERFACES(MillaGenericPlugin)

public:
    SDPlugin();
    virtual ~SDPlugin();

    QString getPluginName()  { return "SDPlugin"; }
    QString getPluginDesc()  { return "Stable Diffusion plugin for all your image generation/scaling needs."; }

    bool isContinous();
    MillaPluginContentType inputContent();
    MillaPluginContentType outputContent() { return MILLA_CONTENT_IMAGE; }

    bool init();
    bool finalize();

    void showUI();
    void setConfigCB(PlugConfCB cb);
    void setProgressCB(ProgressCB cb)      { progress_cb = cb; }

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
    bool self_stop = false;

    upscaler_ctx_t* upscaler = nullptr;

    bool dogen = false;
    bool doupsc = false;
    std::string model, vaemodel, cnmodel, prompt, nprompt, esrgan; // TODO: make use of negprompt and add controlnet image input
    float cfg_scale = 1;
    float style_ratio = 1;
    int steps = 2;
    int batch = 1;
    int seed = -1;
    int scale_fac = 4;

    int curout = 0;
    QList<QPixmap> outputs;
    int delay = SDPLUGIN_DEF_DELAY;
    std::mutex out_mutex;

    bool GenerateBatch();
    QPixmap Scaleup(const QImage &in);
    void Cleanup();
};

#endif // SDPLUGIN_H
