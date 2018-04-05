#ifndef LIFEGENPLUGIN_H
#define LIFEGENPLUGIN_H

#include <QImage>
#include "plugins.h"

#define LIFEGEN_UPDATE 100

class LifeGenPlugin : public QObject, public MillaGenericPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID MILLA_PLUGIN_LID FILE "lifegenerator.json")
    Q_INTERFACES(MillaGenericPlugin)

public:
    LifeGenPlugin(QObject *parent = 0);
    virtual ~LifeGenPlugin() {}

    QString getPluginName() { return "LifeGen"; }
    QString getPluginDesc() { return "A plugin showing Conway's Game Of Life."; }

    bool isFilter()         { return false; }
    bool isContinous()      { return true; }

    bool init()             { return true; }
    bool finalize()         { return true; }

    void showUI();
    void setConfigCB(PlugConfCB cb)     {}
    void setProgressCB(ProgressCB cb)   {}

    QVariant getParam(QString key);
    bool setParam(QString key, QVariant val);

    QVariant action(QVariant in);

private:
    QImage field;
};

#endif // LIFEGENPLUGIN_H
