#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>

namespace Ui {
class SGUICfgDialog;
}

struct SGUIPluginGUIRec {
    QString vfs_fn,startup;
    int timeout = 1000;
    bool valid = false;
};

class SGUICfgDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SGUICfgDialog(QWidget *parent = 0);
    ~SGUICfgDialog();

    SGUIPluginGUIRec getInfo();

    void setFilename(QString const &s);

    void setScript(QString const &s);

private slots:
    void on_pushButton_clicked();

    void on_buttonBox_accepted();

private:
    Ui::SGUICfgDialog *ui;
};

#endif // DIALOG_H
