#ifndef PLUGINDOCK_H
#define PLUGINDOCK_H

#include <QDialog>
#include "testform.h"

namespace Ui {
class PluginDock;
}

class PluginDock : public QDialog
{
    Q_OBJECT

public:
    explicit PluginDock(QWidget *parent = nullptr);
    ~PluginDock();

private:
    Ui::PluginDock *ui;
    TestForm* frm;
};

#endif // PLUGINDOCK_H
