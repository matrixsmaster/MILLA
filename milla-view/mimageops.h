#ifndef MIMAGEOPS_H
#define MIMAGEOPS_H

#include <QDebug>
#include <QPixmap>
#include <QPainter>
#include <algorithm>

class MImageOps
{
public:
    MImageOps();
    virtual ~MImageOps() {}

    static QPixmap concatenate(QPixmap const &a, QPixmap const &b);
};

#endif // MIMAGEOPS_H
