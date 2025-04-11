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
    explicit PluginDock(QWidget *parent = nullptr);
    ~PluginDock();

    QLayout *getDockLayout();

private:
    Ui::PluginDock *ui;
};

#endif // PLUGINDOCK_H
