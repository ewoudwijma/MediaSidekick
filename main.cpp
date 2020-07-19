#include "mainwindow.h"
#include <QApplication>
#include <QSplashScreen>

#include "aglobal.h"

//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
////#include <libswscale/swscale.h>
////#include <libavutil/avutil.h>
////#include <libavdevice/avdevice.h>
////#include <libavfilter/avfilter.h>
////#include <libpostproc/postprocess.h>
////#include <libswresample/swresample.h>
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

#ifdef falseQ_OS_WIN
    AVFormatContext *fmt_ctx = NULL;
    AVDictionaryEntry *tag = NULL;
    int ret;

//    if ((ret = avformat_open_input(&fmt_ctx, "D:\\Video\\Bras_DVR\\Encode2.7K@30.mp4", NULL, NULL)))
#ifdef Q_OS_WIN
    if ((ret = avformat_open_input(&fmt_ctx, "D:\\Video\\2019-04 La Palma archive\\GoPro\\2019-04-19 08.46.10 GoPro-1.mp4", NULL, NULL)))
        return ret;
#else
    if ((ret = avformat_open_input(&fmt_ctx, "/Users/ewoudwijma/Movies/2019-09-02 Fipre/2000-01-19 00-00-00 +53632ms.MP4", NULL, NULL)))
        return ret;
