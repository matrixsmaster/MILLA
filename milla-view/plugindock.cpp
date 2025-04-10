#include "plugindock.h"
#include "ui_plugindock.h"

PluginDock::PluginDock(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::PluginDock)
{
    ui->setupUi(this);
}

PluginDock::~PluginDock()
{
    delete ui;
}
