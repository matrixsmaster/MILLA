#include "movcfgdialog.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MovCfgDialog w;
    w.show();

    return a.exec();
}
