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
    //prepare main fields
    ui->label_6->setText("<a href=" MILLA_SITE ">GitHub site</a>");
    ui->label_4->setText("Version " MILLA_VERSION);
    ui->label_5->setText("Built with OpenCV ver." CV_VERSION);

#ifdef QT_DEBUG
    //Draw "Debug build" string on the logo
    QImage over(ui->label->pixmap()->toImage());
    QPainter paint(&over);
    paint.rotate(63);
    paint.setPen(Qt::white);
    paint.setFont(QFont("Noto Sans",37,QFont::Bold));
    paint.drawText(170,-30,"DEBUG BUILD");
    ui->label->setPixmap(QPixmap::fromImage(over));
#endif

    //open license file, set up the stream and show the license text
    QFile fl(":/license.txt");
    if (!fl.open(QIODevice::Text | QIODevice::ReadOnly)) return;
    QTextStream strm(&fl);
    ui->textBrowser->setText(strm.readAll());
    fl.close();
}
