#include "pluginloader.h"
#include "plugindock.h"
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
    finalizeAllPlugins();
    //unloads will be called automatically
    qDebug() << "[PLUGINS] Unloaded";
}

void MillaPluginLoader::addPluginsToMenu(QMenu &m, ProgressCB pcb)
{
    for (auto &i : plugins) {
        // it should be initializable, otherwise we can't use it
        if (!i.second->init()) {
            qDebug() << "[PLUGINS] Unable to initialize plugin " << i.first;
            continue;
        }

        // create a new menu action
        QAction* a = m.addAction(i.second->getPluginName());
        if (!a) {
            qDebug() << "[PLUGINS] Unable to create GUI action";
            break;
        }

        // setup the action with the plugin name, description and checkable status
        actions[i.second] = a;
        a->setToolTip(i.second->getPluginDesc());
        m.setToolTipsVisible(true);
        if (i.second->isContinous()) a->setCheckable(true);
        i.second->setProgressCB(pcb);

        // connect the new action to plugin action
        connect(a,&QAction::triggered,this,[a,i,this] { this->pluginAction(i.first,a); });

        // check for plugin's ability to work with presets
        if (i.second->isPresettable())
            preset_menus[i.second] = m.addMenu(i.second->getPluginName()+" presets");

        //determine whether plugin should use configuration callbacks
        QVariant cbf(i.second->getParam("use_config_cb"));
        if (cbf.canConvert<bool>() && cbf.value<bool>())
            i.second->setConfigCB([this,i] (auto s, auto v) {
                return this->pluginConfigCallback(i.second,s,v);
            });
    }

    updatePresetLists();
}

