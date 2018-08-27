#ifndef EXPORTFORM_H
#define EXPORTFORM_H

#include <QDialog>

namespace Ui {
class ExportForm;
}

enum ExportFormTable {
    IMPEXP_NONE,
    IMPEXP_STATS,
    IMPEXP_TAGS,
    IMPEXP_STORIES,
    IMPEXP_LINKS
};

struct ExportFormData {
    ExportFormTable table;
    bool loaded_only, header;
    bool filename, views, rating, likes, tags, notes, sha, length;
    bool tagname, tagrate;
    bool story_update, story_title, story_actions;
    bool link_created, link_left, link_right;
    char separator;
    bool imp_noover;
};

class ExportForm : public QDialog
{
    Q_OBJECT

public:
    explicit ExportForm(bool forImport = false, QWidget *parent = 0);
    ~ExportForm();

    ExportFormData getExportData() { return edata; }

private slots:
    void on_radioButton_3_toggled(bool checked);

    void on_radioButton_4_toggled(bool checked);

    void on_groupBox_toggled(bool arg1);

    void on_groupBox_2_toggled(bool arg1);

    void on_groupBox_4_toggled(bool arg1);

    void on_groupBox_5_toggled(bool arg1);

    void on_buttonBox_accepted();

private:
    Ui::ExportForm *ui;
    ExportFormData edata;
};

#endif // EXPORTFORM_H
