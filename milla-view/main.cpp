#include "mviewer.h"
#include <iostream>
#include <QApplication>

using namespace std;

int main(int argc, char *argv[])
{
    cout << MILLA_CLI_BANNER " ver. " MILLA_VERSION << endl;
    cout << "(C) Dmitry 'MatrixS_Master' Soloviov, 2018. All rights reserved." << endl;

    QApplication a(argc, argv);
    setlocale(LC_NUMERIC,"C");
    srandom(time(NULL));

#ifdef QT_DEBUG
    qDebug() << "DEBUG BUILD RUNNING";
#endif

    QDir cfg_path(QDir::homePath() + MILLA_CONFIG_PATH);
    if (!cfg_path.exists()) cfg_path.mkpath(".");

    MViewer w;
    w.show();
    w.processArguments();

    return a.exec();
}
