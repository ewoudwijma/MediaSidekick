#include "amaingraphicsitem.h"

#include <QBrush>
#include <QPainter>

AMainGraphicsItem::AMainGraphicsItem()
{
    setFlag(ItemIsMovable);
//    QPixmap pixmap(":/acvc.ico");

//    setPixmap(pixmap);
}

QRectF AMainGraphicsItem::boundingRect() const
{
    return QRect(0,0,100,100);
}

void AMainGraphicsItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    QRectF rec = boundingRect();
    QBrush brush(Qt::blue);
    painter->fillRect(rec, brush);
//    painter->drawRect(rec);

}

//void AMainGraphicsItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
//{
//    QGraphicsItem::mousePressEvent(event);
//}

//void AMainGraphicsItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
//{
//    QGraphicsItem::mouseReleaseEvent(event);

//}
