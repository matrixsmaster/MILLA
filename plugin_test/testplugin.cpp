#include <QDebug>
#include <QTextStream>
#include <QPainter>
#include "testplugin.h"

TestPlugin::TestPlugin() :
    QObject(),
    MillaGeneratorPlugin()
{
}

void TestPlugin::setImageSize(QSize sz)
{
    qDebug() << "[TEST] Set size " << sz;
    mysize = sz;
}

QSize TestPlugin::getImageSize()
{
    qDebug() << "[TEST] Get size";
    return mysize;
}

bool TestPlugin::config(QString cfg)
{
    qDebug() << "[TEST] config(): " << cfg;
    QTextStream s(&cfg);
    int r,g,b;
    s >> r;
    s >> g;
    s >> b;
    mycolor = QColor(r,g,b);
    return true;
}

QPixmap TestPlugin::generate()
{
    qDebug() << "[TEST] generate()";
    core();
    return myimage;
}

void TestPlugin::update()
{
    qDebug() << "[TEST] update()";
    core();
}

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
