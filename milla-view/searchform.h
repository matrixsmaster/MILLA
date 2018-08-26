#ifndef SEARCHFORM_H
#define SEARCHFORM_H

#include <QDialog>
#include <QDate>
#include <QComboBox>
#include <QRegExp>
#include <QDebug>
#include "cvhelper.h"

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

enum SearchFormScope {
    SRSCP_NEW,
    SRSCP_FOUND,
    SRSCP_CONTINUE,
    SRSCP_LINKS
};

struct SearchFormData {
    int rating, kudos, tags;
    unsigned minviews, maxviews;
    int minface, maxface;
    int colors, orient;
    double liked, similar;
    cv::Mat similar_to;
    time_t minmtime, maxmtime;
    time_t minstime, maxstime;
    size_t minsize, maxsize;
    int minsize_vi, maxsize_vi;
    QString text_notes, text_path, text_fn;
    bool wo_tags;
    bool w_notes;
    SearchFormScope scope;
    SearchFormSort sort;
    size_t maxresults;
};

class SearchForm : public QDialog
{
    Q_OBJECT

public:
    explicit SearchForm(QWidget *parent = 0);
    ~SearchForm();

    void set_data(SearchFormData const &ndata);
    SearchFormData getSearchData() { return sdata; }

private slots:
    void on_buttonBox_accepted();

private:
    Ui::SearchForm *ui;
    SearchFormData sdata;

    size_t getSize(QComboBox* box, bool &ok);
};

#endif // SEARCHFORM_H
