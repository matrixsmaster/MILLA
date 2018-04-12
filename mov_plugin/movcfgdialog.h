#ifndef MOVCFGDIALOG_H
#define MOVCFGDIALOG_H

#include <QDialog>

namespace Ui {
class MovCfgDialog;
}

class MovCfgDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MovCfgDialog(QWidget *parent = 0);
    ~MovCfgDialog();

private:
    Ui::MovCfgDialog *ui;
};

#endif // MOVCFGDIALOG_H