void MillaPluginLoader::updatePresetLists()
{
    for (auto &i : preset_menus) {
        i.second->clear();
        auto lst = listPresetsFor(i.first->getPluginName());
        for (auto &j : lst) {
            if (j.isEmpty()) continue;
            QAction* a = i.second->addAction(j);
            connect(a,&QAction::triggered,this,[i,j,this] { this->pluginPresetAction(j,i.first); });
        }
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

QStringList MillaPluginLoader::listPresetsFor(QString plugname)
{
    QString key = "Preset_" + plugname;
    QString val = DBHelper::getExtraStringVal(key);
    return val.split('|',Qt::SkipEmptyParts);
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
    if (!plug->setParam("filename",fn) || !actions.count(plug)) return false;

    // if can be toggled, then stop it first
    if (actions[plug]->isChecked()) stopPlugin(plug,nullptr);
    actions[plug]->setChecked(true);
    pluginAction(plug->getPluginName(),actions[plug]);

    return true;
}

void MillaPluginLoader::pluginAction(QString name, QAction* sender, bool skip_ui)
{
    //check validity of both context and plugin
    if (!plugins.count(name) || !sender || !context.valid()) return;

    MillaGenericPlugin* plug = plugins.at(name);
    if (!context.current->valid && plug->inputContent() == MILLA_CONTENT_IMAGE) return; //don't waste our time

    MViewer* wnd = dynamic_cast<MViewer*>(context.window);
    if (!wnd) return;

    //determine if we need to show UI for this plugin now
    bool showui = !skip_ui;
    if (!forceUI && !skip_ui) {
        QVariant g(plug->getParam("show_ui"));
        if (!g.canConvert<bool>() || !g.value<bool>()) showui = false;
    }

    //show UI if needed
    if (showui) {
        bool uiok = true;
        if (sender->isCheckable())  {
            if (sender->isChecked()) uiok = showConfig(plug);
        } else
            uiok = showConfig(plug);

        if (!uiok) {
            qDebug() << "[PLUGINS] Plugin action aborted by user";
            if (sender->isCheckable()) sender->setChecked(false);
            return;
        }
    }

    //prepare processing
    wnd->prepareLongProcessing();
    last_plugin = std::pair<QString,QAction*>(name,sender);
    QSize sz(context.area->width(),context.area->height());

    //process
    QVariant res;
    QPixmap out;
    bool skip_convert = false;

    //if generator is continous, check if it is already enabled
    if (plug->isContinous()) {
        //if plugin is auto-firing, don't convert anything yet
        skip_convert = true;
        if (sender->isChecked()) { //check was already toggled before this call
            // make sure all other continuous plugins are disabled first
            for (auto &i : plugins) {
                if (i.second != plug && i.second->isContinous()) stopPlugin(i.second,nullptr);
            }
            startPlugin(plug,sender);
        } else
            stopPlugin(plug,sender);
    }

    // decide the action argument based on requested input content
    switch (plug->inputContent()) {
    case MILLA_CONTENT_IMAGE: // Filters or image processors

        if (context.current->valid) {
            //ok, let's fire up some action
            res = plug->action(context.current->picture);
        }
        break;

    case MILLA_CONTENT_NONE: // Pure generators
    case MILLA_CONTENT_FILE: // File processors

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

void MillaPluginLoader::pluginPresetAction(QString name, MillaGenericPlugin* plug)
{
    if (!plug) return;
    if (!plug->setParam("apply_preset",name)) return;

    if (actions[plug]->isCheckable()) {
        // if can be toggled, then stop it first
        if (actions[plug]->isChecked()) stopPlugin(plug,nullptr);
        actions[plug]->setChecked(true);
    }
    pluginAction(plug->getPluginName(),actions[plug],true);
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

    //make sure the checkbox is unchecked
    QAction* a = actions[plug];
    if (a && a->isCheckable()) a->setChecked(false);

    qDebug() << "[PLUGINS] " << plug->getPluginName() << " stopped";
    return true;
}

bool MillaPluginLoader::showConfig(MillaGenericPlugin *plug)
{
    PluginDock dock(nullptr,plug->getPluginName());
    dock.setPresets(listPresetsFor(plug->getPluginName()));
    bool r = plug->showUI(&dock);

    if (plug->isPresettable()) {
        QString key = "Preset_" + plug->getPluginName();
        QString val = dock.getPresets().join('|');
        DBHelper::setExtraStringVal(key,val);
        updatePresetLists();
    }

    return r;
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

    } else if (key == "get_left_meta" && val.isNull()) {
        QVariant r;
        r.setValue(context.current? *context.current : MImageListRecord());
        return r;

    } else if (key == "set_event_filter" && val.canConvert<QObjectPtr>()) {
        QObjectPtr ptr = val.value<QObjectPtr>();
        if (!(filters.count(plug) && filters.at(plug).first == context.area && filters.at(plug).second == ptr)) {
            context.area->installEventFilter(ptr);
            filters[plug] = std::pair<QObjectPtr,QObjectPtr>(context.area,ptr);
            qDebug() << "[PLUGINS] Registered event filter for " << plug->getPluginName();
        }
        return true;

    } else if (key == "load_key_value" && val.canConvert<QString>()) {
        QString key = plug->getPluginName() + "_" + val.toString();
        QString res = DBHelper::getExtraStringVal(key);
        qDebug() << "[PLUGINS] Plugin " << plug->getPluginName() << " requested a value for " << val.toString() << ": " << res;
        return res;

    } else if (key == "save_key_value" && val.canConvert<QString>()) {
        QStringList l = val.toString().split("=");
        if (l.size() != 2) return false;
        QString key = plug->getPluginName() + "_" + l.at(0);
        qDebug() << "[PLUGINS] Plugin " << plug->getPluginName() << " stores a value for " << l.at(0) << ": " << l.at(1);
        return DBHelper::setExtraStringVal(key,l.at(1));

    } else if (key == "delete_key_value" && val.canConvert<QString>()) {
        QString key = plug->getPluginName() + "_" + val.toString();
        qDebug() << "[PLUGINS] Plugin " << plug->getPluginName() << " deletes key " << val.toString();
        return DBHelper::delExtraLine(key);

    } else if (key == "self_disable" && val.isNull()) {
        return stopPlugin(plug,nullptr);

    } else if (key == "show_message" && val.canConvert<QString>()) {
        MViewer* wnd = dynamic_cast<MViewer*>(context.window);
        if (wnd) wnd->showMessage(val.toString());
        return QVariant(bool(true));

    } else if (key == "long_processing_done" && val.canConvert<bool>()) {
        MViewer* wnd = dynamic_cast<MViewer*>(context.window);
        if (wnd) wnd->prepareLongProcessing(val.value<bool>());

    } else if (key == "is_long_processing" && val.isNull()) {
        MViewer* wnd = dynamic_cast<MViewer*>(context.window);
        return QVariant(bool(wnd? wnd->isInLongProcessing() : false));

    } else if (key == "index_new_file" && val.canConvert<QString>()) {
        QString fn = val.toString();
        MViewer* wnd = dynamic_cast<MViewer*>(context.window);
        if (!wnd) return QVariant();
        // TODO: if the file is in the current dir, add its thumbnails and other loadimage() stuff
        return QVariant(wnd->createStatRecord(fn,true));

    } else if (key == "get_all_tags" && val.isNull()) {
        //TODO

    } else if (key == "append_tags" && val.canConvert<QStringList>()) {
        //TODO

    } else if (key == "append_notes" && val.canConvert<QStringList>()) {
        QStringList lst = val.toStringList();
        MViewer* wnd = dynamic_cast<MViewer*>(context.window);
        if (!wnd) return QVariant();
        wnd->appendNotes(lst.at(1),lst.at(0));
        return QVariant(bool(true));
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
        stopPlugin(i.second,nullptr);
    }
}

void MillaPluginLoader::finalizeAllPlugins()
{
    //stopAllPlugins();
    for (auto &i : plugins) i.second->finalize();
}
