#include "mimageops.h"
#include "thumbnailmodel.h"
#include "dbhelper.h"

using namespace std;

MImageOps::MImageOps(MImageLoader* imgLoader, QObject *parent) :
    QObject(parent),
    loader(imgLoader)
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
    rec.roi = rct;
    rec.result = QPixmap::fromImage(in.picture.toImage().copy(rct));

    add(rec);
    return rec.result;
}

QPixmap MImageOps::fillrect(MImageListRecord const &in, QRect const &rct)
{
    if (!in.valid) return QPixmap();

    MMacroRecord rec;
    rec.action = MMacroRecord::FillRect;
    rec.left = in;
    rec.roi = rct;

    QImage inq(in.picture.toImage());
    QPainter painter(&inq);
    painter.fillRect(rct,CVHelper::determineMediumColor(in.picture,rct));
    rec.result = QPixmap::fromImage(inq);

    add(rec);
    return rec.result;
}

QPixmap MImageOps::colorize(MImageListRecord const &in)
{
    if (!in.valid) return QPixmap();

    MMacroRecord rec;
    rec.action = MMacroRecord::Colorize;
    rec.left = in;

    CVHelper hlp;
    rec.result = hlp.recolorImage(in.picture);

    add(rec);
    return rec.result;
}

QPixmap MImageOps::first()
{
    if (history.empty()) return QPixmap();
    pos = history.begin();
    return current();
}

QPixmap MImageOps::previous(bool commented_only)
{
    while (pos != history.begin()) {
        --pos;
        if (!commented_only || !pos->comment.isEmpty()) break;
    }
    return current();
}

QPixmap MImageOps::next(bool commented_only)
{
    while (pos != history.end()) {
        ++pos;
        if ((!commented_only) || (pos != history.end() && !pos->comment.isEmpty())) break;
    }
    return current();
}

QPixmap MImageOps::current()
{
    if (pos == history.end()) {
        if (history.empty()) return QPixmap();
        --pos;
    }
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

QString MImageOps::serializeFileRecord(const MImageListRecord &rec, int link)
{
    if (link >= 0) return QString();

    QString s;
    QTextStream ss(&s);

    if (!rec.generated) {
        QByteArray sha = DBHelper::getSHAbyFile(rec.filename);
        if (sha.isEmpty()) {
            qDebug() << "Unable to get SHA-256 for " << rec.filename;
            return rec.filename;
        }
        ss << sha.toHex();

    } else {
        QImage img = rec.picture.toImage();
        if (img.isNull()) {
            qDebug() << "[ALERT] Unable to get source image for encoding!";
            return rec.filename;
        }

        QByteArray enc;
        QBuffer buf(&enc);
        buf.open(QIODevice::WriteOnly);
        if (!img.save(&buf,"PNG")) {
            qDebug() << "ERROR: Unable to encode image to PNG";
            return rec.filename;
        }

        ss << "!";
        ss << enc.toBase64(QByteArray::Base64Encoding);
    }

    return s;
}

bool MImageOps::deserializeFileRecord(MImageListRecord &rec, int link)
{
    if (rec.filename.isEmpty()) return (link >= 0);

    switch (rec.filename.at(0).toLatin1()) {
    case '/': //ordinary path
        break;

    case '!': //base64-encoded PNG
    {
        rec.filename.remove(0,1);
        rec.generated = true;

        QImage img;
        QByteArray dec = QByteArray::fromBase64(rec.filename.toLatin1(),QByteArray::Base64Encoding);
        if (dec.isEmpty()) return false;

        rec.valid = img.loadFromData(dec,"PNG");
        rec.picture = QPixmap::fromImage(img);
        rec.filename.clear();
    }
        break;

    default: //by SHA-256 sum
    {
        QByteArray sha = QByteArray::fromHex(rec.filename.toLatin1());
        rec.filename = DBHelper::getFileBySHA(sha);
    }
        break;
    }

    if (!rec.filename.isEmpty())
        rec = loader->loadFull(rec.filename);

    return true;
}

QString MImageOps::serialize()
{
    QString res;
    QTextStream ss(&res);
    for (auto &i : history) {
        ss << i.action << ";";
        ss << "\"" << serializeFileRecord(i.left,i.link_l) << "\";";
        ss << i.link_l << ";";
        ss << "\"" << serializeFileRecord(i.right,i.link_r) << "\";";
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

    //first pass - load files
    for (auto &i : history) {
        if ((!deserializeFileRecord(i.left,i.link_l)) || (!deserializeFileRecord(i.right,i.link_r)))
            qDebug() << "WARNING: One of the source files couldn't be loaded!";
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
            case MMacroRecord::FillRect: i.result = fillrect(i.left,i.roi); break;
            case MMacroRecord::Colorize: i.result = colorize(i.left); break;
            }

            if (i.result.isNull()) done = false;
        }
    }
    loading = false;

    pos = history.end();
    return true;
}
