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

    QString getPluginDescription(QString plugname);

    QStringList getGeneratorsNames();

    void addGeneratorsToMenu(QMenu &m);

private:
    std::map<QString,QString> descriptions;
    std::map<QString,MillaGeneratorPlugin*> generators;
};

#endif // MILLAPLUGINLOADER_H
