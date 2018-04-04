#ifndef TESTPLUGIN_H
#define TESTPLUGIN_H

#include "plugins.h"

class TestPlugin : public QObject, MillaGenericPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID MILLA_PLUGIN_LID FILE "testplugin.json")
    Q_INTERFACES(MillaGenericPlugin)

public:
    TestPlugin();
    virtual ~TestPlugin() {}

    QString getPluginName() { return "TestPlugin"; }
    QString getPluginDesc() { return "A simple test plugin."; }

    bool isFilter()         { return true; }
    bool isContinous()      { return false; }

    bool init();
    bool finalize();

    void setConfigCB(PlugConfCB cb)     { config_cb = cb; }
    void setProgressCB(ProgressCB cb)   { progress_cb = cb; }

    QVariant getParam(QString key);
    bool setParam(QString key, QVariant val);

    QVariant action(QVariant in);

private:
    PlugConfCB config_cb;
    ProgressCB progress_cb;
    double radius = 16;

    void core();
};

#endif // TESTPLUGIN_H
