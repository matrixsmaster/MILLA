#include <QDebug>
#include <QPainter>
#include <QFile>
#include <QDesktopWidget>
#include <opencv2/opencv.hpp>
#include "aboutbox.h"
#include "ui_aboutbox.h"
#include "shared.h"
#include "dbhelper.h"
#include "cvhelper.h"
#include "mmatcher.h"

AboutBox::AboutBox(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutBox)
{
    ui->setupUi(this);
    setGeometry(QStyle::alignedRect(Qt::LeftToRight,Qt::AlignCenter,size(),qApp->desktop()->availableGeometry()));
    ui->label->installEventFilter(this);
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
    QImage logo = ui->label->pixmap()->toImage();
    if (!logo.isNull()) {
        quad_size.setX(logo.width() / MILLA_ABOUT_MOSAIC_SIZE);
        quad_size.setY(logo.height() / MILLA_ABOUT_MOSAIC_SIZE);
        for (int y = 0; y < quad_size.y(); y++) {
            for (int x = 0; x < quad_size.x(); x++) {
                QRect r(x*MILLA_ABOUT_MOSAIC_SIZE,y*MILLA_ABOUT_MOSAIC_SIZE,MILLA_ABOUT_MOSAIC_SIZE,MILLA_ABOUT_MOSAIC_SIZE);
                //convert ROI into Mat
                QImage roi = logo.copy(r);
                cv::Mat in = CVHelper::slowConvert(roi);
                //image histogram
                cv::Mat hst = CVHelper::getHist(in);
                quads.push_back(std::pair<cv::Mat,float>(hst,0));
            }
        }
    }

    //reset everything
    visited.clear();
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
    files = DBHelper::getAllFiles();
    if (!files.isEmpty()) {
        connect(&timer,&QTimer::timeout,this,[this] { this->Mosaic(); });
        timer.start(MILLA_ABOUT_MOSAIC_TIMER);
    }

    //prepare the mutatable logo
    prepareLogo();

    //ok, we're done
    ev->accept();
}

bool AboutBox::eventFilter(QObject *obj, QEvent *event)
{
    if (obj != ui->label) return QObject::eventFilter(obj,event);

    switch (event->type()) {
    case QEvent::MouseButtonPress:
        prepareLogo();
        return true;

    default: break;
    }

    return QObject::eventFilter(obj,event);
}

void AboutBox::Mosaic()
{
    if (int(visited.size()) >= files.size()) {
        prepareLogo();
        return;
    }

    int idx;
    do {
        idx = floor(double(random()) / double(RAND_MAX) * double(files.size()));
    } while (int(visited.size()) < files.size() && visited.count(idx));
    visited.insert(idx);

    MImageListRecord rec;
    rec.filename = files.at(idx);
    if (!DBHelper::getThumbnail(rec)) {
        //no thumbnail for this record, let's try again with a different file
        Mosaic(); //tail recursion
        return;
    }

    MImageExtras exrec = DBHelper::getExtrasFromDB(rec.filename);
    if (!exrec.valid || exrec.hist.empty()) return;

    QRect trct = rec.thumb.rect();
    if (trct.width() > trct.height()) {
        trct.setLeft((trct.width()-trct.height())/2);
        trct.setWidth(trct.height());
    } else {
        trct.setTop((trct.height()-trct.width())/2);
        trct.setHeight(trct.width());
    }

    double df = 0;
    int n = 0, wn = -1;
    for (auto &i : quads) {
        double v = MMatcher::OneTimeMatcher(i.first,exrec.hist);
        if (v > df && v > quads.at(n).second) {
            df = v;
            wn = n;
        }
        n++;
    }

    if (wn < 0) return;
    quads.at(wn).second = df;

    int x = wn % quad_size.x();
    int y = wn / quad_size.y();
    QRect targ(x*MILLA_ABOUT_MOSAIC_SIZE,y*MILLA_ABOUT_MOSAIC_SIZE,MILLA_ABOUT_MOSAIC_SIZE,MILLA_ABOUT_MOSAIC_SIZE);

    QImage over(ui->label->pixmap()->toImage());
    QPainter paint(&over);
    paint.drawPixmap(targ,rec.thumb,trct);
    ui->label->setPixmap(QPixmap::fromImage(over));
}
