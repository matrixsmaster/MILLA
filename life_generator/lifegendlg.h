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

    Ui::LifeGenDlg *ui;

private slots:
    void on_pushButton_clicked();
};

#endif // LIFEGENDLG_H
