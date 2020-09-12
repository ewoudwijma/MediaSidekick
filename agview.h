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

class AGMediaFileRectItem;

typedef struct {
    QString action;
    QString mediaType;
    QGraphicsItem *item;
    QString property;
    QString oldValue;
    QString newValue;
//    bool changed;
} UndoStruct;

class AGView: public QGraphicsView

{
    Q_OBJECT
    QGraphicsScene *scene;
    bool panEnabled = false;
    double panStartX;
    double panStartY;

    bool noFileOrClipDescendants(QGraphicsItem *parentItem);

    void reParent(QGraphicsItem *parentItem, QString prefix = "");

    qreal mediaFileScaleFactor;
    qreal clipScaleFactor;

    qreal mediaHeight = 300 * 9.0 / 16.0;

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

    QRectF arrangeItems(QGraphicsItem *parentItem = nullptr, QString caller = "");

    QGraphicsItem *rootItem = nullptr;

    bool isLoading = false;

    void processAction(QString action);

    void undoOrRedo(QString undoOrRedo);

    void saveModels();
    void saveModel(AGMediaFileRectItem *mediaItem);
    void updateChangedColors(bool debugOn);
    QList<UndoStruct> undoList;
    int undoIndex = -1;
    int undoSavePoint = -1;

public slots:
    void onSetView();
//    void onClipItemChanged(AGClipRectItem *clipItem);
//    void onClipMouseReleased(AGClipRectItem *clipItem);
    void onItemRightClicked(QPoint pos);
    void onHoverPositionChanged(QGraphicsRectItem *rectItem, int progress);
    void onPlayerDialogFinished(int result);

    void onFileChanged(QFileInfo fileInfo);
    void onAddItem(bool changed, QString parentName, QString mediaType, QFileInfo fileInfo = QFileInfo(), int duration = 0, int clipIn = 0, int clipOut = 0, QString tag = "");
    void onDeleteItem(bool changed, QString mediaType, QFileInfo fileInfo, int clipIn = -1, QString tagName = "");

    void onAddUndo(bool changed, QString action, QString mediaType, QGraphicsItem *item, QString property = "", QString oldValue = "", QString newValue = "");
    void onArrangeItems();
protected:
    void keyPressEvent(QKeyEvent *event);
private slots:
    void wheelEvent(QWheelEvent *event);

    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);

    void onSelectionChanged();

signals:
    void fileWatch(QString folderFileName, bool on, bool triggerFileChanged = false);
    void showInStatusBar(QString message, int timeout);

};

#endif // AGVIEW_H
