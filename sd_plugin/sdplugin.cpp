#include <QDebug>
#include "sdplugin.h"
#include "sdcfgdialog.h"

SDPlugin::SDPlugin() :
    QObject(),
    MillaGenericPlugin()
{
    qDebug() << "[SD] Plugin instance created";
}

SDPlugin::~SDPlugin()
{
    qDebug() << "[SD] Plugin destroyed";
}

bool SDPlugin::init()
{
    qDebug() << "[SD] Init OK";
    return true;
}

bool SDPlugin::finalize()
{
    qDebug() << "[SD] Finalized";
    return true;
}

void SDPlugin::showUI()
{
    qDebug() << "[SD] Showing UI...";
    SDCfgDialog dlg;
    skip_gen = !dlg.exec();
    //TODO
}

QVariant SDPlugin::getParam(QString key)
{
    qDebug() << "[SD] requested parameter " << key;
    if (key == "show_ui") {
        return true;
    }
    return QVariant();
}

bool SDPlugin::setParam(QString key, QVariant val)
{
    qDebug() << "[SD] parameter " << key << " sent";
    return false;
}

QVariant SDPlugin::action(QVariant in)
{
    qDebug() << "[SD] Action()";
    if (skip_gen) {
        qDebug() << "[SD] skipped";
        return QVariant();
    }

    //TODO

    return QVariant();
}
