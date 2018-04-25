#include <QDebug>
#include "zipplugin.h"

ZipPlugin::ZipPlugin(QObject *parent) :
    QObject(parent),
    MillaGenericPlugin()
{
}

ZipPlugin::~ZipPlugin()
{
    qDebug() << "[ZipPlugin] Destroyed";
}

bool ZipPlugin::init()
{
    qDebug() << "[ZipPlugin] Init OK";
    return true;
}

bool ZipPlugin::finalize()
{
    qDebug() << "[ZipPlugin] Finalize OK";
    return true;
}

void ZipPlugin::showUI()
{
}

QVariant ZipPlugin::getParam(QString key)
{
    if (key == "supported_formats") {
        return QStringList({"zip"});
    }
    return QVariant();
}

bool ZipPlugin::setParam(QString key, QVariant val)
{
    if (key == "filename") {
        zipfile = QFileInfo(val.toString());
        return zipfile.exists();

    }
    return false;
}

QVariant ZipPlugin::action(QVariant /*in*/)
{
    return QVariant();
}
