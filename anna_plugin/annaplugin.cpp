#include <QDebug>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <functional>
#include <future>
#include <chrono>
#include "annaplugin.h"
#include "annacfgdialog.h"
#include "plugindock.h"

using namespace std;

#define CONFIG_LOAD_CHARSTR(V) do { \
    string tmp; \
    if (doc.object().contains("" TOSTRING(V) "")) tmp = doc.object().value("" TOSTRING(V) "").toString().toStdString(); \
    if (!tmp.empty()) strncpy(V,tmp.c_str(),sizeof(V)-1); \
} while (0)

AnnaPlugin::AnnaPlugin() :
    QObject(),
    MillaGenericPlugin()
{
    qDebug() << "[ANNA] Plugin instance created";
    DefaultConfig();
}

AnnaPlugin::~AnnaPlugin()
{
    qDebug() << "[ANNA] Plugin destroyed";
}

bool AnnaPlugin::init()
{
    qDebug() << "[ANNA] Init OK";
    return true;
}

bool AnnaPlugin::finalize()
{
    qDebug() << "[ANNA] Finalizing...";
    if (brain) {
        delete brain;
        brain = nullptr;
    }
    qDebug() << "[ANNA] Finalized";
    return true;
}

bool AnnaPlugin::showUI(QDialog *dock)
{
    PluginDock* pdock = dynamic_cast<PluginDock*>(dock);
    if (!pdock) return false;

    dialog = new AnnaCfgDialog();
    if (LoadConfig(MILLA_PLUG_DEF_PRESET)) dialog->loadConfig(&config);
    pdock->addContent(dialog);
    pdock->setCallbacks([this] (auto s, auto m) { this->dockCallback(s,m); });

    if (!pdock->exec()) return false;

    dialog->updateConfig(&config);
    SaveConfig(MILLA_PLUG_DEF_PRESET);

    return true;
}

void AnnaPlugin::dockCallback(QString preset, int mode)
{
    switch (mode) {
    case MILLA_PLUGINCB_ADD:
        dialog->updateConfig(&config);
        SaveConfig(preset);
        break;

    case MILLA_PLUGINCB_DEL:
        if (config_cb) config_cb("delete_key_value",preset);
        break;

    case MILLA_PLUGINCB_APPLY:
        LoadConfig(preset);
        dialog->loadConfig(&config);
        break;

    default:
        qDebug() << "[ANNA] Wrong dockCallback mode " << mode;
    }
}

void AnnaPlugin::setConfigCB(PlugConfCB cb)
{
    config_cb = cb;
}

QVariant AnnaPlugin::getParam(QString key)
{
    qDebug() << "[ANNA] requested parameter " << key;
    if (key == "show_ui") {
        return true;
    } else if (key == "use_config_cb") {
        return true;
    }
    return QVariant();
}

bool AnnaPlugin::setParam(QString key, QVariant val)
{
    qDebug() << "[ANNA] parameter " << key << " received";
    if (key == "process_started") {
        if (val.canConvert(QMetaType::Bool) && !val.toBool()) {
            qDebug() << "[ANNA] Stopping plugin by UI request, deleting brain";
            if (brain) {
                delete brain;
                brain = nullptr;
            }
        }
        return true;
    }
    return false;
}

QVariant AnnaPlugin::action(QVariant in)
{
    //sanity check
    if (!config.params.model[0] || !config.params.prompt[0] || cfg_extra.vision_file.empty())
        return QVariant();

    //create or reset brain first
    if (brain) {
        brain->Reset();
        qDebug() << "[ANNA] Brain reset";
    } else {
        brain = new AnnaBrain(&config);
        if (brain->getState() == ANNA_ERROR) {
            qDebug() << "[ANNA] ERROR: " << QString::fromStdString(brain->getError());
            delete brain;
            brain = nullptr;
            return QVariant();
        }
        qDebug() << "[ANNA] Brain created";
    }

    //process prompt first
    brain->setInput(config.params.prompt);
    if (!Generate(true)) {
        qDebug() << "[ANNA] Failed to process the prompt";
        return QVariant();
    }
    qDebug() << "[ANNA] Prompt processed";

    //encode image
    QPixmap img;
    if (in.canConvert<QPixmap>()) img = in.value<QPixmap>();
    if (img.isNull()) {
        qDebug() << "[ANNA] No image is given or unable to read input image";
        return QVariant();
    }
    img.save(AP_IMAGE_TEMP);
    brain->setClipModelFile(cfg_extra.vision_file);
    if (!brain->EmbedImage(AP_IMAGE_TEMP)) {
        qDebug() << "[ANNA] Unable to embed image: " << QString::fromStdString(brain->getError());
        return QVariant();
    }
    qDebug() << "[ANNA] Image encoded";

    //get the result
    brain->setInput(cfg_extra.usr_prefix);
    brain->setPrefix(cfg_extra.ai_prefix);
    if (Generate(false)) {
        QString out = QString::fromStdString(brain->getOutput());
        qDebug() << "[ANNA] Result: " << out;
        return out;
    } else
        qDebug() << "[ANNA] Failed to generate";

    return QVariant();
}

