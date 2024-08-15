#ifndef LIFEGENPLUGIN_H
#define LIFEGENPLUGIN_H

#include <QImage>
#include <QPoint>
#include <QStringList>
#include "plugins.h"

#define LIFEGEN_UPDATE 100

class LifeGenPlugin : public QObject, public MillaGenericPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID MILLA_PLUGIN_LID FILE "lifegenerator.json")
    Q_INTERFACES(MillaGenericPlugin)

public:
    LifeGenPlugin(QObject *parent = nullptr);
    virtual ~LifeGenPlugin() {}

    QString getPluginName()  { return "LifeGen"; }
    QString getPluginDesc()  { return "A plugin showing Conway's Game Of Life."; }

    bool isContinous()       { return true; }

    MillaPluginContentType inputContent()  { return MILLA_CONTENT_NONE; }
    MillaPluginContentType outputContent() { return MILLA_CONTENT_IMAGE; }

    bool init();
    bool finalize();

    bool showUI();
    void setConfigCB(PlugConfCB cb)        { config_cb = cb; }
    void setProgressCB(ProgressCB)         {}

    QVariant getParam(QString key);
    bool setParam(QString key, QVariant val);

    QVariant action(QVariant in);

private:
    PlugConfCB config_cb = 0;
    QImage field;
    QStringList text_life;

    void randomInit(QSize const &sz);
    void imageInit(QSize const &sz, QPixmap const &in);
    void textInit(QSize const &sz);
    void singleStep();
    bool alive(QImage &ref, QPoint const &p);
    void kill(QImage &ref, QPoint const &p);
    void born(QImage &ref, QPoint const &p);
    void fade(QImage &from, QImage &to, QPoint const &p);
    int neighbours(QPoint const &p);
};

#endif // LIFEGENPLUGIN_H
