#include <thread>
#include <QDebug>
#include "sdplugin.h"
#include "sdcfgdialog.h"
#include "ui_sdcfgdialog.h"

SDPlugin::SDPlugin() :
    QObject(),
    MillaGenericPlugin()
{
    qDebug() << "[SD] Plugin instance created";
}

SDPlugin::~SDPlugin()
{
    Cleanup();
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

    config_cb("save_key_value","SD_dogen="+QString::asprintf("%d",dogen));
    config_cb("save_key_value","SD_model="+QString::fromStdString(model));
    config_cb("save_key_value","SD_vae="+QString::fromStdString(vaemodel));
    config_cb("save_key_value","SD_cnet="+QString::fromStdString(cnmodel));
    config_cb("save_key_value","SD_prompt="+QString::fromStdString(prompt));
    config_cb("save_key_value","SD_cfgscale="+QString::asprintf("%.3f",cfg_scale));
    config_cb("save_key_value","SD_styleratio="+QString::asprintf("%.3f",style_ratio));
    config_cb("save_key_value","SD_steps="+QString::asprintf("%d",steps));
    config_cb("save_key_value","SD_batch="+QString::asprintf("%d",batch));

    config_cb("save_key_value","SD_doupsc="+QString::asprintf("%d",doupsc));
    config_cb("save_key_value","SD_esrgan="+QString::fromStdString(esrgan));
    config_cb("save_key_value","SD_scalefac="+QString::asprintf("%d",scale_fac));

    qDebug() << "[SD] Config saved";

    Cleanup();
    return true;
}

void SDPlugin::showUI()
{
    qDebug() << "[SD] Showing UI...";
    SDCfgDialog dlg;

    dlg.ui->doGen->setChecked(dogen);
    dlg.ui->modelFile->setText(QString::fromStdString(model));
    dlg.ui->vaeFile->setText(QString::fromStdString(vaemodel));
    dlg.ui->cnFile->setText(QString::fromStdString(cnmodel));
    dlg.ui->promptEdit->setPlainText(QString::fromStdString(prompt));
    dlg.ui->cfgScale->setValue(cfg_scale);
    dlg.ui->styleRatio->setValue(style_ratio);
    dlg.ui->stepsCnt->setValue(steps);
    dlg.ui->batchCnt->setValue(batch);

    dlg.ui->doUpsc->setChecked(doupsc);
    dlg.ui->upscModel->setText(QString::fromStdString(esrgan));
    dlg.ui->upscFactor->setValue(scale_fac);

    skip_gen = !dlg.exec();
    if (skip_gen) return;

    dogen = dlg.ui->doGen->isChecked();
    model = dlg.ui->modelFile->text().toStdString();
    vaemodel = dlg.ui->vaeFile->text().toStdString();
    cnmodel = dlg.ui->cnFile->text().toStdString();
    prompt = dlg.ui->promptEdit->toPlainText().toStdString();
    cfg_scale = dlg.ui->cfgScale->value();
    style_ratio = dlg.ui->styleRatio->value();
    steps = dlg.ui->stepsCnt->value();
    batch = dlg.ui->batchCnt->value();

    doupsc = dlg.ui->doUpsc->isChecked();
    esrgan = dlg.ui->upscModel->text().toStdString();
    scale_fac = dlg.ui->upscFactor->value();

    outputs.clear();
    curout = 0;

    if (config_cb && dogen) {
        QVariant i;
        i.setValue(QObjectPtr(this));
        config_cb("set_event_filter",i); //insert event filter into main window
    }
}

QVariant SDPlugin::getParam(QString key)
{
    qDebug() << "[SD] requested parameter " << key;
    if (key == "show_ui") {
        return true;
    } else if (key == "update_delay") {
        return delay;
    } else if (key == "use_config_cb") {
        return true;
    }
    return QVariant();
}

bool SDPlugin::setParam(QString key, QVariant val)
{
    qDebug() << "[SD] parameter " << key << " sent";
    if (key == "process_started") {
        if (!val.canConvert(QMetaType::Bool)) return false;
        if (dogen && val.toBool()) {
            qDebug() << "[SD] Run request received";
            if (skip_gen || !GenerateBatch()) return false;
            //skip_gen = true; // requires reset via showUI()
            return true;

        } else if (!val.toBool()) {
            qDebug() << "[SD] Stop request received";
            if (!self_stop) Cleanup();
            self_stop = false;
            return true;
        }
    }
    return false;
}

