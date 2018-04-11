#include "dialog.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    PixMixCfgDialog w;
    w.show();

    return a.exec();
}
