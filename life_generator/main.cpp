#include "dialog.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    LifeCfgDialog w;
    w.show();

    return a.exec();
}
