#include <QFileDialog>
#include "sdcfgdialog.h"
#include "ui_sdcfgdialog.h"

SDCfgDialog::SDCfgDialog(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SDCfgDialog)
{
    ui->setupUi(this);
    ui->seedVal->setMinimum(0x80000000);
    ui->seedVal->setMaximum(0x7fffffff);
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

void SDCfgDialog::on_pushButton_5_clicked()
{
    QString dr = QFileDialog::getExistingDirectory(this,"Select directory");
    if (!dr.isEmpty()) ui->savDir->setText(dr);
}

void SDCfgDialog::on_pushButton_8_clicked()
{
    QString fn = QFileDialog::getOpenFileName(this,"Select CLiP file","",SDPLUGIN_MODEL_FILTER);
    if (!fn.isEmpty()) ui->clipFile->setText(fn);
}

void SDCfgDialog::on_pushButton_9_clicked()
{
    QString fn = QFileDialog::getOpenFileName(this,"Select T5XXL file","",SDPLUGIN_MODEL_FILTER);
    if (!fn.isEmpty()) ui->t5xxlFile->setText(fn);
}

void SDCfgDialog::on_pushButton_10_clicked()
{
    QString dr = QFileDialog::getExistingDirectory(this,"Select LoRA directory");
    if (!dr.isEmpty()) ui->loraDir->setText(dr);
}
