#include <QFileDialog>
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

void SDCfgDialog::on_pushButton_clicked()
{
    QString fn = QFileDialog::getOpenFileName(this,"Select SD model file","",SDPLUGIN_MODEL_FILTER);
    if (!fn.isEmpty()) ui->modelFile->setText(fn);
}

void SDCfgDialog::on_pushButton_2_clicked()
{
    QString fn = QFileDialog::getOpenFileName(this,"Select VAE file","",SDPLUGIN_MODEL_FILTER);
    if (!fn.isEmpty()) ui->vaeFile->setText(fn);
}

void SDCfgDialog::on_pushButton_4_clicked()
{
    QString fn = QFileDialog::getOpenFileName(this,"Select upscaler file","",SDPLUGIN_MODEL_FILTER);
    if (!fn.isEmpty()) ui->upscModel->setText(fn);
}
