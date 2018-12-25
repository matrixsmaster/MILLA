#ifndef SPLASHSCREEN_H
#define SPLASHSCREEN_H

#include <QDialog>

namespace Ui {
class SplashScreen;
}

class SplashScreen : public QDialog
{
    Q_OBJECT

public:
    explicit SplashScreen(QWidget *parent = 0);
    ~SplashScreen();

    bool setProgress(double prg);

private:
    Ui::SplashScreen *ui;
};

#endif // SPLASHSCREEN_H
