#ifndef PLUGINS
#define PLUGINS

#include "shared.h"

#define MILLA_PLUGIN_RELPATH "../share/plugins"
#define MILLA_PLUGIN_LID "org.MatrixS_Master.MILLA.Plugins.Generic"

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define CONFIG_LOAD_PREP(P) \
    auto data = config_cb("load_key_value",preset); \
    if (!data.canConvert<QString>() || data.value<QString>().isEmpty()) return false; \
    QJsonDocument doc = QJsonDocument::fromJson(data.value<QString>().toUtf8()); \
    if (!doc.isObject()) return false;

#define CONFIG_LOAD_DONE(P)

#define CONFIG_SAVE_PREP(P) QJsonObject obj
#define CONFIG_SAVE_DONE(P) \
    QJsonDocument doc(obj); \
    config_cb("save_key_value",(P)+"="+doc.toJson());

#define CONFIG_LOAD_INT(V) if (doc.object().contains("" TOSTRING(V) "")) V = doc.object().value("" TOSTRING(V) "").toInt();
#define CONFIG_LOAD_FLOAT(V) if (doc.object().contains("" TOSTRING(V) "")) V = doc.object().value("" TOSTRING(V) "").toDouble();
#define CONFIG_LOAD_STR(V) if (doc.object().contains("" TOSTRING(V) "")) V = doc.object().value("" TOSTRING(V) "").toString();
#define CONFIG_LOAD_STDSTR(V) if (doc.object().contains("" TOSTRING(V) "")) V = doc.object().value("" TOSTRING(V) "").toString().toStdString();

#define CONFIG_SAVE_INT(V) obj["" TOSTRING(V) ""] = (int)(V);
#define CONFIG_SAVE_FLOAT(V) obj["" TOSTRING(V) ""] = (double)(V);
#define CONFIG_SAVE_STR(V) obj["" TOSTRING(V) ""] = V;
#define CONFIG_SAVE_STDSTR(V) obj["" TOSTRING(V) ""] = QString::fromStdString(V);

enum MillaPluginContentType {
    MILLA_CONTENT_NONE = 0,
    MILLA_CONTENT_IMAGE,
    MILLA_CONTENT_IMAGESET,
    MILLA_CONTENT_FILE,
    MILLA_CONTENT_TEXT_NOTES,
    MILLA_CONTENT_TEXT_STORY,
    MILLA_CONTENT_NUMTYPES
};

enum MillaPluginPresetAction {
    MILLA_PLUGINCB_ADD = 0,
    MILLA_PLUGINCB_DEL,
    MILLA_PLUGINCB_APPLY,
    MILLA_PLUGINCB_NUMACTIONS
};

class MillaGenericPlugin
{
public:
    virtual ~MillaGenericPlugin() {}

    virtual QString getPluginName() = 0;
    virtual QString getPluginDesc() = 0;

    virtual bool isContinous() = 0;
    virtual bool isPresettable() = 0;

    virtual MillaPluginContentType inputContent() = 0;
    virtual MillaPluginContentType outputContent() = 0;

    virtual bool init() = 0;
    virtual bool finalize() = 0;

    virtual bool showUI(QDialog* dock) = 0;
    virtual void setConfigCB(PlugConfCB cb) = 0;
    virtual void setProgressCB(ProgressCB cb) = 0;

    virtual QVariant getParam(QString key) = 0;
    virtual bool setParam(QString key, QVariant val) = 0;

    virtual QVariant action(QVariant in) = 0;
};

Q_DECLARE_INTERFACE(MillaGenericPlugin, MILLA_PLUGIN_LID)

#endif // PLUGINS
