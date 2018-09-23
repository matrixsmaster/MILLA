#include <QDebug>
#include <QApplication>
#include <QMessageBox>
#include "facedetector.h"

using namespace cv;
using namespace std;

CascadeClassifier* FaceDetector::face_cascade = nullptr;
int FaceDetector::face_inst_cnt = 0;

FaceDetector::FaceDetector()
{
    face_inst_cnt++;
    if (!face_cascade) {
        face_cascade = new CascadeClassifier();
        QString fn = QApplication::applicationDirPath();
        fn += FACE_CASCADE_FILE;
        if (!face_cascade->load(fn.toStdString())) {
            QMessageBox::warning(NULL,"Error",QString("Unable to load face cascade from %1").arg(fn));
            delete face_cascade;
            face_cascade = nullptr;
        }
    }
}

void FaceDetector::detectFacesPass1(const Mat &inp, vector<Rect>* store)
{
    if (!face_cascade) return;

    vector<Rect> items;
    Mat work;

    try {
        cvtColor(inp,work,COLOR_BGR2GRAY);
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
    if (--face_inst_cnt <= 0) {
        qDebug() << "Destroying face cascade records...";
        if (face_cascade) delete face_cascade;
        face_cascade = nullptr;
    }
}
