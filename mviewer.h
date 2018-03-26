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
#include <QListWidget>
#include <QTimer>
#include <QtSql/QSqlDatabase>
#include <QCryptographicHash>
#include <thumbnailmodel.h>
#include <sresultmodel.h>
#include <opencv2/opencv.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/objdetect/objdetect.hpp>
#include <opencv2/highgui/highgui.hpp>

#define INTERNAL_DB_VERSION 1
#define FACE_CASCADE_FILE "/tmp/face_cascade.xml"

enum MROIType {
    MROI_GENERIC = 0,
    MROI_FACE_FRONTAL,
    MROI_INVALID // terminator
};

struct MROI {
    MROIType kind = MROI_GENERIC;
    int x = 0, y = 0, w = 0, h = 0;
};

struct MImageExtras {
    bool valid = false;
    QSize picsize;
    std::vector<MROI> rois;
    cv::Mat hist;
    bool color = true;
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

    void on_actionLoad_all_known_triggered();

    void on_listWidget_itemClicked(QListWidgetItem *item);

    void on_actionLoad_everything_slow_triggered();

    void on_radio_search_toggled(bool checked);

    void on_radio_settags_toggled(bool checked);

private:
    Ui::MViewer *ui;
    double scaleFactor;
    QModelIndex current_l, current_r;
    cv::CascadeClassifier* face_cascade;
    std::map<QString,MImageExtras> extra_cache;
    std::map<QString,std::pair<int,bool> > tags_cache;
    QTimer* view_timer = NULL;

    void showNextImage();

    void addTag(QString const &tg, int key, bool check = false);

    void updateTags(QString fn = QString());

    void createStatRecord(QString fn);

    unsigned incViews(bool left = true);

    void scaleImage(QScrollArea* scrl, QLabel* lbl, QModelIndex* idx, double factor);

    cv::Mat quickConvert(QImage &in);

    cv::Mat slowConvert(QImage const &in);

    QByteArray storeMat(cv::Mat const &in);

    cv::Mat loadMat(QByteArray const &arr);

    MImageExtras getExtraCacheLine(QString const &fn, bool forceload = false);

    void detectFaces(const cv::Mat &inp, std::vector<cv::Rect> *store);

    void updateCurrentTags();

    void searchResults(QList<QString> lst);

    void searchByTag();
};

#endif // MVIEWER_H
