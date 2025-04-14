#include "testform.h"
#include "ui_testform.h"

TestForm::TestForm(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::TestForm)
{
    ui->setupUi(this);
}

TestForm::~TestForm()
{
    delete ui;
}
