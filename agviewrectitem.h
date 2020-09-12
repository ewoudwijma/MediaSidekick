#ifndef AGViewRectItem_H
#define AGViewRectItem_H


//#include "agcliprectitem.h"
#include "agprocessthread.h"

#include <QObject>
#include <QGraphicsRectItem>
#include <QDebug>
#include <QKeyEvent>
#include <QGraphicsSceneMouseEvent>
#include <QFileInfo>
#include <QGraphicsView>

#include <QMenu>

#include <QApplication> //to use qApp (icon)

class AGViewRectItem: public QObject, public QGraphicsRectItem
{
    Q_OBJECT

public:
    AGViewRectItem(QGraphicsItem *parent = nullptr, QFileInfo fileInfo = QFileInfo());

    QString mediaType;
    QString itemType;

    QFileInfo fileInfo;

    AGViewRectItem *parentRectItem;

    QGraphicsPixmapItem *pictureItem = nullptr;

    QString itemToString();

    int duration;
    void updateToolTip();

    qreal mediaDefaultHeight = 300 * 9.0 / 16.0;

    QList<AGProcessAndThread *> processes;

    QGraphicsTextItem *subLogItem = nullptr;

//    QGraphicsRectItem *progressRectItem = nullptr;
    QGraphicsProxyWidget *progressSliderProxy = nullptr;

    void onItemRightClicked(QPoint pos);

    bool containsVideo;
    bool containsAudio;
    bool containsImage;

    void processAction(QString action);
    void newProgressItem();
    void newSubLogItem();

    virtual void setTextItem(QTime time, QTime totalTime);
    QString lastOutputString;

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

    QMenu *fileContextMenu = nullptr;

    void recursiveFileRenameCopyIfExists(QString folderName, QString fileName);

signals:
//    void agItemChanged(AGViewRectItem *clipItem);
    void hoverPositionChanged(QGraphicsRectItem *rectItem, double position);
    void showInStatusBar(QString message, int timeout);

private slots:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override;

protected slots:
    void onProcessOutput(QTime time, QTime totalTime, QString event, QString outputString);
};


#endif // AGViewRectItem_H
