#ifndef AGTagTextItem_H
#define AGTagTextItem_H

#include <QFileInfo>
#include <QGraphicsTextItem>
#include <QGraphicsView>
#include <QMenu>

#include <QObject>

class AGTagTextItem: public QGraphicsTextItem
{
    Q_OBJECT

    QMenu *fileContextMenu = nullptr;

public:
    AGTagTextItem(QGraphicsItem *parent = nullptr, QFileInfo fileInfo = QFileInfo(), QString tagName = "");
    void onItemRightClicked(QGraphicsView *view, QPoint pos);

    QString mediaType;
    QString itemType;

    QFileInfo fileInfo;
//    QString fileNameWithoutExtension;
//    QString extension;

    QString tagName;

private slots:
    void mousePressEvent(QGraphicsSceneMouseEvent *event);


};



#endif // AGTagTextItem_H
