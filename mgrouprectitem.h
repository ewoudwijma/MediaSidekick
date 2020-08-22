#ifndef MGROUPRECTITEM_H
#define MGROUPRECTITEM_H

#include "agfolderrectitem.h"
#include "agviewrectitem.h"
#include "mtimelinegrouprectitem.h"

#include <QGraphicsView>

class MTimelineGroupRectItem;
class AGFolderRectItem;

class MGroupRectItem: public AGViewRectItem
{
    Q_OBJECT

public:
    MGroupRectItem(QGraphicsItem *parent = nullptr, QFileInfo fileInfo = QFileInfo());
    void onItemRightClicked(QPoint pos);

    MTimelineGroupRectItem *timelineGroupItem = nullptr;
    AGFolderRectItem *folderItem = nullptr;

//private slots:
//    void onProcessOutput(QTime time, QTime totalTime, QString event, QString outputString);
};


#endif // MGROUPRECTITEM_H
