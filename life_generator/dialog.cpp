#include <QFileDialog>
#include "dialog.h"
#include "ui_dialog.h"

Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);
}

Dialog::~Dialog()
{
    delete ui;
}

void Dialog::on_buttonBox_accepted()
{
    if (ui->lineEdit->text().isEmpty()) return;

    QFile f(ui->lineEdit->text());
    if (!f.exists()) return;

    f.open(QIODevice::Text | QIODevice::ReadOnly);
    QString d = f.readAll();
    f.close();

    d.remove(QChar('\r'));
    dat = d.split(QChar('\n'),QString::SkipEmptyParts);
}

void Dialog::on_pushButton_clicked()
{
    ui->lineEdit->setText(QFileDialog::getOpenFileName(this, tr("Import from"), "", tr("Life 1.05 Files [txt,lif,life] (*.txt *.lif *.life)")));
}
