#include "facedetector.h"

cv::CascadeClassifier* FaceDetector::face_cascade = nullptr;

FaceDetector::FaceDetector()
{
    using namespace cv;

    if (!face_cascade) {
        QFile test(FACE_CASCADE_FILE);
        if (test.exists()) test.remove();
        if (QFile::copy(":/face_cascade.xml",FACE_CASCADE_FILE)) {
            face_cascade = new CascadeClassifier();
            if (!face_cascade->load(FACE_CASCADE_FILE)) {
                qDebug() << "Unable to load cascade from " << FACE_CASCADE_FILE;
                delete face_cascade;
                face_cascade = nullptr;
            }
            test.remove();
        }
    }
}

void FaceDetector::detectFaces(const cv::Mat &inp, std::vector<cv::Rect>* store)
{
    using namespace cv;

    if (!face_cascade) return;

    std::vector<Rect> items;
    Mat work;

    try {
        cvtColor(inp,work,CV_BGR2GRAY);
        equalizeHist(work,work);
        face_cascade->detectMultiScale(work,items,1.1,3,0,Size(32,32));
    } catch (...) {
        qDebug() << "Error";
        return;
    }
    qDebug() << items.size() << " faces detected";

    if (store) store->insert(store->begin(),items.begin(),items.end());

    return;
}

void FaceDetector::Finalize()
{
    if (face_cascade) delete face_cascade;
    face_cascade = nullptr;
}
