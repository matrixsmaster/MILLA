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
    sdata.tags = ui->spinBox_8->value();

    sdata.minviews = ui->spinBox_2->value();
    sdata.maxviews = ui->spinBox_6->value();

    sdata.minface = ui->spinBox_3->value();
    sdata.maxface = ui->spinBox_4->value();

    sdata.colors = ui->radioButton_2->isChecked()? 1 : ((ui->radioButton_9->isChecked())? -1 : 0);
    sdata.orient = ui->radioButton_12->isChecked()? 1 : ((ui->radioButton_11->isChecked())? 0 : -1);

    sdata.liked = ui->doubleSpinBox->value();
    sdata.similar = ui->doubleSpinBox_2->value();

    sdata.minmtime = (ui->checkBox_3->isChecked()? 0 : QDateTime(ui->calendarWidget->selectedDate()).toTime_t());
    sdata.maxmtime = (ui->checkBox_3->isChecked()? 0 : QDateTime(ui->calendarWidget_2->selectedDate()).toTime_t());
    sdata.minstime = (ui->checkBox_5->isChecked()? 0 : QDateTime(ui->calendarWidget_3->selectedDate()).toTime_t());
    sdata.maxstime = (ui->checkBox_5->isChecked()? 0 : QDateTime(ui->calendarWidget_4->selectedDate()).toTime_t());

    sdata.minsize = getSize(ui->comboBox,ok);
    sdata.minsize_vi = ui->comboBox->currentIndex();
    sdata.maxsize = getSize(ui->comboBox_2,ok);
    sdata.maxsize_vi = ui->comboBox_2->currentIndex();
    if (!ok) sdata.maxsize = -1;

    sdata.text_fn = ui->lineEdit->text();
    sdata.text_path = ui->lineEdit_2->text();
    sdata.text_notes = ui->lineEdit_3->text();
    sdata.tags_inc = ui->lineEdit_4->text();
    sdata.tags_exc = ui->lineEdit_5->text();

    sdata.wo_tags = ui->checkBox->isChecked();
    sdata.w_notes = ui->checkBox_4->isChecked();

    sdata.scope = SRSCP_NEW;
    if (ui->radioButton_15->isChecked()) sdata.scope = SRSCP_FOUND;
    if (ui->radioButton_16->isChecked()) sdata.scope = SRSCP_LINKS;
    if (ui->radioButton_17->isChecked()) sdata.scope = SRSCP_CONTINUE;

    sdata.sort = SRFRM_NONE;
    if (ui->radioButton_3->isChecked()) sdata.sort = SRFRM_RATING;
    if (ui->radioButton_10->isChecked()) sdata.sort = SRFRM_KUDOS;
    if (ui->radioButton_5->isChecked()) sdata.sort = SRFRM_VIEWS;
    if (ui->radioButton_6->isChecked()) sdata.sort = SRFRM_DATE;
    if (ui->radioButton_7->isChecked()) sdata.sort = SRFRM_NAME;
    if (ui->radioButton_8->isChecked()) sdata.sort = SRFRM_FACES;
    if (ui->radioButton_4->isChecked()) sdata.sort = SRFRM_LASTSEEN;

    sdata.maxresults = ui->spinBox_7->value();
}

void SearchForm::set_data(SearchFormData const &ndata)
{
    ui->spinBox->setValue(ndata.rating);
    ui->spinBox_5->setValue(ndata.kudos);
    ui->spinBox_8->setValue(ndata.tags);

    ui->spinBox_2->setValue(ndata.minviews);
    ui->spinBox_6->setValue(ndata.maxviews);

    ui->spinBox_3->setValue(ndata.minface);
    ui->spinBox_4->setValue(ndata.maxface);

    switch (ndata.colors) {
    case -1: ui->radioButton_9->setChecked(true); break;
    case 0: ui->radioButton->setChecked(true); break;
    case 1: ui->radioButton_2->setChecked(true); break;
    }

    switch (ndata.orient) {
    case -1: ui->radioButton_13->setChecked(true); break;
    case 0: ui->radioButton_11->setChecked(true); break;
    case 1: ui->radioButton_12->setChecked(true); break;
    }

    ui->doubleSpinBox->setValue(ndata.liked);
    ui->doubleSpinBox_2->setValue(ndata.similar);

    if (ndata.minmtime) ui->calendarWidget->setSelectedDate(QDateTime::fromTime_t(ndata.minmtime).date());
    if (ndata.maxmtime) ui->calendarWidget_2->setSelectedDate(QDateTime::fromTime_t(ndata.maxmtime).date());
    ui->checkBox_3->setChecked(!ndata.minmtime && !ndata.maxmtime);

    if (ndata.minstime) ui->calendarWidget_3->setSelectedDate(QDateTime::fromTime_t(ndata.minstime).date());
    if (ndata.maxstime) ui->calendarWidget_4->setSelectedDate(QDateTime::fromTime_t(ndata.maxstime).date());
    ui->checkBox_5->setChecked(!ndata.minstime && !ndata.maxstime);

    ui->comboBox->setCurrentIndex(ndata.minsize_vi);
    ui->comboBox_2->setCurrentIndex(ndata.maxsize_vi);

    ui->lineEdit->setText(ndata.text_fn);
    ui->lineEdit_2->setText(ndata.text_path);
    ui->lineEdit_3->setText(ndata.text_notes);
    ui->lineEdit_4->setText(ndata.tags_inc);
    ui->lineEdit_5->setText(ndata.tags_exc);

    ui->checkBox->setChecked(ndata.wo_tags);
    ui->checkBox_4->setChecked(ndata.w_notes);

    switch (ndata.scope) {
    case SRSCP_NEW: ui->radioButton_14->setChecked(true); break;
    case SRSCP_FOUND: ui->radioButton_15->setChecked(true); break;
    case SRSCP_LINKS: ui->radioButton_16->setChecked(true); break;
    case SRSCP_CONTINUE: ui->radioButton_17->setChecked(true); break;
    }

    switch (ndata.sort) {
    default: ui->radioButton_3->setChecked(true); break;
    case SRFRM_KUDOS: ui->radioButton_10->setChecked(true); break;
    case SRFRM_VIEWS: ui->radioButton_5->setChecked(true); break;
    case SRFRM_DATE: ui->radioButton_6->setChecked(true); break;
    case SRFRM_NAME: ui->radioButton_7->setChecked(true); break;
    case SRFRM_FACES: ui->radioButton_8->setChecked(true); break;
    case SRFRM_LASTSEEN: ui->radioButton_4->setChecked(true); break;
    }

    ui->spinBox_7->setValue(ndata.maxresults);
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
