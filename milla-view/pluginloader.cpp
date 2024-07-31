#include "pluginloader.h"
#include "mviewer.h"

MillaPluginLoader::MillaPluginLoader() : QObject()
{
    QDir pluginsDir(QApplication::applicationDirPath());
    pluginsDir.cd(MILLA_PLUGIN_RELPATH);

    for (auto &i : pluginsDir.entryList(QDir::Files)) {
        qDebug() << "[PLUGINS] Checking " << pluginsDir.absoluteFilePath(i);
        QPluginLoader loader(pluginsDir.absoluteFilePath(i));
        QObject* plugin = loader.instance();
        if (!plugin) {
            qDebug() << "[PLUGINS] Error loading plugin: " << loader.errorString();
            continue;
        }

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
        if (i.second->inputContent() == MILLA_CONTENT_FILE) {
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
    if (!context.current->valid && plug->inputContent() == MILLA_CONTENT_IMAGE) return; //don't waste our time

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

    //show UI if needed
    if (showui) {
        if (sender->isCheckable())  {
            if (sender->isChecked()) plug->showUI();
        } else
            plug->showUI();
    }

    //process
    QVariant res;
    QPixmap out;
    bool skip_convert = false;

    switch (plug->inputContent()) {
    case MILLA_CONTENT_IMAGE: // Filters or image processors

        if (context.current->valid) {
            //ok, let's fire up some action
            res = plug->action(context.current->picture);
        }
        break;

    case MILLA_CONTENT_NONE: // Pure generators
    case MILLA_CONTENT_FILE: // File processors

        //if generator is continous, check if it is already enabled
        if (plug->isContinous()) {
            //if plugin is auto-firing, don't convert anything yet
            skip_convert = true;
            if (sender->isChecked()) //check was already toggled before this call
                startPlugin(plug,sender);
            else
                stopPlugin(plug,sender);
        }

        //fire up the generation process
        res = plug->action(sz);
        break;

    default:
        qDebug() << "[PLUGINS] Invalid input data type " << plug->inputContent() << " wanted by " << plug->getPluginName();
        skip_convert = true;
    }

    if (!skip_convert) {
        //present the result to the user
        switch (plug->outputContent()) {
        case MILLA_CONTENT_IMAGE:
            if (res.canConvert<QPixmap>()) out = res.value<QPixmap>();
            if (!out.isNull()) wnd->showGeneratedPicture(out);
            break;

        case MILLA_CONTENT_TEXT_NOTES:
            if (res.canConvert<QString>()) wnd->appendNotes(res.value<QString>());
            break;

        default:
            qDebug() << "[PLUGINS] Invalid output data type " << plug->outputContent() << " wanted by " << plug->getPluginName();
        }
    }

    //stop processing
    wnd->prepareLongProcessing(true);
}

bool MillaPluginLoader::startPlugin(MillaGenericPlugin* plug, QAction* sender)
{
    //startup sequence should NOT be changed in future
    QVariant d(plug->getParam("update_delay"));
    int di = (d.canConvert<int>())? d.value<int>() : 0;
    if (di > 0) {
        if (plug->setParam("process_started",true)) {
            qDebug() << "[PLUGINS] Starting timer with interval " << di;
            timers[plug].start(di); //timer created automatically by std::map
            connect(&(timers[plug]),&QTimer::timeout,this,[this,plug] { this->pluginTimedOut(plug); });
        } else {
            qDebug() << "[PLUGINS] Failed to start plugin " << plug->getPluginName();
            sender->setChecked(false);
            return false;
        }

    } else {
        qDebug() << "[PLUGINS] No update interval defined for " << plug->getPluginName();
        return false;
    }

    qDebug() << "[PLUGINS] " << plug->getPluginName() << " started";
    return true;
}

bool MillaPluginLoader::stopPlugin(MillaGenericPlugin* plug, QAction* /*sender*/)
{
    qDebug() << "[PLUGINS] Stopping plugin " << plug->getPluginName() << "...";

    plug->setParam("process_started",false); //ignore result

    //stop timer
    if (timers.count(plug)) {
        timers[plug].stop();
        disconnect(&(timers[plug]),&QTimer::timeout,nullptr,nullptr);
        timers.erase(plug);
        qDebug() << "[PLUGINS] Timer removed for " << plug->getPluginName();
    }

    //remove filter (if any)
    if (filters.count(plug)) {
        filters.at(plug).first->removeEventFilter(filters.at(plug).second);
        filters.erase(plug);
        qDebug() << "[PLUGINS] Event filter removed for " << plug->getPluginName();
    }

    qDebug() << "[PLUGINS] " << plug->getPluginName() << " stopped";
    return true;
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

void MillaPluginLoader::stopAllPlugins()
{
    for (auto &i : plugins) {
        //if (!i.second->isContinous()) continue;
        if (!stopPlugin(i.second,nullptr)) continue;

        QAction* a = actions[i.second->getPluginName()];
        if (a && a->isCheckable()) a->setChecked(false);
    }
}
