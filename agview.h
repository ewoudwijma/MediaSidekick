#ifndef AGVIEW_H
#define AGVIEW_H

#include "aderperviewvideo.h"
#include "agcliprectangleitem.h"

#include <QDateTime>
#include <QGraphicsView>
#include <QMediaPlayer>

static const int mediaTypeIndex = 0;
static const int folderNameIndex = 1;
static const int fileNameIndex = 2;
static const int itemTypeIndex = 3;
static const int mediaWithIndex = 4;
static const int mediaHeightIndex = 5;
static const int mediaDurationIndex = 6;
static const int clipInIndex = 7;
static const int clipOutIndex = 8;
static const int clipTagIndex = 9;
static const int ffMpegMetaIndex = 10;
static const int exifToolMetaIndex = 11;
static const int excludedInFilter = 12;
static const int createDateIndex = 13;

class AGView: public QGraphicsView

{
    Q_OBJECT
    QGraphicsScene *scene;
    QGraphicsItem *rootItem;
    bool rightMousePressed = false;
    double _panStartX;
    double _panStartY;

    bool noFileOrClipDescendants(QGraphicsItem *parentItem);
    QGraphicsItem *drawPoly(QGraphicsItem *clipItem);
    void setItemProperties(QGraphicsItem *parentItem, QString mediaType, QString type, QString folderName, QString fileName, int duration, QSize mediaSize = QSize(), int clipIn = 0, int clipOut = 0, QString tag = "");
    QString itemToString(QGraphicsItem *item);
    void playMedia(QGraphicsRectItem *mediaItem);
    void updateToolTip(QGraphicsItem *item);

    void reParent(QGraphicsItem *parentItem, QString prefix = "");

    QDateTime pseudoCreateDate;
    qreal scaleFactor = 1.0 / 100.0;

    QDialog *playerDialog = nullptr;
    QVideoWidget *dialogVideoWidget;
    void stopAndDeleteAllPlayers();
public:
    AGView(QWidget *parent = nullptr);
    void addItem(QString parentName, QString mediaType, QString folderName, QString fileName, int duration = 0, int clipIn = 0, int clipOut = 0, QString tag = "");
    void deleteItem(QString mediaType, QString folderName, QString fileName);
    QRectF arrangeItems(QGraphicsItem *parentItem = nullptr);
    ~AGView();
    void onSearchTextChanged(QString text);
    void clearAll();
    void setThemeColors(QColor color);
    int loadMediaCompleted = 0;

    void setScaleFactorAndArrange(qreal scaleFactor);
    QMediaPlayer *dialogMediaPlayer = nullptr;
    bool playInDialog;
    void setPlayInDialog(bool checked);
public slots:
    void onSetView();
    void onMediaLoaded(QString folderName, QString fileName, QImage image = QImage(), int duration = 0, QSize mediaSize = QSize(), QString ffmpegMeta = "", QList<int> samples = QList<int>());
    void onCreateClip();
    void onClipItemChanged(QGraphicsItem *clipItem);
    void onClipMouseReleased(QGraphicsItem *clipItem);
    void onItemClicked(QGraphicsRectItem *rectItem);
    void onClipPositionChanged(QGraphicsRectItem *rectItem, int progress);
    void onPlayVideoButton(QMediaPlayer *m_player);
    void onMuteVideoButton(QMediaPlayer *m_player);
    void onFastForward(QMediaPlayer *m_player);
    void onRewind(QMediaPlayer *m_player);
    void onSkipNext(QMediaPlayer *m_player);
    void onSkipPrevious(QMediaPlayer *m_player);
    void onStop(QMediaPlayer *m_player);
    void onMute(QMediaPlayer *m_player);
    void onSetSourceVideoVolume(QMediaPlayer *m_player, int volume);
    void onSetPlaybackRate(QMediaPlayer *m_player, qreal rate);
private slots:
    void onMediaStatusChanged(QMediaPlayer::MediaStatus status);

    void wheelEvent(QWheelEvent *event);

    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);

    void onMetaDataChanged();
    void onMetaDataAvailableChanged(bool available);
    void onSelectionChanged();
    void onPositionChanged(int progress);
    //    void onAudioBufferProbed(QAudioBuffer buffer);
    void onPlayerDialogFinished(int result);
signals:
    void itemSelected(QGraphicsItem *item);
    void mediaLoaded(QString folderName, QString fileName, QImage image = QImage(), int duration = 0, QSize mediaSize = QSize(), QString ffmpegMeta = "", QList<int> samples = QList<int>());
    void getPropertyValue(QString folderFileName, QString key, QVariant *value);

};

#endif // AGVIEW_H
