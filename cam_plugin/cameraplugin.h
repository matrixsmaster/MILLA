#ifndef CAMERAPLUGIN_H
#define CAMERAPLUGIN_H

#include <QObject>
#include <opencv2/opencv.hpp>
#include "plugins.h"

#define LINUX_CAMERA_NODE "/dev/video"
#define MILLA_CAMERA_FPS 20

class CameraPlugin : public QObject, public MillaGenericPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID MILLA_PLUGIN_LID FILE "camplugin.json")
    Q_INTERFACES(MillaGenericPlugin)

public:
    CameraPlugin(QObject *parent = nullptr);
    virtual ~CameraPlugin();

    QString getPluginName()  { return "WebCamera"; }
    QString getPluginDesc()  { return "A simple plugin to capture web camera video stream."; }

    bool isContinous() const { return true; }

    MillaPluginContentType inputContent() const  { return MILLA_CONTENT_NONE; }
    MillaPluginContentType outputContent() const { return MILLA_CONTENT_IMAGE; }

    bool init();
    bool finalize();

    void showUI();
    void setConfigCB(PlugConfCB)        {}
    void setProgressCB(ProgressCB)      {}

    QVariant getParam(QString key);
    bool setParam(QString key, QVariant val);

    QVariant action(QVariant in);

private:
    cv::VideoCapture* camera = nullptr;
    int maxcamnum = 0, cameraidx = 0;
};

#endif // CAMERAPLUGIN_H
