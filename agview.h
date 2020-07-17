#ifndef AGVIEW_H
#define AGVIEW_H

#include "aderperviewmain.h"
#include "aderperviewvideo.h"
#include "agmediafilerectitem.h"
//#include "agcliprectangleitem.h"
//#include "agcliprectitem.h"

#include <QDateTime>
#include <QFileInfo>
#include <QGraphicsView>
#include <QMediaPlayer>

static const int mediaTypeIndex = 0;
static const int folderNameIndex = 1;
static const int fileNameIndex = 2;
static const int itemTypeIndex = 3;
static const int mediaWithIndex = 4;
static const int mediaHeightIndex = 5;
static const int mediaDurationIndex = 6;
static const int ffMpegMetaIndex = 10;
static const int excludedInFilter = 12;
static const int createDateIndex = 13;

//class AGMediaFileRectItem;

class AGView: public QGraphicsView

{
    Q_OBJECT
    QGraphicsScene *scene;
    bool rightMousePressed = false;
    double _panStartX;
    double _panStartY;

    bool noFileOrClipDescendants(QGraphicsItem *parentItem);

    void reParent(QGraphicsItem *parentItem, QString prefix = "");

    qreal mediaFileScaleFactor;
    qreal clipScaleFactor;

    qreal mediaWidth;

    void filterItem(QGraphicsItem *item);

    void assignCreateDates();
public:
    AGView(QWidget *parent = nullptr);
    ~AGView();
    void onSearchTextChanged(QString text);
    void clearAll();
    void setThemeColors(QColor color);

    void setMediaScaleAndArrange(qreal mediaFileScaleFactor);
    QMediaPlayer *dialogMediaPlayer = nullptr;
    bool playInDialog;
    void setPlayInDialog(bool checked);
    void setClipScaleAndArrange(qreal mediaFileScaleFactor);
    void setZoom(int value);

    QString searchText = "";
    bool filtering = false;

    void setOrderBy(QString orderBy);

    void stopAndDeletePlayers(QFileInfo fileInfo = QFileInfo());
    QDialog *playerDialog = nullptr;
    QVideoWidget *dialogVideoWidget = nullptr;

    QString orderBy = "Name";

    QRectF arrangeItems(QGraphicsItem *parentItem = nullptr);

    QGraphicsItem *rootItem = nullptr;

    bool isLoading = false;

public slots:
    void onSetView();
    void onCreateClip();
//    void onClipItemChanged(AGClipRectItem *clipItem);
//    void onClipMouseReleased(AGClipRectItem *clipItem);
    void onItemRightClicked(QPoint pos);
    void onHoverPositionChanged(QGraphicsRectItem *rectItem, int progress);
    void onPlayerDialogFinished(int result);

    void onFileChanged(QFileInfo fileInfo);
    void onAddItem(QString parentName, QString mediaType, QFileInfo fileInfo = QFileInfo(), int duration = 0, int clipIn = 0, int clipOut = 0, QString tag = "");
    void onDeleteItem(QString mediaType, QFileInfo fileInfo);

private slots:
    void wheelEvent(QWheelEvent *event);

    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);

    void onSelectionChanged();
};

#endif // AGVIEW_H
