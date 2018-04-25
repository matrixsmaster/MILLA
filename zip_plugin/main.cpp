#include "zipcfgdialog.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    ZipCfgDialog w;
    w.show();

    return a.exec();
}
