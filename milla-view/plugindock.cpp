#include <QDebug>
#include "plugindock.h"
#include "ui_plugindock.h"
//#include "ui_testform.h"

PluginDock::PluginDock(QWidget *parent, QWidget *child)
    : QDialog(parent)
    , ui(new Ui::PluginDock)
    , docked(child)
{
    ui->setupUi(this);

    if (child) ui->verticalLayout->addWidget(child);
}

PluginDock::~PluginDock()
{
    //if (docked) ui->verticalLayout->removeWidget(docked);
    if (docked) {
        ui->verticalLayout->removeItem(ui->verticalLayout->itemAt(ui->verticalLayout->indexOf(docked)));
        for (auto i : ui->verticalLayout->children()) {
            qDebug() << i;
        }
    }
    delete ui;
}
