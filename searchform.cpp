#include "searchform.h"
#include "ui_searchform.h"

SearchForm::SearchForm(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SearchForm)
{
    ui->setupUi(this);
}

SearchForm::~SearchForm()
{
    delete ui;
}

void SearchForm::on_buttonBox_accepted()
{
    sdata.rating = ui->spinBox->value();
    sdata.views = ui->spinBox_2->value();
    sdata.minface = ui->spinBox_3->value();
    sdata.maxface = ui->spinBox_4->value();
    sdata.grey = ui->radioButton_2->isChecked();
    sdata.mtime_min = QDateTime(ui->calendarWidget->selectedDate()).toTime_t();
    sdata.mtime_max = QDateTime(ui->calendarWidget_2->selectedDate()).toTime_t();
}
