#ifndef MGROUPRECTITEM_H
#define MGROUPRECTITEM_H

#include "agviewrectitem.h"

#include <QGraphicsView>

class MGroupRectItem: public AGViewRectItem
{
    Q_OBJECT

public:
    MGroupRectItem(QGraphicsItem *parent = nullptr, QFileInfo fileInfo = QFileInfo());
    void onItemRightClicked(QGraphicsView *view, QPoint pos);

//private slots:
//    void onProcessOutput(QTime time, QTime totalTime, QString event, QString outputString);
};


#endif // MGROUPRECTITEM_H
