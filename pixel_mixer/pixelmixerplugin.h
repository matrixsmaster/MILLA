#ifndef PIXELMIXERPLUGIN_H
#define PIXELMIXERPLUGIN_H

#include "plugins.h"

class PixelMixerPlugin : public QObject, public MillaGenericPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID MILLA_PLUGIN_LID FILE "pixelmixer.json")
    Q_INTERFACES(MillaGenericPlugin)

public:
    PixelMixerPlugin();
    virtual ~PixelMixerPlugin() {}

    QString getPluginName() { return "PixelMixer"; }
    QString getPluginDesc() { return "A simple plugin mostly for testing purpose."; }

    bool isFilter()         { return true; }
    bool isContinous()      { return false; }

    bool init();
    bool finalize();

    void showUI();
    void setConfigCB(PlugConfCB cb)     { config_cb = cb; }
    void setProgressCB(ProgressCB cb)   { progress_cb = cb; }

    QVariant getParam(QString key);
    bool setParam(QString key, QVariant val);

    QVariant action(QVariant in);

private:
    PlugConfCB config_cb;
    ProgressCB progress_cb;
    double radius = 16;
    bool once = false;

    void core();
};

#endif // PIXELMIXERPLUGIN_H
