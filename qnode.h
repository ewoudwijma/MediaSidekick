#ifndef QNODE_H
#define QNODE_H


#include <QGraphicsItem>
#include <QList>

class QEdge;
class QGraphWidget;
QT_BEGIN_NAMESPACE
class QGraphicsSceneMouseEvent;
QT_END_NAMESPACE

//! [0]
class QNode : public QGraphicsItem
{
public:
    QNode(QGraphWidget *graphWidget);

    void addEdge(QEdge *edge);
    QList<QEdge *> edges() const;

    enum { Type = UserType + 1 };
    int type() const override { return Type; }

    void calculateForces();
    bool advancePosition();

    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    QString nodeName;

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

private:
    QList<QEdge *> edgeList;
    QPointF newPos;
    QGraphWidget *graph;

};
//! [0]

#endif // QNODE_H
