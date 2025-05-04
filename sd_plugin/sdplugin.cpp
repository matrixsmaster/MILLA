#include <cmath>
#include <thread>
#include <QDebug>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainter>
#include "sdplugin.h"
#include "plugindock.h"
#include "ui_sdcfgdialog.h"

#define CONFIG_LOAD_INTT(V,T) if (doc.object().contains("" TOSTRING(V) "")) V = (T)(doc.object().value("" TOSTRING(V) "").toInt());
#define CONFIG_LOAD_STRN(V) (doc.object().contains("" TOSTRING(V) ""))? (doc.object().value("" TOSTRING(V) "").toString()) : ""
#define CONFIG_LOAD_KEYSQ(V) V = V.fromString(CONFIG_LOAD_STRN(V));
#define CONFIG_SAVE_KEYSQ(V) obj["" TOSTRING(V) ""] = V.toString();

using namespace std;

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
    Cleanup();
    return true;
}

bool SDPlugin::LoadConfig(QString preset)
{
    if (!config_cb) return false;

    CONFIG_LOAD_PREP(preset);

    CONFIG_LOAD_INT(dogen);
    CONFIG_LOAD_INT(useleft);
    CONFIG_LOAD_INT(realtime);
    CONFIG_LOAD_INT(usetrees);
    CONFIG_LOAD_STDSTR(model);
    CONFIG_LOAD_STDSTR(vaemodel);
    CONFIG_LOAD_STDSTR(cnmodel);
    CONFIG_LOAD_STDSTR(clipmodel);
    CONFIG_LOAD_STDSTR(t5model);
    CONFIG_LOAD_STDSTR(loradir);
    CONFIG_LOAD_STDSTR(prompt);
    CONFIG_LOAD_STDSTR(nprompt);
    CONFIG_LOAD_FLOAT(cfg_scale);
    CONFIG_LOAD_FLOAT(style_ratio);
    CONFIG_LOAD_FLOAT(guidance);
    CONFIG_LOAD_FLOAT(strength);
    CONFIG_LOAD_INT(sampler);
    CONFIG_LOAD_INT(steps);
    CONFIG_LOAD_INT(batch);

    CONFIG_LOAD_INT(doupsc);
    CONFIG_LOAD_STDSTR(esrgan);
    CONFIG_LOAD_INT(scale_fac);

    CONFIG_LOAD_INTT(autosave,sdplug_autosave_t);
    CONFIG_LOAD_INT(asav_addb);
    CONFIG_LOAD_INT(asav_match);
    CONFIG_LOAD_INT(asav_addtag);
    CONFIG_LOAD_INT(asav_addnote);
    CONFIG_LOAD_STR(asav_dir);
    CONFIG_LOAD_STR(asav_fmt);
    CONFIG_LOAD_STR(asav_pat);
    CONFIG_LOAD_STR(asav_tags);
    CONFIG_LOAD_STR(asav_notes);

    CONFIG_LOAD_KEYSQ(hk_saveone);
    CONFIG_LOAD_KEYSQ(hk_saveall);
    CONFIG_LOAD_KEYSQ(hk_nextstep);

    CONFIG_LOAD_DONE(preset);
    qDebug() << "[SD] Config loaded";

    // make sure key bindings are in place (Do we need it??)
    //if (hk_saveone.isEmpty()) hk_saveone = QKeySequence::fromString(SDPLUGIN_DEFKEY_SAVEONE);
    //if (hk_saveall.isEmpty()) hk_saveall = QKeySequence::fromString(SDPLUGIN_DEFKEY_SAVEALL);
    //if (hk_nextstep.isEmpty()) hk_nextstep = QKeySequence::fromString(SDPLUGIN_DEFKEY_NEXTSTEP);

    seed = 0; // always reset the seed (if UI is not shown, the last generated seed would stuck)
    load_once = true;
    return true;
}

