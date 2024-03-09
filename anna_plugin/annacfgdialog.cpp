#include "annacfgdialog.h"
#include "ui_annacfgdialog.h"

AnnaCfgDialog::AnnaCfgDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AnnaCfgDialog)
{
    ui->setupUi(this);
}

AnnaCfgDialog::~AnnaCfgDialog()
{
    delete ui;
}
