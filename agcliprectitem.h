#ifndef AGCLIPRECTITEM_H
#define AGCLIPRECTITEM_H

#include "agmediafilerectitem.h"
#include "agviewrectitem.h"

#include <QGraphicsView>

class AGClipRectItem: public AGViewRectItem
{
    Q_OBJECT

public:
    AGClipRectItem(QGraphicsItem *parent = nullptr, AGMediaFileRectItem *mediaItem = nullptr, QFileInfo fileInfo = QFileInfo(), int duration = 0, int clipIn = 0, int clipOut = 0);
    void onItemRightClicked(QGraphicsView *view, QPoint pos);
    QString itemToString();
    int clipIn;
    int clipOut;

    QGraphicsItem *drawPoly();

    AGMediaFileRectItem *mediaItem;
};

#endif // AGCLIPRECTITEM_H
