#include "fglobal.h"

#include <QTime>
#include <QDebug>

FGlobal::FGlobal()
{
}

int FGlobal::time_to_frames(const QString time )
{
    QTime timeQ = QTime::fromString(time + "0", "HH:mm:ss.zzz");
    int msec = timeQ.msecsSinceStartOfDay();
    int frameAfterSeconds = msec % 1000 / 10;
    int secs = msec / 1000;
    int frames = secs * frameRate + frameAfterSeconds;

//    qDebug()<<"time_to_frames"<<time<<frameAfterSeconds<<secs<<frames;
    return frames;
}

QString FGlobal::frames_to_time(int frames)
{
    int secs = frames / frameRate;
    int framesAfterSecond = frames % frameRate;

    QTime time = QTime::fromMSecsSinceStartOfDay(secs * 1000 + framesAfterSecond * 10);
//    qDebug()<<"frames_to_time"<<frames<<framesPerSecond<<secs<<framesAfterSecond<<time<<time.toString("HH:mm:ss.zzz");
    return time.toString("HH:mm:ss.zzz").left(11);
}

int FGlobal::msec_to_frames(int msec )
{
//    qDebug()<<"time_to_frames"<<time<<frameAfterSeconds<<secs<<frames;
    return msec_rounded_to_fps(msec) * frameRate / 1000;
}

int FGlobal::frames_to_msec(int frames )
{

//    qDebug()<<"time_to_frames"<<time<<frameAfterSeconds<<secs<<frames;
    return frames * 1000 / frameRate;
}

QString FGlobal::msec_to_time(int msec )
{

//    qDebug()<<"time_to_frames"<<time<<frameAfterSeconds<<secs<<frames;
    return frames_to_time( msec_to_frames(msec));
}

int FGlobal::msec_rounded_to_fps(int msec )
{
    int rounded = msec / (1000 / frameRate);
    rounded *= (1000/frameRate);

//    qDebug()<<"time_to_frames"<<time<<frameAfterSeconds<<secs<<frames;
    return rounded;
}
