#ifndef FACEDETECTOR_H
#define FACEDETECTOR_H

#include <QDebug>
#include <QFile>
#include <vector>
#include <opencv2/opencv.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/objdetect/objdetect.hpp>
#include <opencv2/highgui/highgui.hpp>

#define FACE_CASCADE_FILE "/tmp/face_cascade.xml"

class FaceDetector
{
public:
    FaceDetector();
    virtual ~FaceDetector() {}

    void detectFaces(const cv::Mat &inp, std::vector<cv::Rect>* store);

    void Finalize();

private:
    static cv::CascadeClassifier* face_cascade;
};

#endif // FACEDETECTOR_H
