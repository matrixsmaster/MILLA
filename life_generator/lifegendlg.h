#ifndef LIFEGENDLG_H
#define LIFEGENDLG_H

#include <QWidget>

namespace Ui {
class LifeGenDlg;
}

class LifeGenDlg : public QWidget
{
    Q_OBJECT

public:
    explicit LifeGenDlg(QWidget *parent = nullptr);
    ~LifeGenDlg();

private:
    Ui::LifeGenDlg *ui;
};

#endif // LIFEGENDLG_H
