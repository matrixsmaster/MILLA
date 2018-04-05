#ifndef LIFEGENPLUGIN_H
#define LIFEGENPLUGIN_H

#include <QImage>
#include <QPoint>
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

    bool init();
    bool finalize();

    void showUI();
    void setConfigCB(PlugConfCB)     {}
    void setProgressCB(ProgressCB)   {}

    QVariant getParam(QString key);
    bool setParam(QString key, QVariant val);

    QVariant action(QVariant in);

private:
    QImage field;

    void randomInit(QSize sz);
    void singleStep();
    bool alive(QImage &ref, QPoint const &p);
    void kill(QImage &ref, QPoint const &p);
    void born(QImage &ref, QPoint const &p);
    void fade(QImage &from, QImage &to, QPoint const &p);
    int neighbours(QPoint const &p);
};

#endif // LIFEGENPLUGIN_H
