#include "mimageops.h"

using namespace std;

MImageOps::MImageOps()
{
}

QPixmap MImageOps::concatenate(QPixmap const &a, QPixmap const &b)
{
    if (a.isNull() || b.isNull()) return QPixmap();

    QSize rs(a.size().width()+b.size().width(), std::max(a.size().height(),b.size().height()));
    QPixmap op_a = a.scaledToHeight(rs.height(),Qt::SmoothTransformation);
    QPixmap op_b = b.scaledToHeight(rs.height(),Qt::SmoothTransformation);
    rs.setWidth(op_a.size().width() + op_b.size().height());

    QImage res(rs,QImage::Format_RGB32);
    QPainter p(&res);

    p.drawPixmap(0,0,op_a);
    p.drawPixmap(op_a.size().width(),0,op_b);

    return QPixmap::fromImage(res);
}
