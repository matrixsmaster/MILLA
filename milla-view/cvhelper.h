#ifndef CVHELPER_H
#define CVHELPER_H

#include <QDebug>
#include <QImage>
#include <QPixmap>
#include <opencv2/opencv.hpp>
#include "facedetector.h"

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
    qint64 filelen;
};

class CVHelper
{
public:
    CVHelper() {}
    virtual ~CVHelper();

    static cv::Mat quickConvert(QImage &in);

    static cv::Mat slowConvert(QImage const &in);

    static QByteArray storeMat(cv::Mat const &in);

    static cv::Mat loadMat(QByteArray const &arr);

    MImageExtras collectImageExtraData(QString const &fn, QPixmap const &org);

private:
    FaceDetector facedetector;
};

#endif // CVHELPER_H
