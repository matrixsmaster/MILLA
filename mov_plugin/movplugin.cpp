#include <cmath>
#include <QDebug>
#include "movplugin.h"

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
}

QVariant MovPlugin::getParam(QString key)
{
    if (key == "update_delay") {
        return int(floor(1000.f / (double)MILLA_ANIMATION_FPS));

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
            if (clip->isValid()) {
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
    if (in.isValid() && clip->isValid() && clip->state() == QMovie::Running) {
        return clip->currentPixmap();
    }
    return QVariant();
}
