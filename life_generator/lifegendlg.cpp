#include "QFileDialog"
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

void LifeGenDlg::on_pushButton_clicked()
{
    ui->lineEdit->setText(QFileDialog::getOpenFileName(this,"Import from", "","Life 1.05 Files [txt,lif,life] (*.txt *.lif *.life)"));
}
