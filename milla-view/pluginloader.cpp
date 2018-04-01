#include "pluginloader.h"

MillaPluginLoader::MillaPluginLoader() : QObject()
{
    QDir pluginsDir(QApplication::applicationDirPath());
    pluginsDir.cd("plugins");

    for (auto &i : pluginsDir.entryList(QDir::Files)) {
        QPluginLoader loader(pluginsDir.absoluteFilePath(i));
        QObject* plugin = loader.instance();
        if (!plugin) continue;

        QJsonObject meta = loader.metaData()["MetaData"].toObject();
        if (meta.isEmpty()) continue;
        if (!meta.keys().contains("Name")) {
            qDebug() << "[PLUGINS] Plugin " << loader.fileName() << " have no Name tag (" << meta.keys().join(',') << ").";
            continue;
        }

        MillaGeneratorPlugin* gen = qobject_cast<MillaGeneratorPlugin*>(plugin);
        if (gen) {
            QString nm = meta["Name"].toString();
            descriptions[nm] = meta["ShortDesc"].toString();
            generators[nm] = gen;
            qDebug() << "[PLUGINS] Found " << nm << descriptions[nm];
        }
    }

    qDebug() << "[PLUGINS] " << generators.size() << " generators loaded";
}

MillaPluginLoader::~MillaPluginLoader()
{
    //unloads will be called automatically
    qDebug() << "[PLUGINS] Unloaded";
}

QString MillaPluginLoader::getPluginDescription(QString plugname)
{
    if (!descriptions.count(plugname)) return "Plugin metadata is invalid.";
    return descriptions.at(plugname);
}

QStringList MillaPluginLoader::getGeneratorsNames()
{
    QStringList lst;
    for (auto &i : generators) lst.push_back(i.first);
    return lst;
}

void MillaPluginLoader::addGeneratorsToMenu(QMenu &m)
{
    if (generators.empty()) return;

    for (auto &i : generators) {
        QAction* a = m.addAction(i.first);
        if (!a) continue;

        a->setToolTip(getPluginDescription(i.first));
        connect(a,&QAction::triggered,this,[i] { i.second->setImageSize(QSize(10,20)); }); //FIXME: debug only
    }
}