#endif

    if ((ret = avformat_find_stream_info(fmt_ctx, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot find stream information\n");
        return ret;
    }

    AVInputFormat *iformat = fmt_ctx->iformat;
    qDebug()<<"Input format info"<<iformat->name<<iformat->long_name<<iformat->mime_type<<iformat->codec_tag<<iformat->raw_codec_id;

    AVStream **streams = fmt_ctx->streams;
    AVStream *stream = streams[0];

    AVCodec *codec = avcodec_find_decoder(stream->codecpar->codec_id);

    if (!codec) {
                qDebug() << "Could not find input codec.";
    }

    qDebug()<<"stream and codec info"<<stream->codecpar<<codec;
    qDebug()<<"stream and codec info"<<stream->info<<stream->duration<<stream->avg_frame_rate.num<<stream->nb_frames<<codec->name<<codec->long_name<<codec->type<<codec->id;
    qDebug()<<"stream and codec info"<<stream->info<<stream->duration<<stream->avg_frame_rate.num<<stream->nb_frames<<stream->codecpar->codec_id<<stream->codecpar->codec_type<<stream->codecpar->format<<stream->codecpar->codec_tag<<stream->codecpar->width<<stream->codecpar->height;

    qDebug()<<"file meta"<<av_dict_count(fmt_ctx->metadata);
    while ((tag = av_dict_get(fmt_ctx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX)))
        qDebug()<<tag->key<<tag->value;

    qDebug()<<"Stream meta"<<av_dict_count(stream->metadata);
    while ((tag = av_dict_get(stream->metadata, "", tag, AV_DICT_IGNORE_SUFFIX)))
        qDebug()<<"  "<<tag->key<<tag->value;

    avformat_close_input(&fmt_ctx);
#endif

//    qRegisterMetaType<AJobParams>("AJobParams"); //so signal/slots can use it (jobAddlog)
    qRegisterMetaType<QList<int>>("QList<int>");
    //otherwise: QObject::connect: Cannot queue arguments of type 'QPainterPath' (Make sure 'QPainterPath' is registered using qRegisterMetaType().)

    a.setOrganizationDomain("mediasidekick.org");
    a.setApplicationName("Media Sidekick");
    a.setApplicationVersion("0.4.5.2");

    QSettings::setDefaultFormat(QSettings::IniFormat);
    //C:\Users\<user>\AppData\Roaming\mediasidekick.org

    if (QSettings().value("frameRate").toInt() == 0)
    {
        QSettings().setValue("frameRate", "25");
        QSettings().sync();
        qDebug()<<"frameRate"<<QSettings().value("frameRate")<<QSettings().value("frameRate").toInt();
    }

    QPixmap pixmap(":/MediaSidekick.ico");
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

//https://stackoverflow.com/questions/30814475/qml-module-not-installed-error-running-qt-app-on-embedded-linux

//https://stackoverflow.com/questions/46455360/workaround-for-qt-installer-framework-not-overwriting-existing-installation

//Derperview VCRuntime140_1.dll error: https://aka.ms/vs/16/release/VC_redist.x64.exe
//

//install shotcut
//https://shotcut.org/notes/windowsdev/


/*
 *
 * Certificate:
 *  - https://www.sslforfree.com / mediasidekick.org / manual verification / download / copy certificate and private key
 *  - cp.ashosting.nl / website beheer / website configuratie / ssl / paste certificate and private key
 *  - ftp downloaded files / upload

$QTmingwFolder = C:\Qt\5.14.2\mingw73_64
$QTmingwFolder = C:\Qt\5.14.1\mingw73_64

add to $QTmingwFolder


    - make sure bin, include and lib exists
            storing C:\Users\ewoud\OneDrive\Documents\Media Sidekick project\Media Sidekick support files\additional\Win for the time being

and more... (temp?)
    - lib_win_x86_64 contents in the main folder
    - $QTmingwFolder\bin\ contents in the main folder (a lot)
    - then it works ...


Clean install of both QT/Mingw/QtAV and deploy folder

    - Project ERROR: Unknown module(s) in QT: avwidgets / :-1: error: Unknown module(s) in QT: avwidgets
        - QtAV: run sdk_deploy.bat from D:\ACVC\QtAV-Qt5.9-VS2017x64-release-6067154 to the mingw folder (copying avcodec 57 to bin, conflict with 58???)
            - https://github.com/wang-bin/QtAV/wiki/Use-QtAV-In-Your-Projects
            - https://github.com/wang-bin/QtAV/wiki/Deploy-SDK-Without-Building-QtAV
            - https://forum.qt.io/topic/106288/linker-problem-with-qtav
            - https://github.com/wang-bin/QtAV/wiki/Build-QtAV (not needed, except to get lib_win_x86_64 stuff)
    - D:\ACVC\build-ACVC-Desktop_Qt_5_14_2_MinGW_64_bit-Debug3\debug\ACVC.exe crashed. (skip these steps next time, see next step)
        - QtAV: Copy D:\ACVC\QtAV-depends-windows-x86+x64\bin and lib to mingw\bin and \lib (some files alreadt exists eg avcodec 57, skip these)
        - QtAV: Copy D:\ACVC\QtAV-x64-lib\lib_win_x86_64 to mingw\lib (result of compiling C:\Users\ewoud\Downloads\QtAV-master!!!, looks like this should have been added to the release? ask forum!)
    - Still crashing
        - QtAV: Copy depends and lib_win_x86_64 stuff (both bin and lib) in deploy\debug (maybe not needed to copy to mingw)
            - Libs still needed (to do from other folder)
                LIBS += -LD:\ACVC\QtAV-x64-lib\lib_win_x86_64
                LIBS += -lQtAVd1 -lQtAVWidgetsd1 # -lQt5AV  -lQt5AVWidgets
                - https://github.com/wang-bin/QtAV/issues/1193
                - https://github.com/wang-bin/QtAV/wiki/Build-QtAV
        - Media Sidekick starts
    - Qt5OpenGL.dll was not found
        - copy C:/Qt/5.14.2/mingw73_64/bin/Qt5OpenGLd.dll to deploy... (why not automatic?) -> do this before copy all of deploy
        - installer/data Media Sidekick starts!
        - run acvcinstaller\run.bat to create ACVCInstallerV0.2.2.exe (510 kb?)


OSX Install QTAV
    - https://github.com/wang-bin/QtAV/wiki/Deploy-SDK-Without-Building-QtAV
        - Install player to /Applications
        - run /Applications/player.app/sdk_osx.sh ~/Qt5.5.0/5.5/clang_64/lib to install QtAV libraries to Qt library dir(here assume your Qt is installed in ~/Qt5.5.0) or just run /Applications/player.app/sdk_osx.sh and follow the guide
        - If you want to use QML module, you have to use QMLPlayer.dmg. It also includes QtAVWidgets module since 1.9.0.

        - Issue: https://github.com/wang-bin/QtAV/issues/1220

Background
   - https://forum.qt.io/topic/21312/a-video-player-based-on-qt-and-ffmpeg/2
   - http://dranger.com/ffmpeg/ffmpegtutorial_all.html
   - https://github.com/leandromoreira/ffmpeg-libav-tutorial#video---what-you-see

   otool -L /Users/ewoudwijma/Movies/build-ACVC-Desktop_Qt_5_14_2_clang_64bit-Debug/ACVC.app/Contents/MacOS/ACVC

   install_name_tool -change /Users/ewoudwijma/Movies/ffmpeg-20200121-fc6fde2-macos64-shared/bin/libavcodec.58.dylib "@loader_path/libavcodec.58.dylib" ACVC

General pre
    - setApplicationVersion("0.3.0") in main.cpp

Windows 2020-02-20

    - Prepare for installer
        - deploy in shell (not powershell...)
        - C:\Qt\5.14.2\mingw73_64\bin\qtenv2.bat
        - cd d:\MediaSidekick\build-MediaSidekick-Desktop_Qt_5_14_2_MinGW_64_bit-Debug\debug or cd d:\MediaSidekick\build-MediaSidekick-Desktop_Qt_5_14_2_MinGW_64_bit-Release\release
        - remove exiftool.exe, ffplay.exe, ffprobe.exe and ffmpeg.exe temporary out of debug or release folder
        - C:\Qt\5.14.2\mingw73_64\bin\windeployqt.exe --quick --no-translations --qmldir D:\MediaSidekick\MediaSidekick\ .
        - move exiftool.exe, ffplay.exe, ffprobe.exe and ffmpeg.exe back

    - get latest version of ffmpeg:
        - https://ffmpeg.zeranoe.com/builds/win64/dev/ffmpeg-latest-win64-dev.zip
        - https://ffmpeg.zeranoe.com/builds/win64/shared/ffmpeg-latest-win64-shared.zip
    - latest version exiftool
        - https://exiftool.org/: download zip, remove -k

    - create deploy folder (otherwise crash)
        - Copy contents of D:\MediaSidekick\windows\ffmpeg-latest-win64-shared\bin to debug or release folder (avcodec 58)

    - Media Sidekick exiftool and others missing
        - Copy D:\MediaSidekick\MediaSidekick support files\additional\exiftool.exe
        - D:\MediaSidekick\MediaSidekick support files\additional\scripts to debug or release folder
        - D:\MediaSidekick\MediaSidekick support files\additional\MediaSidekick.ico to debug or release folder
        - D:\MediaSidekick\MediaSidekick support files\additional\MediaSidekick.icns to debug or release folder

    - copy all of deploy to installer/data
    - update version in D:\MediaSidekick\MediaSidekickInstaller\run.bat, D:\MediaSidekick\MediaSidekickInstaller\config\config.xml and D:\MediaSidekick\MediaSidekickInstaller\packages\org.mediasidekick.msk\meta\package.xml

General post
    - Update version.json on mediasidekick.org


MacOS 2020-02-01
    - Get ffmpeg
        - Get https://ffmpeg.zeranoe.com/builds/macos64/shared/ffmpeg-latest-macos64-shared.zip
        - Get https://ffmpeg.zeranoe.com/builds/macos64/dev/ffmpeg-latest-macos64-dev.zip
        - QT Creator run/build release
        - Script
            - export MediaSidekick_build_path=/Users/ewoudwijma/Movies/build-MediaSidekick-Desktop_Qt_5_14_2_clang_64bit-Release
            - cp ~/Downloads/ffmpeg-latest-macos64-shared/bin/* $MediaSidekick_build_path/MediaSidekick.app/Contents/MACOS

    - Get exiftool
        - Get https://exiftool.org/ExifTool-xx.yy.dmg from https://exiftool.org/
        - Install dmg / pkg
        - Script
            cp -r /usr/local/bin/lib $MediaSidekick_build_path/MediaSidekick.app/Contents/MACOS/lib
            cp /usr/local/bin/exiftool $MediaSidekick_build_path/MediaSidekick.app/Contents/MACOS

    - macdeployqt
        - https://doc.qt.io/qt-5/macos-deployment.html
        - https://blog.inventic.eu/2012/08/how-to-deploy-qt-application-on-macos-2/
        - https://doc-snapshots.qt.io/qt5-5.11/osx-deployment.html

     - Make dmg (repeat this step only to create new deployment)
        - export MediaSidekick_build_path=/Users/ewoudwijma/Movies/build-MediaSidekick-Desktop_Qt_5_14_2_clang_64bit-Release
        - /Users/ewoudwijma/Qt/5.14.2/clang_64/bin/macdeployqt $MediaSidekick_build_path/MediaSidekick.app -qmldir=/Users/ewoudwijma/Movies/MediaSidekick/ -dmg -always-overwrite
        - Add license.txt and symbolic link to applications folder
            - https://www.dragly.org/2012/01/13/deploy-qt-applications-for-mac-os-x/
                - open DiskUtility
                    - From menu: Images/Convert...
                    - Find created dmg
                    - Choose Image Format: read/write then Convert. A link will be created on the desktop (if not open the dmg file in the destionation folder Documents)
                - Open terminal
                    - type cd<space> and drag the desktop icon after this
                    - ln -s /Applications ./Applications
                    - cp $MediaSidekick_build_path/../MediaSidekick/license.txt READ_BEFORE_OPENING_MediaSidekick_license.txt
                    - cd (to unlock the image)
                - Open DiskUtility
                    - Select image
                    - Right click: Rename to MediaSidekick vx.y.z (version)
                    - Eject mounted image
                    - From menu: Images/Convert...
                    - Find created dmg
                    - Rename to MediaSidekick vx.y.z
                    - Choose Image Format: compressed then Convert. A link will be created on the desktop

General post
    - Update version.json on mediasidekick.org


Name
    - https://pixabay.com/illustrations/media-audio-photo-video-mobile-3856203/
    - https://pixabay.com/users/fsm-team-8829724/?tab=about
    - https://www.free-stock-music.com/
    - https://pixabay.com/service/license/
*/
