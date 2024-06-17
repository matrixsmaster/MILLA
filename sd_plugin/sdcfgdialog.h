#ifndef SDCFGDIALOG_H
#define SDCFGDIALOG_H

#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui { class SDCfgDialog; }
QT_END_NAMESPACE

class SDCfgDialog : public QDialog
{
    Q_OBJECT

public:
    SDCfgDialog(QWidget *parent = nullptr);
    ~SDCfgDialog();

private:
    Ui::SDCfgDialog *ui;
};
#endif // SDCFGDIALOG_H
