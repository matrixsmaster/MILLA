#ifndef SGUIPLUGIN_H
#define SGUIPLUGIN_H

#include "plugins.h"
#include "include/AbstractIO.h"
#include "include/SGUI.h"
#include "dialog.h"
#include "sguieventsink.h"
#include "vmouse.h"

class SGUIPlugin : public QObject, public AbstractIO, public MillaGenericPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID MILLA_PLUGIN_LID FILE "sguiplugin.json")
    Q_INTERFACES(MillaGenericPlugin)

public:
    SGUIPlugin(QObject *parent = 0);
    virtual ~SGUIPlugin();

    QString getPluginName() { return "SGUI"; }
    QString getPluginDesc() { return "A scriptable GUI add-on."; }

    bool isFilter()         { return false; }
    bool isContinous()      { return true; }

    bool init();
    bool finalize();

    void showUI();
    void setConfigCB(PlugConfCB cb)     { config_cb = cb; }
    void setProgressCB(ProgressCB)      {}

    QVariant getParam(QString key);
    bool setParam(QString key, QVariant val);

    QVariant action(QVariant);

    void getProperty(AIOPropertyType tp, int* ival, char** sval);
    bool setProperty(AIOPropertyType tp, int ival, const char* sval);

    bool PollEvent(AIOEvent* e);
    void DrawFrame(uchar* ptr);
    void MouseControl(AIOMouseControlKind k, bool local, int x, int y);

private:
    SGUI* sgui = nullptr;
    VFS* vfs = nullptr;
    SGUIPluginGUIRec info;
    QImage frame;
    bool autorepeat = true;
    AIOColorOrdering colorder = AIOCO_MEMBGRA;
    int screen_w, screen_h;
    PlugConfCB config_cb = 0;
    SGUIEventSink sink;
    VMouse mouse;

    void cleanUp();
    void fireUp();
};

#endif // SGUIPLUGIN_H
