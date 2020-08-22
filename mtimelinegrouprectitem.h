#ifndef MTIMELINEGROUPRECTITEM_H
#define MTIMELINEGROUPRECTITEM_H

#include "agcliprectitem.h"
#include "agviewrectitem.h"
#include "mgrouprectitem.h"

class MGroupRectItem;
class AGClipRectItem;

class MTimelineGroupRectItem: public AGViewRectItem
{
    Q_OBJECT

public:
    MTimelineGroupRectItem(QGraphicsItem *parent = nullptr, QFileInfo fileInfo = QFileInfo());

    MGroupRectItem *groupItem;

    QList<AGClipRectItem *> clips;
    QList<AGClipRectItem *> filteredClips; //AGClipRectItem causes compile problems
};

#endif // MTIMELINEGROUPRECTITEM_H
