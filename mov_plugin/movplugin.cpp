#include <cmath>
#include <QDebug>
#include <QEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include "movplugin.h"

using namespace Qt;

MovPlugin::MovPlugin(QObject *parent) :
    QObject(parent),
    MillaGenericPlugin()
{
}

MovPlugin::~MovPlugin()
{
    //if (clip) delete clip;
    qDebug() << "[MovPlugin] Destroyed";
}

bool MovPlugin::init()
{
    qDebug() << "[MovPlugin] Supports" << QMovie::supportedFormats().join(", ");
    qDebug() << "[MovPlugin] Init OK";
    return true;
}

bool MovPlugin::finalize()
{
    if (clip) clip->stop();
    qDebug() << "[MovPlugin] Finalize OK";
    return true;
}

void MovPlugin::showUI()
{
    MovCfgDialog dlg;
    if (!dlg.exec()) return;

    if (config_cb && dlg.isInteractive()) {
        QVariant i;
        i.setValue(QObjectPtr(this));
        config_cb("set_event_filter",i); //insert event filter into main window
    }

    float fps = dlg.getFPS();
    delay = floor(1000.f / fps);
    percSpeed = ceil(fps / MILLAMOV_DEFAULT_FPS * 100.f);
    qDebug() << "[MovPlugin] Delay = " << delay << " (FPS = " << fps << "): " << percSpeed << "%";
}

QVariant MovPlugin::getParam(QString key)
{
    if (key == "update_delay") {
        return delay;

    } else if (key == "use_config_cb") {
        return true;

    } else if (key == "supported_formats") {
        QStringList out;
        for (auto &i : QMovie::supportedFormats()) out.push_back(i);
        return out;

    }
    return QVariant();
}

bool MovPlugin::setParam(QString key, QVariant val)
{
    if (key == "process_started") {
        if (val.value<bool>() && clipfile.exists()) {
            if (clip) delete clip;
            clip = new QMovie(clipfile.absoluteFilePath());
            cframe = 0;
            if (clip->isValid()) {
                clip->setCacheMode(QMovie::CacheAll);
                clip->setSpeed(percSpeed);
                clip->start();
                qDebug() << "[MovPlugin] Clip started";
                return true;
            } else {
                qDebug() << "[MovPlugin] ALERT: Unable to load movie clip";
                return false;
            }

        } else if (!val.value<bool>() && clip->state() == QMovie::Running) {
            clip->stop();
            qDebug() << "[MovPlugin] Clip stopped";
            return true;
        }

    } else if (key == "filename") {
        clipfile = QFileInfo(val.toString());
        return true;

    }
    return false;
}

QVariant MovPlugin::action(QVariant in)
{
    if (in.isValid() && clip->isValid()) return clip->currentPixmap();
    return QVariant();
}

bool MovPlugin::eventFilter(QObject *obj, QEvent *event)
{
    switch (event->type()) {
    case QEvent::KeyPress:
    case QEvent::KeyRelease:
//    {
//        QKeyEvent* kev = static_cast<QKeyEvent*>(event);
//        switch (kev->key()) {
//        case Key_Left:
//            break;
//        case Key_Right:
//            break;
//        case Key_Up:
//            break;
//        case Key_Down:
//            break;
//        default: break;
//        }
//    }
        break;

    case QEvent::MouseButtonPress:
//    case QEvent::MouseButtonRelease:
        if (clip && clip->state() == QMovie::Paused) clip->setPaused(false);
        else if (clip && clip->state() == QMovie::Running) clip->setPaused(true);
        break;

    case QEvent::Wheel:
        if (clip) {
            QWheelEvent* wev = static_cast<QWheelEvent*>(event);
            cframe += wev->angleDelta().y() / MILLAMOV_WHEEL_SENSITIVITY;
            if (cframe >= clip->frameCount()) cframe = clip->frameCount() - 1;
            if (cframe < 0) cframe = 0;
            clip->jumpToFrame(cframe);
            qDebug() << "[MovPlugin] Frame " << cframe;
        }
        break;

    default: return QObject::eventFilter(obj,event); //unknown event, move on
    }

    return true;
}
