#ifndef SPLASHSCREEN_H
#define SPLASHSCREEN_H

#include <QDialog>
#include <QPixmap>
#include <chrono>

#define MILLA_SPLASH_FRAMETIME 0.04
#define MILLA_SPLASH_NUMFRAMES 30
#define MILLA_SPLASH_CYCLE_MS 50

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
    void simpleShow();

protected:
    void showEvent(QShowEvent*);
    bool eventFilter(QObject*, QEvent *event);

private:
    Ui::SplashScreen *ui;
    int n_frame;
    bool frame_dir;
    std::chrono::steady_clock::time_point timer;
    std::vector<QPixmap> frames;
    std::vector<QString> license;
    bool clicked;
};

#endif // SPLASHSCREEN_H
