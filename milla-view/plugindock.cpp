#include "plugins.h"
#include "plugindock.h"
#include "ui_plugindock.h"

PluginDock::PluginDock(QWidget *parent, QString title)
    : QDialog(parent)
    , ui(new Ui::PluginDock)
{
    ui->setupUi(this);
    if (!title.isEmpty()) setWindowTitle(title);
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

void PluginDock::setPresets(QStringList lst)
{
    ui->presetName->clear();
    if (lst.isEmpty()) ui->presetName->addItem(MILLA_PLUG_DEF_PRESET);
    else ui->presetName->addItems(lst);
    ui->presetName->setCurrentText(MILLA_PLUG_DEF_PRESET);
}

QStringList PluginDock::getPresets()
{
    QStringList lst;
    for (int i = 0; i < ui->presetName->count(); i++) lst.push_back(ui->presetName->itemText(i));
    return lst;
}

void PluginDock::on_addPreset_clicked()
{
    if (!control_cb) return;
    control_cb(ui->presetName->currentText(),MILLA_PLUGINCB_ADD);
    if (ui->presetName->findText(ui->presetName->currentText()) < 0)
        ui->presetName->addItem(ui->presetName->currentText());
}

void PluginDock::on_delPreset_clicked()
{
    if (!control_cb) return;
    control_cb(ui->presetName->currentText(),MILLA_PLUGINCB_DEL);
    int idx = ui->presetName->findText(ui->presetName->currentText());
    if (idx >= 0) ui->presetName->removeItem(idx);
}

void PluginDock::on_presetName_currentIndexChanged(int index)
{
    if (!control_cb) return;
    control_cb(ui->presetName->currentText(),MILLA_PLUGINCB_APPLY);
}
