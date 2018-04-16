#include "mimageops.h"

using namespace std;

MImageOps::MImageOps()
{
}

void MImageOps::clear()
{
    history.clear();
    pos = history.begin();
}

QPixmap MImageOps::rotate(MImageListRecord const &in, bool cw)
{
    if (!in.valid) return QPixmap();

    MMacroRecord rec;
    rec.action = cw? MMacroRecord::RotateCW : MMacroRecord::RotateCCW;
    rec.left = in;

    QTransform rotmat;
    rotmat.rotate(cw? 90 : -90);
    rec.result = QPixmap::fromImage(in.picture.toImage().transformed(rotmat));

    history.push_back(rec);
    pos = history.end();
    return rec.result;
}

QPixmap MImageOps::flip(MImageListRecord const &in, bool vertical)
{
    if (!in.valid) return QPixmap();

    MMacroRecord rec;
    rec.action = vertical? MMacroRecord::FlipVertical : MMacroRecord::FlipHorizontal;
    rec.left = in;
    rec.result = QPixmap::fromImage(in.picture.toImage().mirrored(!vertical,vertical));

    history.push_back(rec);
    pos = history.end();
    return rec.result;
}

QPixmap MImageOps::concatenate(MImageListRecord const &a, MImageListRecord const &b)
{
    if (!a.valid || !b.valid) return QPixmap();

    QSize rs(a.picture.size().width()+b.picture.size().width(),
             std::max(a.picture.size().height(),b.picture.size().height()));
    QPixmap op_a = a.picture.scaledToHeight(rs.height(),Qt::SmoothTransformation);
    QPixmap op_b = b.picture.scaledToHeight(rs.height(),Qt::SmoothTransformation);
    rs.setWidth(op_a.size().width() + op_b.size().width());

    QImage res(rs,QImage::Format_RGB32);
    QPainter p(&res);

    p.drawPixmap(0,0,op_a);
    p.drawPixmap(op_a.size().width(),0,op_b);

    MMacroRecord rec;
    rec.action = MMacroRecord::Concatenate;
    rec.left = a;
    rec.right = b;
    rec.result = QPixmap::fromImage(res);

    history.push_back(rec);
    pos = history.end();
    return rec.result;
}

QPixmap MImageOps::previous()
{
    if (pos != history.begin()) --pos;
    return current();
}

QPixmap MImageOps::next()
{
    if (pos != history.end()) ++pos;
    return current();
}

QPixmap MImageOps::current()
{
    if (pos == history.end()) return QPixmap();
    return pos->result;
}
