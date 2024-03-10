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

    QString getPluginName()  { return "PixelMixer"; }
    QString getPluginDesc()  { return "A simple plugin mostly for testing purpose."; }

    bool isContinous() const { return false; }

    MillaPluginContentType inputContent() const  { return MILLA_CONTENT_IMAGE; }
    MillaPluginContentType outputContent() const { return MILLA_CONTENT_IMAGE; }

    bool init();
    bool finalize();

    void showUI();
    void setConfigCB(PlugConfCB)        {}
    void setProgressCB(ProgressCB cb)   { progress_cb = cb; }

    QVariant getParam(QString key);
    bool setParam(QString key, QVariant val);

    QVariant action(QVariant in);

private:
    ProgressCB progress_cb = nullptr;
    double radius = 16;
    bool once = false;
};

#endif // PIXELMIXERPLUGIN_H
