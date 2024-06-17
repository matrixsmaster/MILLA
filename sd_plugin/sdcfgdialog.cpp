#include "sdcfgdialog.h"
#include "ui_sdcfgdialog.h"

SDCfgDialog::SDCfgDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SDCfgDialog)
{
    ui->setupUi(this);
}

SDCfgDialog::~SDCfgDialog()
{
    delete ui;
}

