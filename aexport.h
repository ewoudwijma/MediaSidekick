#ifndef AEXPORT_h
#define AEXPORT_h

#include "aclipssortfilterproxymodel.h"
#include "mainwindow.h"

#include "aglobal.h"

#include <QTextStream>
#include <QWidget>
#include <QXmlStreamWriter>

#include "aspinnerlabel.h"

#include <QPushButton>
#include <QSlider>
#include <QThread>

class AExport: public QWidget
{
    Q_OBJECT
public:
    explicit AExport(QWidget *parent = nullptr);

    void exportClips(QAbstractItemModel *ptimelineModel, QString ptarget, QString ptargetSize, QString pframeRate, int ptransitionTimeFrames, QSlider *exportVideoAudioSlider, QString pwatermarkFileName, QComboBox *clipsFramerateComboBox, QComboBox *clipsSizeComboBox);

    AJobTreeView *jobTreeView;

private:
    QTextStream stream;

    void shotcutTransitionTractor(QXmlStreamWriter *stream, int transitionTime);
    void s(QString y, QString arg1 = "", QString arg2 ="", QString arg3="", QString arg4="");

    ASpinnerLabel *spinnerLabel;

    QStandardItem *encodeVideoClips(QStandardItem *parentItem);

    QMap<int,int> clipsMap;
    QMap<int,int> videoClipsMap;
    QMap<int,int> audioClipsMap;
    QAbstractItemModel *timelineModel;
    int transitionTimeFrames;
    QString selectedFolderName;
    QString recycleFolderName;
//    QMap<QString, FileStruct> filesMap;
    QMap<QString, FileStruct> filesMap;
    QMap<QString, FileStruct> videoFilesMap;
    QMap<QString, FileStruct> audioFilesMap;
    QString watermarkFileName;
    int exportVideoAudioValue;
    QString frameRate;
    QString fileNameWithoutExtension;
    QString videoFileExtension;
    QString audioFileExtension;
    QString videoWidth;
    QString videoHeight;
    QString target;

    QStandardItem *muxVideoAndAudio(QStandardItem *parentItem);
    QStandardItem *losslessVideoAndAudio(QStandardItem *parentItem);

    void addPremiereTrack(QString mediaType, QMap<int,int> clipsMap, QMap<QString, FileStruct> filesMap);
    void addPremiereTransitionItem(int startFrames, int endFrames, QString frameRate, QString mediaType, QString startOrEnd);
    void addPremiereClipitem(QString clipId, QString folderName, QString fileName, int startFrames, int endFrames, int inFrames, int outFrames, QString frameRate, QString mediaType, QMap<QString, FileStruct> *filesMap, int channelTrackNr, QString clipAudioChannels, QString imageWidth, QString imageHeight);

    int maxVideoDuration;
    int maxAudioDuration;
    int maxCombinedDurationInFrames;
    void startWorkInAThread();
    void exportShotcut(AJobParams jobParams);
    void exportPremiere(AJobParams jobParams);
signals:
    void getPropertyValue(QString folderFileName, QString key, QVariant *value);

    void loadClips(QStandardItem *parentItem);
    void loadProperties(QStandardItem *parentItem);

    void exportCompleted(QString error);

    void moveFilesToACVCRecycleBin(QStandardItem *parentItem, QString folderName, QString fileName, bool supportingFilesOnly = false);

    void jobAddLog(AJobParams jobParams, QString logMessage);
    void propertyCopy(QStandardItem *parentItem, QString folderNameSource, QString fileNameSource, QString folderNameTarget, QString fileNameTarget);
    void trimC(QStandardItem *parentItem, QStandardItem *&currentItem, QString folderNameSource, QString fileNameSource, QString folderNameTarget, QString fileNameTarget, QTime inTime, QTime outTime);
    void trimF(QStandardItem *parentItem, QStandardItem *&currentItem, QString folderNameSource, QString fileNameSource, QString folderNameTarget);
    void releaseMedia(QString folderName, QString fileName);

};

#endif // AEXPORT_h
