#ifndef AGTagTextItem_H
#define AGTagTextItem_H

#include "agcliprectitem.h"

#include <QFileInfo>
#include <QGraphicsTextItem>
#include <QGraphicsView>
#include <QMenu>

#include <QObject>

class AGClipRectItem;

class AGTagTextItem: public QGraphicsTextItem
{
    Q_OBJECT

    QMenu *fileContextMenu = nullptr;

public:
    AGTagTextItem(QGraphicsItem *parent = nullptr, QFileInfo fileInfo = QFileInfo(), QString tagName = "");
    void onItemRightClicked(QPoint pos);

    QString mediaType;
    QString itemType;

    QFileInfo fileInfo;
//    QString fileNameWithoutExtension;
//    QString extension;

    QString tagName;

    AGClipRectItem *clipItem;

//    bool changed = false;

private slots:
//    void mousePressEvent(QGraphicsSceneMouseEvent *event);

    void hoverMoveEvent(QGraphicsSceneHoverEvent *event);
signals:
    void deleteItem(bool changed, QString mediaType, QFileInfo fileInfo, int clipIn = -1, QString tagName = "");
    void hoverPositionChanged(QGraphicsRectItem *rectItem, double position);

};



#endif // AGTagTextItem_H
