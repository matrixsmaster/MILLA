#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>

namespace Ui {
class Dialog;
}

class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = 0);
    ~Dialog();

    int getRadius();

private slots:
    void on_buttonBox_accepted();

private:
    Ui::Dialog *ui;
    int radius;
};

#endif // DIALOG_H