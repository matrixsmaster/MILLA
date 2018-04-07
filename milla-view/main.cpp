#include "mviewer.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    setlocale(LC_NUMERIC,"C");
    srandom(time(NULL));

#ifdef QT_DEBUG
    qDebug() << "DEBUG BUILD RUNNING";
#endif

    MViewer w;
    w.show();
    w.processArguments();

    return a.exec();
}
