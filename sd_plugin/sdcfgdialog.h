#ifndef SDCFGDIALOG_H
#define SDCFGDIALOG_H

#include <QWidget>

#define SDPLUGIN_MODEL_FILTER "All files (*.*)"

namespace Ui {
class SDCfgDialog;
}

class SDCfgDialog : public QWidget
{
    Q_OBJECT

public:
    explicit SDCfgDialog(QWidget *parent = nullptr);
    ~SDCfgDialog();

    Ui::SDCfgDialog *ui;

private slots:
    void on_pushButton_clicked();
    void on_pushButton_2_clicked();
    void on_pushButton_4_clicked();
    void on_pushButton_5_clicked();
    void on_pushButton_8_clicked();
    void on_pushButton_9_clicked();
    void on_pushButton_10_clicked();
};

#endif // SDCFGDIALOG_H
