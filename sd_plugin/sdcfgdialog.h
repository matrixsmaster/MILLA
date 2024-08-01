#ifndef SDCFGDIALOG_H
#define SDCFGDIALOG_H

#include <QDialog>

#define SDPLUGIN_MODEL_FILTER "All files (*.*)"

QT_BEGIN_NAMESPACE
namespace Ui { class SDCfgDialog; }
QT_END_NAMESPACE

class SDCfgDialog : public QDialog
{
    Q_OBJECT

public:
    SDCfgDialog(QWidget *parent = nullptr);
    ~SDCfgDialog();

    Ui::SDCfgDialog *ui;
private slots:
    void on_pushButton_clicked();
    void on_pushButton_2_clicked();
    void on_pushButton_4_clicked();
};
#endif // SDCFGDIALOG_H
