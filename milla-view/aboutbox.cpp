#include <QDebug>
#include <QPainter>
#include <QFile>
#include <opencv2/opencv.hpp>
#include "aboutbox.h"
#include "ui_aboutbox.h"
#include "shared.h"

AboutBox::AboutBox(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutBox)
{
    ui->setupUi(this);
}

AboutBox::~AboutBox()
{
    delete ui;
}

void AboutBox::showEvent(QShowEvent* /*ev*/)
{
    ui->label_6->setText("<a href=" MILLA_SITE ">GitHub site</a>");
    ui->label_4->setText("Version " MILLA_VERSION);
    ui->label_5->setText("Built with OpenCV ver." CV_VERSION);

#ifdef QT_DEBUG
//    QImage over(ui->label->size(),QImage::Format_ARGB32);
    QImage over(ui->label->pixmap()->toImage());
    QPainter paint(&over);
    paint.rotate(63);
    paint.setPen(Qt::white);
    paint.setFont(QFont("Noto Sans",37,QFont::Bold));
//    paint.drawText(over.rect(), Qt::AlignCenter, "DEBUG BUILD");
    paint.drawText(170,-30,"DEBUG BUILD");

//    QImage main(ui->label->pixmap()->toImage());
//    QPainter sub(&main);
//    sub.drawImage(0,0,)
    ui->label->setPixmap(QPixmap::fromImage(over));
#endif

    //open license file, set up the stream and show the license text
    QFile fl(":/license.txt");
    if (!fl.open(QIODevice::Text | QIODevice::ReadOnly)) return;
    QTextStream strm(&fl);
    ui->textBrowser->setText(strm.readAll());
    fl.close();
}
