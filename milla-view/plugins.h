#ifndef PLUGINS
#define PLUGINS

#include "shared.h"

#define MILLA_PLUGIN_LID "org.MatrixS_Master.MILLA.Plugins.Generic"

class MillaGenericPlugin
{
public:
    virtual ~MillaGenericPlugin() {}

    virtual QString getPluginName() = 0;
    virtual QString getPluginDesc() = 0;

    virtual bool isFilter() = 0;
    virtual bool isContinous() = 0;

    virtual bool init() = 0;
    virtual bool finalize() = 0;

    virtual void setConfigCB(PlugConfCB cb) = 0;
    virtual void setProgressCB(ProgressCB cb) = 0;

    virtual QVariant getParam(QString key) = 0;
    virtual bool setParam(QString key, QVariant val) = 0;

    virtual QVariant action(QVariant in) = 0;
};

Q_DECLARE_INTERFACE(MillaGenericPlugin, MILLA_PLUGIN_LID)

typedef std::function<void(MillaGenericPlugin*,QAction*)> PluginCB;

#endif // PLUGINS
