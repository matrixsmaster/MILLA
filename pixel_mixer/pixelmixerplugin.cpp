#include <QDebug>
#include <QTextStream>
#include <QPainter>
#include <QPixmap>
#include <QImage>
#include <cmath>
#include "pixelmixerplugin.h"
#include "dialog.h"

PixelMixerPlugin::PixelMixerPlugin() :
    QObject(),
    MillaGenericPlugin()
{
}

bool PixelMixerPlugin::init()
{
    qDebug() << "[PixMix] Init OK";
    return true;
}

bool PixelMixerPlugin::finalize()
{
    qDebug() << "[PixMix] Finalize OK";
    return true;
}

bool PixelMixerPlugin::showUI()
{
    qDebug() << "[PixMix] Showing interface";
    PixMixCfgDialog dlg;
    //once = dlg.exec();
    if (!dlg.exec()) return false;
    radius = dlg.getRadius();
    return true;
}

QVariant PixelMixerPlugin::getParam(QString key)
{
    qDebug() << "[PixMix] requested parameter " << key;
    /*if (key == "show_ui") {
        return !once;
    }*/
    return QVariant();
}

bool PixelMixerPlugin::setParam(QString key, QVariant val)
{
    qDebug() << "[PixMix] parameter " << key << " sent";
    if (key == "radius" && val.canConvert<double>()) {
        radius = val.value<double>();
        return true;
    }
    return false;
}

QVariant PixelMixerPlugin::action(QVariant in)
{
    qDebug() << "[PixMix] Action";

    if (!in.canConvert<QPixmap>()) {
        qDebug() << "[PixMix] ALERT: invalid argument (not a pixmap)";
        return QVariant();
    }

    QImage inq(in.value<QPixmap>().toImage().convertToFormat(QImage::Format_RGB32));

    double stp = radius;
    double prg = 0, dp = 100.f / (double)(inq.width()*inq.height());

    for (int i = 0; i < inq.height(); i++) {
        uchar* mp = inq.scanLine(i);

        for (int j = 0; j < inq.width(); j++) {
            int nx = j + floor((double)random() / (double)RAND_MAX * stp / 2) - floor((double)random() / (double)RAND_MAX * stp / 2);
            int ny = i + floor((double)random() / (double)RAND_MAX * stp / 2) - floor((double)random() / (double)RAND_MAX * stp / 2);
            if (nx < 0) nx = 0;
            if (nx >= inq.width()) nx = inq.width() - 1;
            if (ny < 0) ny = 0;
            if (ny >= inq.height()) ny = inq.height() - 1;

            uchar* sp = inq.scanLine(ny);
            sp += 4 * nx;

            uchar sw;
            for (int k = 0; k < 4; k++,sp++,mp++) {
                sw = *mp;
                *mp = *sp;
                *sp = sw;
            }

            prg += dp;
            if (progress_cb && !progress_cb(prg)) break;
        }
        if (progress_cb && !progress_cb(prg)) break;
    }

    return QPixmap::fromImage(inq);
}
