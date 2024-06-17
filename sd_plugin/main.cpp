#include "sdcfgdialog.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    SDCfgDialog w;
    w.show();
    return a.exec();
}
