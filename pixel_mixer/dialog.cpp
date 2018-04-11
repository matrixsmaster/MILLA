#include "dialog.h"
#include "ui_dialog.h"

PixMixCfgDialog::PixMixCfgDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PixMixCfgDialog)
{
    ui->setupUi(this);
    radius = ui->horizontalSlider->minimum();
}

PixMixCfgDialog::~PixMixCfgDialog()
{
    delete ui;
}

int PixMixCfgDialog::getRadius()
{
    return radius;
}

void PixMixCfgDialog::on_buttonBox_accepted()
{
    radius = ui->horizontalSlider->value();
}
