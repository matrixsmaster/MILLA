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
    qDebug() << "[SD] Finalized";
    return true;
}

void SDPlugin::showUI()
{
    qDebug() << "[SD] Showing UI...";
    SDCfgDialog dlg;
    skip_gen = !dlg.exec();

    if (!skip_gen) {
        model = dlg.ui->modelFile->text().toStdString();
        vaemodel = dlg.ui->vaeFile->text().toStdString();
        cnmodel = dlg.ui->cnFile->text().toStdString();
        prompt = dlg.ui->promptEdit->toPlainText().toStdString();
        steps = dlg.ui->stepsCnt->value();
        batch = dlg.ui->batchCnt->value();
    }
}

QVariant SDPlugin::getParam(QString key)
{
    qDebug() << "[SD] requested parameter " << key;
    if (key == "show_ui") {
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

    //TODO: pre-load config (if exists)

    qDebug() << "[SD] Config preloaded";
}

QVariant SDPlugin::action(QVariant in)
{
    qDebug() << "[SD] Action()";
    if (skip_gen) {
        qDebug() << "[SD] skipped";
        return QVariant();
    }

    if (GenerateBatch()) //TODO: navigate through the batch
        return QVariant(outputs.at(0));
    else
        return QVariant();
}

bool SDPlugin::GenerateBatch()
{
    qDebug() << "[SD] Initializing context...";
    sd_ctx_t* ctx = new_sd_ctx(model.c_str(),vaemodel.c_str(),"",cnmodel.c_str(),"","","",true,false,true,std::thread::hardware_concurrency(),SD_TYPE_COUNT,CUDA_RNG,DEFAULT,false,false,false);

    //FIXME: dehardcode the magics (UI or just define?)
    qDebug() << "[SD] Generating...";
    int vseed = (seed == -1)? std::rand() : seed;
    sd_image_t* out = txt2img(ctx,prompt.c_str(),nprompt.c_str(),-1,7,512,512,EULER_A,steps,vseed,batch,NULL,0.9,20,false,"");
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
