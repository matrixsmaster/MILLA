#include <QPainter>
#include "vmouse.h"

VMouse::VMouse()
{
    QFile js(":/cursors.json");
    if (js.open(QIODevice::Text | QIODevice::ReadOnly)) {
        QJsonObject doc = QJsonDocument::fromJson(js.readAll()).object();
        js.close();
        loadCursor("Move",opened,doc);
        loadCursor("Click",closed,doc);
    }
}

void VMouse::loadCursor(QString const &name, VMPCursor &cur, QJsonObject &obj)
{
    if (!obj.contains(name)) return;

    QJsonObject a = obj.take(name).toObject();
    if (a.isEmpty()) return;

    cur.pic = QPixmap(a.take("File").toString());
    cur.hot = QPoint(a.take("HotX").toInt(),a.take("HotY").toInt());
}

QPoint VMouse::Update(AIOMouse state, QSize size)
{
    QPoint dt = QPoint(state.x,state.y) - QPoint(prev.x,prev.y);
    cur += dt;
    prev = state;

    if (cur.x() < 0) cur.setX(0);
    if (cur.y() < 0) cur.setY(0);
    if (cur.x() >= size.width()) cur.setX(size.width()-1);
    if (cur.y() >= size.height()) cur.setY(size.height()-1);

    return cur;
}

void VMouse::Draw(QImage &on)
{
    VMPCursor &c = (prev.button & AIMB_LEFT)? closed : opened;
    QPoint op = cur - c.hot;
    QPainter pt(&on);
    pt.drawPixmap(op.x(),op.y(),c.pic);
}
