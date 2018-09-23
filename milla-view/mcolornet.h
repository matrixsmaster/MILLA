#ifndef MCOLORNET_H
#define MCOLORNET_H

#include "mdnnbase.h"

class MColorNet : public MDNNBase
{
public:
    MColorNet(QString netfile, QString netweights);

    cv::Mat doColor(const cv::Mat &in);
};

#endif // MCOLORNET_H
