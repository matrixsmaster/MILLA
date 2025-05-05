#include <QFileDialog>
#include <QDebug>
#include "sdcfgdialog.h"
#include "ui_sdcfgdialog.h"

SDCfgDialog::SDCfgDialog(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SDCfgDialog)
{
    ui->setupUi(this);
    ui->seedVal->setMinimum(0x80000000);
    ui->seedVal->setMaximum(0x7fffffff);
}

SDCfgDialog::~SDCfgDialog()
{
    delete ui;
}

void SDCfgDialog::on_pushButton_clicked()
{
    QString fn = QFileDialog::getOpenFileName(this,"Select SD model file","",SDPLUGIN_MODEL_FILTER);
    if (!fn.isEmpty()) ui->modelFile->setText(fn);
}

void SDCfgDialog::on_pushButton_2_clicked()
{
    QString fn = QFileDialog::getOpenFileName(this,"Select VAE file","",SDPLUGIN_MODEL_FILTER);
    if (!fn.isEmpty()) ui->vaeFile->setText(fn);
}

void SDCfgDialog::on_pushButton_4_clicked()
{
    QString fn = QFileDialog::getOpenFileName(this,"Select upscaler file","",SDPLUGIN_MODEL_FILTER);
    if (!fn.isEmpty()) ui->upscModel->setText(fn);
}

void SDCfgDialog::on_pushButton_5_clicked()
{
    QString dr = QFileDialog::getExistingDirectory(this,"Select directory");
    if (!dr.isEmpty()) ui->savDir->setText(dr);
}

void SDCfgDialog::on_pushButton_8_clicked()
{
    QString fn = QFileDialog::getOpenFileName(this,"Select CLiP file","",SDPLUGIN_MODEL_FILTER);
    if (!fn.isEmpty()) ui->clipFile->setText(fn);
}

void SDCfgDialog::on_pushButton_9_clicked()
{
    QString fn = QFileDialog::getOpenFileName(this,"Select T5XXL file","",SDPLUGIN_MODEL_FILTER);
    if (!fn.isEmpty()) ui->t5xxlFile->setText(fn);
}

void SDCfgDialog::on_pushButton_10_clicked()
{
    QString dr = QFileDialog::getExistingDirectory(this,"Select LoRA directory");
    if (!dr.isEmpty()) ui->loraDir->setText(dr);
}

void SDCfgDialog::on_pushButton_6_clicked()
{
    int n = ui->treeList->rowCount();
    ui->treeList->insertRow(n);
    for (int i = 0; i < ui->treeList->columnCount(); i++) {
        QTableWidgetItem* old = nullptr;
        if (n) old = ui->treeList->item(n-1,i);
        QString v = old? old->text() : "1";
        ui->treeList->setItem(n,i,new QTableWidgetItem(v));
    }
}

void SDCfgDialog::on_pushButton_7_clicked()
{
    ui->treeList->removeRow(ui->treeList->currentRow());
}

void SDCfgDialog::on_pushButton_13_clicked()
{
    ui->treeList->clearContents();
    ui->treeList->setRowCount(0);
}

void SDCfgDialog::on_pushButton_11_clicked()
{
    int n = ui->treeList->currentRow();
    if (n < 1) return;
    for (int i = 0; i < ui->treeList->columnCount(); i++) {
        auto p = ui->treeList->takeItem(n-1,i);
        ui->treeList->setItem(n-1,i,ui->treeList->takeItem(n,i));
        ui->treeList->setItem(n,i,p);
    }
}

void SDCfgDialog::on_pushButton_12_clicked()
{
    int n = ui->treeList->currentRow();
    if (n < 0 || n >= ui->treeList->rowCount()-1) return;
    for (int i = 0; i < ui->treeList->columnCount(); i++) {
        auto p = ui->treeList->takeItem(n+1,i);
        ui->treeList->setItem(n+1,i,ui->treeList->takeItem(n,i));
        ui->treeList->setItem(n,i,p);
    }
}

QStringList SDCfgDialog::getTreeTable()
{
    QStringList res;
    for (int i = 0; i < ui->treeList->rowCount(); i++) {
        QString acc;
        for (int j = 0; j < ui->treeList->columnCount(); j++) {
            QTableWidgetItem* itm = ui->treeList->item(i,j);
            if (!itm) {
                acc.clear();
                break;
            }
            if (itm->text().indexOf(QRegExp("[^-+0-9.,]+")) >= 0) {
                qDebug() << "[SDPlugin] ERROR: malformed table item '" << itm->text() << "'";
                acc.clear();
                break;
            }
            acc += itm->text() + ";";
            acc.replace(',','.'); // dot is The Only proper decimal separator
        }
        if (!acc.isEmpty()) res.append(acc);
    }
    return res;
}

void SDCfgDialog::setTreeTable(const QStringList &lst)
{
    int n = 0;
    for (auto &i : lst) {
        QStringList sub = i.split(';',Qt::SkipEmptyParts);
        if (sub.size() != ui->treeList->columnCount()) {
            qDebug() << "[SDPlugin] ERROR: malformed table row '" << i << "'";
            continue;
        }
        for (int j = 0; j < ui->treeList->columnCount(); j++) {
            QTableWidgetItem* itm = ui->treeList->item(n,j);
            if (itm) itm->setText(sub.at(j));
            else ui->treeList->setItem(n,j,new QTableWidgetItem(sub.at(j)));
        }
        n++;
    }
}
