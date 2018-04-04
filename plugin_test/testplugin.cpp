#include <QDebug>
#include <QTextStream>
#include <QPainter>
#include <QPixmap>
#include <QImage>
#include "testplugin.h"

TestPlugin::TestPlugin() :
    QObject(),
    MillaGenericPlugin()
{
}

bool TestPlugin::init()
{
    qDebug() << "[Test] Init OK";
    return true;
}

bool TestPlugin::finalize()
{
    qDebug() << "[Test] Finalize OK";
    return true;
}

QVariant TestPlugin::getParam(QString key)
{
    qDebug() << "[Test] requested parameter " << key;
    return QVariant();
}

bool TestPlugin::setParam(QString key, QVariant val)
{
    qDebug() << "[Test] parameter " << key << " sent";
    if (key == "radius" && val.canConvert<double>()) {
        radius = val.value<double>();
        return true;
    }
    return false;
}

QVariant TestPlugin::action(QVariant in)
{
    qDebug() << "[Test] Action";

    if (!in.canConvert<QPixmap>()) {
        qDebug() << "[Test] ALERT: invalid argument (not a pixmap)";
        return QVariant();
    }

    QImage inq(in.value<QPixmap>().toImage().convertToFormat(QImage::Format_RGB32));

    double stp = 2.f + (double)random() / (double)RAND_MAX * radius;
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
