#include "plugindock.h"
#include "ui_plugindock.h"
//#include "ui_testform.h"

PluginDock::PluginDock(QWidget *parent, QWidget *child)
    : QDialog(parent)
    , ui(new Ui::PluginDock)
{
    ui->setupUi(this);
    //ui->widget->ui->fnModel->clear();
    //ui->widget = new TestForm(this);
    //frm = new TestForm(this);
    if (child) ui->verticalLayout->addWidget(child);
}

PluginDock::~PluginDock()
{
    delete ui;
}
