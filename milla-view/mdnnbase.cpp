#include "mdnnbase.h"

using namespace cv;
using namespace cv::dnn;
using namespace std;

Net* MDNNBase::mind = nullptr;
int MDNNBase::inst_cnt = 0;

MDNNBase::MDNNBase(QString netfile, QString netweights)
{
    inst_cnt++;
    nettext = netfile;
    if (!netweights.isEmpty()) {
        netbinary = netweights;
        if (!mind) {
            Net tmp = readNetFromCaffe(nettext.toStdString(),netbinary.toStdString());
            *mind = tmp;
        }
    }
}
