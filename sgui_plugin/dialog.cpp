#include <QFileDialog>
#include "dialog.h"
#include "ui_dialog.h"

SGUICfgDialog::SGUICfgDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SGUICfgDialog)
{
    ui->setupUi(this);
}

SGUICfgDialog::~SGUICfgDialog()
{
    delete ui;
}

void SGUICfgDialog::on_pushButton_clicked()
{
    ui->lineEdit->setText(QFileDialog::getOpenFileName(this, tr("Select VFS file"), "", tr("Virtual File System (*.vfs *.VFS)")));
}

void SGUICfgDialog::on_buttonBox_accepted()
{
}

SGUIPluginGUIRec SGUICfgDialog::getInfo()
{
    SGUIPluginGUIRec r;

    r.vfs_fn = ui->lineEdit->text();
    r.startup = ui->lineEdit_2->text();
    r.timeout = ui->horizontalSlider->value();

    r.valid = true;
    return r;
}

void SGUICfgDialog::setFilename(QString const &s)
{
    ui->lineEdit->setText(s);
}

void SGUICfgDialog::setScript(QString const &s)
{
    ui->lineEdit_2->setText(s);
}
