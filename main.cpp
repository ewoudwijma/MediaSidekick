#include "mainwindow.h"
#include <QApplication>
#include <QSplashScreen>

#include "fglobal.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QSettings::setDefaultFormat(QSettings::IniFormat);

    QPixmap pixmap(":/fiprelogo.png");
    QSplashScreen *splash = new QSplashScreen(pixmap);

    //    splash.set
    splash->show();

    splash->showMessage("Loaded modules");

    a.processEvents();

    MainWindow w;

//    w.setGeometry(1536,512,2048,960);
    w.show();
    splash->showMessage("Established connections");
    splash->finish(&w);

    return a.exec();
}

//deploy
//cd D:\Projects\build-Fipre-Desktop_Qt_5_12_3_MSVC2017_32bit-Release\release>
//d:\Qt\5.12.3\msvc2017\bin\windeployqt.exe --quick --no-translations .

