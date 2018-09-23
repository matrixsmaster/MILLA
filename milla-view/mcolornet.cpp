#include "mcolornet.h"

using namespace cv;
using namespace cv::dnn;
using namespace std;

MColorNet::MColorNet(QString netfile, QString netweights) :
    MDNNBase(netfile,netweights)
{
}

cv::Mat MColorNet::doColor(cv::Mat const &in)
{
#if 0
    // setup additional layers
    int sz[] = {2, 313, 1, 1};
    const Mat pts_in_hull(4, sz, CV_32F, hull_pts);
    Ptr<Layer> class8_ab = net.getLayer("class8_ab");
    class8_ab->blobs.push_back(pts_in_hull);
    Ptr<Layer> conv8_313_rh = net.getLayer("conv8_313_rh");
    conv8_313_rh->blobs.push_back(Mat(1, 313, CV_32F, Scalar(2.606)));

    // extract L channel and subtract mean
    Mat lab, L, input;
    img.convertTo(img, CV_32F, 1.0/255);
    cvtColor(img, lab, COLOR_BGR2Lab);
    extractChannel(lab, L, 0);
    resize(L, input, Size(W_in, H_in));
    input -= 50;

    // run the L channel through the network
    Mat inputBlob = blobFromImage(input);
    net.setInput(inputBlob);
    Mat result = net.forward();

    // retrieve the calculated a,b channels from the network output
    Size siz(result.size[2], result.size[3]);
    Mat a = Mat(siz, CV_32F, result.ptr(0,0));
    Mat b = Mat(siz, CV_32F, result.ptr(0,1));
    resize(a, a, img.size());
    resize(b, b, img.size());

    // merge, and convert back to BGR
    Mat color, chn[] = {L, a, b};
    merge(chn, 3, lab);
    cvtColor(lab, color, COLOR_Lab2BGR);
#endif
}
