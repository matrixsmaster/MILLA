#include "sguieventsink.h"

SGUIEventSink::SGUIEventSink(QObject *parent) : QObject(parent)
{
}

bool SGUIEventSink::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        qDebug("Ate key press %d",keyEvent->key());
        return true;
    } else {
        //standard event processing
        return QObject::eventFilter(obj,event);
    }
}
