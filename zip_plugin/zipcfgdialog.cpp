#include "zipcfgdialog.h"
#include "ui_zipcfgdialog.h"

ZipCfgDialog::ZipCfgDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ZipCfgDialog)
{
    ui->setupUi(this);
}

ZipCfgDialog::~ZipCfgDialog()
{
    delete ui;
}
