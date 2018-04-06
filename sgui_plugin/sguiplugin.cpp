#include "sguiplugin.h"

SGUIPlugin::SGUIPlugin(QObject *parent) :
    QObject(parent),
    MillaGenericPlugin(),
    AbstractIO(0,0)
{

}

SGUIPlugin::~SGUIPlugin()
{

}

bool SGUIPlugin::init()
{

}

bool SGUIPlugin::finalize()
{

}

void SGUIPlugin::showUI()
{

}

QVariant SGUIPlugin::getParam(QString key)
{

}

bool SGUIPlugin::setParam(QString key, QVariant val)
{

}

QVariant SGUIPlugin::action(QVariant in)
{

}

void SGUIPlugin::getProperty(AIOPropertyType tp, int* ival, char** sval)
{

}

bool SGUIPlugin::setProperty(AIOPropertyType tp, int ival, const char* sval)
{

}

bool SGUIPlugin::PollEvent(AIOEvent* e)
{

}

void SGUIPlugin::DrawFrame(uchar* ptr)
{

}

void SGUIPlugin::MouseControl(AIOMouseControlKind k, bool local, int x, int y)
{

}
