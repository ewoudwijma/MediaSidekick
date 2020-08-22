#include "agcliprectangleitem.h"

#include "agcliprectitem.h"
#include "aglobal.h"
#include "agview.h"

#include <QStyleOption>
#include <QDebug>

AGClipRectangleItem::AGClipRectangleItem(QGraphicsItem * parent) :
    QGraphicsRectItem( parent)
{
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);

    this->setFlag(QGraphicsItem::ItemIsSelectable);
    this->setFlag(QGraphicsItem::ItemIsMovable);
    this->setAcceptHoverEvents(true);
    this->setAcceptDrops(true);
}

void AGClipRectangleItem::setRect(const QRectF &rectangle)
{
    draggedWidth = rectangle.width();
    draggedX = rectangle.x();

//    qDebug()<<"AGClipRectangleItem::setRect"<<rectangle<<originalClipIn<<originalClipOut;

    QGraphicsRectItem::setRect(rectangle);
}

QVariant AGClipRectangleItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemPositionHasChanged && scene())
    {
//        qDebug()<<"AGClipRectangleItem::itemChange"<<change<<value;
        emit agItemChanged(this);
    }

    return QGraphicsRectItem::itemChange(change, value);
}

//https://forum.qt.io/topic/32869/solved-resize-qgraphicsitem-with-drag-and-drop/3

void AGClipRectangleItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
//    qDebug()<<"AGClipRectangleItem::paint"<<isPressed;

    QPen pen(Qt::white);
    pen.setWidth(2);
    painter->setPen(pen);
    QRectF rec = boundingRect();

    QBrush brush(Qt::magenta);
    brush.setStyle(Qt::SolidPattern);

    if (option->state & QStyle::State_Selected)
    {
        brush.setColor(Qt::blue);
    }


    painter->fillRect(rec,brush);
    QGraphicsRectItem::paint(painter, option, widget);
}

void AGClipRectangleItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    qDebug()<<"AGClipRectangleItem::mousePressEvent"<<event;

    AGClipRectItem *clipItem = (AGClipRectItem *)this;

    mDragStartPosition = QCursor::pos();
    originalClipIn = clipItem->clipIn;
    originalClipOut = clipItem->clipOut;
    QGraphicsRectItem::mousePressEvent(event);
}

void AGClipRectangleItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{

    prepareGeometryChange();

    if( (event->buttons() == Qt::LeftButton) && !mDragging)
        mDragging = true;

    //    qDebug()<<"AGClipRectangleItem::mouseMoveEvent"<<event<<mDragging<<leftDragZone<<rightDragZone;

    if (mDragging)
    {
        mDragEndPosition = QCursor::pos();

        this->setFlag(QGraphicsItem::ItemIsMovable, false);
//        double multiplier = (mDragEndPosition.x()-mDragStartPosition.x())/rect().width();

        AGClipRectItem *clipItem = (AGClipRectItem *)this;

        if (rightDragZone || midDragZone)
        {
            draggedWidth = rect().width() + mDragEndPosition.x() - mDragStartPosition.x();

            clipItem->clipOut = draggedWidth/rect().width() * originalClipOut;

            qDebug()<<"AGClipRectangleItem::mouseMoveEvent right"<<draggedWidth<<draggedWidth/rect().width()<<AGlobal().msec_to_time(originalClipOut)<<AGlobal().msec_to_time(clipItem->clipOut);
        }

        if (leftDragZone || midDragZone)
        {
            draggedX = rect().x() + mDragEndPosition.x() - mDragStartPosition.x();

            clipItem->clipIn = originalClipIn + draggedX/rect().width() * (originalClipOut - originalClipIn);

            qDebug()<<"AGClipRectangleItem::mouseMoveEvent left"<<draggedX<<draggedX/rect().width()<<draggedWidth<<draggedWidth/rect().width()<<AGlobal().msec_to_time(clipItem->clipIn);
        }

        emit agItemChanged(this); //draws poly based on clipin and clipout

        update();
    }

    QGraphicsRectItem::mouseMoveEvent(event);
}

void AGClipRectangleItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{

    qDebug()<<"AGClipRectangleItem::mouseReleaseEvent"<<event<<draggedX<<draggedWidth;

    prepareGeometryChange();
    update();

    mDragging = false;

    this->setFlag(QGraphicsItem::ItemIsMovable, true);

    setRect(QRectF(rect().x(), rect().y(), draggedWidth-draggedX, rect().height()));

    emit agMouseReleased(this);


    QGraphicsRectItem::mouseReleaseEvent(event);
}

void AGClipRectangleItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
//    qDebug()<<"AGClipRectangleItem::hoverMoveEvent"<<event<<event->pos().x()<<xr;
//    if (!mDragging)
    {
    if(event->pos().x() / draggedWidth <= 0.2 )
    {
        this->setCursor(Qt::SizeHorCursor);
        this->leftDragZone=true;
        this->midDragZone=false;
        this->rightDragZone=false;
    }
    else if(event->pos().x() / draggedWidth >= 0.8)
    {
        this->setCursor(Qt::SizeHorCursor);
        this->leftDragZone=false;
        this->midDragZone=false;
        this->rightDragZone=true;
    }
    else
    {
        this->setCursor(Qt::SizeAllCursor);
        this->leftDragZone=false;
        this->midDragZone=true;
        this->rightDragZone=false;
    }
    }

    AGClipRectItem *clipItem = (AGClipRectItem *)this;

    if (originalClipOut == 0) //MediaFile Base
    {
        qDebug()<<"clip duration"<<clipItem->clipOut - clipItem->clipIn;
        originalClipOut = clipItem->clipOut;
    }



    qDebug()<<__func__<<"emit hoverPositionChanged"<<clipItem->clipIn<<clipItem->clipOut<<event->pos()<<draggedWidth<<event->pos().x()/double(draggedWidth);
    emit hoverPositionChanged(this, clipItem->clipIn + (clipItem->clipOut - clipItem->clipIn) * event->pos().x()/double(draggedWidth));

    QGraphicsRectItem::hoverMoveEvent(event);
}

//updates while dragging
QRectF AGClipRectangleItem::boundingRect() const
{
//    qDebug()<<"AGClipRectangleItem::boundingRect()"<<rect();
    if (mDragging)
        return QRectF(draggedX, rect().y(), draggedWidth-draggedX, rect().height());
    else
        return QGraphicsRectItem::boundingRect();
}
