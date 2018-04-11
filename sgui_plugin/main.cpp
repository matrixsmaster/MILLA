#include "dialog.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    SGUICfgDialog w;
    w.show();

    return a.exec();
}
