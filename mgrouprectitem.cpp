#include "mgrouprectitem.h"

#include "agview.h" //for the constants

#include <QGraphicsEffect>

MGroupRectItem::MGroupRectItem(QGraphicsItem *parent, QFileInfo fileInfo) :
    AGViewRectItem(parent, fileInfo)
{
    this->mediaType = "FileGroup";
    this->itemType = "Base";
//    this->folderItem = (AGFolderRectItem *)parent;

    folderItem = (AGFolderRectItem *)parent;
    if (fileInfo.fileName() == "Parking")
    {
//        folderItem->setFocusProxy(this); //Folder->focusProxy == FileGroupParking
        folderItem->parkingGroupItem = this;
    }

    QPen pen(Qt::transparent);
    setPen(pen);

    setRect(QRectF(0, 0, 200 * 9.0 / 16.0, 200 * 9.0 / 16.0));

//    setItemProperties("FileGroup", "Base", 0, QSize());

    setData(itemTypeIndex, itemType);
    setData(mediaTypeIndex, mediaType);

    updateToolTip();

    pictureItem = new QGraphicsPixmapItem(this);
    QImage image = QImage(":/images/Folder.png");
    QPixmap pixmap = QPixmap::fromImage(image);//.scaled(QSize(200,200 * myImage.height() / myImage.width()))
    pictureItem->setPixmap(pixmap);
    if (image.height() != 0)
        pictureItem->setScale(200.0 * 9.0 / 16.0 / image.height() * 0.8);

    pictureItem->setData(mediaTypeIndex, mediaType);
    pictureItem->setData(itemTypeIndex, "SubPicture");
    pictureItem->setData(folderNameIndex, fileInfo.absolutePath());
    pictureItem->setData(fileNameIndex, fileInfo.fileName());

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
