/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qgraphwidget.h"
#include "qedge.h"
#include "qnode.h"

#include <math.h>

#include <QKeyEvent>
#include <QRandomGenerator>

#include <QDebug>

const int multiplier = 4;
//! [0]
QGraphWidget::QGraphWidget(QWidget *parent)
    : QGraphicsView(parent), timerId(0)
{
    QGraphicsScene *scene = new QGraphicsScene(this);
    scene->setItemIndexMethod(QGraphicsScene::NoIndex);
    scene->setSceneRect(-200, -400, 800, 400);
//    scene->setSceneRect(-200, 200, parent->width(), parent->height());
    setScene(scene);
    setCacheMode(CacheBackground);
    setViewportUpdateMode(BoundingRectViewportUpdate);
    setRenderHint(QPainter::Antialiasing);
    setTransformationAnchor(AnchorUnderMouse);
    scale(qreal(2), qreal(2));
//    setMinimumSize(400, 400);
    setWindowTitle(tr("Elastic Nodes"));
//! [0]

//! [1]
//    QNode *node1 = new QNode(this);
//    node1->nodeName = "Test1";
//    QNode *node2 = new QNode(this);
//    node2->nodeName = "Test2";
//    QNode *node3 = new QNode(this);
//    node3->nodeName = "Test3";
//    QNode *node4 = new QNode(this);
//    node4->nodeName = "Test4";
//    centerNode = new QNode(this);
//    centerNode->nodeName = "cent";
//    QNode *node6 = new QNode(this);
//    node6->nodeName = "Test6";
//    QNode *node7 = new QNode(this);
//    node7->nodeName = "Test7";
//    QNode *node8 = new QNode(this);
//    node8->nodeName = "Test8";
//    QNode *node9 = new QNode(this);
//    node9->nodeName = "Test9";
//    scene->addItem(node1);
//    scene->addItem(node2);
//    scene->addItem(node3);
//    scene->addItem(node4);
//    scene->addItem(centerNode);
//    scene->addItem(node6);
//    scene->addItem(node7);
//    scene->addItem(node8);
//    scene->addItem(node9);
//    scene->addItem(new QEdge(node1, node2));
//    scene->addItem(new QEdge(node2, node3));
//    scene->addItem(new QEdge(node1, centerNode));
//    scene->addItem(new QEdge(node1, node9));
//    scene->addItem(new QEdge(node2, centerNode));
//    scene->addItem(new QEdge(node3, node6));
//    scene->addItem(new QEdge(node4, node1));
//    scene->addItem(new QEdge(node4, centerNode));
//    scene->addItem(new QEdge(centerNode, node6));
//    scene->addItem(new QEdge(centerNode, node8));
//    scene->addItem(new QEdge(node6, node9));
//    scene->addItem(new QEdge(node7, node4));
//    scene->addItem(new QEdge(node8, node7));
//    scene->addItem(new QEdge(node9, node8));

//    node1->setPos(-50 * multiplier, -50 * multiplier);
//    node2->setPos(0, -50 * multiplier);
//    node3->setPos(50 * multiplier, -50 * multiplier);
//    node4->setPos(-50 * multiplier, 0);
//    centerNode->setPos(0, 0);
//    node6->setPos(50 * multiplier, 0);
//    node7->setPos(-50 * multiplier, 50 * multiplier);
//    node8->setPos(0, 50 * multiplier);
//    node9->setPos(50 * multiplier, 50 * multiplier);
}
//! [1]

QNode* QGraphWidget::addNode(QString nodeName, int x, int y)
{
    QNode *node = new QNode(this);
    if (x==-1 || y==-1)
        node->setPos(scene()->items().count() * 50, 0);
    else
        node->setPos(x * 50 - parentWidget()->width() / 4, (y-1)*50 - parentWidget()->height()/2);
    node->nodeName = nodeName;
    scene()->addItem(node);
    return node;

}

void QGraphWidget::connectNodes(QString node1Name, QString node2Name, QString edgeName)
{
    QNode *node1 = nullptr;
    QNode *node2 = nullptr;

    QList<QGraphicsItem *> items = scene()->items();

//    foreach (QGraphicsItem *item, scene()->items()) {
//        if (QNode *node = qgraphicsitem_cast<QNode *>(item))
//            nodes << node;
//    }

    for (int i=0; i< items.count();i++)
    {
        if (QNode *node = qgraphicsitem_cast<QNode *>(items[i]))
        {
//            qDebug()<<"QGraphWidget::connectNodes"<<node->nodeName<<node1Name<<node2Name;
            if (node->nodeName == node1Name)
                node1 = node;
            if (node->nodeName == node2Name)
                node2 = node;
        }
    }

    if (node1 == nullptr)
        node1 = addNode(node1Name);
    if (node2 == nullptr)
        node2 = addNode(node2Name);

//    if (node1 != QNode() && node2 != QNode())
    QEdge *edge = new QEdge(node1, node2);
    edge->edgeName = edgeName;
    scene()->addItem(edge);

}

