#include "exportform.h"
#include "ui_exportform.h"

ExportForm::ExportForm(bool forImport, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ExportForm)
{
    ui->setupUi(this);

    if (forImport) {
        this->setWindowTitle("Import image data");
        ui->groupBox_3->setTitle("Input options");
//        ui->checkBox_11->setEnabled(false);
        ui->radioButton_4->setVisible(false);
    } else
        ui->checkBox_13->setEnabled(false);
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

void ExportForm::on_groupBox_toggled(bool arg1)
{
    if (!arg1) return;
    ui->groupBox_2->setChecked(false);
    ui->groupBox_4->setChecked(false);
    ui->groupBox_5->setChecked(false);
    ui->lineEdit->setText(";");
}

void ExportForm::on_groupBox_2_toggled(bool arg1)
{
    if (!arg1) return;
    ui->groupBox->setChecked(false);
    ui->groupBox_4->setChecked(false);
    ui->groupBox_5->setChecked(false);
    ui->lineEdit->setText(";");
}

void ExportForm::on_groupBox_4_toggled(bool arg1)
{
    if (!arg1) return;
    ui->groupBox->setChecked(false);
    ui->groupBox_2->setChecked(false);
    ui->groupBox_5->setChecked(false);
    ui->lineEdit->setText("|");
}

void ExportForm::on_groupBox_5_toggled(bool arg1)
{
    if (!arg1) return;
    ui->groupBox->setChecked(false);
    ui->groupBox_2->setChecked(false);
    ui->groupBox_4->setChecked(false);
    ui->lineEdit->setText(";");
}

void ExportForm::on_buttonBox_accepted()
{
    edata.loaded_only = ui->checkBox_11->isChecked();
    edata.header = ui->checkBox_12->isChecked();

    edata.table = IMPEXP_NONE;
    if (ui->groupBox->isChecked()) edata.table = IMPEXP_STATS;
    else if (ui->groupBox_2->isChecked()) edata.table = IMPEXP_TAGS;
    else if (ui->groupBox_4->isChecked()) edata.table = IMPEXP_STORIES;
    else if (ui->groupBox_5->isChecked()) edata.table = IMPEXP_LINKS;

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

    edata.story_update = ui->checkBox_14->isChecked();
    edata.story_title = ui->checkBox_15->isChecked();
    edata.story_actions = ui->checkBox_16->isChecked();

    edata.link_created = ui->checkBox_17->isChecked();
    edata.link_left = ui->checkBox_18->isChecked();
    edata.link_right = ui->checkBox_19->isChecked();

    if (ui->radioButton_4->isChecked()) edata.separator = '\n';
    else if (ui->lineEdit->text().isEmpty()) edata.separator = '\t';
    else edata.separator = ui->lineEdit->text().at(0).toLatin1();

    edata.imp_noover = ui->checkBox_13->isChecked();
}
