#ifndef MDNNBASE_H
#define MDNNBASE_H

#include <QString>
#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include "shared.h"

class MDNNBase
{
public:
    MDNNBase(QString netfile, QString netweights);
    virtual ~MDNNBase();

    bool isValid() { return valid; }

protected:
    static cv::dnn::Net* mind;
    static int inst_cnt;
    bool valid = false;
    QString nettext,netbinary;
};

#endif // MDNNBASE_H
