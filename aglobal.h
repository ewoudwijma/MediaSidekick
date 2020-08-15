#ifndef AGLOBAL_H
#define AGLOBAL_H

#include <QGeoCoordinate>
#include <QMap>
#include <QString>

static const int orderBeforeLoadIndex = 0;
static const int orderAtLoadIndex = 1;
static const int orderAfterMovingIndex = 2;
static const int changedIndex = 3;
static const int folderIndex = 4;
static const int fileIndex = 5;
static const int inIndex = 6;
static const int outIndex = 7;
static const int durationIndex = 8;
static const int ratingIndex = 9;
static const int alikeIndex = 10;
static const int hintIndex = 11;
static const int tagIndex = 12;

static const int propertyIndex = 0;
static const int minimumIndex = 1;
static const int deltaIndex = 2;
static const int maximumIndex = 3;
static const int typeIndex = 4;
static const int diffIndex = 5;
static const int firstFileColumnIndex = 6;

typedef struct {
    int counter;
    bool definitionGenerated;
} FileStruct;

typedef struct {
    QString absoluteFilePath;
    QString categoryName;
    QString propertyName;
    QString propertySortOrder;
    QString value; //for mediaItems
    QMap<QString, QString> fileValues; //for group of mediaItems
} ExifToolValueStruct;

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

    QStringList exportMethods = QStringList() << "lossless" << "encode" << "shotcut" << "premiere";
    QStringList videoExtensions = QStringList() << "mp4"<<"avi"<<"wmv"<<"mts"<<"mov"<< "mkv" << "webm";
    QStringList audioExtensions = QStringList() << "mp3"<<"wav"<<"wma"<<"aif"<<"m4a"<<"opus"<<"aac";
    QStringList imageExtensions = QStringList() << "jpg"<<"jpeg"<<"png"<<"heic";
    QStringList projectExtensions = QStringList() << "mlt" << "xml";
    QStringList exportExtensions = QStringList() << videoExtensions;// << projectExtensions;

};

#endif // AGLOBAL_H
