#include "mainwindow.h"
#include <QApplication>
#include <QSplashScreen>

#include "fglobal.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setOrganizationDomain("actioncamvideocompanion.com");
    a.setApplicationName("ACVC");
    a.setApplicationVersion("0.0.3");

    QSettings::setDefaultFormat(QSettings::IniFormat);
    //C:\Users\<user>\AppData\Roaming\actioncamvideocompanion.com

    if (QSettings().value("framerate").toInt() == 0)
    {
        QSettings().setValue("frameRate", 25);
        QSettings().sync();
    }

    QPixmap pixmap(":/acvc.ico");
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

//deploy in shell (not powershell...)
//cd D:\Projects\build-ACVC-Desktop_Qt_5_12_5_MinGW_64_bit-Release\release
//d:\Qt\5.12.3\msvc2017\bin\windeployqt.exe --quick --no-translations .
//C:\Qt\5.12.6\mingw73_64\bin\qtenv2.bat
//c:\Qt\5.12.6\mingw73_64\bin\windeployqt.exe --quick --no-translations .

//http://www.gnu.org/licenses/lgpl-3.0.html
//https://doc.qt.io/qtinstallerframework/index.html

//    ..\..\bin\binarycreator.exe -c config\config.xml -p packages ACVCInstaller.exe

//<p><a href="https://ffmpeg.zeranoe.com/builds/">FFMpeg</a> (extract, move to c:\ffmpeg\, add \bin in environment variabe)</p>

//<p><a href="https://www.sno.phy.queensu.ca/~phil/exiftool/">Exiftool</a> (download zip, remove -k, move to c:\ffmpeg\bin)</p>
