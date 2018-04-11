#include <QDebug>
#include <QTextStream>
#include "lifegenplugin.h"
#include "dialog.h"

LifeGenPlugin::LifeGenPlugin(QObject *parent) :
    QObject(parent),
    MillaGenericPlugin()
{
}

bool LifeGenPlugin::init()
{
    qDebug() << "[LifeGen] Init OK";
    return true;
}

bool LifeGenPlugin::finalize()
{
    qDebug() << "[LifeGen] Finalize OK";
    return true;
}

void LifeGenPlugin::showUI()
{
    LifeCfgDialog dlg;
    if (dlg.exec()) text_life = dlg.getData();
    else text_life.clear();
}

QVariant LifeGenPlugin::getParam(QString key)
{
    if (key == "update_delay") {
        return int(LIFEGEN_UPDATE);

    } else if (key == "use_config_cb") {
        return true;

    }
    return QVariant();
}

bool LifeGenPlugin::setParam(QString key, QVariant val)
{
    if (key == "process_started" && val.value<bool>()) {
        field = QImage();
        return true;
    }
    return false;
}

void LifeGenPlugin::randomInit(QSize const &sz)
{
    field = QImage(sz,QImage::Format_RGB32);

    for (int i = 0; i < sz.height(); i++) {
        uint32_t* line = reinterpret_cast<uint32_t*>(field.scanLine(i));
        for (int j = 0; j < sz.width(); j++,line++)
            *line = (random() < (RAND_MAX / 10))? 0xffffffff : 0xff000000;
    }
}

void LifeGenPlugin::imageInit(QSize const &sz, QPixmap const &in)
{
    if (in.isNull()) return;

    field = in.scaled(sz).toImage().convertToFormat(QImage::Format_RGB32);

    for (int i = 0; i < field.size().height(); i++) {
        uchar* px = field.scanLine(i);
        for (int j = 0; j < field.size().width(); j++,px+=4) {
            if (px[0] > 0x50 && px[2] > 0x50) {
                for (int k = 0; k < 4; k++) px[k] = 0xff;
            }
        }
    }
}

void LifeGenPlugin::textInit(QSize const &sz)
{
    if (text_life.isEmpty()) return;

    field = QImage(sz,QImage::Format_RGB32);

    for (int i = 0; i < sz.height(); i++) {
        uint32_t* line = reinterpret_cast<uint32_t*>(field.scanLine(i));
        for (int j = 0; j < sz.width(); j++,line++) *line = 0xff000000;
    }

    int fsm = 0, ccx = 0, cx = 0, cy = 0;
    for (auto &i : text_life) {
        QString s = i.trimmed();
        if (s.isEmpty()) continue;

        switch (fsm) {
        case 0:
            if (s.startsWith("#Life 1.05",Qt::CaseInsensitive)) fsm++; //file type
            break;

        case 1:
            if (s.startsWith("#N",Qt::CaseInsensitive)) fsm++; //normal (standard) rules
            break;

        case 2:
        case 3:
            if (s.startsWith("#P",Qt::CaseInsensitive)) {
                s.remove(0,2);
                QTextStream ss(&s);
                int dx,dy;
                ss >> dx;
                ss >> dy;
                qDebug() << "[LifeGen] P-tag: " << dx << dy;

                cx = sz.width() / 2 + dx;
                cy = sz.height() / 2 + dy;
                if (cx < 0) cx = 0;
                if (cx >= sz.width()) cx = sz.width() - 1;
                if (cy < 0) cy = 0;
                if (cy >= sz.height()) cy = sz.height() - 1;
                ccx = cx;

                fsm++;

            } else if (fsm == 3) {
                for (auto &j : s.toLatin1()) {
                    if (j == '*') {
                        uint32_t* line = reinterpret_cast<uint32_t*>(field.scanLine(cy));
                        line += cx;
                        *line = 0xffffffff;
                    }
                    if (++cx >= sz.width()) break;
                }
                cx = ccx;
                if (++cy >= sz.height()) fsm = 2; //skip everything to the next P tag
            }
        }
    }

    text_life.clear();
    qDebug() << "[LifeGen] Parsing done";
}

void LifeGenPlugin::singleStep()
{
    QImage nwfld(field);

    for (int i = 0; i < field.size().height(); i++) {
        uint32_t* line = reinterpret_cast<uint32_t*>(field.scanLine(i));
        for (int j = 0; j < field.size().width(); j++,line++) {
            QPoint p(j,i);
            int n = neighbours(p);
            if (alive(field,p)) {
                if (n < 2 || n > 3) kill(nwfld,p);
            } else {
                if (n == 3) born(nwfld,p);
                else fade(field,nwfld,p);
            }
        }
    }

    field = nwfld; //blit
}

bool LifeGenPlugin::alive(QImage &ref, QPoint const &p)
{
    uint32_t* line = reinterpret_cast<uint32_t*>(ref.scanLine(p.y()));
    line += p.x();
    return *line == 0xffffffff;
}

void LifeGenPlugin::kill(QImage &ref, QPoint const &p)
{
    uint32_t* line = reinterpret_cast<uint32_t*>(ref.scanLine(p.y()));
    line += p.x();
    *line = 0xff808080;
}

void LifeGenPlugin::born(QImage &ref, QPoint const &p)
{
    uint32_t* line = reinterpret_cast<uint32_t*>(ref.scanLine(p.y()));
    line += p.x();
    *line = 0xffffffff;
}

void LifeGenPlugin::fade(QImage &from, QImage &to, QPoint const &p)
{
    uint32_t* line = reinterpret_cast<uint32_t*>(from.scanLine(p.y()));
    line += p.x();
    if (!(*line & 0x00ffffff)) return; //already fully dead

    int n = static_cast<int>((*line) & 0x00ffffff);
    n -= 0x040504;
    if (n < 0) n = 0;

    line = reinterpret_cast<uint32_t*>(to.scanLine(p.y()));
    line += p.x();
    *line = (static_cast<uint32_t>(n) & 0x00ffffff) | 0xff000000;
}

int LifeGenPlugin::neighbours(QPoint const &p)
{
    int n = 0;
    for (int i = -1; i < 2; i++) {
        if (p.y() + i < 0 || p.y() + i >= field.size().height()) continue;
        for (int j = -1; j < 2; j++) {
            if (i == j && j == 0) continue;
            if (p.x() + j < 0 || p.x() + j >= field.size().width()) continue;
            QPoint pp(p.x() + j, p.y() + i);
            if (alive(field,pp)) n++;
        }
    }
    return n;
}

QVariant LifeGenPlugin::action(QVariant in)
{
    if (!in.canConvert<QSize>()) {
        qDebug() << "[LifeGen] Action(): invalid QVariant";
        return QVariant();
    }
    QSize sz = in.value<QSize>();

    if (field.isNull() && !text_life.isEmpty())
        textInit(sz);

    if (field.isNull() && config_cb) {
        QVariant r(config_cb("get_left_image",QVariant()));
        if (r.canConvert<QPixmap>()) imageInit(sz,r.value<QPixmap>());
    }

    if (field.isNull())
        randomInit(sz);

    if (!field.isNull()) {
        singleStep();
        return QPixmap::fromImage(field);
    }

    return QVariant();
}
