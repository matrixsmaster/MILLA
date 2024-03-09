#ifndef ANNACFGDIALOG_H
#define ANNACFGDIALOG_H

#include <QDialog>

namespace Ui {
class AnnaCfgDialog;
}

class AnnaCfgDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AnnaCfgDialog(QWidget *parent = nullptr);
    ~AnnaCfgDialog();

private:
    Ui::AnnaCfgDialog *ui;
};

#endif // ANNACFGDIALOG_H
