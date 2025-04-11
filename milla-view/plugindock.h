#ifndef PLUGINDOCK_H
#define PLUGINDOCK_H

#include <QDialog>

namespace Ui {
class PluginDock;
}

class PluginDock : public QDialog
{
    Q_OBJECT

public:
    explicit PluginDock(QWidget *parent = nullptr, QWidget* child = nullptr);
    ~PluginDock();

private:
    Ui::PluginDock *ui;
    QWidget* docked;
};

#endif // PLUGINDOCK_H
