#ifndef QGRAPHWIDGET_H
#define QGRAPHWIDGET_H


#include <QGraphicsView>

class QNode;

//! [0]
class QGraphWidget : public QGraphicsView
{
    Q_OBJECT

public:
    QGraphWidget(QWidget *parent = nullptr);

    void itemMoved();

    QNode* addNode(QString nodeName, int x = -1, int y = -1);
    void connectNodes(QString node1Name, QString node2Name, QString edgeName);
public slots:
    void shuffle();
    void zoomIn();
    void zoomOut();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void timerEvent(QTimerEvent *event) override;
#if QT_CONFIG(wheelevent)
    void wheelEvent(QWheelEvent *event) override;
#endif
    void drawBackground(QPainter *painter, const QRectF &rect) override;

    void scaleView(qreal scaleFactor);

private:
    int timerId;
    QNode *centerNode;
};
//! [0]

#endif // QGRAPHWIDGET_H
