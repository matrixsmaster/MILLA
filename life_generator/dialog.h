#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QStringList>

namespace Ui {
class LifeCfgDialog;
}

class LifeCfgDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LifeCfgDialog(QWidget *parent = 0);
    ~LifeCfgDialog();

    QStringList const& getData() { return dat; }

private slots:
    void on_buttonBox_accepted();

    void on_pushButton_clicked();

private:
    Ui::LifeCfgDialog *ui;
    QStringList dat;
};

#endif // DIALOG_H
