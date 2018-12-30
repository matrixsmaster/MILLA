#ifndef ABOUTBOX_H
#define ABOUTBOX_H

#include <QDialog>
#include <QTimer>
#include <QPixmap>
#include <QShowEvent>
#include <QStringList>
#include <vector>
#include <set>

#define MILLA_ABOUT_MOSAIC_TIMER 100
#define MILLA_ABOUT_MOSAIC_SIZE 80
#define MILLA_ABOUT_MOSAIC_MINDIFF 60
#define MILLA_ABOUT_MOSAIC_MAXDIFF 9999
#define MILLA_ABOUT_MOSAIC_MAXMISS 200
#define MILLA_ABOUT_LOGOS { "milla", "milla-icon", "splash_01", "splash_02", "milla" }

namespace Ui {
class AboutBox;
}

class AboutBox : public QDialog
{
    Q_OBJECT

public:
    explicit AboutBox(QWidget *parent = 0);
    ~AboutBox();

    void Mosaic();

protected:
    void showEvent(QShowEvent* ev);

private:
    Ui::AboutBox *ui;
    QStringList files;
    QTimer timer;
    std::vector<std::pair<QColor,int>> quads;
    QPoint quad_size;
    std::set<int> visited;
    int misscount;

    void prepareLogo();
};

#endif // ABOUTBOX_H
