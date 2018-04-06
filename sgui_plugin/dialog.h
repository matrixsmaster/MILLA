#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>

namespace Ui {
class Dialog;
}

struct SGUIPluginGUIRec {
    QString vfs_fn,startup;
    int timeout = 1000;
    bool valid = false;
};

class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = 0);
    ~Dialog();

    SGUIPluginGUIRec getInfo();

private slots:
    void on_pushButton_clicked();

    void on_buttonBox_accepted();

private:
    Ui::Dialog *ui;
};

#endif // DIALOG_H
