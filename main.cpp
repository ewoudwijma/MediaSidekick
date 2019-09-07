#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QSettings::setDefaultFormat(QSettings::IniFormat);

    MainWindow w;

//    w.setGeometry(1536,512,2048,960);
    w.show();

    return a.exec();
}

//deploy
//cd D:\Projects\build-Fipre-Desktop_Qt_5_12_3_MSVC2017_32bit-Release\release>
//d:\Qt\5.12.3\msvc2017\bin\windeployqt.exe --quick --no-translations .
