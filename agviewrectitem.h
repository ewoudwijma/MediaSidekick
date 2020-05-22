#ifndef AGViewRectItem_H
#define AGViewRectItem_H


#include "agprocessthread.h"

#include <QObject>
#include <QGraphicsRectItem>
#include <QDebug>
#include <QKeyEvent>
#include <QGraphicsSceneMouseEvent>
#include <QFileInfo>

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

    int mediaWidth;

    QList<AGProcessAndThread *> processes;

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

    void setItemProperties(QString mediaType, QString itemType, int duration, QSize mediaSize);

    QMenu *fileContextMenu = nullptr;

    void recursiveFileRenameCopyIfExists(QString folderName, QString fileName);

signals:
//    void agItemChanged(AGViewRectItem *clipItem);
    void hoverPositionChanged(QGraphicsRectItem *rectItem, double position);

private slots:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override;
};


#endif // AGViewRectItem_H
