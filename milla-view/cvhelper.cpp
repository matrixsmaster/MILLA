#include <QApplication>
#include "cvhelper.h"
#include "dbhelper.h"

using namespace cv;

CVHelper::CVHelper()
{
    QString pth = QApplication::applicationDirPath();

    //preload DNNs since at least one instance of CVHelper is always on
    color_net = new MColorNet(pth+COLORIZATION_NET_FILE,pth+COLORIZATION_NET_WEIGHT);
}

CVHelper::~CVHelper()
{
    facedetector.Finalize();
    if (color_net) delete color_net;
}

Mat CVHelper::quickConvert(QImage &in) //FIXME: not always working
{
    if (in.format() != QImage::Format_RGB888) {
        qDebug() << "Converting";
        try {
            in = in.convertToFormat(QImage::Format_RGB888);
        } catch(...) {
            qDebug() << "Error converting";
            return Mat();
        }
    }
    return cv::Mat(in.size().height(),in.size().width(),CV_8UC3,in.bits());
}

Mat CVHelper::slowConvert(QImage const &in)
{
    QImage n;
    if (in.format() != QImage::Format_RGB888) {
        qDebug() << "Converting";
        try {
            n = in.convertToFormat(QImage::Format_RGB888);
        } catch(...) {
            qDebug() << "Error converting";
            return Mat();
        }
    } else
        n = in;

    //slowly, but surely
    Mat r(n.size().height(),n.size().width(),CV_8UC3);
    for (int j,i = 0; i < r.rows; i++) {
        uchar* ptr = n.scanLine(i);
        for (j = 0; j < r.cols; j++) {
            r.at<Vec3b>(Point(j,i)) = Vec3b(*(ptr+2),*(ptr+1),*(ptr));
            ptr += 3;
        }
    }

    return r;
}

QImage CVHelper::quickConvertBack(Mat &in)
{
    QImage n(in.cols,in.rows,QImage::Format_RGBA8888);
    try {
        //TODO: Use Mat::reshape()
        if (in.type() != CV_8UC4) {
            qDebug() << "Converting";
            in.convertTo(in,CV_8UC4);
        }
    } catch(...) {
        qDebug() << "Error converting";
        return n;
    }
    memcpy(n.bits(),in.ptr(),in.cols*in.rows*4);
    return n;
}

QImage CVHelper::slowConvertBack(Mat &in)
{
    QImage n(in.cols,in.rows,QImage::Format_RGB888);
    try {
        if (in.type() != CV_8UC3) {
            qDebug() << "Converting";
            in.convertTo(in,CV_8UC3);
        }
    } catch(...) {
        qDebug() << "Error converting";
        return n;
    }

    uchar* frm = in.ptr();
    for (int j,i = 0; i < n.height(); i++) {
        uchar* ptr = n.scanLine(i);
        for (j = 0; j < n.width(); j++,frm+=3) {
            *ptr++ = frm[2];
            *ptr++ = frm[1];
            *ptr++ = frm[0];
        }
    }

    return n;
}

QByteArray CVHelper::storeMat(Mat const &in)
{
    QByteArray harr;
    harr.append((char*)&(in.cols),sizeof(in.cols));
    harr.append((char*)&(in.rows),sizeof(in.rows));
    harr.append((char*)&(in.dims),sizeof(in.dims));

    int typ = in.type();
    harr.append((char*)&(typ),sizeof(typ));

    for (int i = 0; i < in.dims; i++)
        harr.append((char*)&(in.size[i]),sizeof(in.size[i]));

    size_t tmp = in.elemSize() * in.total();
    harr.append((char*)&tmp,sizeof(tmp));
    harr.append((char*)in.ptr(),tmp);

    return harr;
}

Mat CVHelper::loadMat(QByteArray const &arr)
{
    Mat res;
    const char* ptr = arr.constData();
    const int* iptr = (const int*)ptr;
    const size_t* uptr;

    int cols = *iptr++;
    int rows = *iptr++;
    int dims = *iptr++;
    int type = *iptr++;

    if (rows < 0 && cols < 0) {
        res.create(dims,iptr,type);
        iptr += dims;
    } else
        res.create(rows,cols,type);

    uptr = (const size_t*)iptr;
    size_t tot = *uptr;
    if (tot != res.elemSize() * res.total()) {
        qDebug() << "ALERT: matrix size invalid";
        return res;
    }
    uptr++;

    ptr = (const char*)uptr;
    memcpy(res.ptr(),ptr,tot);

    return res;
}

Mat CVHelper::getHist(Mat &in)
{
    int histSize[] = {64, 64, 64};
    float rranges[] = {0, 256};
    const float* ranges[] = {rranges, rranges, rranges};
    int channels[] = {0, 1, 2};
    Mat out;

    try {
        calcHist(&in,1,channels,Mat(),out,3,histSize,ranges,true,false);
    } catch(...) {
        qDebug() << "Error creating histogram";
    }

    return out;
}

Mat CVHelper::getHist(QPixmap const &img)
{
    QImage orgm(img.toImage());
    if (orgm.isNull()) return Mat();
    Mat in = CVHelper::slowConvert(orgm);
    return getHist(in);
}

