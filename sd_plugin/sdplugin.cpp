#include <thread>
#include <QDebug>
#include "sdplugin.h"
#include "sdcfgdialog.h"
#include "ui_sdcfgdialog.h"
#include "stable-diffusion.h"

SDPlugin::SDPlugin() :
    QObject(),
    MillaGenericPlugin()
{
    qDebug() << "[SD] Plugin instance created";
}

SDPlugin::~SDPlugin()
{
    qDebug() << "[SD] Plugin destroyed";
}

bool SDPlugin::init()
{
    qDebug() << "[SD] Init OK";
    return true;
}

bool SDPlugin::finalize()
{
    qDebug() << "[SD] Finalizing...";

    //save the settings
    if (!config_cb) return true;
    config_cb("save_key_value","SD_model="+QString::fromStdString(model));
    config_cb("save_key_value","SD_vae="+QString::fromStdString(vaemodel));
    config_cb("save_key_value","SD_cnet="+QString::fromStdString(cnmodel));
    config_cb("save_key_value","SD_prompt="+QString::fromStdString(prompt));
    config_cb("save_key_value","SD_cfgscale="+QString::asprintf("%.3f",cfg_scale));
    config_cb("save_key_value","SD_styleratio="+QString::asprintf("%.3f",style_ratio));
    config_cb("save_key_value","SD_steps="+QString::asprintf("%d",steps));
    config_cb("save_key_value","SD_batch="+QString::asprintf("%d",batch));
    qDebug() << "[SD] Config saved";
    return true;
}

void SDPlugin::showUI()
{
    qDebug() << "[SD] Showing UI...";
    SDCfgDialog dlg;

    dlg.ui->modelFile->setText(QString::fromStdString(model));
    dlg.ui->vaeFile->setText(QString::fromStdString(vaemodel));
    dlg.ui->cnFile->setText(QString::fromStdString(cnmodel));
    dlg.ui->promptEdit->setPlainText(QString::fromStdString(prompt));
    dlg.ui->cfgScale->setValue(cfg_scale);
    dlg.ui->styleRatio->setValue(style_ratio);
    dlg.ui->stepsCnt->setValue(steps);
    dlg.ui->batchCnt->setValue(batch);

    skip_gen = !dlg.exec();
    if (skip_gen) return;

    model = dlg.ui->modelFile->text().toStdString();
    vaemodel = dlg.ui->vaeFile->text().toStdString();
    cnmodel = dlg.ui->cnFile->text().toStdString();
    prompt = dlg.ui->promptEdit->toPlainText().toStdString();
    cfg_scale = dlg.ui->cfgScale->value();
    style_ratio = dlg.ui->styleRatio->value();
    steps = dlg.ui->stepsCnt->value();
    batch = dlg.ui->batchCnt->value();
}

QVariant SDPlugin::getParam(QString key)
{
    qDebug() << "[SD] requested parameter " << key;
    if (key == "show_ui") {
        return true;
    } else if (key == "use_config_cb") {
        return true;
    }
    return QVariant();
}

bool SDPlugin::setParam(QString key, QVariant val)
{
    qDebug() << "[SD] parameter " << key << " sent";
    return false;
}

void SDPlugin::setConfigCB(PlugConfCB cb)
{
    config_cb = cb;
    if (!cb) return;

    //load previous settings
    if (load_once) return;

    QVariant r;
    r = config_cb("load_key_value","SD_model");
    if (r.canConvert<QString>())
        model = r.value<QString>().toStdString();

    r = config_cb("load_key_value","SD_vae");
    if (r.canConvert<QString>())
        vaemodel = r.value<QString>().toStdString();

    r = config_cb("load_key_value","SD_cnet");
    if (r.canConvert<QString>())
        cnmodel = r.value<QString>().toStdString();

    r = config_cb("load_key_value","SD_prompt");
    if (r.canConvert<QString>())
        prompt = r.value<QString>().toStdString();

    r = config_cb("load_key_value","SD_cfgscale");
    if (r.canConvert<QString>())
        cfg_scale = r.value<QString>().toFloat();

    r = config_cb("load_key_value","SD_styleratio");
    if (r.canConvert<QString>())
        style_ratio = r.value<QString>().toFloat();

    r = config_cb("load_key_value","SD_steps");
    if (r.canConvert<QString>())
        steps = r.value<QString>().toInt();

    r = config_cb("load_key_value","SD_batch");
    if (r.canConvert<QString>())
        batch = r.value<QString>().toInt();

    load_once = true;
    qDebug() << "[SD] Config preloaded";
}

