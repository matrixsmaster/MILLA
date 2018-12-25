#ifndef SPLASHSCREEN_H
#define SPLASHSCREEN_H

#include <QDialog>
#include <QPixmap>
#include <chrono>

#define MILLA_SPLASH_FRAMETIME 0.04
#define MILLA_SPLASH_NUMFRAMES 30

namespace Ui {
class SplashScreen;
}

class SplashScreen : public QDialog
{
    Q_OBJECT

public:
    explicit SplashScreen(QWidget *parent = 0);
    ~SplashScreen();

    void postShow();

    bool setProgress(double prg);

protected:
    void showEvent(QShowEvent*);

private:
    Ui::SplashScreen *ui;
    int n_frame;
    bool frame_dir;
    std::chrono::steady_clock::time_point timer;
    std::vector<QPixmap> frames;
    std::vector<QString> license;
};

#endif // SPLASHSCREEN_H
