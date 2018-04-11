#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>

namespace Ui {
class PixMixCfgDialog;
}

class PixMixCfgDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PixMixCfgDialog(QWidget *parent = 0);
    ~PixMixCfgDialog();

    int getRadius();

private slots:
    void on_buttonBox_accepted();

private:
    Ui::PixMixCfgDialog *ui;
    int radius;
};

#endif // DIALOG_H
