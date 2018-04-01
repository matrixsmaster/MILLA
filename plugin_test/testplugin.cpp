#include <QDebug>
#include "testplugin.h"

TestPlugin::TestPlugin() :
    QObject(),
    MillaGeneratorPlugin()
{
}

void TestPlugin::setImageSize(QSize sz)
{
    qDebug() << "[TEST] Set size " << sz;
}

QSize TestPlugin::getImageSize()
{
    qDebug() << "[TEST] Get size";
    return QSize(0,0);
}

bool TestPlugin::config(QString cfg)
{
    qDebug() << "[TEST] config(): " << cfg;
    return true;
}

QPixmap TestPlugin::generate()
{
    qDebug() << "[TEST] generate()";
    return QPixmap();
}

void TestPlugin::update()
{
    qDebug() << "[TEST] update()";
}
