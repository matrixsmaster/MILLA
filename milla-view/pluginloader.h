#ifndef MILLAPLUGINLOADER_H
#define MILLAPLUGINLOADER_H

#include <QApplication>
#include <QPluginLoader>
#include <QJsonObject>
#include <QDebug>
#include <QDir>
#include <QList>
#include <map>
#include "plugins.h"

class MillaPluginLoader
{
public:
    MillaPluginLoader();
    virtual ~MillaPluginLoader();

    QStringList getGeneratorsNames();

private:
    std::map<QString,MillaGeneratorPlugin*> generators;
};

#endif // MILLAPLUGINLOADER_H
