#include <QFileDialog>
#include "dialog.h"
#include "ui_dialog.h"

Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);
}

Dialog::~Dialog()
{
    delete ui;
}

void Dialog::on_pushButton_clicked()
{
    ui->lineEdit->setText(QFileDialog::getOpenFileName(this, tr("Select VFS file"), "", tr("Virtual File System (*.vfs *.VFS)")));
}

void Dialog::on_buttonBox_accepted()
{
}

SGUIPluginGUIRec Dialog::getInfo()
{
    SGUIPluginGUIRec r;

    r.vfs_fn = ui->lineEdit->text();
    r.startup = ui->lineEdit_2->text();
    r.timeout = ui->horizontalSlider->value();

    r.valid = true;
    return r;
}
