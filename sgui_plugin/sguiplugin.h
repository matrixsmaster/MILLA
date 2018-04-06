#ifndef SGUIPLUGIN_H
#define SGUIPLUGIN_H

#include "plugins.h"
#include "include/AbstractIO.h"

class SGUIPlugin : public QObject, public MillaGenericPlugin, public AbstractIO
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
    void setConfigCB(PlugConfCB cb)     {}
    void setProgressCB(ProgressCB)      {}

    QVariant getParam(QString key);
    bool setParam(QString key, QVariant val);

    QVariant action(QVariant in);

    void getProperty(AIOPropertyType tp, int* ival, char** sval);
    bool setProperty(AIOPropertyType tp, int ival, const char* sval);

    bool PollEvent(AIOEvent* e);
    void DrawFrame(uchar* ptr);
    void MouseControl(AIOMouseControlKind k, bool local, int x, int y);
};

#endif // SGUIPLUGIN_H
