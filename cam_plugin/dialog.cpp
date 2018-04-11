#include "dialog.h"
#include "ui_dialog.h"

CamCfgDialog::CamCfgDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CamCfgDialog)
{
    ui->setupUi(this);
}

CamCfgDialog::~CamCfgDialog()
{
    delete ui;
}

void CamCfgDialog::setMaxID(int n)
{
    ui->spinBox->setMaximum(n);
}

int CamCfgDialog::getID()
{
    return ui->spinBox->value();
}
