#include "movcfgdialog.h"
#include "ui_movcfgdialog.h"

MovCfgDialog::MovCfgDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MovCfgDialog)
{
    ui->setupUi(this);
}

MovCfgDialog::~MovCfgDialog()
{
    delete ui;
}
