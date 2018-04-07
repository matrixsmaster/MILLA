#ifndef SGUIEVENTSINK_H
#define SGUIEVENTSINK_H

#include <QObject>
#include <QEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include "include/AbstractIO.h"

class SGUIEventSink : public QObject
{
    Q_OBJECT

public:
    SGUIEventSink(QObject *parent = 0);
    virtual ~SGUIEventSink() {}

    bool pullEvent(AIOEvent* e) { return false; } //FIXME : debug only

protected:
    bool eventFilter(QObject *obj, QEvent *event);

private:
    QList<AIOEvent> buffer;
};

#endif // SGUIEVENTSINK_H
