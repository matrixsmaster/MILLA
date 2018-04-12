#ifndef MOVPLUGIN_H
#define MOVPLUGIN_H

#include <QObject>
#include <QFileInfo>
#include <QMovie>
#include "plugins.h"

#define MILLA_ANIMATION_FPS 15

class MovPlugin : public QObject, public MillaGenericPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID MILLA_PLUGIN_LID FILE "movplugin.json")
    Q_INTERFACES(MillaGenericPlugin)

public:
    MovPlugin(QObject *parent = 0);
    virtual ~MovPlugin() {}

    QString getPluginName() { return "SilentMovie"; }
    QString getPluginDesc() { return "A silent animated clip player."; }

    bool isFilter()         { return false; }
    bool isContinous()      { return true; }
    bool isFileFormat()     { return true; }

    bool init();
    bool finalize();

    void showUI();
    void setConfigCB(PlugConfCB)        {}
    void setProgressCB(ProgressCB)      {}

    QVariant getParam(QString key);
    bool setParam(QString key, QVariant val);

    QVariant action(QVariant in);

private:
    QFileInfo clipfile;
    QMovie clip;
};

#endif // MOVPLUGIN_H
