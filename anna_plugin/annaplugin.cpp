#include <QDebug>
#include <QTextStream>
#include "annaplugin.h"
#include "annacfgdialog.h"

AnnaPlugin::AnnaPlugin() :
    QObject(),
    MillaGenericPlugin()
{
    qDebug() << "[ANNA] Plugin instance created";
    DefaultConfig();
}

AnnaPlugin::~AnnaPlugin()
{
    if (brain) delete brain;
    qDebug() << "[ANNA] Plugin destroyed";
}

bool AnnaPlugin::init()
{
    qDebug() << "[ANNA] Init OK";
    return true;
}

bool AnnaPlugin::finalize()
{
    qDebug() << "[ANNA] Finalize OK";
    return true;
}

void AnnaPlugin::showUI()
{
    qDebug() << "[ANNA] Showing interface...";
    AnnaCfgDialog dlg;
    if (dlg.exec()) {
        //set some config options
        dlg.updateConfig(&config);
        qDebug() << "[ANNA] Config updated";
    }
}

QVariant AnnaPlugin::getParam(QString key)
{
    qDebug() << "[ANNA] requested parameter " << key;
    if (key == "show_ui") {
        return true;
    }
    return QVariant();
}

bool AnnaPlugin::setParam(QString key, QVariant val)
{
    qDebug() << "[ANNA] parameter " << key << " sent";
    //TODO
    return false;
}

QVariant AnnaPlugin::action(QVariant in)
{
    //(re-)create brain first
    if (brain) delete brain;
    brain = new AnnaBrain(&config);
    if (brain->getState() == ANNA_ERROR) {
        qDebug() << "[ANNA] ERROR: " << QString::fromStdString(brain->getError());
        delete brain;
        brain = nullptr;
        return QVariant();
    }

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
        qDebug() << "[ANNA] Unable to embed image";
        return QVariant();
    }

    //get the result
    brain->setInput(cfg_extra.usr_prefix);
    brain->setPrefix(cfg_extra.ai_prefix);
    if (Generate()) return QString::fromStdString(brain->getOutput());
    return QVariant();
}

void AnnaPlugin::DefaultConfig()
{
    config.convert_eos_to_nl = false;
    config.nl_to_turnover = false;
    config.verbose_level = 0;
    config.user = &cfg_extra;

    gpt_params* p = &config.params;
    p->seed = 0;
    p->n_threads = std::thread::hardware_concurrency();
    if (p->n_threads < 1) p->n_threads = 1;
    p->n_predict = -1;
    p->n_ctx = AP_DEFAULT_CONTEXT;
    p->n_batch = AP_DEFAULT_BATCH;
    p->n_gpu_layers = 0;
    p->model[0] = 0;
    p->prompt[0] = 0;
    p->sparams.temp = AP_DEFAULT_TEMP;
}

bool AnnaPlugin::Generate()
{
    while (brain) {
        AnnaState s = brain->Processing(false);
        switch (s) {
        case ANNA_TURNOVER:
            return true;
        case ANNA_READY:
        case ANNA_PROCESSING:
            // nothing to do, waiting
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
