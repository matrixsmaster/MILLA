#ifndef SGUIEVENTSINK_H
#define SGUIEVENTSINK_H

#include <QDebug>
#include <QObject>
#include <QEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <deque>
#include "include/AbstractIO.h"

class SGUIEventSink : public QObject
{
    Q_OBJECT

public:
    SGUIEventSink(QObject *parent = 0);
    virtual ~SGUIEventSink() {}

    void pushEvent(AIOEvent* e);
    bool popEvent(AIOEvent* e);

protected:
    bool eventFilter(QObject *obj, QEvent *event);
    AIOEKey recode(int qt_key);

private:
    std::deque<AIOEvent> buffer;
};

#endif // SGUIEVENTSINK_H
