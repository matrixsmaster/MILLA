#ifndef MOVPLUGIN_H
#define MOVPLUGIN_H

#include <QObject>
#include <QFileInfo>
#include <QMovie>
#include "plugins.h"
#include "movcfgdialog.h"

#define MILLAMOV_DEFAULT_FPS 15.f
#define MILLAMOV_DEFAULT_DELAY (1000.f / MILLAMOV_DEFAULT_FPS)
#define MILLAMOV_WHEEL_SENSITIVITY 100

class MovPlugin : public QObject, public MillaGenericPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID MILLA_PLUGIN_LID FILE "movplugin.json")
    Q_INTERFACES(MillaGenericPlugin)

public:
    MovPlugin(QObject *parent = nullptr);
    virtual ~MovPlugin();

    QString getPluginName() { return "SilentMovie"; }
    QString getPluginDesc() { return "A silent animated clip player."; }

    bool isFilter()         { return false; }
    bool isContinous()      { return true; }
    bool isFileFormat()     { return true; }
    bool isContainer()      { return false; }

    bool init();
    bool finalize();

    void showUI();
    void setConfigCB(PlugConfCB cb)     { config_cb = cb; }
    void setProgressCB(ProgressCB)      {}

    QVariant getParam(QString key);
    bool setParam(QString key, QVariant val);

    QVariant action(QVariant in);

protected:
    bool eventFilter(QObject *obj, QEvent *event);

private:
    QFileInfo clipfile;
    QMovie* clip = nullptr;
    PlugConfCB config_cb = 0;
    int delay = MILLAMOV_DEFAULT_DELAY;
    int percSpeed = 100;
    int cframe = 0;
};

#endif // MOVPLUGIN_H
