#include <thread>
#include <QDebug>
#include "sdplugin.h"
#include "sdcfgdialog.h"
#include "ui_sdcfgdialog.h"

#define CONFIG_SAVE_STR(K,V) config_cb("save_key_value",(K "=")+V);
#define CONFIG_SAVE_STDSTR(K,V) config_cb("save_key_value",(K "=")+QString::fromStdString(V));
#define CONFIG_SAVE_INT(K,V) config_cb("save_key_value",(K "=")+QString::asprintf("%d",(V)));
#define CONFIG_SAVE_FLOAT(K,V) config_cb("save_key_value",(K "=")+QString::asprintf("%.3f",(V)));

#define CONFIG_LOAD_STR(K,V) r = config_cb("load_key_value",K); if (r.canConvert<QString>()) V = r.value<QString>();
#define CONFIG_LOAD_STDSTR(K,V) r = config_cb("load_key_value",K); if (r.canConvert<QString>()) V = r.value<QString>().toStdString();
#define CONFIG_LOAD_INT(K,V) r = config_cb("load_key_value",K); if (r.canConvert<QString>()) V = r.value<QString>().toInt();
#define CONFIG_LOAD_INTT(K,V,T) r = config_cb("load_key_value",K); if (r.canConvert<QString>()) V = (T)(r.value<QString>().toInt());
#define CONFIG_LOAD_FLOAT(K,V) r = config_cb("load_key_value",K); if (r.canConvert<QString>()) V = r.value<QString>().toFloat();

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
    ConfigSave();
    Cleanup();
    return true;
}

void SDPlugin::ConfigLoad()
{
    QVariant r;
    if (!config_cb) {
        qDebug() << "[SD] ERROR: Unable to load configuration!";
        return;
    }

    CONFIG_LOAD_INT("SD_dogen",dogen);
    CONFIG_LOAD_STDSTR("SD_model",model);
    CONFIG_LOAD_STDSTR("SD_vae",vaemodel);
    CONFIG_LOAD_STDSTR("SD_cnet",cnmodel);
    CONFIG_LOAD_STDSTR("SD_prompt",prompt);
    CONFIG_LOAD_STDSTR("SD_nprompt",nprompt);
    CONFIG_LOAD_FLOAT("SD_cfgscale",cfg_scale);
    CONFIG_LOAD_FLOAT("SD_styleratio",style_ratio);
    CONFIG_LOAD_INT("SD_steps",steps);
    CONFIG_LOAD_INT("SD_batch",batch);

    CONFIG_LOAD_INT("SD_doupsc",doupsc);
    CONFIG_LOAD_STDSTR("SD_esrgan",esrgan);
    CONFIG_LOAD_INT("SD_scalefac",scale_fac);

    CONFIG_LOAD_INTT("SD_autosave",autosave,sdplug_autosave_t);
    CONFIG_LOAD_INT("SD_asav_addb",asav_addb);
    CONFIG_LOAD_INT("SD_asav_match",asav_match);
    CONFIG_LOAD_INT("SD_asav_addtag",asav_addtag);
    CONFIG_LOAD_INT("SD_asav_addnote",asav_addnote);
    CONFIG_LOAD_STR("SD_asav_dir",asav_dir);
    CONFIG_LOAD_STR("SD_asav_fmt",asav_fmt);
    CONFIG_LOAD_STR("SD_asav_pat",asav_pat);
    CONFIG_LOAD_STR("SD_asav_tags",asav_tags);
    CONFIG_LOAD_STR("SD_asav_notes",asav_notes);

    qDebug() << "[SD] Config loaded";
}

