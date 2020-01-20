#ifndef AGLOBAL_H
#define AGLOBAL_H

#include <QGeoCoordinate>
#include <QString>

static const int orderBeforeLoadIndex = 0;
static const int orderAtLoadIndex = 1;
static const int orderAfterMovingIndex = 2;
static const int changedIndex = 3;
static const int folderIndex = 4;
static const int fileIndex = 5;
static const int fpsIndex = 6;
static const int fileDurationIndex = 7;
static const int inIndex = 8;
static const int outIndex = 9;
static const int durationIndex = 10;
static const int ratingIndex = 11;
static const int alikeIndex = 12;
static const int hintIndex = 13;
static const int tagIndex = 14;

static const int propertyIndex = 0;
static const int minimumIndex = 1;
static const int deltaIndex = 2;
static const int maximumIndex = 3;
static const int typeIndex = 4;
static const int diffIndex = 5;
static const int firstFileColumnIndex = 6;

typedef struct {
    QString folderName;
    QString fileName;
    int counter;
    bool definitionGenerated;
} FileStruct;

class AGlobal
{
public:
    AGlobal();
    int time_to_frames(const QString time);
    QString frames_to_time(int frames);
    int msec_to_frames(int msec);
    int msec_rounded_to_fps(int msec);
    QString msec_to_time(int msec);
    int frames_to_msec(int frames);
    QString secondsToString(qint64 seconds);
    QString secondsToCSV(qint64 seconds);
    QString csvToString(QString csv);
    qint64 csvToSeconds(QString csv);
    QGeoCoordinate csvToGeoCoordinate(QString csv);
};

#endif // AGLOBAL_H
