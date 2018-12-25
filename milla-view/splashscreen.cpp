#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QApplication>
#include <QDesktopWidget>
#include "splashscreen.h"
#include "ui_splashscreen.h"

using namespace std;
using namespace std::chrono;

SplashScreen::SplashScreen(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SplashScreen)
{
    ui->setupUi(this);
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setGeometry(QStyle::alignedRect(Qt::LeftToRight,Qt::AlignCenter,size(),qApp->desktop()->availableGeometry()));
}

SplashScreen::~SplashScreen()
{
    delete ui;
}

static void interpolateImages(QImage &a, QImage &b, QImage &r, double by)
{
    int cr,cg,cb;
    for (int y = 0; y < a.height(); y++) {
        QRgb* sa = reinterpret_cast<QRgb*>(a.scanLine(y));
        QRgb* sb = reinterpret_cast<QRgb*>(b.scanLine(y));
        QRgb* sr = reinterpret_cast<QRgb*>(r.scanLine(y));
        for (int x = 0; x < a.width(); x++,sa++,sb++,sr++) {
            cr = std::max(0,int((qRed(*sb) - qRed(*sa)) * by + qRed(*sa)));
            cg = std::max(0,int((qGreen(*sb) - qGreen(*sa)) * by + qGreen(*sa)));
            cb = std::max(0,int((qBlue(*sb) - qBlue(*sa)) * by + qBlue(*sa)));
            *sr = qRgb(cr,cg,cb);
        }
    }
}

void SplashScreen::showEvent(QShowEvent* /*ev*/)
{
    n_frame = 0;
    frame_dir = true;
    timer = steady_clock::now();
}

void SplashScreen::postShow()
{
    //load start and end frame states
    QImage start(":/splash_01.png");
    QImage end(":/splash_02.png");

    //create interpolated frames
    for (int i = 0; i < MILLA_SPLASH_NUMFRAMES; i++) {
        QImage fr(start.width(),start.height(),start.format());
        interpolateImages(start,end,fr,double(1)/double(MILLA_SPLASH_NUMFRAMES)*double(i));
        frames.push_back(QPixmap::fromImage(fr));
        QCoreApplication::processEvents();
    }
    qDebug() << "[SPLASH] " << frames.size() << " frames created";

    //open license file, set up the stream and show the license text
    QFile fl(":/license.txt");
    if (fl.open(QIODevice::Text | QIODevice::ReadOnly)) {
        QTextStream strm(&fl);
        while (!strm.atEnd())
            license.push_back(strm.readLine());
        fl.close();
    }
    qDebug() << "[SPLASH] " << license.size() << " lines read";

    //reset the frame timer
    timer = steady_clock::now();
}

bool SplashScreen::setProgress(double prg)
{
    ui->label_3->setText("Reading DB...");
    ui->progressBar->setValue(prg);

    duration<double> sec = duration_cast<duration<double>>(steady_clock::now() - timer);
    if (sec.count() > MILLA_SPLASH_FRAMETIME) {
        timer = steady_clock::now();

        if (!frames.empty()) {
            ui->label->setPixmap(frames.at(n_frame));

            if (frame_dir) {
                if (++n_frame >= int(frames.size())) {
                    n_frame--;
                    frame_dir = false;
                }
            } else {
                if (--n_frame < 0) {
                    n_frame++;
                    frame_dir = true;
                }
            }
        }

        if (!license.empty()) {
            QString str;
            auto it = license.begin() + int(prg / 100.f * double(license.size()));
            for (; it < license.end(); ++it) {
                str += *it;
                str += '\n';
            }
            ui->label_2->setText(str);
        }

        QCoreApplication::processEvents();
    }

    return true;
}
