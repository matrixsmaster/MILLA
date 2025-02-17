#include <QDebug>
#include <QTextStream>
#include "sguiplugin.h"

SGUIPlugin::SGUIPlugin(QObject *parent) :
    QObject(parent),
    AbstractIO(0,0),
    MillaGenericPlugin()
{
}

SGUIPlugin::~SGUIPlugin()
{
    cleanUp();
    qDebug() << "[SGUIPlugin] destroyed";
}

bool SGUIPlugin::init()
{
    cleanUp();
    qDebug() << "[SGUIPlugin] Init OK";
    return true;
}

bool SGUIPlugin::finalize()
{
    cleanUp();
    qDebug() << "[SGUIPlugin] Finalize OK";
    return true;
}

void SGUIPlugin::cleanUp()
{
    qDebug() << "[SGUIPlugin] Cleaning up";

    if (sgui) {
        delete sgui;
        sgui = nullptr;
    }
    if (vfs) {
        delete vfs;
        vfs = nullptr;
    }
}

void SGUIPlugin::showUI()
{
    SGUICfgDialog dlg;

    //recall the last successfully loaded VFS filename
    if (config_cb) {
        QVariant s = config_cb("load_key_value","sgui_last_vfs");
        if (s.canConvert<QString>() && !s.toString().isEmpty()) dlg.setFilename(s.toString());
        s = config_cb("load_key_value","sgui_last_script");
        if (s.canConvert<QString>() && !s.toString().isEmpty()) dlg.setScript(s.toString());
    }

    //fire up the config dialog and retreive the information
    if (!dlg.exec()) return;
    info = dlg.getInfo();

    info.valid = false; //temporarily invalidate configuration information
    if (!info.vfs_fn.isEmpty() && !info.startup.isEmpty()) {
        QFileInfo fi(info.vfs_fn);
        if (fi.exists()) info.valid = true; //file is OK, now info is valid
    }
}

void SGUIPlugin::fireUp()
{
    if (!info.valid) {
        qDebug() << "[SGUIPlugin] Unable to retreive configuration information";
        cleanUp();
        return;
    }

    vfs = new VFS(info.vfs_fn.toStdString(),true);
    vfs->setReadOnly(true);

    sgui = new SGUI(vfs,this);
    //sgui->setFrameskip(true);

    if (!sgui->setScriptF(info.startup.toStdString().c_str(),false,true,true)) {
        qDebug() << "[SGUIPlugin] Unable to load startup script " << info.startup;
        cleanUp();
        return;
    }

    if (config_cb) {
        QVariant i;
        i.setValue(QObjectPtr(&sink));
        config_cb("set_event_filter",i); //insert event filter into main window
        config_cb("save_key_value","sgui_last_vfs="+info.vfs_fn); //save the last successfully loaded VFS filename
        config_cb("save_key_value","sgui_last_script="+info.startup); //save the startup script path
    }

    qDebug() << "[SGUIPlugin] SGUI started. Version " << sgui->getVersion();
}

QVariant SGUIPlugin::getParam(QString key)
{
    if (key == "update_delay") {
        return info.timeout;

    } else if (key == "use_config_cb") {
        return true;

    } else if (key == "supported_formats") {
        return QStringList({"vfs"});
    }
    return QVariant();
}

bool SGUIPlugin::setParam(QString key, QVariant val)
{
    if (key == "process_started" && val.value<bool>()) {
        fireUp();
        return true;

    } else if (key == "filename") {
        //clipfile = QFileInfo(val.toString());
        return true;
    }
    return false;
}

QVariant SGUIPlugin::action(QVariant in)
{
    if (!sgui || !vfs) return QVariant();
    if (!in.canConvert<QSize>()) {
        qDebug() << "[SGUIPlugin] Action(): invalid QVariant";
        return QVariant();
    }
    QSize sz = in.value<QSize>();
    avail_w = sz.width();
    avail_h = sz.height();

    bool quit;
    sgui->UpdateIO(quit);

    if (quit) {
        qDebug() << "[SGUIPlugin] SGUI called it quits";
        cleanUp();
        return QPixmap();
    }

    return QPixmap::fromImage(frame);
}

void SGUIPlugin::getProperty(AIOPropertyType tp, int* ival, char** sval)
{
    switch (tp) {
    case AIOP_SHEIGHT:
        *ival = screen_h;
        break;

    case AIOP_SWIDTH:
        *ival = screen_w;
        break;

    case AIOP_MWIDTH:
        *ival = avail_w;
        break;

    case AIOP_MHEIGHT:
        *ival = avail_h;
        break;

    case AIOP_AUTOREPEAT:
        *ival = autorepeat;
        break;

    case AIOP_COLORCHANS:
        *ival = 4;
        break;

    case AIOP_COLORORDER:
        *ival = colorder;
        break;

    case AIOP_CAPTION:
        strncpy(*sval,"Fixed Title",*ival); //TODO: make it changeable
        break;

    default: break;
    }
}

bool SGUIPlugin::setProperty(AIOPropertyType tp, int ival, const char* sval)
{
    switch (tp) {
    case AIOP_CAPTION:
        if (!sval) return false;
        //TODO: caption
        return true;

    case AIOP_AUTOREPEAT:
        autorepeat = ival;
        return true;

    case AIOP_COLORORDER:
        colorder = static_cast<AIOColorOrdering>(ival);
        qDebug() << "[SGUIPlugin] Setting color order to " << ival;
        return true;

    case AIOP_COLORCHANS:
        return (ival == 4);

    case AIOP_SWIDTH:
        screen_w = ival;
        return true;

    case AIOP_SHEIGHT:
        screen_h = ival;
        return true;

    default:
        return false;
    }
}

bool SGUIPlugin::PollEvent(AIOEvent* e)
{
    if (!sink.popEvent(e)) return false;

    switch (e->type) {
    case AIOE_MOUSEDOWN:
    case AIOE_MOUSEUP:
    case AIOE_MOUSEMOVE:
    //case AIOE_MOUSEWHEEL:
    {
        QPoint p = mouse.Update(e->mouse,QSize(screen_w,screen_h));
        e->mouse.x = p.x();
        e->mouse.y = p.y();
    }
        break;

    default: break;
    }

    return true;
}

void SGUIPlugin::DrawFrame(uchar* ptr)
{
    QSize sz(screen_w,screen_h);

    if (frame.isNull() || frame.size() != sz) {
        frame = QImage(sz,QImage::Format_RGBA8888);
    }

    memcpy(frame.bits(),ptr,sz.width()*sz.height()*4);

    mouse.Draw(frame);
}

void SGUIPlugin::MouseControl(AIOMouseControlKind k, bool local, int x, int y)
{
    switch (k) {
    case AIOM_SETPOSITION:
        if (local) mouse.forcePosition(QPoint(x,y));
        break;

    case AIOM_SHOWCURSOR:
        mouse.showCursor(x);
        break;
    }
}
