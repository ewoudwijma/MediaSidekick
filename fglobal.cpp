#include "fglobal.h"

#include <QTime>
#include <QDebug>
#include <QSettings>

FGlobal::FGlobal()
{
}

int FGlobal::time_to_frames(const QString time )
{
    QTime timeQ = QTime::fromString(time + "0", "HH:mm:ss.zzz");
    int msec = timeQ.msecsSinceStartOfDay();
    int frameAfterSeconds = msec % 1000 / 10;
    int secs = msec / 1000;
    int frames = secs * QSettings().value("frameRate").toInt() + frameAfterSeconds;

//    qDebug()<<"time_to_frames"<<time<<frameAfterSeconds<<secs<<frames;
    return frames;
}

QString FGlobal::frames_to_time(int frames)
{
    int secs = frames / QSettings().value("frameRate").toInt();
    int framesAfterSecond = frames % QSettings().value("frameRate").toInt();

    QTime time = QTime::fromMSecsSinceStartOfDay(secs * 1000 + framesAfterSecond * 10);
//    qDebug()<<"frames_to_time"<<frames<<QSettings().value("frameRate").toInt()<<secs<<framesAfterSecond<<time<<time.toString("HH:mm:ss.zzz");
    return time.toString("HH:mm:ss.zzz").left(11);
}

int FGlobal::msec_to_frames(int msec )
{
//    qDebug()<<"time_to_frames"<<time<<frameAfterSeconds<<secs<<frames;
    return qRound(msec * QSettings().value("frameRate").toInt() / 1000.0);
}

int FGlobal::frames_to_msec(int frames )
{

//    qDebug()<<"FGlobal::frames_to_msec"<<frames<<qRound(1000.0 / QSettings().value("frameRate").toInt())<<frames*qRound(1000.0 / QSettings().value("frameRate").toInt());
    return qRound(frames * 1000.0 / QSettings().value("frameRate").toInt());
}

QString FGlobal::msec_to_time(int msec )
{

//    qDebug()<<"time_to_frames"<<time<<frameAfterSeconds<<secs<<frames;
    return frames_to_time( msec_to_frames(msec));
}

int FGlobal::msec_rounded_to_fps(int msec )
{
    int frames = msec_to_frames(msec);
    return qRound(frames * 1000.0 / QSettings().value("frameRate").toInt());

//    qDebug()<<"msec_rounded_to_fps"<<time<<frameAfterSeconds<<secs<<frames;
}
