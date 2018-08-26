#include <QDebug>
#include <QMessageBox>
#include "listeditor.h"
#include "ui_listeditor.h"

ListEditor::ListEditor(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ListEditor)
{
    ui->setupUi(this);
}

ListEditor::~ListEditor()
{
    delete ui;
}

void ListEditor::setTextLabel(QString const &s)
{
    ui->label->setText(s);
}

void ListEditor::setList(QStringList const &lst)
{
    for (auto &i : lst) ui->listWidget->addItem(i);
}

QStringList ListEditor::getList()
{
    QStringList l;
    for (int i = 0; i < ui->listWidget->count(); i++) {
        l.push_back(ui->listWidget->item(i)->text());
    }
    return l;
}

void ListEditor::on_pushButton_clicked()
{
    if (!ui->lineEdit->text().isEmpty())
        ui->listWidget->addItem(ui->lineEdit->text());
    ui->lineEdit->clear();
}

void ListEditor::on_pushButton_2_clicked()
{
    int i = ui->listWidget->currentRow();
    qDebug() << i;
    if (i >= 0 && i < ui->listWidget->count()) {
        QListWidgetItem* p = ui->listWidget->takeItem(i);
        delete p;
    }
}

void ListEditor::on_pushButton_3_clicked()
{
    if (ui->listWidget->count() < 1) return;
    if (QMessageBox::question(this, tr("Question"), tr("Are you sure you want to erase the whole list?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
        ui->listWidget->clear();
}
