#ifndef AGMEDIARECTANGLEITEM_H
#define AGMEDIARECTANGLEITEM_H


#include <QObject>
#include <QGraphicsRectItem>
#include <QDebug>
#include <QKeyEvent>
#include <QGraphicsSceneMouseEvent>

class AGMediaRectangleItem: public QObject, public QGraphicsRectItem
{
    Q_OBJECT

public:
    AGMediaRectangleItem(QGraphicsItem *parent = nullptr);

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

signals:
//    void agItemChanged(AGMediaRectangleItem *clipItem);
    void clipPositionChanged(QGraphicsRectItem *rectItem, double position);

private slots:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override;
};


#endif // AGMEDIARECTANGLEITEM_H
