#ifndef SEARCHFORM_H
#define SEARCHFORM_H

#include <QDialog>
#include <QDate>

namespace Ui {
class SearchForm;
}

struct SearchFormData {
    int rating = 1;
    int views = 0;
    QDate mtime_min, mtime_max;
};

class SearchForm : public QDialog
{
    Q_OBJECT

public:
    explicit SearchForm(QWidget *parent = 0);
    ~SearchForm();

    SearchFormData getSearchData() { return sdata; }

private slots:
    void on_buttonBox_accepted();

private:
    Ui::SearchForm *ui;
    SearchFormData sdata;
};

#endif // SEARCHFORM_H
