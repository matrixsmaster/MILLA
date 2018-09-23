#include <QDebug>
#include <QMessageBox>
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
            Net tmp;
            try {
                tmp = readNetFromCaffe(nettext.toStdString(),netbinary.toStdString());
            } catch (Exception &e) {
                cout << "OCV exception: " << e.what() << endl;
            }
            if (tmp.empty())
                QMessageBox::warning(NULL,"Error",QString("Unable to load neural network from %1 : %2").arg(netfile,netweights));
            else {
                mind = new Net();
                *mind = tmp;
                valid = true;
                qDebug() << "Mind loaded" << mind;
            }
        } else
            valid = true;
    }
}

MDNNBase::~MDNNBase()
{
    if ((--inst_cnt <= 0) && (mind)) {
        qDebug() << "Mind destroyed" << mind;
        delete mind;
        mind = nullptr;
        valid = false;
    }
}
