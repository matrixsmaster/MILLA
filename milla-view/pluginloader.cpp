#include "pluginloader.h"

MillaPluginLoader::MillaPluginLoader() : QObject()
{
    QDir pluginsDir(QApplication::applicationDirPath());
    pluginsDir.cd("plugins");

    for (auto &i : pluginsDir.entryList(QDir::Files)) {
        QPluginLoader loader(pluginsDir.absoluteFilePath(i));
        QObject* plugin = loader.instance();
        if (!plugin) continue;

        MillaGenericPlugin* plug = qobject_cast<MillaGenericPlugin*>(plugin);
        if (!plug) continue;

        plugins[plug->getPluginName()] = plug;
        qDebug() << "[PLUGINS] Found " << plug->getPluginName();

    }

    qDebug() << "[PLUGINS] " << plugins.size() << " plugins loaded";
}

MillaPluginLoader::~MillaPluginLoader()
{
    for (auto &i : plugins) i.second->finalize();
    //unloads will be called automatically
    qDebug() << "[PLUGINS] Unloaded";
}

void MillaPluginLoader::addPluginsToMenu(QMenu &m, PluginCB mcb)
{
    for (auto &i : plugins) {
        QAction* a = m.addAction(i.second->getPluginName());
        if (!a) continue;

        i.second->init();
        a->setToolTip(i.second->getPluginDesc());
        if (i.second->isContinous()) a->setCheckable(true);

        connect(a,&QAction::triggered,this,[i,this] { this->pluginCallback(i.first); });
    }

    menu_cb = mcb;
}

void MillaPluginLoader::pluginCallback(QString sender)
{
    if (plugins.count(sender) && menu_cb) menu_cb(plugins.at(sender));
}

QString MillaPluginLoader::listPlugins()
{
    QString out;
    int n = 1;
    for (auto &i : plugins) {
        out += QString::asprintf("%02d) %s: %s\n",n++,i.first.toStdString().c_str(),i.second->getPluginDesc().toStdString().c_str());
    }
    return out;
}