bool SDPlugin::SaveConfig(QString preset)
{
    if (!config_cb) return false;

    CONFIG_SAVE_PREP(preset);

    CONFIG_SAVE_INT(dogen);
    CONFIG_SAVE_INT(useleft);
    CONFIG_SAVE_INT(realtime);
    CONFIG_SAVE_INT(usetrees);
    CONFIG_SAVE_STDSTR(model);
    CONFIG_SAVE_STDSTR(vaemodel);
    CONFIG_SAVE_STDSTR(cnmodel);
    CONFIG_SAVE_STDSTR(clipmodel);
    CONFIG_SAVE_STDSTR(t5model);
    CONFIG_SAVE_STDSTR(loradir);
    CONFIG_SAVE_STDSTR(prompt);
    CONFIG_SAVE_STDSTR(nprompt);
    CONFIG_SAVE_FLOAT(cfg_scale);
    CONFIG_SAVE_FLOAT(style_ratio);
    CONFIG_SAVE_FLOAT(guidance);
    CONFIG_SAVE_FLOAT(strength);
    CONFIG_SAVE_INT(sampler);
    CONFIG_SAVE_INT(steps);
    CONFIG_SAVE_INT(batch);

    CONFIG_SAVE_INT(doupsc);
    CONFIG_SAVE_STDSTR(esrgan);
    CONFIG_SAVE_INT(scale_fac);

    CONFIG_SAVE_INT(autosave);
    CONFIG_SAVE_INT(asav_addb);
    CONFIG_SAVE_INT(asav_match);
    CONFIG_SAVE_INT(asav_addtag);
    CONFIG_SAVE_INT(asav_addnote);
    CONFIG_SAVE_STR(asav_dir);
    CONFIG_SAVE_STR(asav_fmt);
    CONFIG_SAVE_STR(asav_pat);
    CONFIG_SAVE_STR(asav_tags);
    CONFIG_SAVE_STR(asav_notes);

    CONFIG_SAVE_KEYSQ(hk_saveone);
    CONFIG_SAVE_KEYSQ(hk_saveall);
    CONFIG_SAVE_KEYSQ(hk_nextstep);

    CONFIG_SAVE_DONE(preset);
    qDebug() << "[SD] Config saved";
    return true;
}

void SDPlugin::setConfigUI()
{
    dialog->ui->doGen->setChecked(dogen);
    dialog->ui->useLeft->setChecked(useleft);
    dialog->ui->showRT->setChecked(realtime);
    dialog->ui->useTrees->setChecked(usetrees);
    dialog->ui->modelFile->setText(QString::fromStdString(model));
    dialog->ui->vaeFile->setText(QString::fromStdString(vaemodel));
    dialog->ui->cnFile->setText(QString::fromStdString(cnmodel));
    dialog->ui->clipFile->setText(QString::fromStdString(clipmodel));
    dialog->ui->t5xxlFile->setText(QString::fromStdString(t5model));
    dialog->ui->loraDir->setText(QString::fromStdString(loradir));
    dialog->ui->promptEdit->setPlainText(QString::fromStdString(prompt));
    dialog->ui->negPromptEdit->setPlainText(QString::fromStdString(nprompt));
    dialog->ui->cfgScale->setValue(cfg_scale);
    dialog->ui->styleRatio->setValue(style_ratio);
    dialog->ui->samplerBox->setCurrentIndex(sampler);
    dialog->ui->guidanceK->setValue(guidance);
    dialog->ui->strengthK->setValue(strength);
    dialog->ui->stepsCnt->setValue(steps);
    dialog->ui->batchCnt->setValue(batch);

    dialog->ui->doUpsc->setChecked(doupsc);
    dialog->ui->upscModel->setText(QString::fromStdString(esrgan));
    dialog->ui->upscFactor->setValue(scale_fac);

    switch (autosave) {
    case SDP_ASAV_NONE: dialog->ui->savNone->setChecked(true); break;
    case SDP_ASAV_ALL: dialog->ui->savAll->setChecked(true); break;
    case SDP_ASAV_USER: dialog->ui->savUser->setChecked(true); break;
    }
    dialog->ui->savDir->setText(asav_dir);
    dialog->ui->savFmt->setCurrentText(asav_fmt);
    dialog->ui->savPat->setText(asav_pat);
    dialog->ui->savDB->setChecked(asav_addb);
    dialog->ui->savMatch->setChecked(asav_match);
    dialog->ui->savAddNote->setChecked(asav_addnote);
    dialog->ui->savAddTag->setChecked(asav_addtag);
    dialog->ui->savTags->setText(asav_tags);
    dialog->ui->savNotes->setPlainText(asav_notes);

    dialog->ui->hkSaveOne->setKeySequence(hk_saveone);
    dialog->ui->hkSaveAll->setKeySequence(hk_saveall);
    dialog->ui->hkNextStep->setKeySequence(hk_nextstep);
}

