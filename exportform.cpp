#include "exportform.h"
#include "ui_exportform.h"

ExportForm::ExportForm(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ExportForm)
{
    ui->setupUi(this);
}

ExportForm::~ExportForm()
{
    delete ui;
}

void ExportForm::on_radioButton_3_toggled(bool checked)
{
    if (checked) ui->frame->setEnabled(true);
}

void ExportForm::on_radioButton_4_toggled(bool checked)
{
    if (checked) ui->frame->setEnabled(false);
}

void ExportForm::on_groupBox_2_toggled(bool arg1)
{
    ui->groupBox->setChecked(!arg1);
}

void ExportForm::on_groupBox_toggled(bool arg1)
{
    ui->groupBox_2->setChecked(!arg1);
}

void ExportForm::on_buttonBox_accepted()
{
    edata.loaded_only = ui->checkBox_11->isChecked();

    edata.table = ui->groupBox->isChecked()? 1 : 2;

    edata.filename = ui->checkBox->isChecked();
    edata.views = ui->checkBox_2->isChecked();
    edata.rating = ui->checkBox_3->isChecked();
    edata.likes = ui->checkBox_4->isChecked();
    edata.tags = ui->checkBox_5->isChecked();
    edata.notes = ui->checkBox_6->isChecked();
    edata.sha = ui->checkBox_7->isChecked();
    edata.length = ui->checkBox_8->isChecked();

    edata.tagname = ui->checkBox_9->isChecked();
    edata.tagrate = ui->checkBox_10->isChecked();

    if (ui->radioButton_4->isChecked()) edata.separator = '\n';
    else if (ui->lineEdit->text().isEmpty()) edata.separator = '\t';
    else edata.separator = ui->lineEdit->text().at(0).toLatin1();
}
