#include "pluginloader.h"

MillaPluginLoader::MillaPluginLoader() : QObject()
{
    QDir pluginsDir(QApplication::applicationDirPath());
    pluginsDir.cd(MILLA_PLUGIN_RELPATH);

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

void MillaPluginLoader::addPluginsToMenu(QMenu &m, PluginCB mcb, ProgressCB pcb)
{
    for (auto &i : plugins) {
        if (!i.second->init()) {
            qDebug() << "[PLUGINS] Unable to initialize plugin " << i.first;
            continue;
        }

        QAction* a = m.addAction(i.second->getPluginName());
        if (!a) {
            qDebug() << "[PLUGINS] Unable to create GUI action";
            break;
        }

        a->setToolTip(i.second->getPluginDesc());
        if (i.second->isContinous()) a->setCheckable(true);
        i.second->setProgressCB(pcb);

        connect(a,&QAction::triggered,this,[a,i,this] { this->pluginCallback(i.first,a); });
    }

    menu_cb = mcb;
}

void MillaPluginLoader::pluginCallback(QString name, QAction* sender)
{
    if (plugins.count(name) && sender && menu_cb) menu_cb(plugins.at(name),sender);
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
