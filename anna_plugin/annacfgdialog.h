#ifndef ANNACFGDIALOG_H
#define ANNACFGDIALOG_H

#include <QDialog>
#include "brain.h"

namespace Ui {
class AnnaCfgDialog;
}

class AnnaCfgDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AnnaCfgDialog(QWidget *parent = nullptr);
    ~AnnaCfgDialog();

    void updateConfig(AnnaConfig* cfg);

private slots:
    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

private:
    Ui::AnnaCfgDialog *ui;
};

#endif // ANNACFGDIALOG_H
