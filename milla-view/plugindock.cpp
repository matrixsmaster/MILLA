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

void PluginDock::addContent(QWidget* child)
{
    if (child) ui->internalBox->addWidget(child);
}

void PluginDock::setCallbacks(PresetCB cb)
{
    control_cb = cb;
}
