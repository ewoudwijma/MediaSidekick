#ifndef AGMEDIAFILERECTITEM_H
#define AGMEDIAFILERECTITEM_H

#include "aderperviewmain.h"
#include "agfolderrectitem.h"
#include "agview.h"
#include "agviewrectitem.h"
#include "agprocessthread.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QMediaPlayer>

#include "aglobal.h"

class AGView;
class AGMediaFileRectItem;

class AGMediaFileRectItem: public AGViewRectItem
{
    Q_OBJECT

    void onRewind(QMediaPlayer *m_player);

    void initPlayer(bool startPlaying);

    void loadMedia(AGProcessAndThread *process);

    ADerperView *derperView; //not a local variable to make connect onStopThreadProcess work

public:
    AGMediaFileRectItem(QGraphicsItem *parent = nullptr, QFileInfo fileInfo = QFileInfo(), int duration = 0, qreal mediaWidth = 0);
    ~AGMediaFileRectItem();

    AGViewRectItem *groupItem = nullptr;

    QGraphicsVideoItem *playerItem = nullptr;
    QMediaPlayer *m_player = nullptr;

    QGraphicsRectItem *durationLine = nullptr;

    QMap<QString, QMap<QString, ExifToolValueStruct>> exiftoolMap;
    QMap<QString, ExifToolValueStruct> exiftoolValueMap;

public slots:
    void onItemRightClicked(AGView *view, QPoint pos);

    void onPlayVideoButton(QMediaPlayer *m_player);
    void onMuteVideoButton(QMediaPlayer *m_player);
    void onFastForward(QMediaPlayer *m_player);
    void onSkipNext(QMediaPlayer *m_player);
    void onSkipPrevious(QMediaPlayer *m_player);
    void onStop(QMediaPlayer *m_player);
    void onMute(QMediaPlayer *m_player);
    void onSetSourceVideoVolume(QMediaPlayer *m_player, int volume);
    void onSetPlaybackRate(QMediaPlayer *m_player, qreal rate);
    void onProcessOutput(QTime time, QTime totalTime, QString event, QString outputString);

    void onMediaFileChanged();

signals:
    void propertyCopy(QStandardItem *parentItem, QString folderNameSource, QString fileNameSource, QString folderNameTarget, QString fileNameTarget);
    void trimAll(QStandardItem *parentItem, QStandardItem *&currentItem, QString folderName, QString fileName, bool moveToBin = true);
//    void exportClips(QStandardItem *parentItem, QStandardItem *&currentItem, QString folderName, QString fileName, bool moveToBin = true);
    void mediaLoaded(QFileInfo fileInfo, QImage image = QImage(), int duration = 0, QSize mediaSize = QSize(), QString ffmpegMeta = "", QList<int> samples = QList<int>());

private slots:
    void onPositionChanged(int progress);
    void onMetaDataAvailableChanged(bool available);
    void onMetaDataChanged();
    void onMediaStatusChanged(QMediaPlayer::MediaStatus status);
    void onMediaLoaded(QFileInfo fileInfo, QImage image, int duration, QSize mediaSize, QString ffmpegMeta, QList<int> samples);
};

#endif // AGMEDIAFILERECTITEM_H
