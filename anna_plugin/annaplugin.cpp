#include <QDebug>
#include <QTextStream>
#include <functional>
#include <future>
#include <chrono>
#include "annaplugin.h"
#include "annacfgdialog.h"

using namespace std;

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

void AnnaPlugin::showUI()
{
    qDebug() << "[ANNA] Showing interface...";
    AnnaCfgDialog dlg;
    dlg.loadConfig(&config);
    if (!dlg.exec()) return;

    //set some config options
    dlg.updateConfig(&config);
    qDebug() << "[ANNA] Config updated";

    //and save them
    if (!config_cb) return;
    QVariant r;
    //FIXME: check results??
    r = config_cb("save_key_value","ANNA_fmodel="+QString(config.params.model));
    r = config_cb("save_key_value","ANNA_fvision="+QString::fromStdString(cfg_extra.vision_file));
    r = config_cb("save_key_value","ANNA_prefai="+QString::fromStdString(cfg_extra.ai_prefix));
    r = config_cb("save_key_value","ANNA_prefusr="+QString::fromStdString(cfg_extra.usr_prefix));
    r = config_cb("save_key_value","ANNA_prompt="+QString(config.params.prompt));
    qDebug() << "[ANNA] Config saved";
}

void AnnaPlugin::setConfigCB(PlugConfCB cb)
{
    config_cb = cb;
    if (!cb) return;

    //pre-load config (if exists)
    string tmp;
    QVariant r;
    r = config_cb("load_key_value","ANNA_fmodel");
    if (r.canConvert<QString>()) {
        tmp = r.value<QString>().toStdString();
        if (!tmp.empty()) strncpy(config.params.model,tmp.c_str(),sizeof(config.params.model)-1);
    }
    r = config_cb("load_key_value","ANNA_fvision");
    if (r.canConvert<QString>() && !r.value<QString>().isEmpty())
        cfg_extra.vision_file = r.value<QString>().toStdString();
    r = config_cb("load_key_value","ANNA_prefai");
    if (r.canConvert<QString>() && !r.value<QString>().isEmpty())
        cfg_extra.ai_prefix = r.value<QString>().toStdString();
    r = config_cb("load_key_value","ANNA_prefusr");
    if (r.canConvert<QString>() && !r.value<QString>().isEmpty())
        cfg_extra.usr_prefix = r.value<QString>().toStdString();
    r = config_cb("load_key_value","ANNA_prompt");
    if (r.canConvert<QString>()) {
        tmp = r.value<QString>().toStdString();
        if (!tmp.empty()) strncpy(config.params.prompt,tmp.c_str(),sizeof(config.params.prompt)-1);
    }
    qDebug() << "[ANNA] Config preloaded";
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
