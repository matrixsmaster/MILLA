#include "pluginloader.h"
#include "mviewer.h"

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

    last_plugin = std::pair<QString,QAction*>(QString(),nullptr);

    qDebug() << "[PLUGINS] " << plugins.size() << " plugins loaded";
}

MillaPluginLoader::~MillaPluginLoader()
{
    for (auto &i : plugins) i.second->finalize();
    //unloads will be called automatically
    qDebug() << "[PLUGINS] Unloaded";
}

void MillaPluginLoader::addPluginsToMenu(QMenu &m, ProgressCB pcb)
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

        actions[i.second->getPluginName()] = a;
        a->setToolTip(i.second->getPluginDesc());
        m.setToolTipsVisible(true);
        if (i.second->isContinous()) a->setCheckable(true);
        i.second->setProgressCB(pcb);

        connect(a,&QAction::triggered,this,[a,i,this] { this->pluginAction(i.first,a); });
    }
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

void MillaPluginLoader::updateSupportedFileFormats(QStringList &lst)
{
    for (auto &i : plugins)
        if (i.second->isFileFormat()) {
            QVariant d(i.second->getParam("supported_formats"));
            if (!d.canConvert<QStringList>()) continue;
            for (auto &j : d.value<QStringList>()) {
                lst.push_back(j);
                formats[i.second].push_back(j);
            }
        }
}

bool MillaPluginLoader::openFileFormat(QString const &fn)
{
    QFileInfo fi(fn);
    if (!fi.exists()) return false;
    QString ext = fi.suffix().toLower();
    auto fmt = std::find_if(formats.begin(),formats.end(),[ext] (auto const &l) { return l.second.contains(ext); });
    if (fmt == formats.end()) return false;

    MillaGenericPlugin* plug = fmt->first;
    if (!plug->setParam("filename",fn) || !actions.count(plug->getPluginName())) return false;

    QAction* act = actions.at(plug->getPluginName());
    act->toggle();
    pluginAction(plug->getPluginName(),act);
    if (act->isCheckable() && !act->isChecked()) { //repeat action if it was disabled
        act->toggle();
        pluginAction(plug->getPluginName(),act);
    }
    return true;
}

void MillaPluginLoader::pluginAction(QString name, QAction* sender)
{
    //check validity of both context and plugin
    if (!plugins.count(name) || !sender || !context.valid()) return;

    MillaGenericPlugin* plug = plugins.at(name);
    if (!context.current->valid && plug->isFilter()) return; //don't waste our time

    MViewer* wnd = dynamic_cast<MViewer*>(context.window);
    if (!wnd) return;

    //determine whether plugin should use configuration callbacks
    QVariant cbf(plug->getParam("use_config_cb"));
    if (cbf.canConvert<bool>() && cbf.value<bool>())
        plug->setConfigCB([this,plug] (auto s, auto v) {
            return this->pluginConfigCallback(plug,s,v);
        });

    //determine if we need to show UI for this plugin now
    bool showui = true;
    if (!forceUI) {
        QVariant g(plug->getParam("show_ui"));
        if (!g.canConvert<bool>() || !g.value<bool>()) showui = false;
    }

    //prepare processing
    wnd->prepareLongProcessing();
    last_plugin = std::pair<QString,QAction*>(name,sender);
    QSize sz(context.area->width(),context.area->height());

    //process
    QPixmap out;
    if (plug->isFilter() && context.current->valid) { //Filter plugin

        //show UI if needed
        if (showui) plug->showUI();
        //ok, let's fire up some action
        QVariant r(plug->action(context.current->picture));
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
                    connect(&(timers[plug]),&QTimer::timeout,this,[this,plug] { this->pluginTimedOut(plug); });

                    //if plugin is timed, skip everything down there
                    wnd->prepareLongProcessing(true);
                    return;

                } else
                    qDebug() << "[PLUGINS] No update interval defined for " << plug->getPluginName();

                qDebug() << "[PLUGINS] " << plug->getPluginName() << " started";
            }
        }

        //fire up the generation process
        QVariant r(plug->action(sz));

        //grab the result if it's available and valid
        if (r.canConvert<QPixmap>()) out = r.value<QPixmap>();
    }

    wnd->prepareLongProcessing(true);
    if (!out.isNull()) wnd->showGeneratedPicture(out);
}

void MillaPluginLoader::pluginTimedOut(MillaGenericPlugin* plug)
{
    //qDebug() << "[PLUGINS] Timeout for " << plug->getPluginName();
    MViewer* wnd = dynamic_cast<MViewer*>(context.window);
    if (!wnd) return;
    //receive another "frame"
    QVariant r(plug->action(QSize(context.area->width(),context.area->height())));
    wnd->showGeneratedPicture((r.canConvert<QPixmap>())? r.value<QPixmap>() : QPixmap());
}

QVariant MillaPluginLoader::pluginConfigCallback(MillaGenericPlugin* plug, QString const &key, QVariant const &val)
{
    if (plug->isFilter()) return QVariant(); //for now, filter plugins doesn't support configuration callbacks

    MViewer* wnd = dynamic_cast<MViewer*>(context.window);
    if (!wnd) return QVariant();

    if (key == "get_left_image" && val.isNull()) {
        return (context.current->valid)? context.current->picture : QPixmap();

    } else if (key == "set_event_filter" && val.canConvert<QObjectPtr>()) {
        filters[plug] = std::pair<QObjectPtr,QObjectPtr>(context.area,val.value<QObjectPtr>());
        context.area->installEventFilter(val.value<QObjectPtr>());

        qDebug() << "[PLUGINS] Registered event filter for " << plug->getPluginName();
        return true;

    } else if (key == "load_key_value" && val.canConvert<QString>()) {
        QString res = DBHelper::getExtraStringVal(val.toString());
        qDebug() << "[PLUGINS] Plugin " << plug->getPluginName() << " requested a value for " << val.toString() << ": " << res;
        return res;

    } else if (key == "save_key_value" && val.canConvert<QString>()) {
        QStringList l = val.toString().split("=");
        if (l.size() != 2) return false;

        qDebug() << "[PLUGINS] Plugin " << plug->getPluginName() << " stores a value for " << l.at(0) << ": " << l.at(1);
        return DBHelper::setExtraStringVal(l.at(0),l.at(1));
    }
    return QVariant();
}

void MillaPluginLoader::repeatLastPlugin()
{
    if (last_plugin.first.isEmpty() || !last_plugin.second) return;

    //toggle checkbox before calling triggered() method
    if (last_plugin.second->isCheckable()) last_plugin.second->toggle();

    //now call main method
    pluginAction(last_plugin.first,last_plugin.second);
}