void SDPlugin::setConfigCB(PlugConfCB cb)
{
    config_cb = cb;
    if (!cb) return;

    //load previous settings
    if (load_once) return;

    QVariant r;
    r = config_cb("load_key_value","SD_dogen");
    if (r.canConvert<QString>())
        dogen = r.value<QString>().toInt();

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

    r = config_cb("load_key_value","SD_doupsc");
    if (r.canConvert<QString>())
        doupsc = r.value<QString>().toInt();

    r = config_cb("load_key_value","SD_esrgan");
    if (r.canConvert<QString>())
        esrgan = r.value<QString>().toStdString();

    r = config_cb("load_key_value","SD_scalefac");
    if (r.canConvert<QString>())
        scale_fac = r.value<QString>().toInt();

    load_once = true;
    qDebug() << "[SD] Config preloaded";
}

QVariant SDPlugin::action(QVariant in)
{
    if (skip_gen) return QVariant();

    QPixmap px;
    if (dogen) {
        out_mutex.lock();
        if (curout >= 0 && curout < outputs.count())
            px = outputs.at(curout);
        out_mutex.unlock();

    } else if (doupsc && in.canConvert<QPixmap>()) {
        QImage img = in.value<QPixmap>().toImage();
        img.convertTo(QImage::Format_RGB888); // upscaler doesn't care about alpha
        px = Scaleup(img);

        if (config_cb) {
            self_stop = true;
            config_cb("self_disable",QVariant());
        }
    }

    if (px.isNull()) return QVariant();
    return QVariant(px);
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
    sd_set_log_callback(log_helper,nullptr);
    sd_set_progress_callback(progress_helper,this);

    qDebug() << "[SD] Initializing context...";
    sd_ctx_t* ctx = new_sd_ctx(model.c_str(),vaemodel.c_str(),"",cnmodel.c_str(),"","","",true,false,true,get_num_physical_cores(),SD_TYPE_COUNT,STD_DEFAULT_RNG,DEFAULT,false,false,false);
    if (!ctx) {
        qDebug() << "[SD] ERROR: Unable to create generator context!";
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
            img.bits(); // force copying
            QPixmap pix;
            if (doupsc) pix = Scaleup(img);
            else pix = QPixmap::fromImage(img);

            out_mutex.lock();
            outputs.push_back(pix);
            out_mutex.unlock();

            free(out[i].data);
            out[i].data = NULL;
        }
    } else
        qDebug() << "[SD] ERROR: Generation failed!";

    free(out);
    free_sd_ctx(ctx);
    return out; // if it was OK, it'll evaluate to true
}

bool SDPlugin::isContinous()
{
    if (!load_once) return true; // by default, we want the UI to show it as a togglable plugin
    return dogen;
}

MillaPluginContentType SDPlugin::inputContent()
{
    return (!dogen && doupsc)? MILLA_CONTENT_IMAGE : MILLA_CONTENT_NONE;
}

QPixmap SDPlugin::Scaleup(const QImage &in)
{
    qDebug() << "[SD] Scale up()";
    sd_set_log_callback(log_helper,nullptr);
    sd_set_progress_callback(progress_helper,this);

    if (!upscaler) {
        out_mutex.lock();
        upscaler = new_upscaler_ctx(esrgan.c_str(),get_num_physical_cores(),SD_TYPE_COUNT);
        out_mutex.unlock();
        if (!upscaler) {
            qDebug() << "[SD] ERROR: Unable to create upscaler context!";
            return QPixmap();
        }
    }

    QImage tmp;
    sd_image_t img;
    img.channel = in.depth() / 8;
    img.width = in.width();
    img.width += SDPLUGIN_TILE_SIZE - (img.width % SDPLUGIN_TILE_SIZE); // make sure it's divisible by smallest tile size
    img.height = in.height();
    img.height += SDPLUGIN_TILE_SIZE - (img.height % SDPLUGIN_TILE_SIZE);
    img.data = (uint8_t*)in.constBits(); // we ain't gonna change the data

    // pad the area if needed
    if (img.width > (unsigned)in.width() || img.height > (unsigned)in.height()) {
        tmp = QImage(img.width,img.height,in.format());
        tmp.fill(0);
        for (int i = 0; i < in.height(); i++)
            memcpy(tmp.scanLine(i),in.scanLine(i),in.bytesPerLine());
        img.data = (uint8_t*)tmp.constBits();
    }

    sd_image_t res = upscale(upscaler,img,scale_fac);
    if (res.data) {
        qDebug() << "[SD] Upscale complete";
        QImage out(res.data,res.width,res.height,((res.channel == 4)? QImage::Format_ARGB32 : QImage::Format_RGB888));
        QPixmap final = QPixmap::fromImage(out);
        free(res.data);
        return final;
    }

    qDebug() << "[SD] ERROR: Upscale failed!";
    return QPixmap();
}

void SDPlugin::Cleanup()
{
    out_mutex.lock();
    outputs.clear();
    if (upscaler) free_upscaler_ctx(upscaler);
    upscaler = nullptr;
    out_mutex.unlock();
    qDebug() << "[SD] Cleanup complete";
}