#include <QDebug>
#include <QFileInfo>
#include <QTextStream>
#include "cameraplugin.h"
#include "dialog.h"
#include "cvhelper.h"

using namespace cv;

CameraPlugin::CameraPlugin(QObject *parent) :
    QObject(parent),
    MillaGenericPlugin()
{
}

CameraPlugin::~CameraPlugin()
{
    if (camera) finalize();
}

bool CameraPlugin::init()
{
    maxcamnum = 0;
    for (;; maxcamnum++) {
        QString nm = QString::asprintf(LINUX_CAMERA_NODE "%d", maxcamnum);
        QFileInfo fi(nm);
        if (!fi.exists()) break;
    }
    qDebug() << "[CAMPlugin] Total cameras count:" << maxcamnum;
    qDebug() << "[CAMPlugin] Init OK";
    return true;
}

bool CameraPlugin::finalize()
{
    if (camera) {
        delete camera;
        camera = nullptr;
    }
    qDebug() << "[CAMPlugin] Finalize OK";
    return true;
}

void CameraPlugin::showUI()
{
    CamCfgDialog dlg;
    dlg.setMaxID(maxcamnum-1);
    if (dlg.exec()) cameraidx = dlg.getID();
    else cameraidx = 0;
}

QVariant CameraPlugin::getParam(QString key)
{
    if (key == "update_delay") {
        return int(floor(1000.f / (double)MILLA_CAMERA_FPS));
    }
    return QVariant();
}

bool CameraPlugin::setParam(QString key, QVariant val)
{
    if (key == "process_started") {
        if (val.value<bool>() && !camera) {
            camera = new VideoCapture(cameraidx);
            return true;
        } else if (!val.value<bool>() && camera) {
            delete camera;
            camera = nullptr;
            return true;
        }
    }
    return false;
}

QVariant CameraPlugin::action(QVariant in)
{
    if (!camera || !in.canConvert<QSize>()) return QVariant();

    Mat frame;
    *camera >> frame;
    QImage out = CVHelper::slowConvertBack(frame);

    return QPixmap::fromImage(out);
}
