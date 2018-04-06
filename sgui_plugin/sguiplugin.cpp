#include <QDebug>
#include <QTextStream>
#include <QFileInfo>
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
    //TODO: some settings like (timeout?) + maybe some statistics
    Dialog dlg;
    if (dlg.exec()) {
        info = dlg.getInfo();

        info.valid = false; //temporarily invalidate configuration information
        if (!info.vfs_fn.isEmpty() && !info.startup.isEmpty()) {
            QFileInfo fi(info.vfs_fn);
            if (fi.exists()) info.valid = true;
        }
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

    sgui = new SGUI(vfs,vfs,this);
    sgui->setFrameskip(true);

    if (!sgui->setScript(info.startup.toStdString().c_str(),false,true,true)) {
        qDebug() << "[SGUIPlugin] Unable to load startup script " << info.startup;
        cleanUp();
        return;
    }

    qDebug() << "[SGUIPlugin] SGUI started";
}

QVariant SGUIPlugin::getParam(QString key)
{
    if (key == "update_delay") {
        return info.timeout;

    } else if (key == "use_config_cb") {
        return false;

    }
    return QVariant();
}

bool SGUIPlugin::setParam(QString key, QVariant val)
{
    if (key == "process_started" && val.value<bool>()) {
        fireUp();
        return true;
    }
    return false;
}

QVariant SGUIPlugin::action(QVariant)
{
    if (!sgui || !vfs) return QVariant();

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
    return false;
}

void SGUIPlugin::DrawFrame(uchar* ptr)
{
    QSize sz(screen_w,screen_h);

    if (frame.isNull() || frame.size() != sz) {
        frame = QImage(sz,QImage::Format_RGBA8888);
    }

    memcpy(frame.bits(),ptr,sz.width()*sz.height()*4);
}

void SGUIPlugin::MouseControl(AIOMouseControlKind k, bool local, int x, int y)
{

}
