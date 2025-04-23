#ifndef SDPLUGIN_H
#define SDPLUGIN_H

#include <QObject>
#include <thread>
#include <mutex>
#include <future>
#include "plugins.h"
#include "sdcfgdialog.h"
#include "stable-diffusion.h"

#define SDPLUGIN_IMGSIZE 512
#define SDPLUGIN_DEF_DELAY 50
#define SDPLUGIN_TILE_SIZE 32
#define SDPLUGIN_ASAVE_START 1
#define SDPLUGIN_ASAVE_MAX 500000
#define SDPLUGIN_ASAVE_REGEX "([A-Za-z_-+.]*)([#]+)"
#define SDPLUGIN_UI_UPDATE 50ms

typedef enum {
    SDP_ACT_GEN_ONLY = 0x01,
    SDP_ACT_SCALE_ONLY = 0x02,
    SDP_ACT_GEN_SCALE = 0x03,
} sdplug_action_t;

typedef enum {
    SDP_ASAV_NONE = 0,
    SDP_ASAV_ALL,
    SDP_ASAV_USER
} sdplug_autosave_t;

struct SDOutputRec {
    QPixmap img;
    bool saved;
};

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
    bool isPresettable()     { return true; }

    MillaPluginContentType inputContent();
    MillaPluginContentType outputContent() { return MILLA_CONTENT_IMAGE; }

    bool init();
    bool finalize();

    bool showUI(QDialog* dock);
    void setConfigCB(PlugConfCB cb)        { config_cb = cb; }
    void setProgressCB(ProgressCB cb)      { progress_cb = cb; }

    QVariant getParam(QString key);
    bool setParam(QString key, QVariant val);

    QVariant action(QVariant in);

    bool progress(double val);
    void dockCallback(QString preset, int mode);
    void imageCallback(sd_image_t* img);

protected:
    bool eventFilter(QObject *obj, QEvent *event);

private:
    PlugConfCB config_cb = nullptr;
    ProgressCB progress_cb = nullptr;
    SDCfgDialog* dialog = nullptr;
    sd_ctx_t* sdcontext = nullptr;

    bool load_once = false;
    bool skip_gen = false; //FIXME: do we still need it, or the new abort-by-UI-Cancel is enough
    bool self_stop = false;

    upscaler_ctx_t* upscaler = nullptr;

    bool dogen = false;
    bool doupsc = false;
    bool useleft = false;
    bool realtime = false;
    std::string model, vaemodel, cnmodel, clipmodel, t5model, loradir, esrgan; // TODO: add controlnet image input
    std::string prompt, nprompt;
    float cfg_scale = 1;
    float style_ratio = 0;
    float guidance = 3.5;
    float strength = 0.5;
    int steps = 2;
    int batch = 1;
    int seed = 0;
    int scale_fac = 4;
    int sampler = EULER_A;
    sdplug_autosave_t autosave = SDP_ASAV_NONE;
    bool asav_addb = false;
    bool asav_match = false;
    bool asav_addtag = false;
    bool asav_addnote = false;
    QString asav_dir, asav_fmt, asav_pat, asav_tags, asav_notes;

    int curout = 0;
    QList<SDOutputRec> outputs;
    int delay = SDPLUGIN_DEF_DELAY;
    std::mutex out_mutex;
    QImage inp_img_convert;
    std::future<sd_image_t*> split_exec;

    bool LoadConfig(QString preset);
    bool SaveConfig(QString preset);
    void getConfigUI();
    void setConfigUI();

    bool RunStop(bool start);
    bool GenerateBatch(const QImage &in);
    void AddImage(sd_image_t* in, bool scale);
    QPixmap Scaleup(const QImage &in);
    void Cleanup();
    void AutosaveImage(SDOutputRec &rec);
    QString ScanNextImageFn();
    QString TextualizeConfig();
};

#endif // SDPLUGIN_H
