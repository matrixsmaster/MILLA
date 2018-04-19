#include "mimageops.h"
#include "thumbnailmodel.h"

using namespace std;

MImageOps::MImageOps()
{
}

void MImageOps::clear()
{
    history.clear();
    pos = history.begin();
}

void MImageOps::add(MMacroRecord &rec)
{
    if (loading) return;

    if (rec.left.generated || rec.right.generated) {
        bool gl = rec.left.generated && rec.left.valid;
        bool gr = rec.right.generated && rec.right.valid;
        QImage il,ir;
        if (gl) il = rec.left.picture.toImage();
        if (gr) ir = rec.right.picture.toImage();

        int n = 0;
        for (auto &i : history) {
            QImage tmp = i.result.toImage();
            if (gl && tmp == il) {
                qDebug() << "[ImageOps] Left source detected as result of" << n << "th operation";
                gl = false;
                rec.link_l = n;
                if (!gr) break;
            }
            if (gr && tmp == ir) {
                qDebug() << "[ImageOps] Right source detected as result of" << n << "th operation";
                gr = false;
                rec.link_r = n;
                if (!gl) break;
            }
            n++;
        }
    }

    history.push_back(rec);
    pos = history.end();
}

QPixmap MImageOps::append(MImageListRecord const &in)
{
    if (!in.valid) return QPixmap();

    MMacroRecord rec;
    rec.action = MMacroRecord::None;
    rec.left = in;
    rec.result = in.picture;

    add(rec);
    return rec.result;
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

    add(rec);
    return rec.result;
}

QPixmap MImageOps::flip(MImageListRecord const &in, bool vertical)
{
    if (!in.valid) return QPixmap();

    MMacroRecord rec;
    rec.action = vertical? MMacroRecord::FlipVertical : MMacroRecord::FlipHorizontal;
    rec.left = in;
    rec.result = QPixmap::fromImage(in.picture.toImage().mirrored(!vertical,vertical));

    add(rec);
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

    add(rec);
    return rec.result;
}

QPixmap MImageOps::crop(MImageListRecord const &in, QRect const &rct)
{
    if (!in.valid) return QPixmap();

    MMacroRecord rec;
    rec.action = MMacroRecord::Crop;
    rec.left = in;
    rec.result = QPixmap::fromImage(in.picture.toImage().copy(rct));

    add(rec);
    return rec.result;
}

QPixmap MImageOps::first()
{
    if (history.empty()) return QPixmap();
    pos = history.begin();
    return current();
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

bool MImageOps::moveCurrent(bool backward)
{
    if (history.size() < 2) return false;
    bool last = (pos == history.end());
    if (last && !backward) return false;
    if (last) --pos;

    auto b = backward? pos-1 : pos+1;
    int ip = pos - history.begin();
    int ib = b - history.begin();

    for (auto &i : history) {
        if (i.link_l == ip) i.link_l = ib;
        else if (i.link_l == ib) i.link_l = ip;
        if (i.link_r == ip) i.link_r = ib;
        else if (i.link_r == ib) i.link_r = ip;
    }
    swap<MMacroRecord>(*pos,*b);

    if (last) pos = history.end();
    return true;
}

bool MImageOps::addComment(QString const &com)
{
    if (pos == history.end()) return false;
    pos->comment = com;
    pos->comment.replace('\"','\'');
    return true;
}

QString MImageOps::getComment()
{
    if (pos == history.end()) return QString();
    return pos->comment;
}

QString MImageOps::serialize()
{
    QString res;
    QTextStream ss(&res);
    for (auto &i : history) {
        ss << i.action << ";";
        ss << "\"" << i.left.filename << "\";";
        ss << i.link_l << ";";
        ss << "\"" << i.right.filename << "\";";
        ss << i.link_r << ";";
        ss << i.roi.x() << ";" << i.roi.y() << ";";
        ss << i.roi.width() << ";" << i.roi.height() << ";";
        ss << "\"" << i.comment << "\";";
    }
    return res;
}

bool MImageOps::deserialize(QString const &in)
{
    clear();
    if (in.isEmpty()) return true;

    int fsm = 0;
    QStringList lst = in.split(';');
    QString acc;
    MMacroRecord r;
    for (auto &i : lst) {
        acc += i;
        if (!acc.isEmpty() && acc.at(0) == '\"') {
            if (acc.at(acc.length()-1) != '\"') continue;
            else acc = acc.mid(1,acc.size()-2);
        }

        switch (fsm) {
        case 0:
            r.action = static_cast<MMacroRecord::ActionType>(acc.toInt());
            break;
        case 1:
            r.left.filename = acc;
            break;
        case 2:
            r.link_l = acc.toInt();
            break;
        case 3:
            r.right.filename = acc;
            break;
        case 4:
            r.link_r = acc.toInt();
            break;
        case 5:
            r.roi.setX(acc.toInt());
            break;
        case 6:
            r.roi.setY(acc.toInt());
            break;
        case 7:
            r.roi.setWidth(acc.toInt());
            break;
        case 8:
            r.roi.setHeight(acc.toInt());
            break;
        case 9:
            r.comment = acc;
            break;
        }

        if (++fsm == 10) {
            fsm = 0;
            history.push_back(r);
            r = MMacroRecord();
        }
        acc.clear();
    }

    //first pass - load files using ThumbnailModel instance
    for (auto &i : history) {
        QStringList ls = { i.left.filename, i.right.filename };
        ThumbnailModel* ptm = new ThumbnailModel(ls,0);
        ptm->LoadUp(0);
        ptm->LoadUp(1);

        if (!i.left.filename.isEmpty())
            i.left = ptm->data(ptm->getRecordIndex(i.left.filename),MImageListModel::FullDataRole).value<MImageListRecord>();
        if (!i.right.filename.isEmpty())
            i.right = ptm->data(ptm->getRecordIndex(i.right.filename),MImageListModel::FullDataRole).value<MImageListRecord>();

        delete ptm;
    }

    //second pass(es) - resolve links and create images
    loading = true;
    bool done = false;
    for (int failsafe = 0; !done && failsafe < history.size(); failsafe++) {
        done = true;
        for (auto &i : history) {
            if (!i.result.isNull()) continue;

            if (i.link_l >= 0 && i.link_l < history.size()) i.left.picture = history.at(i.link_l).result;
            if (i.link_r >= 0 && i.link_r < history.size()) i.right.picture = history.at(i.link_r).result;
            i.left.generated = i.left.filename.isEmpty();
            i.left.valid = !i.left.picture.isNull();
            i.right.generated = i.right.filename.isEmpty();
            i.right.valid = !i.right.picture.isNull();

            switch (i.action) {
            case MMacroRecord::None: i.result = append(i.left); break;
            case MMacroRecord::FlipVertical: i.result = flip(i.left,true); break;
            case MMacroRecord::FlipHorizontal: i.result = flip(i.left,false); break;
            case MMacroRecord::RotateCW: i.result = rotate(i.left,true); break;
            case MMacroRecord::RotateCCW: i.result = rotate(i.left,false); break;
            case MMacroRecord::Concatenate: i.result = concatenate(i.left,i.right); break;
            case MMacroRecord::Crop: i.result = crop(i.left,i.roi); break;
            }

            if (i.result.isNull()) done = false;
        }
    }
    loading = false;

    pos = history.end();
    return true;
}
