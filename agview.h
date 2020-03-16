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


class AGView: public QGraphicsView

{
    Q_OBJECT
    QGraphicsScene *scene;
    QGraphicsItem *rootItem;

    bool rightMousePressed = false;
    double _panStartX;
    double _panStartY;

    void folderScan(QGraphicsItem *parentItem, QString mode);
    bool noFileOrClipDescendants(QGraphicsItem *parentItem);
    QGraphicsItem *drawPoly(QGraphicsItem *parentItem);
    void setItemProperties(QGraphicsItem *parentItem, QString mediaType, QString type, QString folderName, QString fileName, int duration, QSize mediaSize = QSize(), int clipIn = 0, int clipOut = 0, QString tag = "");
    QString itemToString(QGraphicsItem *item);
    void playMedia(QGraphicsItem *mediaItem);
    void updateToolTip(QGraphicsItem *item);
public:
    AGView(QWidget *parent = nullptr);
    QGraphicsItem *addItem(QGraphicsItem *parentItem, QString type, QString folderName, QString fileName, int duration = 0, int clipIn = 0, int clipOut = 0, QString tag = "");
    QRectF arrangeItems(QGraphicsItem *parentItem);
    ~AGView();
    void onSearchTextChanged(QString text);
    void clearAll();
public slots:
    void onTimelineView();
    void onFileView();
    void onMediaLoaded(QString folderName, QString fileName, QImage image, int duration, QSize mediaSize);
    void onCreateClip();
    void onClipItemChanged(AGClipRectangleItem *clipItem);
    void onClipMouseReleased(AGClipRectangleItem *clipItem);
    void onClipPositionChanged(AGClipRectangleItem *clipItem, int progress);
    void onPlayVideoButton();
    void onMuteVideoButton();
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
};

#endif // AGVIEW_H
