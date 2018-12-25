#include "splashscreen.h"
#include "ui_splashscreen.h"

SplashScreen::SplashScreen(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SplashScreen)
{
    ui->setupUi(this);
}

SplashScreen::~SplashScreen()
{
    delete ui;
}

bool SplashScreen::setProgress(double prg)
{
    ui->progressBar->setValue(prg);
    QCoreApplication::processEvents();
    return true;
}
