#include "sguieventsink.h"

SGUIEventSink::SGUIEventSink(QObject *parent) : QObject(parent)
{
}

bool SGUIEventSink::eventFilter(QObject *obj, QEvent *event)
{
    AIOEvent ev;

    switch (event->type()) {
    case QEvent::KeyPress:
        ev.type = AIOE_KEYDOWN;
        break;
    case QEvent::KeyRelease:
        ev.type = AIOE_KEYUP;
        break;
    case QEvent::MouseMove:
        ev.type = AIOE_MOUSEMOVE;
        break;
    case QEvent::MouseButtonPress:
        ev.type = AIOE_MOUSEDOWN;
        break;
    case QEvent::MouseButtonRelease:
        ev.type = AIOE_MOUSEUP;
        break;
    case QEvent::Wheel:
        ev.type = AIOE_MOUSEWHEEL;
        break;
    default: break;
    }

    switch (ev.type) {
    case AIOE_KEYDOWN:
    case AIOE_KEYUP:
    {
        QKeyEvent* kev = static_cast<QKeyEvent*>(event);
        ev.key = AK_EMPTY;//recode(kev->key());
    }
        break;

    case AIOE_MOUSEMOVE:
    case AIOE_MOUSEDOWN:
    case AIOE_MOUSEUP:
    {
        QMouseEvent* mev = static_cast<QMouseEvent*>(event);
        ev.mouse.x = mev->x();
        ev.mouse.y = mev->y();
        ev.mouse.button = mev->buttons();
        qDebug() << "Mouse: " << ev.mouse.x << ev.mouse.y << ev.mouse.button;
    }
        break;

    case AIOE_MOUSEWHEEL:
    {
        QWheelEvent* wev = static_cast<QWheelEvent*>(event);
        ev.mouse.wheel = wev->angleDelta().y() / 15;
        ev.mouse.x = wev->x();
        ev.mouse.y = wev->y();
        qDebug() << "Wheel: " << ev.mouse.wheel;
    }
        break;

    default:
        return QObject::eventFilter(obj,event); //unknown event, move on
    }

    buffer.push_back(ev);
    return true;
}

void SGUIEventSink::pushEvent(AIOEvent* e)
{
    buffer.push_front(*e);
}

bool SGUIEventSink::popEvent(AIOEvent* e)
{
    if (buffer.empty()) return false;
    *e = buffer.front();
    buffer.pop_front();
    return true;
}