//! [2]
void QGraphWidget::itemMoved()
{
//    if (!timerId)
//        timerId = startTimer(1000 / 25);
//        timerId = startTimer(1);
}
//! [2]

//! [3]
void QGraphWidget::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Up:
        centerNode->moveBy(0, -20);
        break;
    case Qt::Key_Down:
        centerNode->moveBy(0, 20);
        break;
    case Qt::Key_Left:
        centerNode->moveBy(-20, 0);
        break;
    case Qt::Key_Right:
        centerNode->moveBy(20, 0);
        break;
    case Qt::Key_Plus:
        zoomIn();
        break;
    case Qt::Key_Minus:
        zoomOut();
        break;
    case Qt::Key_Space:
    case Qt::Key_Enter:
        shuffle();
        break;
    default:
        QGraphicsView::keyPressEvent(event);
    }
}
//! [3]

//! [4]
void QGraphWidget::timerEvent(QTimerEvent *event)
{
    Q_UNUSED(event);

    QList<QNode *> nodes;
    foreach (QGraphicsItem *item, scene()->items()) {
        if (QNode *node = qgraphicsitem_cast<QNode *>(item))
            nodes << node;
    }

    foreach (QNode *node, nodes)
        node->calculateForces();

    bool itemsMoved = false;
    foreach (QNode *node, nodes) {
        if (node->advancePosition())
            itemsMoved = true;
    }

    if (!itemsMoved) {
        killTimer(timerId);
        timerId = 0;
    }
}
//! [4]

#if QT_CONFIG(wheelevent)
//! [5]
void QGraphWidget::wheelEvent(QWheelEvent *event)
{
    scaleView(pow((double)2, -event->delta() / 240.0));
}
//! [5]
#endif

//! [6]
void QGraphWidget::drawBackground(QPainter *painter, const QRectF &rect)
{
//    return;
    Q_UNUSED(rect);

    // Shadow
    QRectF sceneRect = this->sceneRect();
    QRectF rightShadow(sceneRect.right(), sceneRect.top() + 5, 5, sceneRect.height());
    QRectF bottomShadow(sceneRect.left() + 5, sceneRect.bottom(), sceneRect.width(), 5);
    if (rightShadow.intersects(rect) || rightShadow.contains(rect))
        painter->fillRect(rightShadow, Qt::darkGray);
    if (bottomShadow.intersects(rect) || bottomShadow.contains(rect))
        painter->fillRect(bottomShadow, Qt::darkGray);

    // Fill
    QLinearGradient gradient(sceneRect.topLeft(), sceneRect.bottomRight());
    gradient.setColorAt(0, Qt::white);
    gradient.setColorAt(1, Qt::lightGray);
    painter->fillRect(rect.intersected(sceneRect), gradient);
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(sceneRect);

    // Text
    QRectF textRect(sceneRect.left() + 4, sceneRect.top() + 4,
                    sceneRect.width() - 4, sceneRect.height() - 4);
//    QString message(tr("Click and drag the nodes around, and zoom with the mouse "
//                       "wheel or the '+' and '-' keys"));

    QFont font = painter->font();
    font.setBold(true);
    font.setPointSize(14);
    painter->setFont(font);
    painter->setPen(Qt::lightGray);
//    painter->drawText(textRect.translated(2, 2), message);
//    painter->setPen(Qt::black);
//    painter->drawText(textRect, message);
}
//! [6]

//! [7]
void QGraphWidget::scaleView(qreal scaleFactor)
{
    qreal factor = transform().scale(scaleFactor, scaleFactor).mapRect(QRectF(0, 0, 1, 1)).width();
    if (factor < 0.07 || factor > 100)
        return;

    scale(scaleFactor, scaleFactor);
}
//! [7]

void QGraphWidget::shuffle()
{
    foreach (QGraphicsItem *item, scene()->items()) {
        if (qgraphicsitem_cast<QNode *>(item))
            item->setPos(-150 + QRandomGenerator::global()->bounded(300), -150 + QRandomGenerator::global()->bounded(300));
    }
}

void QGraphWidget::zoomIn()
{
    scaleView(qreal(1.2));
}

void QGraphWidget::zoomOut()
{
    scaleView(1 / qreal(1.2));
}
