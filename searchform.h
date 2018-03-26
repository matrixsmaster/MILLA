#ifndef SEARCHFORM_H
#define SEARCHFORM_H

#include <QDialog>
#include <QDate>

namespace Ui {
class SearchForm;
}

struct SearchFormData {
    int rating = 1;
    unsigned views = 0;
    int minface = 0;
    int maxface = 100;
    bool grey = false;
    time_t mtime_min = 0;
    time_t mtime_max = 0;
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
