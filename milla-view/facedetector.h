#ifndef FACEDETECTOR_H
#define FACEDETECTOR_H

#include <QDebug>
#include <QFile>
#include <vector>
#include <opencv2/opencv.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/highgui.hpp>
#include "shared.h"

#define FACEDETECT_SCALE 1.1
#define FACEDETECT_NEIGHBORS 3
#define FACEDETECT_SIZE_W 32
#define FACEDETECT_SIZE_H 32

class FaceDetector
{
public:
    FaceDetector();
    virtual ~FaceDetector() {}

    void detectFacesPass1(const cv::Mat &inp, std::vector<cv::Rect>* store);

    void Finalize();

private:
    static cv::CascadeClassifier* face_cascade;
    static int face_inst_cnt;
};

#endif // FACEDETECTOR_H