void SDPlugin::getConfigUI()
{
    dogen = dialog->ui->doGen->isChecked();
    useleft = dialog->ui->useLeft->isChecked();
    realtime = dialog->ui->showRT->isChecked();
    usetrees = dialog->ui->useTrees->isChecked();
    model = dialog->ui->modelFile->text().toStdString();
    vaemodel = dialog->ui->vaeFile->text().toStdString();
    cnmodel = dialog->ui->cnFile->text().toStdString();
    clipmodel = dialog->ui->clipFile->text().toStdString();
    t5model = dialog->ui->t5xxlFile->text().toStdString();
    loradir = dialog->ui->loraDir->text().toStdString();
    prompt = dialog->ui->promptEdit->toPlainText().toStdString();
    nprompt = dialog->ui->negPromptEdit->toPlainText().toStdString();
    cfg_scale = dialog->ui->cfgScale->value();
    style_ratio = dialog->ui->styleRatio->value();
    guidance = dialog->ui->guidanceK->value();
    strength = dialog->ui->strengthK->value();
    sampler = dialog->ui->samplerBox->currentIndex();
    steps = dialog->ui->stepsCnt->value();
    batch = dialog->ui->batchCnt->value();
    seed = dialog->ui->seedVal->value();

    doupsc = dialog->ui->doUpsc->isChecked();
    esrgan = dialog->ui->upscModel->text().toStdString();
    scale_fac = dialog->ui->upscFactor->value();

    autosave = SDP_ASAV_NONE;
    if (dialog->ui->savAll->isChecked()) autosave = SDP_ASAV_ALL;
    if (dialog->ui->savUser->isChecked()) autosave = SDP_ASAV_USER;
    asav_dir = dialog->ui->savDir->text();
    asav_fmt = dialog->ui->savFmt->currentText();
    asav_pat = dialog->ui->savPat->text();
    asav_addb = dialog->ui->savDB->isChecked();
    asav_match = dialog->ui->savMatch->isChecked();
    asav_addnote = dialog->ui->savAddNote->isChecked();
    asav_addtag = dialog->ui->savAddTag->isChecked();
    asav_tags = dialog->ui->savTags->text();
    asav_notes = dialog->ui->savNotes->toPlainText();

    hk_saveone = dialog->ui->hkSaveOne->keySequence();
    hk_saveall = dialog->ui->hkSaveAll->keySequence();
    hk_nextstep = dialog->ui->hkNextStep->keySequence();
}

bool SDPlugin::showUI(QDialog* dock)
{
    PluginDock* pdock = dynamic_cast<PluginDock*>(dock);
    if (!pdock) return false;

    dialog = new SDCfgDialog();
    if (LoadConfig(MILLA_PLUG_DEF_PRESET)) setConfigUI();
    pdock->addContent(dialog);
    pdock->setCallbacks([this] (auto s, auto m) { this->dockCallback(s,m); });

    skip_gen = !pdock->exec();
    if (skip_gen) return false;

    getConfigUI();
    //TODO: validate autosave pattern against SDPLUGIN_ASAVE_REGEX, maybe?
    SaveConfig(MILLA_PLUG_DEF_PRESET);

    return true;
}

bool SDPlugin::RunStop(bool start)
{
    if (!start) {
        qDebug() << "[SD] Stop request received";
        if (split_exec.valid()) {
            split_exec.wait();
            auto ptr = split_exec.get();
            if (ptr) free(ptr);
            split_exec = std::future<sd_image_t*>();
        }
        if (!self_stop) Cleanup();
        self_stop = false;
        return true;
    }

    if (!dogen) return true;

    qDebug() << "[SD] Run request received";
    outputs.clear();
    curout = 0;

    QImage srcimg;
    if (useleft) {
        if (!config_cb) return false;
        QVariant rl(config_cb("get_left_image",QVariant()));
        if (!rl.canConvert<QPixmap>()) return false;
        srcimg = rl.value<QPixmap>().toImage();
    }
    if (skip_gen || !GenerateBatch(srcimg)) return false;

    if (config_cb && dogen) {
        QVariant i;
        i.setValue(QObjectPtr(this));
        config_cb("set_event_filter",i); //insert event filter into main window
    }
    return true;
}

