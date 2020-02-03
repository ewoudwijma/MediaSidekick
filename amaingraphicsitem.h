#ifndef AMAINGRAPHICSITEM_H
#define AMAINGRAPHICSITEM_H

#include <QGraphicsItem>



class AMainGraphicsItem: public QGraphicsPixmapItem
{
public:
    AMainGraphicsItem();
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

protected:
//    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
//    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
};

#endif // AMAINGRAPHICSITEM_H
