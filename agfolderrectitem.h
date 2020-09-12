#ifndef AGFOLDERRECTITEM_H
#define AGFOLDERRECTITEM_H

#include "agviewrectitem.h"
#include "mgrouprectitem.h"

#include <QGraphicsView>

class MGroupRectItem;

class AGFolderRectItem: public AGViewRectItem
{
    Q_OBJECT

public:
    AGFolderRectItem(QGraphicsItem *parent = nullptr, QFileInfo fileInfo = QFileInfo());
    void onItemRightClicked(QPoint pos);

    QString transitionValueChangedBy;

    MGroupRectItem *parkingGroupItem = nullptr;
    QList<MGroupRectItem *> groups;

    void setTextItem(QTime time, QTime totalTime);
};

#endif // AGFOLDERRECTITEM_H
