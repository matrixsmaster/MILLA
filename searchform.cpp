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
    sdata.mtime_min = ui->calendarWidget->selectedDate();
    sdata.mtime_max = ui->calendarWidget_2->selectedDate();
}
