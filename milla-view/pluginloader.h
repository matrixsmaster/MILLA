#ifndef MILLAPLUGINLOADER_H
#define MILLAPLUGINLOADER_H

#include <QApplication>
#include <QPluginLoader>
#include <QJsonObject>
#include <QDebug>
#include <QDir>
#include <QList>
#include <QObject>
#include <QVariant>
#include <QMenu>
#include <QAction>
#include <QTimer>
#include <QScrollArea>
#include <QLabel>
#include <QMainWindow>
#include <map>
#include "plugins.h"

struct MillaPluginContext {
    MImageListRecord* current;
    QScrollArea* area;
    QLabel* widget;
    QMainWindow* window;

    bool valid() const {
        return (current && area && widget && window);
    }
};

class MillaPluginLoader : public QObject
{
    Q_OBJECT

public:
    MillaPluginLoader();
    virtual ~MillaPluginLoader();

    void setViewerContext(MillaPluginContext const &ctx) { context = ctx; }

    void setForceUI(bool f) { forceUI = f; }

    void addPluginsToMenu(QMenu &m, ProgressCB pcb);

    QString listPlugins();

    void updateSupportedFileFormats(QStringList &lst);

    bool openFileFormat(QString const &fn);

    void repeatLastPlugin();

private:
    MillaPluginContext context;
    std::pair<QString,QAction*> last_plugin;
    bool forceUI = false;

    std::map<QString,MillaGenericPlugin*> plugins;
    std::map<QString,QAction*> actions;
    std::map<MillaGenericPlugin*,QTimer> timers;
    std::map<MillaGenericPlugin*,std::pair<QObjectPtr,QObjectPtr>> filters;

    void pluginAction(QString name, QAction* sender);
    void pluginTimedOut(MillaGenericPlugin* plug);
    QVariant pluginConfigCallback(MillaGenericPlugin* plug, QString const &key, QVariant const &val);
};

#endif // MILLAPLUGINLOADER_H
