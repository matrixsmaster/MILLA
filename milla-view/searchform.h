#ifndef SEARCHFORM_H
#define SEARCHFORM_H

#include <QDialog>
#include <QDate>
#include <QComboBox>
#include <QRegExp>
#include <QDebug>

namespace Ui {
class SearchForm;
}

enum SearchFormSort {
    SRFRM_NONE,
    SRFRM_RATING,
    SRFRM_KUDOS,
    SRFRM_VIEWS,
    SRFRM_DATE,
    SRFRM_NAME,
    SRFRM_FACES,
    SRFRM_LASTSEEN
};

struct SearchFormData {
    int rating, kudos;
    unsigned minviews, maxviews;
    int minface, maxface;
    int colors;
    time_t minmtime, maxmtime;
    time_t minstime, maxstime;
    size_t minsize, maxsize;
    QString text_notes, text_path, text_fn;
    bool wo_tags;
    bool w_notes;
    bool linked_only;
    SearchFormSort sort;
    size_t maxresults;
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

    size_t getSize(QComboBox* box, bool &ok);
};

#endif // SEARCHFORM_H
