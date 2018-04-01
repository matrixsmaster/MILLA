#include "pluginloader.h"

MillaPluginLoader::MillaPluginLoader()
{
    QDir pluginsDir(QApplication::applicationDirPath());
    pluginsDir.cd("plugins");

    for (auto &i : pluginsDir.entryList(QDir::Files)) {
        QPluginLoader loader(pluginsDir.absoluteFilePath(i));
        QObject* plugin = loader.instance();
        if (!plugin) continue;

        QJsonObject meta = loader.metaData();
        if (!meta.keys().contains("Name")) {
            qDebug() << "[PLUGINS] Plugin " << loader.fileName() << " have no Name tag (" << meta.keys().join(',') << ").";
            continue;
        }

        MillaGeneratorPlugin* gen = qobject_cast<MillaGeneratorPlugin*>(plugin);
        if (gen) generators[meta["Name"].toString()] = gen;
    }

    qDebug() << "[PLUGINS] " << generators.size() << " generators loaded";
}

MillaPluginLoader::~MillaPluginLoader()
{
    //unloads will be called automatically
}

QStringList MillaPluginLoader::getGeneratorsNames()
{
    QStringList lst;
    for (auto &i : generators) lst.push_back(i.first);
    return lst;
}
