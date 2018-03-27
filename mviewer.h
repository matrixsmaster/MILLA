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
#include <QProgressBar>
#include <QTimer>
#include <QtSql/QSqlDatabase>
#include <QCryptographicHash>
#include <QInputDialog>
#include <thumbnailmodel.h>
#include <sresultmodel.h>
#include <opencv2/opencv.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/objdetect/objdetect.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <ctime>

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
    QByteArray sha;
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

    void on_actionJump_to_triggered();

    void on_actionRefine_search_triggered();

    void on_actionSwap_images_triggered();

    void on_actionClear_results_triggered();

    void on_actionQuit_triggered();

    void on_actionLink_left_to_right_triggered();

private:
    Ui::MViewer *ui;
    QProgressBar* progressBar;
    double scaleFactor;
    MImageListRecord current_l, current_r;
    cv::CascadeClassifier* face_cascade;
    std::map<QString,MImageExtras> extra_cache;
    std::map<QString,std::pair<int,bool> > tags_cache;
    QTimer* view_timer = NULL;

    void showNextImage();

    void addTag(QString const &tg, int key, bool check = false);

    void updateTags(QString fn = QString());

    void createStatRecord(QString fn);

    unsigned incViews(bool left = true);

    void scaleImage(const MImageListRecord &rec, QScrollArea* scrl, QLabel* lbl, double factor);

    cv::Mat quickConvert(QImage &in);

    cv::Mat slowConvert(QImage const &in);

    QByteArray storeMat(cv::Mat const &in);

    cv::Mat loadMat(QByteArray const &arr);

    MImageExtras getExtraCacheLine(QString const &fn, bool forceload = false);

    void detectFaces(const cv::Mat &inp, std::vector<cv::Rect> *store);

    void updateCurrentTags();

    void resultsPresentation(QList<QString> lst, QListView *view, int tabIndex);

    void searchResults(QList<QString> lst);

    void searchByTag();

    void displayLinkedImages(QString const &fn);
};

#endif // MVIEWER_H
