#ifndef MIMAGEOPS_H
#define MIMAGEOPS_H

#include <QDebug>
#include <QList>
#include <QString>
#include <QPixmap>
#include <QPainter>
#include <algorithm>
#include <utility>
#include "shared.h"

struct MMacroRecord {
    enum ActionType {
        FlipVertical,
        FlipHorizontal,
        RotateCW,
        RotateCCW,
        Concatenate,
        Crop
    } action;
    MImageListRecord left, right;
    int link_l = -1, link_r = -1;
    QPixmap result;
    QString comment;
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

    QPixmap first();

    QPixmap previous();

    QPixmap next();

    QPixmap current();

    bool moveCurrent(bool backward);

    bool addComment(QString const &com);

    QString getComment();

    QString serialize();

    bool deserialize(QString const &in);

private:
    QList<MMacroRecord> history;
    QList<MMacroRecord>::iterator pos;
    bool loading = false;

    void add(MMacroRecord &rec);
};

#endif // MIMAGEOPS_H
