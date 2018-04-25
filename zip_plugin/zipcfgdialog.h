#ifndef ZIPCFGDIALOG_H
#define ZIPCFGDIALOG_H

#include <QDialog>

namespace Ui {
class ZipCfgDialog;
}

class ZipCfgDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ZipCfgDialog(QWidget *parent = 0);
    ~ZipCfgDialog();

private:
    Ui::ZipCfgDialog *ui;
};

#endif // ZIPCFGDIALOG_H
