#ifndef MCOLORNET_H
#define MCOLORNET_H

#include "mdnnbase.h"

class MColorNet : public MDNNBase
{
public:
    MColorNet(QString netfile, QString netweights);

    cv::Mat doColor(cv::Mat const &in);
};

#endif // MCOLORNET_H