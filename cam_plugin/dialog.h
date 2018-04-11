#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>

namespace Ui {
class CamCfgDialog;
}

class CamCfgDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CamCfgDialog(QWidget *parent = 0);
    ~CamCfgDialog();

    void setMaxID(int n);
    int getID();

private:
    Ui::CamCfgDialog *ui;
};

#endif // DIALOG_H
