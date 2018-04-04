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
#include <map>
#include "plugins.h"

class MillaPluginLoader : public QObject
{
    Q_OBJECT

public:
    MillaPluginLoader();
    virtual ~MillaPluginLoader();

    void addPluginsToMenu(QMenu &m, PluginCB mcb);

    QString listPlugins();

private:
    PluginCB menu_cb;

    std::map<QString,MillaGenericPlugin*> plugins;

    void pluginCallback(QString sender);
};

#endif // MILLAPLUGINLOADER_H
