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
    void setPresets(QStringList lst);
    QStringList getPresets();

private slots:
    void on_addPreset_clicked();

    void on_delPreset_clicked();

    void on_presetName_currentIndexChanged(int index);

private:
    Ui::PluginDock *ui;
    PresetCB control_cb = nullptr;
};

#endif // PLUGINDOCK_H
