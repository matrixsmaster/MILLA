#include <QDebug>
#include <QTextStream>
#include "annaplugin.h"
#include "annacfgdialog.h"

AnnaPlugin::AnnaPlugin() :
    QObject(),
    MillaGenericPlugin()
{
}

bool AnnaPlugin::init()
{
    qDebug() << "[ANNA] Init OK";
    return true;
}

bool AnnaPlugin::finalize()
{
    qDebug() << "[ANNA] Finalize OK";
    return true;
}

void AnnaPlugin::showUI()
{
    qDebug() << "[ANNA] Showing interface";
    AnnaCfgDialog dlg;
    //TODO
}

QVariant AnnaPlugin::getParam(QString key)
{
    qDebug() << "[ANNA] requested parameter " << key;
    //TODO
    return QVariant();
}

bool AnnaPlugin::setParam(QString key, QVariant val)
{
    qDebug() << "[ANNA] parameter " << key << " sent";
    //TODO
    return false;
}

QVariant AnnaPlugin::action(QVariant in)
{
    return QVariant();
}
