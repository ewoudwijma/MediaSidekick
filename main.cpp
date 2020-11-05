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
    a.setApplicationVersion("0.5.2.3");

    QSettings::setDefaultFormat(QSettings::IniFormat);
    //C:\Users\<user>\AppData\Roaming\mediasidekick.org

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