bool AnnaPlugin::LoadConfig(QString preset)
{
    if (!config_cb) return false;

    CONFIG_LOAD_PREP(preset);
    CONFIG_LOAD_CHARSTR(config.params.model);
    CONFIG_LOAD_CHARSTR(config.params.prompt);
    CONFIG_LOAD_STDSTR(cfg_extra.vision_file);
    CONFIG_LOAD_STDSTR(cfg_extra.ai_prefix);
    CONFIG_LOAD_STDSTR(cfg_extra.usr_prefix);
    CONFIG_LOAD_DONE(preset);

    qDebug() << "[ANNA] Config loaded";
    return true;
}

bool AnnaPlugin::SaveConfig(QString preset)
{
    if (!config_cb) return false;

    CONFIG_SAVE_PREP(preset);
    CONFIG_SAVE_STDSTR(config.params.model);
    CONFIG_SAVE_STDSTR(config.params.prompt);
    CONFIG_SAVE_STDSTR(cfg_extra.vision_file);
    CONFIG_SAVE_STDSTR(cfg_extra.ai_prefix);
    CONFIG_SAVE_STDSTR(cfg_extra.usr_prefix);
    CONFIG_SAVE_DONE(preset);

    qDebug() << "[ANNA] Config saved";
    return true;
}

void AnnaPlugin::DefaultConfig()
{
    config.convert_eos_to_nl = true;
    config.nl_to_turnover = false;
    config.verbose_level = 2; //FIXME: DEBUG ONLY!!!
    config.user = &cfg_extra;

    gpt_params* p = &config.params;
    p->seed = 0;
    p->n_threads = thread::hardware_concurrency();
    if (p->n_threads < 1) p->n_threads = 1;
    p->n_predict = -1;
    p->n_ctx = AP_DEFAULT_CONTEXT;
    p->n_batch = AP_DEFAULT_BATCH;
    p->n_gpu_layers = 32; //FIXME: DEBUG ONLY!!!
    p->model[0] = 0;
    p->prompt[0] = 0;
    p->sparams.temp = AP_DEFAULT_TEMP;
}

bool AnnaPlugin::Generate(bool no_sample)
{
    while (brain) {
        //detach actual processing into separate thread
        auto rhnd = async(launch::async,[&]() -> auto {
            return brain->Processing(no_sample);
        });

        //while calling wait callback
        double dummy = 0;
        while (rhnd.wait_for(AP_WAITCB_PERIOD) != future_status::ready) {
            if (progress_cb) progress_cb(dummy);
            dummy += AP_WAITCB_INC;
            if (dummy >= 100.f) dummy = 0;
        }
        if (progress_cb) progress_cb(100.f);

        //finally we can acquire the request result
        AnnaState s = rhnd.get();

        //and decide how to continue
        switch (s) {
        case ANNA_TURNOVER:
            return true;
        case ANNA_READY:
            if (no_sample) return true;
            //qDebug() << QString::fromStdString(brain->getOutput());
            //fall-thru
        case ANNA_PROCESSING:
            //nothing to do, waiting
            break;
        case ANNA_ERROR:
            qDebug() << "[ANNA] Error: " << QString::fromStdString(brain->getError());
            return false;
        default:
            qDebug() << "[ANNA] Wrong brain state: " << QString::fromStdString(AnnaBrain::StateToStr(s));
            return false;
        }
    }
    return true;
}