void SDPlugin::CheckAsync()
{
    if (!split_exec.valid()) return;

    // test by waiting for a very small amount of time
    if (split_exec.wait_for(1ms) == future_status::ready) {
        if (config_cb) config_cb("long_processing_done",true); // stop progress bar
        split_exec.get(); // invalidate the async object

    } else {
        // make sure the progress bar is active
        if (config_cb) {
            auto r = config_cb("is_long_processing",QVariant());
            if (r.canConvert<bool>() && !r.value<bool>())
                config_cb("long_processing_done",false);
        }
        // update actual progress bar
        last_progr_ret = progress_cb? progress_cb(last_progress) : true;
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
        return RunStop(val.toBool());

    } else if (key == "apply_preset" && val.canConvert<QString>()) {
        return LoadConfig(val.value<QString>());
    }
    return false;
}

QVariant SDPlugin::action(QVariant in)
{
    if (skip_gen) return QVariant();

    QPixmap px;
    if (dogen) {
        // first of all, update async progress
        CheckAsync();

        // how to present the data?
        if (usetrees) px = ShowTreeState(in);
        else {
            // in normal mode, just show currently selected image
            out_mutex.lock();
            if (curout >= 0 && curout < outputs.count())
                px = outputs.at(curout).img;
            out_mutex.unlock();
        }

    } else if (doupsc && in.canConvert<QPixmap>()) {
        // this action doesn't require Start/Stop, as it's a one-shot conversion, but we still need to worry about the progress bar's state
        if (config_cb) config_cb("long_processing_done",false); // start processing, prepare Stop button
        last_progr_ret = true;

        // convert the input
        QImage img = in.value<QPixmap>().toImage();
        img.convertTo(QImage::Format_RGB888); // upscaler doesn't care about alpha
        px = Upscale(img);

        // stop ourselves (to remove active action flag from the UI)
        if (config_cb) {
            self_stop = true;
            config_cb("self_disable",QVariant());
        }
        if (config_cb) config_cb("long_processing_done",true); // stop processing explicitly
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
            QKeySequence seq(kev->modifiers() | kev->key());

            // navigation
            out_mutex.lock();
            switch (kev->key()) {
            case Qt::Key_PageUp:
                if (curout > 0) curout--;
                break;
            case Qt::Key_PageDown:
                if (curout < outputs.size()-1) curout++;
                break;
            case Qt::Key_Home:
                curout = 0;
                break;
            case Qt::Key_End:
                curout = outputs.empty()? 0 : outputs.size()-1;
                break;
            }

            // other hotkeys
            if (seq == hk_saveone) {
                if (autosave == SDP_ASAV_USER && curout >= 0 && curout < outputs.size()) {
                    qDebug() << "[SDPlugin] Saving image " << curout;
                    AutosaveImage(outputs[curout]);
                }
            } else if (seq == hk_saveall) {
                if (autosave == SDP_ASAV_USER && !outputs.empty()) {
                    qDebug() << "[SDPlugin] Saving all images by user's request";
                    for (auto &&i : outputs) AutosaveImage(i);
                }
            }
            out_mutex.unlock();
        }
        break;

    default: return QObject::eventFilter(obj,event); //unknown event, move on
    }

    return true;
}

bool SDPlugin::progress(double val)
{
    // to make sure we're not updating values from one thread into UI thread, we need to defer the update
    last_progress = val;
    return last_progr_ret;
}

void SDPlugin::dockCallback(QString preset, int mode)
{
    switch (mode) {
    case MILLA_PLUGINCB_ADD:
        getConfigUI();
        SaveConfig(preset);
        break;

    case MILLA_PLUGINCB_DEL:
        if (config_cb) config_cb("delete_key_value",preset);
        break;

    case MILLA_PLUGINCB_APPLY:
        LoadConfig(preset);
        setConfigUI();
        break;

    default:
        qDebug() << "[SDPlugin] Wrong dockCallback mode " << mode;
    }
}

