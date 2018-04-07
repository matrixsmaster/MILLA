#ifndef VMOUSE_H
#define VMOUSE_H

#include <QPixmap>
#include <QImage>
#include <QPoint>
#include <QDebug>
#include <QFile>
#include <QSize>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include "include/AbstractIO.h"

struct VMPCursor {
    QPixmap pic;
    QPoint hot;
};

class VMouse
{
public:
    VMouse();
    virtual ~VMouse() {}

    QPoint Update(AIOMouse state, QSize size);

    void Draw(QImage &on);

    void forcePosition(QPoint const &pt) { cur = pt; }

    void showCursor(bool show)           { hide_cursor = !show; }

protected:
    void loadCursor(QString const &name, VMPCursor &cur, QJsonObject &obj);

private:
    VMPCursor opened, closed;
    QSize area;
    QPoint cur;
    AIOMouse prev;
    bool hide_cursor = false;
};

#endif // VMOUSE_H
