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
#include <opencv2/objdetect/objdetect.hpp>
#include <opencv2/highgui/highgui.hpp>

#define FLATS_MINHESSIAN 400
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
    std::vector<MROI> rois;
    cv::Mat hist;
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

    void on_actionDetect_face_triggered();

private:
    Ui::MViewer *ui;
    double scaleFactor;
    QModelIndex current_l, current_r;
    cv::CascadeClassifier* face_cascade;
    std::map<QString,MImageExtras> extra_cache;

    void scaleImage(QScrollArea* scrl, QLabel* lbl, QModelIndex* idx, double factor);
    cv::Mat quickConvert(QImage const &in);
    MImageExtras getExtraCacheLine(QString const &fn);
    void DetectFaces(const QPixmap &in);
};

#endif // MVIEWER_H
