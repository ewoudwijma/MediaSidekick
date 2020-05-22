#ifndef AGCLIPRECTANGLEITEM_H
#define AGCLIPRECTANGLEITEM_H


#include <QObject>
#include <QGraphicsRectItem>
#include <QDebug>
#include <QKeyEvent>
#include <QGraphicsSceneMouseEvent>

class AGClipRectangleItem: public QObject, public QGraphicsRectItem
{
    Q_OBJECT

    int draggedWidth;
    int draggedX;

    QPointF mDragStartPosition;
    QPointF mDragEndPosition;
    bool mDragging;
    bool leftDragZone;
    bool midDragZone;
    bool rightDragZone;

    int originalClipIn;
    int originalClipOut;
public:
    AGClipRectangleItem(QGraphicsItem *parent = nullptr);
    void setRect(const QRectF &rectangle);

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

signals:
    void agItemChanged(QGraphicsItem *clipItem);
    void agMouseReleased(QGraphicsItem *clipItem);
    void hoverPositionChanged(QGraphicsItem *clipItem, double position);

private slots:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    QRectF boundingRect() const override;
};

#endif // AGCLIPRECTANGLEITEM_H
