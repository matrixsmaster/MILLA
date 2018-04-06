#ifndef MILLAPLUGINLOADER_H
#define MILLAPLUGINLOADER_H

#include <QApplication>
#include <QPluginLoader>
#include <QJsonObject>
#include <QDebug>
#include <QDir>
#include <QList>
#include <QObject>
#include <QMenu>
#include <QAction>
#include <QTimer>
#include <map>
#include "plugins.h"

class MillaPluginLoader : public QObject
{
    Q_OBJECT

public:
    MillaPluginLoader();
    virtual ~MillaPluginLoader();

    void addPluginsToMenu(QMenu &m, PluginCB mcb, ProgressCB pcb);

    QString listPlugins();

    QPixmap pluginAction(bool forceUI, MImageListRecord const &pic, MillaGenericPlugin* plug, QAction* sender, QSize const &space, PlugConfCB f_config, PluginCB f_timeout);

private:
    PluginCB menu_cb;

    std::map<QString,MillaGenericPlugin*> plugins;
    std::map<MillaGenericPlugin*,QTimer> timers;

    void pluginCallback(QString name, QAction *sender);
};

#endif // MILLAPLUGINLOADER_H