MImageExtras CVHelper::collectImageExtraData(QString const &fn, QPixmap const &org)
{
    MImageExtras res;

    //convert Pixmap into Mat
    QImage orgm(org.toImage());
    if (orgm.isNull()) return res;
    Mat in = CVHelper::slowConvert(orgm);

    res.picsize = org.size();

    //image histogram (3D)
    res.hist = getHist(in);

    //grayscale detection: fast approach
    res.color = false;
    for (int k = 0; k < res.hist.size[0]; k++) {
        float a = res.hist.at<float>(k,0,0);
        float b = res.hist.at<float>(0,k,0);
        float c = res.hist.at<float>(0,0,k);
        if (a != b || b != c || c != a) {
            res.color = true;
            break;
        }
    }
    if (!res.color && in.isContinuous()) {
        //grayscale detection: slow approach, as we're still not completely sure
        uchar* _ptr = in.ptr();
        for (int k = 0; k < in.rows && !res.color; k++)
            for (int kk = 0; kk < in.cols && !res.color; kk++) {
                if (_ptr[0] != _ptr[1] || _ptr[1] != _ptr[2] || _ptr[2] != _ptr[0]) {
                    res.color = true;
                    qDebug() << "[grsdetect] Deep scan mismatch: " << _ptr[0] << _ptr[1] << _ptr[2];
                }
                _ptr += 3;
            }
    }

    //face detector
    std::vector<cv::Rect> faces;
    facedetector.detectFacesPass1(in,&faces);
    for (auto &i : faces) {
        MROI roi;
        roi.kind = MROI_FACE_FRONTAL;
        roi.x = i.x;
        roi.y = i.y;
        roi.w = i.width;
        roi.h = i.height;
        res.rois.push_back(roi);
    }

    //finally, SHA-256 and length
    res.sha = DBHelper::getSHA256(fn,&(res.filelen));

    //now this entry is valid
    res.valid = true;
    return res;
}

QPixmap CVHelper::drawROIs(QPixmap const &on, QRect &visBound, MImageExtras const &ext, bool calc_only, int index)
{
    if (!ext.valid || ext.rois.empty()) return on;

    QImage inq(on.toImage());
    QPainter painter(&inq);
    QPen paintpen(Qt::red);
    paintpen.setWidth(2);       //TODO: move it to user settings
    painter.setPen(paintpen);

    int maxarea = 0;
    int cidx = 0;
    MROI winner;
    for (auto &i : ext.rois) {
        if (!calc_only && i.kind == MROI_FACE_FRONTAL)
            painter.drawRect(QRect(i.x,i.y,i.w,i.h));
        //TODO: add different colors for different ROIs

        if (index < 0 && i.w * i.h > maxarea) {
            maxarea = i.w * i.h;
            winner = i;

        } else if (index >= 0) {
            if (cidx == index) winner = i;
            else cidx++;
        }
    }

    if (maxarea > 0 || cidx == index)
        visBound = QRect(winner.x+winner.w/2, winner.y+winner.h/2, winner.w/2, winner.h/2);

    return calc_only? on : QPixmap::fromImage(inq);
}

Vec3b CVHelper::determineMainColor(cv::Mat const &in)
{
    Mat out,tmp;
    Vec3b r(0,0,0);
    try {
        cvtColor(in,out,COLOR_BGR2HSV);

        int hsz[] = {30,32}; //x6, x8
        float hrng[] = {0,180}; //hue
        float srng[] = {0,256}; //sat
        const float* rng[] = {hrng,srng};
        int chans[] = {0,1};
        calcHist(&out,1,chans,Mat(),tmp,2,hsz,rng,true,false);

        double vmax;
        Point pmax;
        minMaxLoc(tmp,0,&vmax,0,&pmax);

        Mat tmp(1,1,CV_8UC3);
        tmp.at<Vec3b>(0,0) = Vec3b(pmax.y*6,pmax.x*8,255);
        cvtColor(tmp,tmp,COLOR_HSV2BGR);
        r = tmp.at<Vec3b>(0,0);
        qDebug() << r[2] << r[1] << r[0];

    } catch (Exception &e) {
        std::cout << "OCV exception: " << e.what() << std::endl;
    }
    return r;
}

QColor CVHelper::determineMainColor(QPixmap const &img, QRect const &area)
{
    Mat in = slowConvert(img.copy(area).toImage());
    Vec3b r = determineMainColor(in);
    return QColor(r[2],r[1],r[0]);
}

Vec3b CVHelper::determineMediumColor(cv::Mat const &in)
{
    Vec3b r(0,0,0);
    double vr[3] = {0,0,0};
    const uchar* frm = in.ptr();

    for (int k,j,i = 0; i < in.rows; i++) {
        for (j = 0; j < in.cols; j++,frm+=3) {
            for (k = 0; k < 3; k++)
                vr[k] += frm[k];
        }
    }

    for (int k = 0; k < 3; k++)
        r[k] = floor(vr[k] / static_cast<double>(in.rows*in.cols));

    qDebug() << r[2] << r[1] << r[0];
    return r;
}

QColor CVHelper::determineMediumColor(QPixmap const &img, QRect const &area)
{
    Mat in = slowConvert(img.copy(area).toImage());
    Vec3b r = determineMediumColor(in);
    return QColor(r[2],r[1],r[0]);
}

QPixmap CVHelper::colorToGrayscale(QPixmap const &img)
{
    Mat out,in = slowConvert(img.toImage());
    try {
        cvtColor(in,out,COLOR_BGR2GRAY);
        cvtColor(out,in,COLOR_GRAY2BGR);
    } catch(...) {
        qDebug() << "OCV Error";
    }
    return QPixmap::fromImage(slowConvertBack(in));
}

QPixmap CVHelper::recolorImage(QPixmap const &img)
{
    if (!color_net->isValid()) return QPixmap();
    Mat in = slowConvert(img.toImage());
    Mat out = color_net->doColor(in);
    return QPixmap::fromImage(slowConvertBack(out));
}
