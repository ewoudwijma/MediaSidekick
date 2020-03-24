#ifndef AGVIEW_H
#define AGVIEW_H

#include "aderperviewvideo.h"
#include "agcliprectangleitem.h"

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


class AGView: public QGraphicsView

{
    Q_OBJECT
    QGraphicsScene *scene;
    QGraphicsItem *rootItem;

    bool rightMousePressed = false;
    double _panStartX;
    double _panStartY;

    void folderScan(QGraphicsItem *parentItem, QString mode, QString prefix = "");
    QString viewMode;
    bool noFileOrClipDescendants(QGraphicsItem *parentItem);
    QGraphicsItem *drawPoly(QGraphicsItem *parentItem);
    void setItemProperties(QGraphicsItem *parentItem, QString mediaType, QString type, QString folderName, QString fileName, int duration, QSize mediaSize = QSize(), int clipIn = 0, int clipOut = 0, QString tag = "");
    QString itemToString(QGraphicsItem *item);
    void playMedia(QGraphicsItem *mediaItem);
    void updateToolTip(QGraphicsItem *item);
public:
    AGView(QWidget *parent = nullptr);
    void addItem(QString parentFileName, QString mediaType, QString folderName, QString fileName, int duration = 0, int clipIn = 0, int clipOut = 0, QString tag = "");
    QRectF arrangeItems(QGraphicsItem *parentItem);
    ~AGView();
    void onSearchTextChanged(QString text);
    void clearAll();
    void setTextItemsColor(QColor color);
    int loadMediaCompleted = 0;

public slots:
    void onTimelineView();
    void onFileView();
    void onMediaLoaded(QString folderName, QString fileName, QImage image, int duration, QSize mediaSize = QSize(), QString ffmpegMeta = "");
    void onCreateClip();
    void onClipItemChanged(QGraphicsItem *clipItem);
    void onClipMouseReleased(QGraphicsItem *clipItem);
    void onItemClicked(QGraphicsItem *clipItem);
    void onClipPositionChanged(QGraphicsItem *clipItem, int progress);
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
signals:
    void itemSelected(QGraphicsItem *item);
    void mediaLoaded(QString folderName, QString fileName, QImage image, int duration, QSize mediaSize, QString ffmpegMeta);
};

#endif // AGVIEW_H
