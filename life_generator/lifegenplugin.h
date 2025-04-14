#ifndef LIFEGENPLUGIN_H
#define LIFEGENPLUGIN_H

#include <QImage>
#include <QPoint>
#include <QStringList>
#include "plugins.h"
#include "plugindock.h"
#include "lifegendlg.h"

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
    bool isPresettable()     { return true; }

    MillaPluginContentType inputContent()  { return MILLA_CONTENT_NONE; }
    MillaPluginContentType outputContent() { return MILLA_CONTENT_IMAGE; }

    bool init();
    bool finalize();

    bool showUI(QDialog *dock);
    void setConfigCB(PlugConfCB cb)        { config_cb = cb; }
    void setProgressCB(ProgressCB)         {}

    QVariant getParam(QString key);
    bool setParam(QString key, QVariant val);

    QVariant action(QVariant in);

    void dockCallback(QString preset, int mode);

private:
    PlugConfCB config_cb = 0;
    QImage field;
    QString life_file;
    QStringList text_life;
    LifeGenDlg* dialog = 0;

    void attempLoadFile(QString fn);
    void randomInit(QSize const &sz);
    void imageInit(QSize const &sz, QPixmap const &in);
    void textInit(QSize const &sz);
    void singleStep();
    bool alive(QImage &ref, QPoint const &p);
    void kill(QImage &ref, QPoint const &p);
    void born(QImage &ref, QPoint const &p);
    void fade(QImage &from, QImage &to, QPoint const &p);
    int neighbours(QPoint const &p);

    bool LoadConfig(QString preset);
    bool SaveConfig(QString preset);
    void getConfigUI();
    void setConfigUI();
};

#endif // LIFEGENPLUGIN_H
