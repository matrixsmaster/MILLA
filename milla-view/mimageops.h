#ifndef MIMAGEOPS_H
#define MIMAGEOPS_H

#include <QObject>
#include <QDebug>
#include <QList>
#include <QString>
#include <QPixmap>
#include <QPainter>
#include <algorithm>
#include <utility>
#include "shared.h"
#include "mimageloader.h"

struct MMacroRecord {
    enum ActionType {
        None,
        FlipVertical,
        FlipHorizontal,
        RotateCW,
        RotateCCW,
        Concatenate,
        Crop
    } action;
    MImageListRecord left, right;
    int link_l = -1, link_r = -1;
    QRect roi;
    QPixmap result;
    QString comment;
};

class MImageOps : public QObject
{
    Q_OBJECT

public:
    explicit MImageOps(MImageLoader* imgLoader, QObject *parent = 0);
    virtual ~MImageOps() {}

    void clear();

    bool isActive() { return !history.empty(); }

    int size()      { return history.size(); }

    int position()  { return pos - history.begin(); }

    QPixmap append(MImageListRecord const &in);

    QPixmap rotate(MImageListRecord const &in, bool cw);

    QPixmap flip(MImageListRecord const &in, bool vertical);

    QPixmap concatenate(MImageListRecord const &a, MImageListRecord const &b);

    QPixmap crop(MImageListRecord const &in, QRect const &rct);

    QPixmap first();

    QPixmap previous(bool commented_only = false);

    QPixmap next(bool commented_only = false);

    QPixmap current();

    bool moveCurrent(bool backward);

    bool addComment(QString const &com);

    QString getComment();

    QString serializeFileRecord(const MImageListRecord &rec, int link);

    bool deserializeFileRecord(MImageListRecord &rec, int link);

    QString serialize();

    bool deserialize(QString const &in);

private:
    MImageLoader* loader;
    QList<MMacroRecord> history;
    QList<MMacroRecord>::iterator pos;
    bool loading = false;

    void add(MMacroRecord &rec);
};

#endif // MIMAGEOPS_H
