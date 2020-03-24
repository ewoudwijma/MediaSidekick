#include "agmediarectangleitem.h"

#include "agview.h"

AGMediaRectangleItem::AGMediaRectangleItem(QGraphicsItem * parent) :
    QGraphicsRectItem( parent)
{

    this->setFlag(QGraphicsItem::ItemIsSelectable);
    this->setAcceptHoverEvents(true);
}

QVariant AGMediaRectangleItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemPositionHasChanged && scene())
    {
//        qDebug()<<"AGMediaRectangleItem::itemChange"<<change<<value;
//        emit agItemChanged(this);
    }

    return QGraphicsRectItem::itemChange(change, value);
}

//https://forum.qt.io/topic/32869/solved-resize-qgraphicsitem-with-drag-and-drop/3

void AGMediaRectangleItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
//    qDebug()<<"AGMediaRectangleItem::mousePressEvent"<<event;
    emit itemClicked(this);
}

void AGMediaRectangleItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
//    qDebug()<<"AGMediaRectangleItem::hoverMoveEvent"<<event<<event->pos()<<rect();

//    qDebug()<<"emit clipPositionChanged"<<data(clipInIndex).toInt()<<data(clipOutIndex).toInt()<<event->pos()<<draggedWidth<<event->pos().x()/double(draggedWidth);

    if (event->pos().y() > rect().height() * 0.7) //only hover on timeline
    {
        int clipIn = data(clipInIndex).toInt();
        int clipOut = data(clipOutIndex).toInt();

        int progress;
        if (clipOut != 0)
        {
            progress = clipIn + (clipOut - clipIn) * event->pos().x()/rect().width();
        }
        else
            progress = data(mediaDurationIndex).toInt() * event->pos().x()/rect().width();

        emit clipPositionChanged(this, progress);
    }

    QGraphicsRectItem::hoverMoveEvent(event);
}