void SDPlugin::imageCallback(sd_image_t *img)
{
    AddImage(img,false); // temporarily we forbid upscaling while showing images in realtime due to VRAM considerations
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

static void imageshow_helper(sd_image_t* img, void* user)
{
    SDPlugin* self = reinterpret_cast<SDPlugin*>(user);
    self->imageCallback(img);
}

bool SDPlugin::GenerateBatch(const QImage &in)
{
    sd_set_log_callback(log_helper,nullptr);
    sd_set_progress_callback(progress_helper,this);
    last_progress = 0;
    last_progr_ret = true;

    if (useleft) {
        if (in.isNull()) {
            qDebug() << "[SD] ERROR: no left image given!";
            return false;
        }
        inp_img_convert = in.scaled(QSize(SDPLUGIN_IMGSIZE,SDPLUGIN_IMGSIZE),Qt::KeepAspectRatioByExpanding);
        inp_img_convert.convertTo(QImage::Format_RGB888);
    }

    // create new SD context
    // TODO: make context disjointed so we can reuse it for faster generation
    qDebug() << "[SD] Initializing context...";
    string bmdl = t5model.empty()? model.c_str() : "";
    string dmdl = t5model.empty()? "" : model.c_str();
    bool vae_decode_only = !useleft;
    if (sdcontext) free_sd_ctx(sdcontext);
    sdcontext = new_sd_ctx(bmdl.c_str(),clipmodel.c_str(),"",t5model.c_str(),dmdl.c_str(),vaemodel.c_str(),"",cnmodel.c_str(),loradir.c_str(),"","",vae_decode_only,false,true,get_num_physical_cores(),SD_TYPE_COUNT,STD_DEFAULT_RNG,DEFAULT,false,false,false,false);
    if (!sdcontext) {
        qDebug() << "[SD] ERROR: Unable to create generator context!";
        return false;
    }
    if (realtime) set_imageready(sdcontext,imageshow_helper,this);

    qDebug() << "[SD] Generating...";
    if (!seed) seed = rand();

    // detach the execution into a separate threaded object
    split_exec = async(launch::async,[this]() -> sd_image_t* {
        sample_method_t smpl = (sample_method_t)sampler;
        if (!useleft)
            return txt2img(sdcontext,prompt.c_str(),nprompt.c_str(),-1,cfg_scale,guidance,0.f,SDPLUGIN_IMGSIZE,SDPLUGIN_IMGSIZE,smpl,steps,seed,batch,NULL,0.9,style_ratio,false,"",nullptr,0,0,0,0);

        sd_image_t leftimg,maskimg;
        vector<uint8_t> maskholder;
        qDebug() << "[SD] Resizing input image...";
        leftimg.channel = inp_img_convert.depth() / 8;
        leftimg.width = inp_img_convert.width();
        leftimg.height = inp_img_convert.height();
        leftimg.data = (uint8_t*)inp_img_convert.constBits(); // we ain't gonna change the data
        maskholder.resize(SDPLUGIN_IMGSIZE*SDPLUGIN_IMGSIZE,255);
        maskimg.width = SDPLUGIN_IMGSIZE;
        maskimg.height = SDPLUGIN_IMGSIZE;
        maskimg.channel = 1;
        maskimg.data = maskholder.data();
        qDebug() << "[SD] input and mask images are ready";

        return img2img(sdcontext,leftimg,maskimg,prompt.c_str(),nprompt.c_str(),-1,cfg_scale,guidance,0.f,SDPLUGIN_IMGSIZE,SDPLUGIN_IMGSIZE,smpl,steps,strength,seed,batch,NULL,0.9,style_ratio,false,"",nullptr,0,0,0,0);
    });

    // for realtime updates, we have to exit now and use action() for the rest of the process
    if (realtime) return true;

    // for bulk updates, we need to keep updating the async progress
    while (split_exec.wait_for(SDPLUGIN_UI_UPDATE) != future_status::ready)
        last_progr_ret = progress_cb? progress_cb(last_progress) : true;

    // now we can push all decoded images into the output vector
    sd_image_t* out = split_exec.get();
    if (out) {
        qDebug() << "[SD] Generation has finished!";
        for (int i = 0; i < batch; i++) {
            if (out[i].data) AddImage(out+i,true);
        }
        free(out);

    } else
        qDebug() << "[SD] ERROR: Generation failed!";

    // make sure split_exec is invalid, and free other resources
    split_exec = std::future<sd_image_t*>();
    free_sd_ctx(sdcontext);
    sdcontext = nullptr;
    return out; // if it was OK, it'll evaluate to true
}

void SDPlugin::AddImage(sd_image_t* in, bool scale)
{
    QImage img(in->data,in->width,in->height,((in->channel == 4)? QImage::Format_ARGB32 : QImage::Format_RGB888));
    img.bits(); // force copying

    SDOutputRec rec;
    if (doupsc && scale) rec.img = Upscale(img);
    else rec.img  = QPixmap::fromImage(img);
    rec.saved = false;
    if (autosave == SDP_ASAV_ALL) AutosaveImage(rec);

    out_mutex.lock();
    outputs.push_back(rec);
    out_mutex.unlock();

    free(in->data);
    in->data = NULL;

    if (split_exec.valid()) curout = outputs.size() - 1;
}

bool SDPlugin::isContinous()
{
    if (!load_once) return true; // by default, we want the UI to show it as a togglable plugin
    // if generating - we're going to create a serie of images
    return dogen;
}

MillaPluginContentType SDPlugin::inputContent()
{
    if (!dogen && doupsc) return MILLA_CONTENT_IMAGE;
    if (dogen && useleft) return MILLA_CONTENT_IMAGE;
    return MILLA_CONTENT_NONE;
}

QPixmap SDPlugin::Upscale(const QImage &in)
{
    qDebug() << "[SD] Upscale()";
    sd_set_log_callback(log_helper,nullptr);
    sd_set_progress_callback(progress_helper,this);

    if (!upscaler) {
        out_mutex.lock();
        upscaler = new_upscaler_ctx(esrgan.c_str(),get_num_physical_cores());
        out_mutex.unlock();
        if (!upscaler) {
            qDebug() << "[SD] ERROR: Unable to create upscaler context!";
            return QPixmap();
        }
    }

    QImage tmp;
    sd_image_t img;
    img.channel = in.depth() / 8;
    if (in.width() < SDPLUGIN_TILE_SIZE) img.width = SDPLUGIN_TILE_SIZE;
    else if (in.width() % SDPLUGIN_TILE_SIZE) img.width = (in.width() / SDPLUGIN_TILE_SIZE + 1) * SDPLUGIN_TILE_SIZE;
    else img.width = in.width();
    if (in.height() < SDPLUGIN_TILE_SIZE) img.height = SDPLUGIN_TILE_SIZE;
    else if (in.height() % SDPLUGIN_TILE_SIZE) img.height = (in.height() / SDPLUGIN_TILE_SIZE + 1) * SDPLUGIN_TILE_SIZE;
    else img.height = in.height();
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
    if (sdcontext) free_sd_ctx(sdcontext);
    sdcontext = nullptr;
    outputs.clear();
    if (upscaler) free_upscaler_ctx(upscaler);
    upscaler = nullptr;
    out_mutex.unlock();
    qDebug() << "[SD] Cleanup complete";
}

void SDPlugin::AutosaveImage(SDOutputRec &rec)
{
    if (rec.img.isNull()) {
        qDebug() << "[SD] ERROR: AutosaveImage(): Null image supplied!";
        return;
    }
    if (rec.saved) {
        qDebug() << "[SD] WARN: Image has been already saved, ignoring...";
        if (config_cb)
            config_cb("show_message",QVariant("Already saved"));
        return;
    }

    QString fn = ScanNextImageFn();
    if (fn.isEmpty()) return;
    rec.saved = rec.img.save(fn);

    if (rec.saved) {
        qDebug() << "[SD] Image saved to " << fn;
        if (config_cb)
            config_cb("show_message",QVariant("File " + fn + " saved"));
        QFileInfo fi(fn);
        if (fi.exists()) fn = fi.canonicalFilePath();
    } else {
        qDebug() << "[SD] ERROR: Unable to save image to " << fn;
        if (config_cb)
            config_cb("show_message",QVariant("Unable to save image to '" + fn + "' !"));
    }

    if (!asav_addb || !config_cb) return;
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
    if (lst.length() < 2 || lst.length() > 3) {
        qDebug() << "[SD] ERROR: Incorrect number of captures: " << lst.length();
        return res;
    }
    QString fmt = QString::asprintf("%%0%dd",lst.back().length()); // last capture defines number of digits
    //qDebug() << "[SD] fmt = '" << fmt << "'";

    for (int i = SDPLUGIN_ASAVE_START; i < SDPLUGIN_ASAVE_MAX; i++) {
        res = asav_dir + "/";
        if (lst.length() == 3) res += lst.at(1); // if both parts have been captured, use the prefix
        res += QString::asprintf(fmt.toStdString().c_str(),i) + "." + asav_fmt.toLower();
        res.replace("//","/");
        //qDebug() << "[SD] testing filename " << res;
        if (!QFile::exists(res)) return res;
    }

    qDebug() << "[SD] ERROR: Exhausted file name search space!";
    return QString();
}

QString SDPlugin::TextualizeConfig()
{
    QString r;

    if (dogen) {
        string s;
        s += "Image generated with " + model + ", using encoder " + vaemodel;
        if (!cnmodel.empty()) s += ", with control net " + cnmodel;
        if (!clipmodel.empty()) s += ", with CLiP " + clipmodel;
        if (!t5model.empty()) s += ", with T5XXL " + t5model;
        if (!loradir.empty()) s += ", using path " + loradir + " for LoRAs ";
        s += ".\n";

        if (useleft && config_cb) {
            auto mt = config_cb("get_left_meta",QVariant());
            if (mt.isValid() && mt.value<MImageListRecord>().valid && !mt.value<MImageListRecord>().generated)
                s += "Based on " + mt.value<MImageListRecord>().filename.toStdString() + "\n";
        }

        s += "Prompt used:\n" + prompt + "\n";
        if (!nprompt.empty()) s += "Negative prompt used:\n" + nprompt + "\n";

        r = QString::fromStdString(s);
        r += QString::asprintf("cfg_scale = %.2f; style_ratio = %.2f; guidance = %.2f; sampler = %d; steps = %d; seed = %d; batch = %d/%d",
                               cfg_scale,style_ratio,guidance,sampler,steps,seed,curout+1,batch);
    }

    if (doupsc) {
        r += "Image upscaled using " + QString::fromStdString(esrgan);
        r += QString::asprintf(" with scale factor of %d.",scale_fac);
    }

    return r;
}

QPixmap SDPlugin::ShowTreeState(QVariant sz)
{
    lock_guard<mutex> lock(out_mutex);

    int w = SDPLUGIN_IMGSIZE, h = SDPLUGIN_IMGSIZE;
    if (sz.canConvert<QSize>()) {
        w = sz.value<QSize>().width();
        h = sz.value<QSize>().height();
    }

    int xm = floor(sqrt(w*h*outputs.size()));
    int nw = 1, nh = 1;
    for (int s = xm; s > 0; s--) {
        nw = w / s;
        nh = h / s;
        xm = s;
        if (nw * nh >= outputs.size()) break;
    }
    //qDebug() << "[SD] xm = " << xm << "; nw = " << nw << "; nh = " << nh;

    QImage img(w,h,QImage::Format_RGB888);
    img.fill(QColor(0,0,0));
    QPainter p(&img);
    int cx = 0, cy = 0, ww = 0;
    for (auto &i : outputs) {
        QRectF rto(cx,cy,xm,xm);
        QRectF rfr(0,0,SDPLUGIN_IMGSIZE,SDPLUGIN_IMGSIZE);
        p.drawPixmap(rto,i.img,rfr);

        if (++ww >= nw) {
            ww = 0;
            cx = 0;
            cy += xm;
        } else
            cx += xm;
    }

    return QPixmap::fromImage(img);
}