void SDPlugin::ConfigSave()
{
    if (!config_cb) {
        qDebug() << "[SD] ERROR: Unable to save configuration!";
        return;
    }

    CONFIG_SAVE_INT("SD_dogen",dogen);
    CONFIG_SAVE_STDSTR("SD_model",model);
    CONFIG_SAVE_STDSTR("SD_vae",vaemodel);
    CONFIG_SAVE_STDSTR("SD_cnet",cnmodel);
    CONFIG_SAVE_STDSTR("SD_prompt",prompt);
    CONFIG_SAVE_STDSTR("SD_nprompt",nprompt);
    CONFIG_SAVE_FLOAT("SD_cfgscale",cfg_scale);
    CONFIG_SAVE_FLOAT("SD_styleratio",style_ratio);
    CONFIG_SAVE_INT("SD_steps",steps);
    CONFIG_SAVE_INT("SD_batch",batch);

    CONFIG_SAVE_INT("SD_doupsc",doupsc);
    CONFIG_SAVE_STDSTR("SD_esrgan",esrgan);
    CONFIG_SAVE_INT("SD_scalefac",scale_fac);

    CONFIG_SAVE_INT("SD_autosave",autosave);
    CONFIG_SAVE_INT("SD_asav_addb",asav_addb);
    CONFIG_SAVE_INT("SD_asav_match",asav_match);
    CONFIG_SAVE_INT("SD_asav_addtag",asav_addtag);
    CONFIG_SAVE_INT("SD_asav_addnote",asav_addnote);
    CONFIG_SAVE_STR("SD_asav_dir",asav_dir);
    CONFIG_SAVE_STR("SD_asav_fmt",asav_fmt);
    CONFIG_SAVE_STR("SD_asav_pat",asav_pat);
    CONFIG_SAVE_STR("SD_asav_tags",asav_tags);
    CONFIG_SAVE_STR("SD_asav_notes",asav_notes);

    qDebug() << "[SD] Config saved";
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
    dlg.ui->negPromptEdit->setPlainText(QString::fromStdString(nprompt));
    dlg.ui->cfgScale->setValue(cfg_scale);
    dlg.ui->styleRatio->setValue(style_ratio);
    dlg.ui->stepsCnt->setValue(steps);
    dlg.ui->batchCnt->setValue(batch);

    dlg.ui->doUpsc->setChecked(doupsc);
    dlg.ui->upscModel->setText(QString::fromStdString(esrgan));
    dlg.ui->upscFactor->setValue(scale_fac);

    switch (autosave) {
        case SDP_ASAV_NONE: dlg.ui->savNone->setChecked(true); break;
        case SDP_ASAV_ALL: dlg.ui->savAll->setChecked(true); break;
        case SDP_ASAV_USER: dlg.ui->savUser->setChecked(true); break;
    }
    dlg.ui->savDir->setText(asav_dir);
    dlg.ui->savFmt->setCurrentText(asav_fmt);
    dlg.ui->savPat->setText(asav_pat);
    dlg.ui->savDB->setChecked(asav_addb);
    dlg.ui->savMatch->setChecked(asav_match);
    dlg.ui->savAddNote->setChecked(asav_addnote);
    dlg.ui->savAddTag->setChecked(asav_addtag);
    dlg.ui->savTags->setText(asav_tags);
    dlg.ui->savNotes->setPlainText(asav_notes);

    skip_gen = !dlg.exec();
    if (skip_gen) return;

    dogen = dlg.ui->doGen->isChecked();
    model = dlg.ui->modelFile->text().toStdString();
    vaemodel = dlg.ui->vaeFile->text().toStdString();
    cnmodel = dlg.ui->cnFile->text().toStdString();
    prompt = dlg.ui->promptEdit->toPlainText().toStdString();
    nprompt = dlg.ui->negPromptEdit->toPlainText().toStdString();
    cfg_scale = dlg.ui->cfgScale->value();
    style_ratio = dlg.ui->styleRatio->value();
    steps = dlg.ui->stepsCnt->value();
    batch = dlg.ui->batchCnt->value();

    doupsc = dlg.ui->doUpsc->isChecked();
    esrgan = dlg.ui->upscModel->text().toStdString();
    scale_fac = dlg.ui->upscFactor->value();

    autosave = SDP_ASAV_NONE;
    if (dlg.ui->savAll->isChecked()) autosave = SDP_ASAV_ALL;
    if (dlg.ui->savUser->isChecked()) autosave = SDP_ASAV_USER;
    asav_dir = dlg.ui->savDir->text();
    asav_fmt = dlg.ui->savFmt->currentText();
    asav_pat = dlg.ui->savPat->text();
    asav_addb = dlg.ui->savDB->isChecked();
    asav_match = dlg.ui->savMatch->isChecked();
    asav_addnote = dlg.ui->savAddNote->isChecked();
    asav_addtag = dlg.ui->savAddTag->isChecked();
    asav_tags = dlg.ui->savTags->text();
    asav_notes = dlg.ui->savNotes->toPlainText();

    //TODO: validate autosave pattern against SDPLUGIN_ASAVE_REGEX, maybe?

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
    if (!load_once) ConfigLoad();
    load_once = true;
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

    case QEvent::KeyPress:
        if (!outputs.empty()) {
            QKeyEvent* kev = static_cast<QKeyEvent*>(event);
            if (kev->key() == Qt::Key_Space && autosave == SDP_ASAV_USER) {
                out_mutex.lock();
                if (curout >= 0 && curout < outputs.size()) {
                    qDebug() << "[SDPlugin] Saving image " << curout;
                    AutosaveImage(outputs.at(curout));
                } else
                    qDebug() << "[SDPlugin] Invalid image selected: " << curout;
                out_mutex.unlock();
            }
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

            if (autosave == SDP_ASAV_ALL) AutosaveImage(pix);

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

void SDPlugin::AutosaveImage(const QPixmap &img)
{
    if (img.isNull()) {
        qDebug() << "[SD] ERROR: AutosaveImage(): Null image supplied!";
        return;
    }

    QString fn = ScanNextImageFn();
    if (fn.isEmpty()) return;
    img.save(fn);
    qDebug() << "[SD] Image saved to " << fn;

    if (!asav_addb || !config_cb) return;

    config_cb("show_message",QVariant("File " + fn + " saved"));

    QVariant r = config_cb("index_new_file",QVariant(fn));
    if (!r.isValid() || !r.toBool()) {
        qDebug() << "[SD] ERROR: Unable to index newly created file, aborting...";
        return;
    }

    if (asav_match) {
        //TODO: get_all_tags
    }

    if (asav_addtag) {
        //TODO: append_tags
    }

    QStringList lst;
    lst.append(fn);
    if (asav_addnote) lst.append(asav_notes + "\n" + TextualizeConfig());
    else lst.append(TextualizeConfig());
    config_cb("append_notes",QVariant(lst));
}

QString SDPlugin::ScanNextImageFn()
{
    QString res;
    QRegExp ex(SDPLUGIN_ASAVE_REGEX);
    if (ex.indexIn(asav_pat) < 0) {
        qDebug() << "[SD] ERROR: Unable to match pattern " << asav_pat;
        return res;
    }
    auto lst = ex.capturedTexts();
    if (lst.length() != 3) {
        qDebug() << "[SD] ERROR: Incorrect number of captures: " << lst.length();
        return res;
    }
    QString fmt = QString::asprintf("%%0%dd",lst.at(2).length());
    //qDebug() << "[SD] fmt = '" << fmt << "'";

    for (int i = SDPLUGIN_ASAVE_START; i < SDPLUGIN_ASAVE_MAX; i++) {
        res = asav_dir + "/" + lst.at(1) + QString::asprintf(fmt.toStdString().c_str(),i) + "." + asav_fmt.toLower();
        res.replace("//","/");
        //qDebug() << "[SD] testing " << res;
        if (!QFile::exists(res)) return res;
    }

    qDebug() << "[SD] ERROR: Exhausted file name search space!";
    return QString();
}

QString SDPlugin::TextualizeConfig()
{
    QString r;

    if (dogen) {
        std::string s;
        s += "Image generated with " + model + ", using encoder " + vaemodel;
        if (!cnmodel.empty()) s += " with control net " + cnmodel;
        s += ".\n";
        s += "Prompt used:\n" + prompt + "\n";
        if (!nprompt.empty()) s += "Negative prompt used:\n" + nprompt + "\n";
        r = QString::fromStdString(s);
        r += QString::asprintf("cfg_scale = %.3f; style_ratio = %.3f; steps = %d; batch = %d/%d",cfg_scale,style_ratio,steps,curout+1,batch);
    }

    if (doupsc) {
        r += "Image upscaled using " + QString::fromStdString(esrgan);
        r += QString::asprintf(" with scale factor of %d.",scale_fac);
    }

    return r;
}
