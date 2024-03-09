#include <QApplication>
#include "annacfgdialog.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    AnnaCfgDialog w;
    w.show();

    return a.exec();
}
