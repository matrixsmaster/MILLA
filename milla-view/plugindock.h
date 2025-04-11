#ifndef PLUGINDOCK_H
#define PLUGINDOCK_H

#include <QDialog>
#include "shared.h"

namespace Ui {
class PluginDock;
}

class PluginDock : public QDialog
{
    Q_OBJECT

public:
    explicit PluginDock(QWidget *parent = nullptr);
    ~PluginDock();

    void addContent(QWidget* child);
    void setCallbacks(PresetCB cb);

private:
    Ui::PluginDock *ui;
    PresetCB control_cb;
};

#endif // PLUGINDOCK_H
