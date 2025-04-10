#include "lifegendlg.h"
#include "ui_lifegendlg.h"

LifeGenDlg::LifeGenDlg(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::LifeGenDlg)
{
    ui->setupUi(this);
}

LifeGenDlg::~LifeGenDlg()
{
    delete ui;
}
