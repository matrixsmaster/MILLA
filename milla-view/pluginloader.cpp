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

QPixmap MillaPluginLoader::pluginAction(bool forceUI, MImageListRecord const &pic, MillaGenericPlugin* plug, QAction* sender, QSize const &space, PlugConfCB f_config, PluginCB f_timeout)
{
    QPixmap out;

    //determine whether plugin should use configuration callbacks
    QVariant cbf(plug->getParam("use_config_cb"));
    if (cbf.canConvert<bool>() && cbf.value<bool>()) plug->setConfigCB(f_config);

    //determine if we need to show UI for this plugin now
    bool showui = true;
    if (!forceUI) {
        QVariant g(plug->getParam("show_ui"));
        if (!g.canConvert<bool>() || !g.value<bool>()) showui = false;
    }

    if (plug->isFilter() && pic.valid) { //Filter plugin

        //show UI if needed
        if (showui) plug->showUI();
        //ok, let's fire up some action
        QVariant r(plug->action(pic.picture));
        //and present the result to the user
        if (r.canConvert<QPixmap>()) out = r.value<QPixmap>();

    } else if (!plug->isFilter()) { //Generator plugin

        //if generator is continous, check if it is already enabled
        if (plug->isContinous()) {
            if (!sender->isChecked()) { //since check was toggled before this call, check is inverted
                //stop it
                if (timers.count(plug)) {
                    plug->setParam("process_started",false); //ignore result
                    timers[plug].stop();
                    disconnect(&(timers[plug]),&QTimer::timeout,nullptr,nullptr);
                    timers.erase(plug);
                }
                //remove filter (if any)
                if (filters.count(plug)) {
                    filters.at(plug).first->removeEventFilter(filters.at(plug).second);
                    filters.erase(plug);
                    qDebug() << "[PLUGINS] Event filter removed for " << plug->getPluginName();
                }
                qDebug() << "[PLUGINS] " << plug->getPluginName() << " stopped";

            } else { //startup sequence should NOT be changed in future
                //show UI if needed
                if (showui) plug->showUI();
                //start it
                QVariant d(plug->getParam("update_delay"));
                int di = (d.canConvert<int>())? d.value<int>() : 0;
                if (di > 0) {
                    plug->setParam("process_started",true); //ignore result
                    qDebug() << "[PLUGINS] Starting timer with interval " << di;
                    timers[plug].start(di); //timer created automatically by std::map
                    connect(&(timers[plug]),&QTimer::timeout,this,[plug,f_timeout] { f_timeout(plug,nullptr); });

                    //if plugin is timed, skip everything down there
                    return out;

                } else
                    qDebug() << "[PLUGINS] No update interval defined for " << plug->getPluginName();

                qDebug() << "[PLUGINS] " << plug->getPluginName() << " started";
            }
        }

        //fire up the generation process
        QVariant r(plug->action(space));

        //grab the result if it's available and valid
        if (r.canConvert<QPixmap>()) out = r.value<QPixmap>();
    }

    return out;
}

void MillaPluginLoader::addFilter(MillaGenericPlugin* plug, QObjectPtr obj, QObjectPtr flt)
{
    filters[plug] = std::pair<QObjectPtr,QObjectPtr>(obj,flt);
    obj->installEventFilter(flt);

    qDebug() << "[PLUGINS] Registered event filter for " << plug->getPluginName();
}
