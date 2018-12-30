#include <QDebug>
#include <QPainter>
#include <QFile>
#include <opencv2/opencv.hpp>
#include "aboutbox.h"
#include "ui_aboutbox.h"
#include "shared.h"
#include "dbhelper.h"
#include "mmatcher.h"


//TODO: tidy this mess up a bit

AboutBox::AboutBox(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutBox)
{
    ui->setupUi(this);
//    timer = nullptr;
//    busy = false;
}

AboutBox::~AboutBox()
{
    delete ui;
}

void AboutBox::prepareLogo()
{
    std::vector<QString> logos = MILLA_ABOUT_LOGOS;
    int idx = floor(double(random()) / double(RAND_MAX) * double(logos.size()));

    QImage recolor(":/"+logos.at(idx)+".png");
    int shift = floor(double(random()) / double(RAND_MAX) * 360.f);
    for (int y = 0; y < recolor.height(); y++) {
        QRgb* sp = reinterpret_cast<QRgb*>(recolor.scanLine(y));
        for (int x = 0; x < recolor.width(); x++,sp++) {
            QColor nc = QColor(*sp).toHsl();
            int nh = nc.hue() + shift;
            if (nh > 360) nh -= 360;
            nc.setHsl(nh,nc.saturation(),nc.lightness());
            *sp = nc.rgb();
        }
    }
    ui->label->setPixmap(QPixmap::fromImage(recolor));

#ifdef QT_DEBUG
    //Draw "Debug build" string on the logo
    QImage over(ui->label->pixmap()->toImage());
    QPainter paint(&over);
    paint.rotate(60);
    paint.setPen(Qt::white);
    paint.setFont(QFont("Noto Sans",37,QFont::Bold));
    paint.drawText(170,-50,"DEBUG BUILD");
    ui->label->setPixmap(QPixmap::fromImage(over));
#endif

    //populate quads data of the logo image
    quads.clear();
    QPixmap logo = *ui->label->pixmap();//->scaled(ui->label->size(),Qt::KeepAspectRatio,Qt::SmoothTransformation);
    if (!logo.isNull()) {
        qDebug() << logo.width() << logo.height();
        quad_size.setX(logo.width() / MILLA_ABOUT_MOSAIC_SIZE);
        quad_size.setY(logo.height() / MILLA_ABOUT_MOSAIC_SIZE);
        qDebug() << quad_size.x() << quad_size.y();
        for (int y = 0; y < quad_size.y(); y++) {
            for (int x = 0; x < quad_size.x(); x++) {
                QRect r(x*MILLA_ABOUT_MOSAIC_SIZE,y*MILLA_ABOUT_MOSAIC_SIZE,MILLA_ABOUT_MOSAIC_SIZE,MILLA_ABOUT_MOSAIC_SIZE);
#if 1
                QColor c = CVHelper::determineMediumColor(logo,r);
                qDebug() << r << c;
                quads.push_back(std::pair<QColor,int>(c,MILLA_ABOUT_MOSAIC_MAXDIFF));
#else
                QPixmap rp = logo.copy(r);
                std::pair<cv::Mat,double> c;
                c.first = CVHelper::getHist(rp);
                c.second = MILLA_ABOUT_MOSAIC_MINDIFF;
                quads.push_back(c);
#endif
            }
        }
    }

    //reset everything
    visited.clear();
    misscount = 0;
}

void AboutBox::showEvent(QShowEvent* ev)
{
    //prepare main fields
    ui->label_6->setText("<a href=" MILLA_SITE ">GitHub site</a>");
    ui->label_4->setText("Version " MILLA_VERSION);
    ui->label_5->setText("Built with OpenCV ver." CV_VERSION);

    //open license file, set up the stream and show the license text
    QFile fl(":/license.txt");
    if (!fl.open(QIODevice::Text | QIODevice::ReadOnly)) return;
    QTextStream strm(&fl);
    ui->textBrowser->setText(strm.readAll());
    fl.close();

    //get the list of all files and start the timer
//    timer = new QTimer();
    files = DBHelper::getAllFiles();
    if (!files.isEmpty()) {
        connect(&timer,&QTimer::timeout,this,[this] { this->Mosaic(); });
        timer.start(MILLA_ABOUT_MOSAIC_TIMER);
    }

    //prepare the mutatable logo
    prepareLogo();

    //add stopper
//    connect(this,&QDialog::finished,this,[this] { this->Stop(); });

    //ok, we're done
    ev->accept();
}

#if 0
void AboutBox::Stop()
{
//    if (!timer) return;
//    timer.stop();
//    delete timer;
//    timer = nullptr;
    qDebug() << "Stopped";
}
#endif

void AboutBox::Mosaic()
{
    if (int(visited.size()) >= files.size()) {
        prepareLogo();
        return;
    }

//    busy = true;

    int idx;
    do {
        idx = floor(double(random()) / double(RAND_MAX) * double(files.size()));
    } while (int(visited.size()) < files.size() && visited.count(idx));
    visited.insert(idx);

    MImageListRecord rec;
    rec.filename = files.at(idx);
    if (!DBHelper::getThumbnail(rec)) {
//        busy = false;
        return;
    }

#if 1
    QColor col = CVHelper::determineMediumColor(rec.thumb,rec.thumb.rect());
    qDebug() << idx << rec.filename << col << rec.thumb.rect();

    int ha = col.toHsl().hue();
    int df = MILLA_ABOUT_MOSAIC_MAXDIFF;
    int n = 0, wn = -1;
    for (auto &i : quads) {
        int v = std::abs(i.first.toHsl().hue()-ha);
//        qDebug() << v;
        if (v < df) {
            df = v;
            wn = n;
        }
        n++;
    }
    qDebug() << ha << df << wn;
    if (wn < 0 || df > quads.at(wn).second) {
        if (++misscount >= MILLA_ABOUT_MOSAIC_MAXMISS) prepareLogo();
//        busy = false;
        return;
    }
#else
    cv::Mat his = CVHelper::getHist(rec.thumb);
    int df = MILLA_ABOUT_MOSAIC_MINDIFF;
    int n = 0, wn = -1;
    for (auto &i : quads) {
        double v = MMatcher::OneTimeMatcher(his,i.first);
        if (v > df) {
            df = v;
            wn = n;
        }
        n++;
    }
    qDebug() << df << wn;
    if (wn < 0 || df < quads.at(wn).second) return;
#endif

    quads.at(wn).second = df;

    int x = wn % quad_size.x();
    int y = wn / quad_size.y();
    QRect targ(x*MILLA_ABOUT_MOSAIC_SIZE,y*MILLA_ABOUT_MOSAIC_SIZE,MILLA_ABOUT_MOSAIC_SIZE,MILLA_ABOUT_MOSAIC_SIZE);

    QImage over(ui->label->pixmap()->toImage());
    QPainter paint(&over);
    paint.drawPixmap(targ,rec.thumb,rec.thumb.rect());
    ui->label->setPixmap(QPixmap::fromImage(over));
}
