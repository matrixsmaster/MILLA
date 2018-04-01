#ifndef TESTPLUGIN_H
#define TESTPLUGIN_H

#include "plugins.h"

class TestPlugin : public QObject, MillaGeneratorPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID MILLA_PLUGIN_GEN_LID FILE "testplugin.json")
    Q_INTERFACES(MillaGeneratorPlugin)

public:
    TestPlugin();
    virtual ~TestPlugin() {}

    virtual void setImageSize(QSize sz);
    virtual QSize getImageSize();

    virtual bool config(QString cfg);
    virtual QPixmap generate();
    virtual void update();
};

#endif // TESTPLUGIN_H
