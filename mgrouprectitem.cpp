#include "mgrouprectitem.h"

#include "agview.h" //for the constants

#include <QGraphicsEffect>

MGroupRectItem::MGroupRectItem(QGraphicsItem *parent, QFileInfo fileInfo) :
    AGViewRectItem(parent, fileInfo)
{
    this->mediaType = "FileGroup";
    this->itemType = "Base";

    QPen pen(Qt::transparent);
    setPen(pen);

    setRect(QRectF(0, 0, 200 * 9 / 16, 200 * 9 / 16));

    setItemProperties("FileGroup", "Base", 0, QSize());

    pictureItem = new QGraphicsPixmapItem(this);
    QImage image = QImage(":/images/Folder.png");
    QPixmap pixmap = QPixmap::fromImage(image);//.scaled(QSize(200,200 * myImage.height() / myImage.width()))
    pictureItem->setPixmap(pixmap);
    if (image.height() != 0)
        pictureItem->setScale(200.0 * 9.0 / 16.0 / image.height() * 0.8);
//            setItemProperties(pictureItem, mediaType, "SubPicture", fileInfo.absolutePath() + "/", fileName, duration);
    pictureItem->setData(mediaTypeIndex, mediaType);
    pictureItem->setData(itemTypeIndex, "SubPicture");
    pictureItem->setData(folderNameIndex, fileInfo.absolutePath());
    pictureItem->setData(fileNameIndex, fileInfo.fileName());

//    pictureItem->setData(mediaDurationIndex, duration);
    pictureItem->setData(mediaWithIndex, 0);
    pictureItem->setData(mediaHeightIndex, 0);

    QGraphicsColorizeEffect *bef = new QGraphicsColorizeEffect();
    if (fileInfo.fileName() == "Audio")
        bef->setColor(Qt::darkGreen);
    else if (fileInfo.fileName() == "Image")
        bef->setColor(Qt::yellow);
    else if (fileInfo.fileName() == "Project")
        bef->setColor(Qt::cyan);
    else if (fileInfo.fileName() == "Parking")
        bef->setColor(Qt::red);
    else //video
        bef->setColor(Qt::blue);
    pictureItem->setGraphicsEffect(bef); //make it significantly slower
}

void MGroupRectItem::onItemRightClicked(QPoint pos)
{
    fileContextMenu->clear();

    AGViewRectItem::onItemRightClicked(pos);

    QPointF map = scene()->views().first()->mapToGlobal(QPoint(pos.x()+10, pos.y()));
    fileContextMenu->popup(QPoint(map.x(), map.y()));
}
