#include "mviewer.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    MViewer w;
    w.show();
    w.processArguments();

    return a.exec();
}