QVariant SDPlugin::action(QVariant in)
{
    qDebug() << "[SD] Action()";
    if (skip_gen) {
        qDebug() << "[SD] skipped";
        return QVariant();
    }

    //TODO: navigate through the batch
    outputs.clear();
    if (GenerateBatch())
        return QVariant(outputs.at(0));
    else
        return QVariant();
}

bool SDPlugin::eventFilter(QObject *obj, QEvent *event)
{
    switch (event->type()) {
    case QEvent::Wheel:
        if (!outputs.empty()) {
            QWheelEvent* wev = static_cast<QWheelEvent*>(event);
            if (wev->angleDelta().y() > 0) {
                if (++curout >= outputs.size()) curout = 0;
            } else {
                if (--curout < 0) curout = outputs.size() - 1;
            }
            // TODO: send signal to viewer to force showing new picture
            qDebug() << "[SDPlugin] Selecting image " << curout;
        }
        break;

    default: return QObject::eventFilter(obj,event); //unknown event, move on
    }

    return true;
}

bool SDPlugin::progress(double val)
{
    if (progress_cb) return progress_cb(val);
    return true;
}

static void log_helper(sd_log_level_t level, const char* text, void* /*data*/)
{
    QString txt(text);
    while (txt.endsWith('\n')) txt.chop(1);
    qDebug() << "[SD] " << level << ": " << txt;
}

static bool progress_helper(int step, int steps, float /*time*/, void* data)
{
    SDPlugin* self = reinterpret_cast<SDPlugin*>(data);
    return self->progress((float)step / (float)steps * 100.f);
}

bool SDPlugin::GenerateBatch()
{
    qDebug() << "[SD] Initializing context...";
    sd_set_log_callback(log_helper,nullptr);
    sd_set_progress_callback(progress_helper,this);
    sd_ctx_t* ctx = new_sd_ctx(model.c_str(),vaemodel.c_str(),"",cnmodel.c_str(),"","","",true,false,true,get_num_physical_cores(),SD_TYPE_COUNT,STD_DEFAULT_RNG,DEFAULT,false,false,false);
    if (!ctx) {
        qDebug() << "[SD] ERROR: Unable to create context!";
        return false;
    }

    //FIXME: dehardcode the magics (UI or just define?)
    qDebug() << "[SD] Generating...";
    int vseed = (seed == -1)? std::rand() : seed;
    sd_image_t* out = txt2img(ctx,prompt.c_str(),nprompt.c_str(),-1,cfg_scale,SDPLUGIN_IMGSIZE,SDPLUGIN_IMGSIZE,EULER_A,steps,vseed,batch,NULL,0.9,style_ratio,false,"");
    if (out) {
        qDebug() << "[SD] Generation has finished!";
        for (int i = 0; i < batch; i++) {
            if (!out[i].data) continue;

            QImage img(out[i].data,out[i].width,out[i].height,((out[i].channel == 4)? QImage::Format_ARGB32 : QImage::Format_RGB888));
            outputs.push_back(QPixmap::fromImage(img));

            free(out[i].data);
            out[i].data = NULL;
        }
    } else
        qDebug() << "[SD] ERROR: Generation failed!";

    free(out);
    free_sd_ctx(ctx);
    return out; // if it was OK, it'll evaluate to true
}
