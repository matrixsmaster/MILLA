#include "dialog.h"
#include "ui_dialog.h"

Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);
    radius = ui->horizontalSlider->minimum();
}

Dialog::~Dialog()
{
    delete ui;
}

int Dialog::getRadius()
{
    return radius;
}

void Dialog::on_buttonBox_accepted()
{
    radius = ui->horizontalSlider->value();
}