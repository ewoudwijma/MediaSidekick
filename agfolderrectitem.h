#ifndef AGFOLDERRECTITEM_H
#define AGFOLDERRECTITEM_H

#include "agviewrectitem.h"

#include <QGraphicsView>

class AGFolderRectItem: public AGViewRectItem
{
    Q_OBJECT

public:
    AGFolderRectItem(QGraphicsItem *parent = nullptr, QFileInfo fileInfo = QFileInfo());
    void onItemRightClicked(QPoint pos);

    QString transitionValueChangedBy;

};

#endif // AGFOLDERRECTITEM_H