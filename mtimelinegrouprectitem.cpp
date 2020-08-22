#include "mtimelinegrouprectitem.h"

#include "agview.h" //for the constants

MTimelineGroupRectItem::MTimelineGroupRectItem(QGraphicsItem *parent, QFileInfo fileInfo) :
    AGViewRectItem(parent, fileInfo)
{
    this->mediaType = "TimelineGroup";
    this->itemType = "Base";

    groupItem = (MGroupRectItem *)parent;
//    groupItem->setFocusProxy(this); //FileGroupItem->focusProxy == timelineGroupItem
    groupItem->timelineGroupItem = this;

    setRect(QRectF(0, 0, 0, 200)); //invisible
    setBrush(Qt::red);

    //        setBrush(Qt::darkCyan);
    QPen pen(Qt::transparent);
    setPen(pen);

    setData(mediaTypeIndex, mediaType);
    setData(itemTypeIndex, itemType);

    updateToolTip();

}
