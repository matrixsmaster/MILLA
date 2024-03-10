#include <QFileDialog>
#include "annacfgdialog.h"
#include "ui_annacfgdialog.h"
#include "annaplugin.h"

AnnaCfgDialog::AnnaCfgDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AnnaCfgDialog)
{
    ui->setupUi(this);
}

AnnaCfgDialog::~AnnaCfgDialog()
{
    delete ui;
}

void AnnaCfgDialog::updateConfig(AnnaConfig *cfg)
{
    if (!cfg) return;

    strncpy(cfg->params.model,ui->fnModel->text().toStdString().c_str(),sizeof(cfg->params.model)-1);
    strncpy(cfg->params.prompt,ui->prompt->toPlainText().toStdString().c_str(),sizeof(cfg->params.prompt)-1);

    AnnaPluginExtra* ex = (AnnaPluginExtra*)(cfg->user);
    ex->vision_file = ui->fnVision->text().toStdString();
    ex->ai_prefix = ui->prefAi->text().toStdString();
    ex->usr_prefix = ui->prefUsr->text().toStdString();
}

void AnnaCfgDialog::on_pushButton_clicked()
{
    QString fn = QFileDialog::getOpenFileName(this,"Open LLM file","","GGUF files (*.gguf)");
    if (!fn.isEmpty()) ui->fnModel->setText(fn);
}

void AnnaCfgDialog::on_pushButton_2_clicked()
{
    QString fn = QFileDialog::getOpenFileName(this,"Open vision encoder file","","GGUF files (*.gguf)");
    if (!fn.isEmpty()) ui->fnVision->setText(fn);
}
