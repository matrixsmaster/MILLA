#ifndef CVHELPER_H
#define CVHELPER_H

#include <QDebug>
#include <QImage>
#include <QPixmap>
#include <QRect>
#include <QPainter>
#include <opencv2/opencv.hpp>
#include "facedetector.h"
#include "mcolornet.h"

enum MROIType {
    MROI_GENERIC = 0,
    MROI_FACE_FRONTAL,
    //TODOs:
    MROI_FACE_EYES,
    MROI_FACE_MOUTH,
    MROI_FACE_NOSE,
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
    qint64 filelen;
};

class CVHelper
{
public:
    CVHelper() {}
    virtual ~CVHelper();

    static cv::Mat quickConvert(QImage &in);

    static cv::Mat slowConvert(QImage const &in);

    static QImage quickConvertBack(cv::Mat &in);

    static QImage slowConvertBack(cv::Mat &in);

    static QByteArray storeMat(cv::Mat const &in);

    static cv::Mat loadMat(QByteArray const &arr);

    static cv::Mat getHist(cv::Mat &in);

    static cv::Mat getHist(QPixmap const &img);

    MImageExtras collectImageExtraData(QString const &fn, QPixmap const &org);

    static QPixmap drawROIs(QPixmap const &on, QRect &visBound, MImageExtras const &ext, bool calc_only, int index = -1);

    static QColor determineMainColor(QPixmap const &img, QRect const &area);

    static QColor determineMediumColor(QPixmap const &img, QRect const &area);

    static QPixmap colorToGrayscale(QPixmap const &img);

    QPixmap recolorImage(QPixmap const &img);

private:
    FaceDetector facedetector;
    MColorNet* color_net = nullptr;
};

#endif // CVHELPER_H
