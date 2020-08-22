#ifndef AGCLIPRECTITEM_H
#define AGCLIPRECTITEM_H

#include "agmediafilerectitem.h"
#include "agtagtextitem.h"
#include "agviewrectitem.h"

#include <QGraphicsView>

class AGMediaFileRectItem;
class AGTagTextItem;

class AGClipRectItem: public AGViewRectItem
{
    Q_OBJECT

    QString keyBuffer;

public:
    AGClipRectItem(QGraphicsItem *parent = nullptr, AGMediaFileRectItem *mediaItem = nullptr, QFileInfo fileInfo = QFileInfo(), int duration = 0, int clipIn = 0, int clipOut = 0);
    void onItemRightClicked(QPoint pos);
    QString itemToString();
    int clipIn;
    int clipOut;

    QGraphicsItem *drawPoly();

    AGMediaFileRectItem *mediaItem;

    void processAction(QString action);
    ~AGClipRectItem();

    QList<AGTagTextItem *> tags;

//    bool changed = false;
signals:
    void addItem(bool changed, QString parentName, QString mediaType, QFileInfo fileInfo = QFileInfo(), int duration = 0, int clipIn = 0, int clipOut = 0, QString tag = "");
    void deleteItem(bool changed, QString mediaType, QFileInfo fileInfo, int clipIn = -1, QString tagName = "");
    void addUndo(bool changed, QString action, QString mediaType, QGraphicsItem *item, QString property = "", QString oldValue = "", QString newValue = "");

};

#endif // AGCLIPRECTITEM_H
