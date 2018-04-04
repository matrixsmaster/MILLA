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
}

bool TestPlugin::finalize()
{
    qDebug() << "[Test] Finalize OK";
}

QVariant TestPlugin::getParam(QString key)
{
    qDebug() << "[Test] requested parameter " << key;
}

bool TestPlugin::setParam(QString key, QVariant val)
{
    qDebug() << "[Test] parameter " << key << " sent";
}

QVariant TestPlugin::action(QVariant in)
{
    qDebug() << "[Test] Action";
}

/*
void TestPlugin::core()
{
    QImage inq(mysize,QImage::Format_RGB32);
    if (inq.isNull()) return;

    QPainter painter(&inq);
    QPen paintpen(mycolor);
    QBrush paintbrush(mycolor);

    paintpen.setWidth(2);
    painter.setPen(paintpen);
    painter.setBrush(paintbrush);

    painter.drawRect(QRect(QPoint(0,0),mysize));
}
*/
