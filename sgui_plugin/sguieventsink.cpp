#include "sguieventsink.h"

/*
 * 	AK_EMPTY = 0,
    AK_ESC,
    AK_F1, AK_F2, AK_F3, AK_F4, AK_F5, AK_F6, AK_F7, AK_F8, AK_F9, AK_F10, AK_F11, AK_F12,
    AK_1, AK_2, AK_3, AK_4, AK_5, AK_6, AK_7, AK_8, AK_9, AK_0,
    AK_MINUS, AK_PLUS, AK_TILDE, AK_BACKSPACE,
    AK_TAB, AK_CAPS, AK_LSHIFT, AK_RSHIFT, AK_LCTRL, AK_RCTRL, AK_LALT, AK_RALT,
    AK_ENTER,
    AK_PAUSE, AK_PRTSCRN, AK_DELETE,
    AK_SPACE,
    AK_LEFT, AK_RIGHT, AK_UP, AK_DOWN,
    AK_Q, AK_W, AK_E, AK_R, AK_T, AK_Y, AK_U, AK_I, AK_O, AK_P,
    AK_A, AK_S, AK_D, AK_F, AK_G, AK_H, AK_J, AK_K, AK_L,
    AK_Z, AK_X, AK_C, AK_V, AK_B, AK_N, AK_M,
    AK_OPEN, AK_CLOSE, AK_BACKSLASH,
    AK_SEMICOLON, AK_APOS,
    AK_COLON, AK_DOT, AK_SLASH,
    AK_INVALID
 */

using namespace Qt;

static const int scantable[] = {
    Key_unknown,
    Key_Escape,
    Key_F1, Key_F2, Key_F3, Key_F4, Key_F5, Key_F6, Key_F7, Key_F8, Key_F9, Key_F10, Key_F11, Key_F12,
    Key_1, Key_2, Key_3, Key_4, Key_5, Key_6, Key_7, Key_8, Key_9, Key_0,
    Key_Minus, Key_Plus, Key_AsciiTilde, Key_Backspace,
    Key_Tab, Key_CapsLock,
    Key_Shift, Key_Shift, Key_Control, Key_Control, Key_Alt, Key_Alt,
    Key_Return,
    Key_Pause, Key_Print, Key_Delete,
    Key_Space,
    Key_Left, Key_Right, Key_Up, Key_Down,
    Key_Q, Key_W, Key_E, Key_R, Key_T, Key_Y, Key_U, Key_I, Key_O, Key_P,
    Key_A, Key_S, Key_D, Key_F, Key_G, Key_H, Key_J, Key_K, Key_L,
    Key_Z, Key_X, Key_C, Key_V, Key_B, Key_N, Key_M,
    Key_BracketLeft, Key_BracketRight, Key_Backslash,
    Key_Semicolon, Key_Apostrophe,
    Key_Comma, Key_Period, Key_Slash
};


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
        ev.key = recode(kev->key());
        qDebug() << "Kbd: Qt code " << kev->key() << " recoded to " << (int)ev.key;
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

AIOEKey SGUIEventSink::recode(int qt_key)
{
    for (int i = 0; i < AK_INVALID; i++) {
        if (qt_key == scantable[i])
            return static_cast<AIOEKey>(i);
    }
    return AK_EMPTY;
}
