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
    bool ok;

    sdata.rating = ui->spinBox->value();
    sdata.kudos = ui->spinBox_5->value();

    sdata.minviews = ui->spinBox_2->value();
    sdata.maxviews = ui->spinBox_6->value();

    sdata.minface = ui->spinBox_3->value();
    sdata.maxface = ui->spinBox_4->value();

    sdata.colors = ui->radioButton_2->isChecked()? 1 : ((ui->radioButton_9->isChecked())? -1 : 0);

    sdata.minmtime = (ui->checkBox_3->isChecked()? 0 : QDateTime(ui->calendarWidget->selectedDate()).toTime_t());
    sdata.maxmtime = (ui->checkBox_3->isChecked()? -1 : QDateTime(ui->calendarWidget_2->selectedDate()).toTime_t());

    sdata.minsize = getSize(ui->comboBox,ok);
    sdata.maxsize = getSize(ui->comboBox_2,ok);
    if (!ok) sdata.maxsize = -1;

    sdata.wo_tags = ui->checkBox->isChecked();
    sdata.w_notes = ui->checkBox_4->isChecked();
    sdata.linked_only = ui->checkBox_2->isChecked();

    sdata.sort = SRFRM_NONE;
    if (ui->radioButton_3->isChecked()) sdata.sort = SRFRM_RATING;
    if (ui->radioButton_10->isChecked()) sdata.sort = SRFRM_KUDOS;
    if (ui->radioButton_5->isChecked()) sdata.sort = SRFRM_VIEWS;
    if (ui->radioButton_6->isChecked()) sdata.sort = SRFRM_DATE;
    if (ui->radioButton_7->isChecked()) sdata.sort = SRFRM_NAME;
    if (ui->radioButton_8->isChecked()) sdata.sort = SRFRM_FACES;

    sdata.maxresults = ui->spinBox_7->value();
}

size_t SearchForm::getSize(QComboBox* box, bool &ok)
{
    ok = false;
    if (!box) return 0;

    QRegExp ex(".+\\((\\d+)x(\\d+)\\)");
    int pos = ex.indexIn(box->currentText());
    if (pos >= 0) {
        size_t a = ex.cap(1).toUInt();
        size_t b = ex.cap(2).toUInt();
        qDebug() << "a = " << a << "; b = " << b;
        ok = true;
        return a*b;
    }

    return 0;
}
