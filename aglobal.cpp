#include "aglobal.h"

#include <QTime>
#include <QDebug>
#include <QSettings>

#include <qmath.h>

AGlobal::AGlobal()
{
}

int AGlobal::time_to_frames(const QString time )
{
    QTime timeQ = QTime::fromString(time + "0", "HH:mm:ss.zzz");
    int msec = timeQ.msecsSinceStartOfDay();
    int frameAfterSeconds = msec % 1000 / 10;
    int secs = msec / 1000;
    int frames = secs * QSettings().value("frameRate").toInt() + frameAfterSeconds;

//    qDebug()<<"time_to_frames"<<time<<frameAfterSeconds<<secs<<frames;
    return frames;
}

QString AGlobal::frames_to_time(int frames)
{
    int secs = frames / QSettings().value("frameRate").toInt();
    int framesAfterSecond = frames % QSettings().value("frameRate").toInt();

    QTime time = QTime::fromMSecsSinceStartOfDay(secs * 1000 + framesAfterSecond * 10);
    return time.toString("HH:mm:ss.zzz").left(11);
}

int AGlobal::msec_to_frames(int msec )
{
//    qDebug()<<"time_to_frames"<<time<<frameAfterSeconds<<secs<<frames;
    return qRound(msec * QSettings().value("frameRate").toInt() / 1000.0);
}

int AGlobal::frames_to_msec(int frames )
{

//    qDebug()<<"AGlobal::frames_to_msec"<<frames<<qRound(1000.0 / QSettings().value("frameRate").toInt())<<frames*qRound(1000.0 / QSettings().value("frameRate").toInt());
    return qRound(frames * 1000.0 / QSettings().value("frameRate").toInt());
}

QString AGlobal::msec_to_time(int msec )
{

//    qDebug()<<"time_to_frames"<<time<<frameAfterSeconds<<secs<<frames;
    return frames_to_time( msec_to_frames(msec));
}

int AGlobal::msec_rounded_to_fps(int msec )
{
    int frames = msec_to_frames(msec);
    return qRound(frames * 1000.0 / QSettings().value("frameRate").toInt());

//    qDebug()<<"msec_rounded_to_fps"<<time<<frameAfterSeconds<<secs<<frames;
}

QString AGlobal::secondsToString(qint64 seconds)
{
    const qint64 DAY = 86400;
    qint64 days = seconds / DAY;
    QTime t = QTime(0,0).addSecs(seconds % DAY);

  //  return QString("%1 days, %2 hours, %3 minutes, %4 seconds")
  //    .arg(days).arg(t.hour()).arg(t.minute()).arg(t.second());

    QStringList result;
    if (days > 0)
        result << QString::number(days) + "d";
    if (t.hour() > 0)
                  result << QString::number(t.hour()) + "h";
    if (t.minute() > 0)
                  result << QString::number(t.minute()) + "m";
    if (t.second() > 0)
                  result << QString::number(t.second()) + "s";

    if (result.count() == 0)
        result << "0s";

    return result.join(", ");
}

QString AGlobal::secondsToCSV(qint64 seconds)
{
    const qint64 DAY = 86400;
    qint64 days = seconds / DAY;
    QTime t = QTime(0,0).addSecs(seconds % DAY);

    QStringList result;
    result << QString::number(days);
    result << QString::number(t.hour()) ;
    result << QString::number(t.minute()) ;
    result << QString::number(t.second()) ;

    return result.join(";");
}

QString AGlobal::csvToString(QString csv)
{

    QStringList csvList = csv.split(";");
    QString result;

    if (csvList.count() == 0)
        result = "00:00:00";
    else
    {
        if (csvList.count() > 0 && csvList[0] != "0")
            result = csvList[0] + "d ";

        if (csvList.count() > 1 && csvList[1] != "0")
            result += QString::number(csvList[1].toInt()).rightJustified(2, '0') + ":";
        else
            result += "00:";

        if (csvList.count() > 1 && csvList[2] != "0")
            result += QString::number(csvList[2].toInt()).rightJustified(2, '0') + ":";
        else
            result += "00:";

        if (csvList.count() > 1 && csvList[3] != "0")
            result += QString::number(csvList[3].toInt()).rightJustified(2, '0');
        else
            result += "00";
    }

    if (result.count() == 0)
        result = "00:00:00";

    return result;
}

qint64 AGlobal::csvToSeconds(QString csv)
{

    QStringList valueList = csv.split(";");
    qint64 seconds = 0;

    {
        if (valueList.count() > 0 && valueList[0].toInt() != 0)
            seconds += valueList[0].toInt() * 3600 * 24;
        if (valueList.count() > 1 && valueList[1].toInt() != 0)
            seconds += valueList[1].toInt() * 3600;
        if (valueList.count() > 2 && valueList[2].toInt() != 0)
            seconds += valueList[2].toInt() * 60;
        if (valueList.count() > 3 && valueList[3].toInt() != 0)
            seconds += valueList[3].toInt();
    }

    return seconds;
}

QGeoCoordinate AGlobal::csvToGeoCoordinate(QString csv)
{
    double latitude = 0;
    double longitude = 0;
    int altitude = 0;
    QStringList valueList = csv.split(";");
    if (valueList.count() > 0 && valueList[0].toDouble() != 0)
        latitude = valueList[0].toDouble();
    if (valueList.count() > 1 && valueList[1].toDouble() != 0)
        longitude = valueList[1].toDouble();
    if (valueList.count() > 2 && valueList[2].toInt() != 0)
        altitude = valueList[2].toInt();

    return QGeoCoordinate(latitude, longitude, altitude);
}
