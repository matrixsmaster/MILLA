#ifndef MIMAGEOPS_H
#define MIMAGEOPS_H

#include <QDebug>
#include <QList>
#include <QPixmap>
#include <QPainter>
#include <algorithm>
#include "shared.h"

struct MMacroRecord {
    enum {
        FlipVertical,
        FlipHorizontal,
        RotateCW,
        RotateCCW,
        Concatenate,
        Crop
    } action;
    MImageListRecord left, right;
    QPixmap result;
};

class MImageOps
{
public:
    MImageOps();
    virtual ~MImageOps() {}

    void clear();

    bool isActive() { return !history.empty(); }

    int size()      { return history.size(); }

    int position()  { return pos - history.begin(); }

    QPixmap rotate(MImageListRecord const &in, bool cw);

    QPixmap flip(MImageListRecord const &in, bool vertical);

    QPixmap concatenate(MImageListRecord const &a, MImageListRecord const &b);

    QPixmap crop(MImageListRecord const &in, QRect const &rct);

    QPixmap previous();

    QPixmap next();

    QPixmap current();

private:
    QList<MMacroRecord> history;
    QList<MMacroRecord>::iterator pos;
};

#endif // MIMAGEOPS_H
