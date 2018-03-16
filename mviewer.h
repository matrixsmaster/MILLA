#ifndef MVIEWER_H
#define MVIEWER_H

#include <QDebug>
#include <QMainWindow>
#include <QMessageBox>
#include <QFileDialog>
#include <QScrollBar>
#include <QDirIterator>
#include <QFileInfo>
#include <QScrollArea>
#include <QtSql/QSqlDatabase>
#include <thumbnailmodel.h>
#include <opencv2/opencv.hpp>
#include <opencv2/nonfree/nonfree.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/highgui/highgui.hpp>

#define FLATS_MINHESSIAN 400

struct MMatcherCacheRec {
    std::vector<cv::KeyPoint> kpv;
    cv::Mat desc;
    bool valid = false;
    cv::MatND hist;
    cv::Mat tmp_img;
};

namespace Ui {
class MViewer;
}

class MViewer : public QMainWindow
{
    Q_OBJECT

public:
    explicit MViewer(QWidget *parent = 0);
    ~MViewer();

private slots:
    void on_pushButton_clicked();

    void on_actionOpen_triggered();

    void on_actionFit_triggered();

    void on_actionMatch_triggered();

    void on_actionTest_triggered();

    void on_actionLoad_all_known_triggered();

private:
    Ui::MViewer *ui;
    double scaleFactor;
    QModelIndex current_l, current_r;
    std::map<QString,MMatcherCacheRec> match_cache;

    void scaleImage(QScrollArea* scrl, QLabel* lbl, QModelIndex* idx, double factor);
    cv::Mat quickConvert(QImage const &in);
    MMatcherCacheRec getMatchCacheLine(QString const &fn);
};

#endif // MVIEWER_H
