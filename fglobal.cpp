#include "fglobal.h"

#include <QTime>
#include <QDebug>

FGlobal::FGlobal()
{

}

int FGlobal::time_to_frames(int fps, const QString time )
{
    QTime timeQ = QTime::fromString(time + "0", "HH:mm:ss.zzz");
    int msec = timeQ.msecsSinceStartOfDay();
    int frameAfterSeconds = msec % 1000 / 10;
    int secs = msec / 1000;
    int frames = secs * fps + frameAfterSeconds;

//    qDebug()<<"time_to_frames"<<time<<frameAfterSeconds<<secs<<frames;
    return frames;
}

QString FGlobal::frames_to_time(int fps, int frames)
{
    int secs = frames / fps;
    int framesAfterSecond = frames % fps;

    QTime time = QTime::fromMSecsSinceStartOfDay(secs * 1000 + framesAfterSecond * 10);
//    qDebug()<<"frames_to_time"<<frames<<secs<<framesAfterSecond<<time<<time.toString("HH:mm:ss.zzz");
    return time.toString("HH:mm:ss.zzz").left(11);
}

int FGlobal::msec_to_frames(int fps, int msec )
{

//    qDebug()<<"time_to_frames"<<time<<frameAfterSeconds<<secs<<frames;
    return msec_rounded_to_fps(fps, msec) * fps / 1000;
}

int FGlobal::frames_to_msec(int fps, int frames )
{

//    qDebug()<<"time_to_frames"<<time<<frameAfterSeconds<<secs<<frames;
    return frames * 1000 / fps;
}

QString FGlobal::msec_to_time(int fps, int msec )
{

//    qDebug()<<"time_to_frames"<<time<<frameAfterSeconds<<secs<<frames;
    return frames_to_time(fps, msec_to_frames(fps,msec));
}

int FGlobal::msec_rounded_to_fps(int fps, int msec )
{
    int rounded = msec / (1000 / fps);
    rounded *= (1000/fps);

//    qDebug()<<"time_to_frames"<<time<<frameAfterSeconds<<secs<<frames;
    return rounded;
}
