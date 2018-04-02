#include "cvhelper.h"
#include "dbhelper.h"

CVHelper::~CVHelper()
{
    facedetector.Finalize();
}

cv::Mat CVHelper::quickConvert(QImage &in) //FIXME: not always working
{
    if (in.format() != QImage::Format_RGB888) {
        in = in.convertToFormat(QImage::Format_RGB888);
        qDebug() << "converting";
    }
    return cv::Mat(in.size().height(),in.size().width(),CV_8UC3,in.bits());
}

cv::Mat CVHelper::slowConvert(QImage const &in)
{
    using namespace cv;

    QImage n;
    if (in.format() != QImage::Format_RGB888) {
        qDebug() << "converting";
        n = in.convertToFormat(QImage::Format_RGB888);
    } else
        n = in;

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

QByteArray CVHelper::storeMat(cv::Mat const &in)
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

cv::Mat CVHelper::loadMat(QByteArray const &arr)
{
    cv::Mat res;
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
        qDebug() << "[db] ALERT: matrix size invalid";
        return res;
    }
    uptr++;

    ptr = (const char*)uptr;
    memcpy(res.ptr(),ptr,tot);

    return res;
}

MImageExtras CVHelper::collectImageExtraData(QString const &fn, QPixmap const &org)
{
    using namespace cv;
    MImageExtras res;

    //convert Pixmap into Mat
    QImage orgm(org.toImage());
    if (orgm.isNull()) return res;
    Mat in = CVHelper::slowConvert(orgm);

    res.picsize = org.size();

    //image histogram (3D)
    int histSize[] = {64, 64, 64};
    float rranges[] = {0, 256};
    const float* ranges[] = {rranges, rranges, rranges};
    int channels[] = {0, 1, 2};
    calcHist(&in,1,channels,Mat(),res.hist,3,histSize,ranges,true,false);

    res.color = false;
    //grayscale detection: fast approach
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
    facedetector.detectFaces(in,&faces);
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
