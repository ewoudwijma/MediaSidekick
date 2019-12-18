#ifndef AEXPORT_h
#define AEXPORT_h

#include "aclipssortfilterproxymodel.h"
#include "mainwindow.h"

#include <QProgressBar>
#include <QTextStream>
#include <QWidget>
#include <QXmlStreamWriter>

#include <QLabel>
#include <QPushButton>
#include <QSlider>

#include <QStatusBar>

typedef struct {
    QString folderName;
    QString fileName;
    int counter;
    bool definitionGenerated;
} FileStruct;

class AExport: public QWidget
{
    Q_OBJECT
public:
    explicit AExport(QWidget *parent = nullptr);

    void exportClips(QAbstractItemModel *ptimelineModel, QString ptarget, QString ptargetSize, QString pframeRate, int ptransitionTimeFrames, QProgressBar *p_progressBar, QSlider *exportVideoAudioSlider, QLabel *pSpinnerLabel, QString pwatermarkFileName, QPushButton *pExportButton, QComboBox *clipsFramerateComboBox, QComboBox *clipsSizeComboBox, QStatusBar *statusBar);
private:
//    MainWindow *mainWindow;
    AProcessManager *processManager;
    QProgressBar *progressBar;
    QTextStream stream;
    QPushButton *exportButton;
    QStatusBar *statusBar;

    void shotcutTransitionTractor(QXmlStreamWriter *stream, int transitionTime);
    void s(QString y, QString arg1 = "", QString arg2 ="", QString arg3="", QString arg4="");

    QString processError = "";

    QLabel *spinnerLabel;

    void encodeVideoClips();

    QMap<int,int> clipsMap;
    QMap<int,int> videoClipsMap;
    QMap<int,int> audioClipsMap;
    QAbstractItemModel *timelineModel;
//    int transitionTimeMSecs;
//    QTime transitionTime;
    int transitionTimeFrames;
    QString currentDirectory ;
//    QMap<QString, FileStruct> filesMap;
    QMap<QString, FileStruct> filesMap;
    QMap<QString, FileStruct> videoFilesMap;
    QMap<QString, FileStruct> audioFilesMap;
    QString watermarkFileName;
    int exportVideoAudioValue;
    QString frameRate;
    QString fileNameWithoutExtension;
    QString videoFileExtension;
    QString videoWidth;
    QString videoHeight;
    QString target;

    void muxVideoAndAudio();
    void losslessVideoAndAudio();
    void removeTemporaryFiles();

    void addPremiereTrack(QString mediaType, QMap<int,int> clipsMap, QMap<QString, FileStruct> filesMap);
    void addPremiereTransitionItem(int startFrames, int endFrames, QString frameRate, QString mediaType, QString startOrEnd);
    void addPremiereClipitem(QString clipId, QString folderName, QString fileName, int startFrames, int endFrames, int inFrames, int outFrames, QString frameRate, QString mediaType, QMap<QString, FileStruct> *filesMap, int channelTrackNr, QString clipAudioChannels);

    int maxVideoDuration;
    int maxAudioDuration;
    int maxCombinedDuration;
    void processFinished(QMap<QString, QString> parameters);
    void processOutput(QMap<QString, QString> parameters, QString result, int percentageStart, int percentageDelta);
signals:
    void addJobsEntry(QString folder, QString file, QString action, QString *id);
    void addToJob(QString function, QString log);
    void getPropertyValue(QString fileName, QString key, QString *value);

    void reloadClips();
    void reloadProperties();

public slots:
    void onPropertyUpdate(QString folderName, QString fileNameSource, QString fileNameTarget);

    void onTrimC(QString folderName, QString fileNameSource, QString fileNameTarget, QTime inTime, QTime outTime, int progressPercentage);
    void onReloadAll(bool includingSRT);
};

#endif // AEXPORT_h
