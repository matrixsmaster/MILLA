#ifndef CVHELPER_H
#define CVHELPER_H

#include <QDebug>
#include <QImage>
#include <opencv2/opencv.hpp>

class CVHelper
{
public:
    CVHelper() {}
    virtual ~CVHelper() {}

public:
    static cv::Mat quickConvert(QImage &in);
    static cv::Mat slowConvert(QImage const &in);
    static QByteArray storeMat(cv::Mat const &in);
    static cv::Mat loadMat(QByteArray const &arr);
};

#endif // CVHELPER_H
