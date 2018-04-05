#include <QDebug>
#include "lifegenplugin.h"
#include "dialog.h"

LifeGenPlugin::LifeGenPlugin(QObject *parent) :
    QObject(parent),
    MillaGenericPlugin()
{
}

void LifeGenPlugin::showUI()
{
    Dialog dlg;
    dlg.exec();
}

QVariant LifeGenPlugin::getParam(QString key)
{
    if (key == "update_delay") {
        return int(LIFEGEN_UPDATE);
    }
    return QVariant();
}

bool LifeGenPlugin::setParam(QString key, QVariant val)
{
    //
    return true;
}

QVariant LifeGenPlugin::action(QVariant in)
{
    //
    qDebug() << "ACTION";
    return QVariant();
}
