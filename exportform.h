#ifndef EXPORTFORM_H
#define EXPORTFORM_H

#include <QDialog>

namespace Ui {
class ExportForm;
}

struct ExportFormData {
    int table;
    bool loaded_only;
    bool filename, views, rating, likes, tags, notes, sha, length;
    bool tagname, tagrate;
    char separator;
};

class ExportForm : public QDialog
{
    Q_OBJECT

public:
    explicit ExportForm(QWidget *parent = 0);
    ~ExportForm();

    ExportFormData getExportData() { return edata; }

private slots:
    void on_radioButton_3_toggled(bool checked);

    void on_radioButton_4_toggled(bool checked);

    void on_groupBox_2_toggled(bool arg1);

    void on_groupBox_toggled(bool arg1);

    void on_buttonBox_accepted();

private:
    Ui::ExportForm *ui;
    ExportFormData edata;
};

#endif // EXPORTFORM_H
