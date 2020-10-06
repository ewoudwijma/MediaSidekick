#ifndef AGLOBAL_H
#define AGLOBAL_H

#include <QGeoCoordinate>
#include <QMap>
#include <QString>

static const int propertyIndex = 0;
static const int minimumIndex = 1;
static const int deltaIndex = 2;
static const int maximumIndex = 3;
static const int typeIndex = 4;
static const int diffIndex = 5;
static const int firstFileColumnIndex = 6;

static const int mediaTypeIndex = 0;
//static const int folderNameIndex = 1;
//static const int fileNameIndex = 2;
static const int itemTypeIndex = 3;
static const int excludedInFilter = 12;
static const int createDateIndex = 13;

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
} MMetaDataStruct;

class AGlobal
{
public:
    AGlobal();
    QString msec_to_timeNoFPS(int msec);
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
