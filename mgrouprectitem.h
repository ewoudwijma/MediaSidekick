#ifndef MGROUPRECTITEM_H
#define MGROUPRECTITEM_H

#include "agfolderrectitem.h"
#include "agviewrectitem.h"
#include "mtimelinegrouprectitem.h"

#include <QGraphicsView>
#include <agmediafilerectitem.h>

class MTimelineGroupRectItem;
class AGFolderRectItem;
class AGMediaFileRectItem;

class MGroupRectItem: public AGViewRectItem
{
    Q_OBJECT

public:
    MGroupRectItem(QGraphicsItem *parent = nullptr, QFileInfo fileInfo = QFileInfo());
    void onItemRightClicked(QPoint pos);

    MTimelineGroupRectItem *timelineGroupItem = nullptr;
    AGFolderRectItem *folderItem = nullptr;
    QList<AGMediaFileRectItem *> mediaFiles;

    QGraphicsProxyWidget* audioLevelSliderProxy = nullptr;

    void setTextItem(QTime time, QTime totalTime);
};

#endif // MGROUPRECTITEM_H
