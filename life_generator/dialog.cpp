#include <QFileDialog>
#include "dialog.h"
#include "ui_dialog.h"

LifeCfgDialog::LifeCfgDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LifeCfgDialog)
{
    ui->setupUi(this);
}

LifeCfgDialog::~LifeCfgDialog()
{
    delete ui;
}

void LifeCfgDialog::on_buttonBox_accepted()
{
    if (ui->lineEdit->text().isEmpty()) return;

    QFile f(ui->lineEdit->text());
    if (!f.exists()) return;

    f.open(QIODevice::Text | QIODevice::ReadOnly);
    QString d = f.readAll();
    f.close();

    d.remove(QChar('\r'));
    dat = d.split(QChar('\n'),Qt::SkipEmptyParts);
}

void LifeCfgDialog::on_pushButton_clicked()
{
    ui->lineEdit->setText(QFileDialog::getOpenFileName(this,"Import from", "","Life 1.05 Files [txt,lif,life] (*.txt *.lif *.life)"));
}
