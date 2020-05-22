#include "aglobal.h"
#include "agviewrectitem.h"
#include "agmediafilerectitem.h"
#include "agview.h"
#include "agfolderrectitem.h"
#include "agcliprectitem.h"
#include "agtagtextitem.h"

#include <QGraphicsItem>

#include <QDebug>
#include <QGraphicsVideoItem>
#include <QKeyEvent>
#include <QTimeLine>
#include <QScrollBar>
#include <QGraphicsBlurEffect>
#include <QTime>
#include <QSettings>
#include <QDialog>
#include <QVBoxLayout>
#include <QProcess>
#include <QDir>
#include <QDesktopServices>
#include <QGroupBox>
#include <QLabel>
#include <QMenu>
#include <QTextBrowser>

#include <qmath.h>

#include <QStyle>

AGView::AGView(QWidget *parent)
    : QGraphicsView(parent)
{
    scene = new QGraphicsScene(this);
    setScene(scene);

    connect(scene, &QGraphicsScene::selectionChanged, this, &AGView::onSelectionChanged);

//    QPixmap bgPix(":/images/karton.jpg");
//    setBackgroundBrush(bgPix);

    mediaWidth = 300;
}

AGView::~AGView()
{
//    qDebug()<<"AGView::~AGView"; //not called yet!

    clearAll();
}

void AGView::clearAll()
{
//    qDebug()<<"AGView::clearAll";
    if (!playInDialog)
        stopAndDeletePlayers();
    else
    {
        if (dialogMediaPlayer != nullptr)
            dialogMediaPlayer->stop();
    }

    scene->clear();
}

void AGView::stopAndDeletePlayers(QFileInfo fileInfo)
{
    foreach (QGraphicsItem *item, scene->items())
    {
        if (item->data(mediaTypeIndex).toString() == "MediaFile" && item->data(itemTypeIndex).toString() == "Base")
        {
            AGMediaFileRectItem *mediaItem = (AGMediaFileRectItem *)item;
            if (fileInfo == QFileInfo() || mediaItem->fileInfo == fileInfo)
            {
                AGMediaFileRectItem *mediaFile = (AGMediaFileRectItem *)item;

                if (mediaFile->m_player != nullptr)
                {
                    delete mediaFile->m_player;
                    mediaFile->m_player = nullptr;
                }

                if (mediaFile->playerItem != nullptr)
                {
                    delete mediaFile->playerItem;
                    mediaFile->playerItem = nullptr;
                }

                if (mediaFile->progressRectItem != nullptr)
                {
                    delete mediaFile->progressRectItem;
                    mediaFile->progressRectItem = nullptr;
                }
            }
        }
    }
}

void AGView::onSelectionChanged()
{
    foreach (QGraphicsItem *item, scene->selectedItems())
//        qDebug()<<"AGView::onSelectionChanged()"<<itemToString(item);

    if (scene->selectedItems().count() == 1)
    {
//        emit itemSelected(scene->selectedItems().first()); //triggers MainWindow::onGraphicsItemSelected
    }
}

void AGView::onAddItem(QString parentName, QString mediaType, QFileInfo fileInfo, int duration, int clipIn, int clipOut, QString tag)
{
    QString parentMediaType;
    if (mediaType == "Folder")
        parentMediaType = "Folder";
    else if (mediaType == "FileGroup")
        parentMediaType = "Folder";
    else if (mediaType == "TimelineGroup")
        parentMediaType = "FileGroup";
    else if (mediaType == "MediaFile")
        parentMediaType = "FileGroup";
    else if (mediaType == "Clip")
        parentMediaType = QSettings().value("viewMode")=="SpotView"?"MediaFile":"TimelineGroup";
    else if (mediaType == "Tag")
        parentMediaType = "Clip";

    QGraphicsItem *parentItem = nullptr;
    QGraphicsItem *proxyItem = nullptr;
    //find the parentitem

    foreach (QGraphicsItem *item, scene->items())
    {
        if (item->data(itemTypeIndex).toString() == "Base")
        {
            QFileInfo viewFileInfo;
            if (item->data(mediaTypeIndex).toString() == "Tag")
            {
                AGTagTextItem *viewItem = (AGTagTextItem *)item;
                viewFileInfo = viewItem->fileInfo;
            }
            else
            {
                AGViewRectItem *viewItem = (AGViewRectItem *)item;
                viewFileInfo = viewItem->fileInfo;
            }

            //if clip added then find the corresponding mediafile
            if (mediaType == "Clip" && item->data(mediaTypeIndex).toString() == "MediaFile" && fileInfo.absolutePath() == viewFileInfo.absolutePath() && fileInfo.completeBaseName() == viewFileInfo.completeBaseName())
            {
                proxyItem = item;
                if (fileInfo.fileName() != viewFileInfo.fileName())
                {
//                    qDebug()<<"Rename"<<mediaType<<fileName<<viewFileInfo.fileName();
                    fileInfo.setFile(viewFileInfo.absoluteFilePath());//adjust the fileName to the filename of the corresponding mediaFile
                }
            }
            if (mediaType == "Tag" && item->data(mediaTypeIndex).toString() == "Clip" && viewFileInfo.absolutePath() == fileInfo.absolutePath() && viewFileInfo.completeBaseName() == fileInfo.completeBaseName())
            {
                if (fileInfo.fileName() != viewFileInfo.fileName())
                {
                    fileInfo.setFile(viewFileInfo.absoluteFilePath());//adjust the fileName to the filename of the corresponding mediaFile
                }
            }

            AGClipRectItem *clipItem = (AGClipRectItem *)item;

            if (item->data(mediaTypeIndex).toString() == parentMediaType)
            {
                if (mediaType == "Folder" && viewFileInfo.absolutePath() + "/" + viewFileInfo.fileName() == fileInfo.absolutePath())
                    parentItem = item;
                else if (mediaType == "FileGroup" && viewFileInfo.absolutePath() + "/" + viewFileInfo.fileName() == fileInfo.absolutePath())
                    parentItem = item;
                else if (mediaType == "TimelineGroup" && viewFileInfo.absolutePath() == fileInfo.absolutePath() && viewFileInfo.fileName() == parentName)
                    parentItem = item;
                else if (mediaType == "MediaFile" && viewFileInfo.absolutePath() == fileInfo.absolutePath() && viewFileInfo.fileName() == parentName)
                    parentItem = item;
                else if (mediaType == "Clip" && viewFileInfo.absolutePath() == fileInfo.absolutePath() && viewFileInfo.completeBaseName() == fileInfo.completeBaseName()) //MediaFile (do not match extension as clips can be called with .srt)
                    parentItem = item;
//                    qDebug()<<"Parent"<<mediaType<<folderName<<fileName<<itemToString(parentItem);
                else if (mediaType == "Clip" && viewFileInfo.absolutePath() == fileInfo.absolutePath() && viewFileInfo.fileName() == parentName) //Timeline
                    parentItem = item;
                else if (mediaType == "Tag" && viewFileInfo.absolutePath() == fileInfo.absolutePath() && viewFileInfo.completeBaseName() == fileInfo.completeBaseName() && clipItem->clipIn ==  clipIn)
                    parentItem = item;
            }
        }
    }

//    if (parentItem == nullptr)
//    qDebug()<<"AGView::addItem"<<(QThread::currentThread() == qApp->thread()?"Main":"Thread")<<parentName<<mediaType<<fileInfo.absoluteFilePath()<<parentMediaType<<parentItem;

    if (QThread::currentThread() != qApp->thread())
        qDebug()<<"AGView::addItem thread problem"<<parentName<<mediaType<<fileInfo.absolutePath() + "/"<<fileInfo.fileName()<<parentMediaType<<parentItem;

    QGraphicsItem *childItem = nullptr;

    if (mediaType == "Folder")
    {
        AGFolderRectItem *folderItem = new AGFolderRectItem(parentItem, fileInfo);

        folderItem->updateToolTip();

        childItem = folderItem;
    }
    else if (mediaType.contains("Group"))
    {
        AGViewRectItem *rectItem = new AGViewRectItem(parentItem, fileInfo);

        if (mediaType == "TimelineGroup")
        {
            rectItem->setRect(QRectF(0, 0, 0, 200)); //invisible
            rectItem->setBrush(Qt::red);

            parentItem->setFocusProxy(rectItem); //FileGroupItem->focusProxy == TimelineGroup
        }
        else //fileGroup
        {
            rectItem->setRect(QRectF(0, 0, 200 * 9 / 16, 200 * 9 / 16));

            rectItem->pictureItem = new QGraphicsPixmapItem(rectItem);
            QImage image = QImage(":/images/Folder.png");
            QPixmap pixmap = QPixmap::fromImage(image);//.scaled(QSize(200,200 * myImage.height() / myImage.width()))
            rectItem->pictureItem->setPixmap(pixmap);
            if (image.height() != 0)
                rectItem->pictureItem->setScale(200.0 * 9.0 / 16.0 / image.height() * 0.8);
//            setItemProperties(rectItem->pictureItem, mediaType, "SubPicture", fileInfo.absolutePath() + "/", fileName, duration);
            rectItem->pictureItem->setData(mediaTypeIndex, mediaType);
            rectItem->pictureItem->setData(itemTypeIndex, "SubPicture");
            rectItem->pictureItem->setData(folderNameIndex, fileInfo.absolutePath());
            rectItem->pictureItem->setData(fileNameIndex, fileInfo.fileName());

            rectItem->pictureItem->setData(mediaDurationIndex, duration);
            rectItem->pictureItem->setData(mediaWithIndex, 0);
            rectItem->pictureItem->setData(mediaHeightIndex, 0);

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
            rectItem->pictureItem->setGraphicsEffect(bef); //make it significantly slower

            if (fileInfo.fileName() == "Parking")
                parentItem->setFocusProxy(rectItem); //Folder->focusProxy == FileGroupParking
        }
//        rectItem->setBrush(Qt::darkCyan);
        QPen pen(Qt::transparent);
        rectItem->setPen(pen);

//        setItemProperties(rectItem, mediaType, "Base", fileInfo.absolutePath() + "/", newFileName, duration);
        rectItem->setData(mediaTypeIndex, mediaType);
        rectItem->setData(itemTypeIndex, "Base");

        rectItem->setData(mediaDurationIndex, duration);
        rectItem->setData(mediaWithIndex, 0);
        rectItem->setData(mediaHeightIndex, 0);


        childItem = rectItem;
    }
    else if (mediaType == "MediaFile")
    {
        AGMediaFileRectItem *mediaItem = new AGMediaFileRectItem(parentItem, fileInfo, duration, mediaWidth);

        connect(mediaItem, &AGMediaFileRectItem::getPropertyValue, this, &AGView::getPropertyValue); //doorgeefluik

//        connect(mediaItem, &AGMediaFileRectItem::propertyCopy, ui->propertyTreeView, &APropertyTreeView::onPropertyCopy);
//        connect(mediaItem, &AGMediaFileRectItem::exportClips, this, &MainWindow::onExportClips);


        mediaItem->updateToolTip();

//        connect(mediaItem, &AGClipRectangleItem::agItemChanged, this, &AGView::onClipItemChanged);
//        connect(mediaItem, &AGClipRectangleItem::agMouseReleased, this, &AGView::onClipMouseReleased);
        connect(mediaItem, &AGMediaFileRectItem::hoverPositionChanged, this, &AGView::onHoverPositionChanged);

        childItem = mediaItem;
        //https://doc.qt.io/qt-5/qtdatavisualization-audiolevels-example.html

        arrangeItems(nullptr);
    }
    else if (mediaType == "Clip")
    {
//        qDebug()<<"Clip"<<fileName<<fileInfo.fileName();
        if (parentItem == nullptr) //workaround in case srt file found without a mediafile
            return;

        AGClipRectItem *clipItem = new AGClipRectItem(parentItem, (AGMediaFileRectItem *)proxyItem, fileInfo, duration, clipIn, clipOut);

//        connect(clipItem, &AGViewRectItem::agItemChanged, this, &AGView::onClipItemChanged);
//        connect(clipItem, &AGViewRectItem::agMouseReleased, this, &AGView::onClipMouseReleased);
        connect(clipItem, &AGClipRectItem::hoverPositionChanged, this, &AGView::onHoverPositionChanged);

//        qDebug()<<"setFocusProxy"<<itemToString(clipItem)<<itemToString(parentItem)<<itemToString(proxyItem);

        childItem = clipItem;

        arrangeItems(parentItem); //in case of new clip added later
    }
    else if (mediaType == "Tag")
    {
        if (parentItem == nullptr) //workaround in case srt file found without a mediafile
            return;

        AGTagTextItem *tagItem = new AGTagTextItem(parentItem, fileInfo, tag);

        childItem = tagItem;
    }

//    childItem->setFlag(QGraphicsItem::ItemIsMovable, true);
    childItem->setFlag(QGraphicsItem::ItemIsSelectable, true);

    if (parentItem == nullptr)
    {
        scene->addItem(childItem);
        rootItem = childItem;
    }

    if (mediaType == "Folder" || mediaType == "FileGroup" || mediaType == "MediaFile")//  && AGlobal().audioExtensions.contains(extension, Qt::CaseInsensitive)))
//    if (mediaType != "Clip" && mediaType != "Tag" && mediaType != "TimelineGroup") //all the others get text
    {
        QGraphicsTextItem *subNameItem;
        subNameItem = new QGraphicsTextItem(childItem);
        if (QSettings().value("theme") == "Black")
            subNameItem->setDefaultTextColor(Qt::white);
        else
            subNameItem->setDefaultTextColor(Qt::black);
        subNameItem->setPlainText(fileInfo.fileName());

//        setItemProperties(subNameItem, mediaType, "SubName", fileInfo.absolutePath() + "/", fileName, duration);
        subNameItem->setData(mediaTypeIndex, mediaType);
        subNameItem->setData(itemTypeIndex, "SubName");
        subNameItem->setData(folderNameIndex, fileInfo.absolutePath());
        subNameItem->setData(fileNameIndex, fileInfo.fileName());

        subNameItem->setData(mediaDurationIndex, duration);
        subNameItem->setData(mediaWithIndex, 0);
        subNameItem->setData(mediaHeightIndex, 0);

        subNameItem->setTextWidth(childItem->boundingRect().width() * 0.8);
    }

    //position new item at the bottom (arrangeitem will put it right
//    childItem->setPos(QPointF(0, rootItem->boundingRect().height()));
}

void AGView::onDeleteItem(QString mediaType, QFileInfo fileInfo)
{
    qDebug()<<"AGView::onDeleteItem"<<(QThread::currentThread() == qApp->thread()?"Main":"Thread")<<fileInfo.absoluteFilePath()<<mediaType;

    //if (mediaType is clip then find the original extension!) //return value?

    if (QThread::currentThread() != qApp->thread())
        qDebug()<<"AGMediaFileRectItem::onDeleteItem thread problem"<<fileInfo.fileName();


    foreach (QGraphicsItem *item, scene->items())
    {
        if (item->data(itemTypeIndex) == "Base" && item->data(mediaTypeIndex) == mediaType)
        {
            AGViewRectItem *viewItem = (AGViewRectItem *)item;
            bool matchOnFile;
            if (mediaType == "Clip")
            {
                matchOnFile = viewItem->fileInfo.completeBaseName() == fileInfo.completeBaseName();
            }
            else
                matchOnFile = viewItem->fileInfo.fileName() == fileInfo.fileName();

            if (viewItem->fileInfo.absolutePath() == fileInfo.absolutePath() && matchOnFile)
            {
                if (mediaType == "MediaItem")
                {
                    AGMediaFileRectItem *mediaItem = (AGMediaFileRectItem *)item;
                    if (!playInDialog)
                    {
                        if (mediaItem->m_player != nullptr)
                        {
        //                        m_player->stop();
                            delete mediaItem->m_player;
                            mediaItem->m_player = nullptr;
                        }
                    }
                }

    //            qDebug()<<"  Item"<<itemToString(item)<<item;
    //            scene->removeItem(item);
                delete item;

                arrangeItems(nullptr);
            }
        }
    }
}

void AGView::setThemeColors(QColor color)
{
    foreach (QGraphicsItem *item, scene->items())
    {
        if (item->data(itemTypeIndex).toString() == "SubName" || item->data(itemTypeIndex).toString() == "SubLog" || item->data(mediaTypeIndex).toString() == "Tag")
        {
            QGraphicsTextItem *textItem = (QGraphicsTextItem *)item;
            textItem->setDefaultTextColor(color);
        }
        if (item->data(itemTypeIndex).toString() == "Base" && (item->data(mediaTypeIndex) == "MediaFile" || item->data(mediaTypeIndex) == "Clip"))
        {
            QGraphicsRectItem *rectItem = (QGraphicsRectItem *)item;
//            qDebug()<<"setThemeColors"<<itemToString(rectItem);
            rectItem->setPen(QPen(color));
        }
    }
}

//void AGView::onClipItemChanged(AGClipRectItem *clipItem)
//{
//    clipItem->drawPoly();
//}

//void AGView::onClipMouseReleased(AGClipRectItem *clipItem)
//{
//    //update clipIn and out
//    clipItem->updateToolTip();
//    clipItem->drawPoly();
//    arrangeItems(nullptr);
//}

void AGView::onItemRightClicked(QPoint pos)
{
    QGraphicsItem *itemAtScreenPos = itemAt(pos);

    QGraphicsItem *item = nullptr;
    //as rectItem stays the same if rightclick pressed twice, we look at the item at the screen position.
    if (itemAtScreenPos == nullptr)
        return;
    else
    {
        if (itemAtScreenPos->data(itemTypeIndex).toString().contains("Sub"))
            item = itemAtScreenPos->parentItem();
        else //Base
            item = itemAtScreenPos;
    }
//    qDebug()<<"AGView::onItemRightClicked itemAtScreenPos"<<pos<<mapFromGlobal(pos)<<itemToString(itemAtScreenPos)<<itemToString(mediaItem);
//    qDebug()<<"AGView::onItemRightClicked"<<folderName<<fileName<<screenPos<<itemToString(mediaItem);

    if (item->data(mediaTypeIndex) == "Folder")
    {
        AGFolderRectItem *folderItem = (AGFolderRectItem *)item;

        folderItem->onItemRightClicked(this, pos);
    }
    else if (item->data(mediaTypeIndex) == "MediaFile")
    {
        AGMediaFileRectItem *mediaFileItem = (AGMediaFileRectItem *)item;

        mediaFileItem->onItemRightClicked(this, pos);
    }
    else if (item->data(mediaTypeIndex) == "Clip")
    {
        AGClipRectItem *clipRectItem = (AGClipRectItem *)item;

        clipRectItem->onItemRightClicked(this, pos);
    }
    else if (item->data(mediaTypeIndex) == "Tag")
    {
        AGTagTextItem *tagItem = (AGTagTextItem *)item;

        tagItem->onItemRightClicked(this, pos);
    }
}

void AGView::onPlayerDialogFinished(int result)
{
//    qDebug()<<"AGView::onPlayerDialogFinished"<<result;
    if (dialogMediaPlayer != nullptr)
        dialogMediaPlayer->pause();
//    delete dialogMediaPlayer;
//    delete dialogVideoWidget;
//    delete playerDialog;
    dialogMediaPlayer = nullptr;
    dialogVideoWidget = nullptr;
    playerDialog = nullptr;
}

void AGView::setPlayInDialog(bool checked)
{
    if (playInDialog && !checked) //from dialog to in-item player
    {
//        dialogMediaPlayer->pause();
        stopAndDeletePlayers(); //delete progresslines
        if (playerDialog != nullptr)
            playerDialog->close();
        playInDialog = false;
    }
    if (!playInDialog && checked) //from initem to dialog player
    {
        //close all players
        stopAndDeletePlayers();
        playInDialog = true;
//        playMedia((QGraphicsRectItem *)scene->selectedItems().first());
    }
}

void AGView::onHoverPositionChanged(QGraphicsRectItem *rectItem, int progress)
{
    AGMediaFileRectItem *mediaItem = nullptr;
    if (rectItem->data(mediaTypeIndex).toString() == "Clip")
        mediaItem = (AGMediaFileRectItem *)rectItem->focusProxy();
    else if (rectItem->data(mediaTypeIndex).toString() == "MediaFile")
        mediaItem = (AGMediaFileRectItem *)rectItem;

//    qDebug()<<"AGView::onHoverPositionChanged"<<mediaItem->itemToString()<<progress<<mediaItem->progressLineItem;

    int duration = 0;
    if (!playInDialog)
    {
        if (mediaItem->m_player != nullptr)
        {
                mediaItem->m_player->setPosition(progress);
                duration = mediaItem->m_player->duration();
        }
    }
    else
    {
        if (dialogMediaPlayer != nullptr)
        {
            dialogMediaPlayer->setPosition(progress);
            duration = dialogMediaPlayer->duration();
        }
    }

    if (mediaItem->progressRectItem != nullptr && duration != 0)
        mediaItem->progressRectItem->setRect(QRectF(0,0, mediaItem->progressRectItem->parentItem()->boundingRect().width() * progress / duration, 10));
}

void AGView::onFileChanged(QFileInfo fileInfo)
{
    //main

    if (QThread::currentThread() != qApp->thread())
        qDebug()<<"AGView::onMediaFileChanged thread problem"<<fileInfo.fileName();

    AGMediaFileRectItem *mediaItem = nullptr;
    foreach (QGraphicsItem *item, scene->items())
    {
        if (item->data(itemTypeIndex) == "Base" && item->data(mediaTypeIndex) == "MediaFile")
        {
            AGViewRectItem *viewItem = (AGViewRectItem *)item;
            if (viewItem->fileInfo == fileInfo)
                mediaItem = (AGMediaFileRectItem *)item;
        }
    }

    if (mediaItem != nullptr)
    {
        mediaItem->onMediaFileChanged();

        arrangeItems(mediaItem->parentItem()); //arrange the folder / foldergroup
    } //mediaItem != nullptr
}

void AGView::reParent(QGraphicsItem *parentItem, QString prefix)
{
    if (parentItem->data(itemTypeIndex) == "Base")
    {
        AGViewRectItem *parentViewItem = (AGViewRectItem *)parentItem;
//        qDebug()<<"AGView::reParent"<<prefix + itemToString(parentItem);
        if (parentItem->data(mediaTypeIndex) == "MediaFile" && parentViewItem->parentRectItem->fileInfo.fileName() != "Export" && parentViewItem->parentRectItem->fileInfo.fileName() != "Project")
        {
            filterItem(parentItem);

            QGraphicsItem *parkingFileGroup = parentItem->focusProxy()->parentItem()->focusProxy();
            if (parentItem->data(excludedInFilter).toBool()) //filtered
            {
                //MediaFile: move to parking
//                qDebug()<<"AGView::reParent"<<prefix + itemToString(parentItem) << itemToString(parentItem->focusProxy()) << itemToString(parentItem->focusProxy()->parentItem()) << itemToString(parentItem->focusProxy()->parentItem()->focusProxy());
                parentItem->setParentItem(parkingFileGroup);

                //clips
                if (QSettings().value("viewMode").toString() == "SpotView") //Clips: move to mediafile
                {
                    foreach (QGraphicsItem *clipItem, scene->items())
                    {
                        if (clipItem->focusProxy() == parentItem && clipItem->data(itemTypeIndex).toString() == "Base" && clipItem->data(mediaTypeIndex).toString() == "Clip") //find the clips
                            clipItem->setParentItem(clipItem->focusProxy());
                    }
                }
                else //move to parking timeline
                {
                    foreach (QGraphicsItem *clipItem, scene->items())
                    {
                        if (clipItem->focusProxy() == parentItem && clipItem->data(itemTypeIndex).toString() == "Base" && clipItem->data(mediaTypeIndex).toString() == "Clip") //find the clips
                            clipItem->setParentItem(parkingFileGroup->focusProxy());
                    }
                }
            }
            else //not filtered
            {
                if (QSettings().value("viewMode").toString() == "SpotView")
                    parentItem->setParentItem(parentItem->focusProxy());
                else //TimelineView
                {
                    bool clipsFound = false;
                    foreach (QGraphicsItem *clipItem, scene->items())
                    {
                        if (clipItem->focusProxy() == parentItem && clipItem->data(itemTypeIndex).toString() == "Base" && clipItem->data(mediaTypeIndex).toString() == "Clip") //find the clips
                                clipsFound = true;
                    }

                    //if no clips then parking
                    if (clipsFound)
                        parentItem->setParentItem(parentItem->focusProxy());
                    else
                        parentItem->setParentItem(parkingFileGroup);
                }

                //clips
                if (QSettings().value("viewMode").toString() == "SpotView") //Clips: move to mediafile
                {
                    foreach (QGraphicsItem *clipItem, scene->items())
                    {
                        if (clipItem->focusProxy() == parentItem && clipItem->data(itemTypeIndex).toString() == "Base" && clipItem->data(mediaTypeIndex).toString() == "Clip") //find the clips
                        {
                            if (clipItem->data(excludedInFilter).toBool()) //clip filtered
                                clipItem->setParentItem(parkingFileGroup->focusProxy()); //parking timeline
                            else
                                clipItem->setParentItem(clipItem->focusProxy());
                        }
                    }
                }
                else //move to FileGroup timeline
                {
                    foreach (QGraphicsItem *clipItem, scene->items())
                    {
                        if (clipItem->focusProxy() == parentItem && clipItem->data(itemTypeIndex).toString() == "Base" && clipItem->data(mediaTypeIndex).toString() == "Clip") //find the clips
                        {
                            if (clipItem->data(excludedInFilter).toBool()) //clip filtered
                                clipItem->setParentItem(parkingFileGroup->focusProxy()); //parking timeline
                            else
                                clipItem->setParentItem(parentItem->focusProxy()->focusProxy());
                        }
                    }
                }

            }
        }

        //reparent the children
        foreach (QGraphicsItem *childItem, parentItem->childItems())
        {
            reParent(childItem, prefix + "  ");
        }
    }
}

void AGView::onSetView()
{
    //clips as childs of file
    //find the timeline of each folder
    //find the files of each folder
    //get the clips of the timeline and reparent to the files

//    foreach (QGraphicsItem *item, parkingItem->childItems())
//    {
//        qDebug()<<"setParkingBack"<<itemToString(item)<<itemToString(item->focusProxy());

//        item->setParentItem(item->focusProxy());
//    }

    arrangeItems(nullptr);
}

void AGView::onCreateClip()
{
    //selectedclip at selectedposition
    AGViewRectItem *selectedItem = nullptr;
    if (scene->selectedItems().count() == 1)
        selectedItem = (AGViewRectItem *)scene->selectedItems().first();

    if (selectedItem != nullptr)
    {
        //find current position
//        qDebug()<<"Find position of"<<itemToString(selectedItem);
        //check if it fits within current clips
        //add clip

        onAddItem("Media", "Clip", selectedItem->fileInfo, 3000, 2000, 5000);
        //selectedItem
        arrangeItems(nullptr);
    }

}


bool AGView::noFileOrClipDescendants(QGraphicsItem *parentItem)
{
    foreach (QGraphicsItem *childItem, parentItem->childItems())
    {
        if ((childItem->data(mediaTypeIndex).toString() == "Clip" || childItem->data(mediaTypeIndex).toString() == "MediaFile") && !childItem->data(itemTypeIndex).toString().contains("Sub"))
            return false;
        bool x = noFileOrClipDescendants(childItem);
        if (!x)
            return false;
    }
    return true;
}

void AGView::assignCreateDates()
{
    //foreach mediafile
    //check createdate

    QMap<QString, AGMediaFileRectItem *> orderMap;

    foreach (QGraphicsItem *item, scene->items())
    {
        if (item->data(mediaTypeIndex).toString() == "MediaFile" && item->data(itemTypeIndex).toString() == "Base")
        {
            AGMediaFileRectItem *mediaItem = (AGMediaFileRectItem *)item;

            orderMap[mediaItem->fileInfo.absolutePath() + "/" + mediaItem->fileInfo.completeBaseName().toLower()] = mediaItem;
        }
    }

    qreal zValueMediaFile = 0;

    QMapIterator<QString, AGMediaFileRectItem *> rowIterator(orderMap);
    while (rowIterator.hasNext()) //all files in reverse order
    {
        rowIterator.next();
        AGMediaFileRectItem *mediaItem = rowIterator.value();
//    foreach (AGMediaFileRectItem *mediaItem, orderMap) //alphabetically ordered! (instead of scene->items)
//    {
        QString createDateString = "";
        if (orderBy == "Date")
        {
            createDateString = mediaItem->data(createDateIndex).toString();

            //temporaty, will be replaced by new propertyload
            if (createDateString == "")
            {
                QVariant  *createDatePointer = new QVariant();
                emit getPropertyValue(mediaItem->fileInfo.absoluteFilePath(), "CreateDate", createDatePointer);
                createDateString = createDatePointer->toString();
                if (createDateString == "0000:00:00 00:00:00")
                    createDateString = "";

                if (createDateString != "")
                    mediaItem->setData(createDateIndex, createDateString);
            }
        }

        if (createDateString != "")
        {
            QDateTime createDate = QDateTime::fromString(createDateString, "yyyy:MM:dd HH:mm:ss");
            mediaItem->setZValue(createDate.toSecsSinceEpoch());
//            qDebug()<<"AssignCreateDates"<<mediaItem->fileInfo.absolutePath() + "/" + mediaItem->fileInfo.completeBaseName().toLower()<<createDateString<<mediaItem->zValue();
        }
        else
        {
            zValueMediaFile += 10000;
            mediaItem->setZValue(zValueMediaFile);
//            qDebug()<<"AssignCreateDates"<<mediaItem->fileInfo.absolutePath() + "/" + mediaItem->fileInfo.completeBaseName().toLower()<<zValueMediaFile<<mediaItem->zValue();
        }

        mediaItem->updateToolTip();

        foreach (QGraphicsItem *item, scene->items())
        {
            if (item->focusProxy() == mediaItem && item->data(itemTypeIndex).toString() == "Base" && item->data(mediaTypeIndex).toString() == "Clip") //find the clips
            {
                AGClipRectItem *clipItem = (AGClipRectItem *)item;

                clipItem->setZValue(mediaItem->zValue() + clipItem->clipIn / 1000.0); //add secs
                clipItem->updateToolTip();
            }
        }
    }
}

QRectF AGView::arrangeItems(QGraphicsItem *parentItem)
{
    if (parentItem == nullptr)
    {
        parentItem = rootItem;
        reParent(rootItem);
        assignCreateDates();
    }

    int spaceBetween = 10;

    QString parentMediaType = parentItem->data(mediaTypeIndex).toString();
    QString parentItemType = parentItem->data(itemTypeIndex).toString();

    int mediaDuration = parentItem->data(mediaDurationIndex).toInt();

    //set groups and folders without descendants invisible
    if ((parentMediaType.contains("Group") || parentMediaType == "Folder") && parentItemType == "Base") //|| parentMediaType == "TimelineGroup"
    {
        if (noFileOrClipDescendants(parentItem))
        {
            parentItem->setVisible(false);
            parentItem->setScale(0);
            return QRectF(0,0,0,0);
        }
        else
        {
            parentItem->setVisible(true);
            parentItem->setScale(1);
        }
    }

    //set MediaFiles without clips invisible, in case of timelineview
    else if (parentMediaType.contains("MediaFile") && parentItemType == "Base")
    {
        //set rect proportional to duration
        AGMediaFileRectItem *mediaFileItem = (AGMediaFileRectItem *)parentItem;

        if (mediaFileItem->parentRectItem->fileInfo.fileName() == "Export")
            mediaFileItem->setRect(QRectF(0, 0, qMax(mediaDuration * clipScaleFactor, mediaWidth), mediaWidth * 9.0 / 16.0));
        else
            mediaFileItem->setRect(QRectF(0, 0, qMax(mediaDuration * mediaFileScaleFactor, mediaWidth), mediaWidth * 9.0 / 16.0));
    }
    else if (parentMediaType.contains("Clip") && parentItemType == "Base")
    {
        //set rect proportional to duration
        QGraphicsRectItem *clipItem = (QGraphicsRectItem *)parentItem;

        clipItem->setRect(QRectF(0, 0, qMax(mediaDuration  * clipScaleFactor, 0.0), 100));
    }

    //set the children start position
    QPointF nextPos = QPointF(0, 0);// parentItem->boundingRect().height());// + spaceBetween
    QPointF lastClipNextPos = nextPos;

    bool alternator = false;
    bool firstChild = true;

//    if (parentItem == rootItem)
//        qDebug()<<"arrangeItems"<<(QThread::currentThread() == qApp->thread()?"Main":"Thread")<<parentMediaType<<parentItemType<<parentItem->data(fileNameIndex).toString();

    if (QThread::currentThread() != qApp->thread())
        qDebug()<<"arrangeItems thread problem"<<parentItemType<<parentItem<<parentMediaType<<thread();

    qreal heightUntilNow = 0;
    foreach (QGraphicsItem *childItem, parentItem->childItems())
    {
        QString childMediaType = childItem->data(mediaTypeIndex).toString();
        QString childItemType = childItem->data(itemTypeIndex).toString();

//        qDebug()<<"  child"<< childMediaType << childItemType <<childItem->data(fileNameIndex).toString() <<childItem->parentItem()->data(mediaTypeIndex).toString()<<childItem->parentItem()->data(itemTypeIndex).toString()<<childItem->parentItem()->data(fileNameIndex).toString();

        if (childItemType == "Base")
        {
            if ((childMediaType == "Folder" || childMediaType.contains("Group")))
            {
                if (childMediaType == "TimelineGroup")
                    nextPos = QPointF(parentItem->boundingRect().width() + spaceBetween, mediaWidth * 9 / 16 + spaceBetween); //parentItem->boundingRect().height()
                else //FileGroup or Folder
                    nextPos = QPointF(parentItem->boundingRect().width() + spaceBetween, nextPos.y()); //horizontally
            }
            else if (childMediaType == "MediaFile" && firstChild)
            {
                nextPos = QPointF(parentItem->boundingRect().width() + spaceBetween, 0); // width of folder or group
                firstChild = false;
            }
            else if (childMediaType == "Clip" && firstChild )
            {
                if (parentMediaType == "TimelineGroup")
                    nextPos = QPointF(parentItem->boundingRect().width() + spaceBetween, 100); //TimelineView mode: first clip next to timelineGroup (parent)
                else //mediaFile
                {
                    nextPos = QPointF(0, parentItem->boundingRect().height() + spaceBetween + 100); //SpotView mode: first clip below mediafile (parent)
                }
                firstChild = false;
                alternator = false;
            }
            else if (childMediaType == "Tag" && firstChild)
            {
                nextPos = QPointF(0, 0); //first tag next to clip (parent)
//                qDebug()<<"firsttag"<<itemToString(childItem)<<nextPos;

                firstChild = false;
            }

            if (childMediaType == "Clip") //first and notfirst: find the clipposition based on the position on the mediaitem timeline
            {
                if (parentMediaType == "MediaFile" && mediaDuration != 0)
                {
                    AGClipRectItem *clipItem = (AGClipRectItem *)childItem;
                    //position of clipIn on MediaItem
    //                qDebug()<<"position of clipIn on MediaItem"<<clipIn<<clipOut<<duration;
                    nextPos = QPointF(qMax(mediaDuration==0?0:(parentItem->boundingRect().width() * (clipItem->clipIn + clipItem->clipOut) / 2.0 / mediaDuration - childItem->boundingRect().width() / 2.0), nextPos.x()), nextPos.y());
                }
            }
        }
        else if (childItemType == "SubDurationLine")
        {
            nextPos = QPointF(0, 0);
        }
        else if (childItemType == "SubProgressLine")
        {
            QGraphicsRectItem *progressLineItem = (QGraphicsRectItem *)childItem;

//            qDebug()<<"SubProgressLine"<<itemToString(parentItem)<<itemToString(childItem)<<parentItem->boundingRect();

            nextPos = QPointF(0, parentItem->boundingRect().height() - progressLineItem->pen().width() / 2.0);
        }
        else if (childItemType.contains("SubName")) //other subs (subpic and subtxt and subplayer)
        {
            nextPos = QPointF(parentItem->boundingRect().height() * 0.1, parentItem->boundingRect().height() );
        }
        else if (childItemType.contains("SubLog")) //other subs (subpic and subtxt and subplayer)
        {
            nextPos = QPointF(parentItem->boundingRect().height() * 0.1, parentItem->boundingRect().height() - childItem->boundingRect().height() );
        }
        else if (childItemType.contains("SubWave")) //other subs (subpic and subtxt and subplayer)
        {
            nextPos = QPointF(0, 0);
        }
        else if (childItemType.contains("Sub")) //other subs (subpic and subtxt and subplayer)
        {
            nextPos = QPointF(parentItem->boundingRect().height() * 0.1, parentItem->boundingRect().height() * 0.1);
        }
        else //"poly"
            nextPos = QPointF(0,0);

        childItem->setPos(nextPos);

        QRectF rectChildren = arrangeItems(childItem);

//        qDebug()<<"AGView::arrangeItems"<<rectChildren<<parentItem->toolTip()<<childItem->toolTip();

        //add rect of children

        if (childItemType == "Base")
        {
            if (childMediaType == "Folder")
                nextPos = QPointF(nextPos.x(), nextPos.y() + rectChildren.height() + spaceBetween); //vertical
            else if (childMediaType.contains("Group"))
                nextPos = QPointF(nextPos.x(), nextPos.y() + rectChildren.height() + spaceBetween * 3); //vertical *3: more space between folders
            else if (childMediaType == "Clip" && parentMediaType == "TimelineGroup")
            {
                int transitionTimeFrames = QSettings().value("transitionTime").toInt();
                int frameRate = QSettings().value("frameRate").toInt();
                double transitionTimeMSec;
                if (frameRate != 0)
                    transitionTimeMSec = 1000.0 * transitionTimeFrames / frameRate;
                else
                    transitionTimeMSec = 0;

                nextPos = QPointF(nextPos.x() + rectChildren.width() - transitionTimeMSec  * clipScaleFactor, nextPos.y() + (alternator?-0:0)); //horizontal alternating //
            }
            else if (childMediaType == "MediaFile")
            {
                bool horizontal = true;

                AGViewRectItem *parentViewItem = (AGViewRectItem *)parentItem;

                if (parentViewItem->fileInfo.fileName() == "Export")
                    horizontal = false;
                else if (parentViewItem->fileInfo.fileName() == "Project")
                    horizontal = true;
                else if (QSettings().value("viewMode") == "SpotView" && QSettings().value("viewDirection") == "Down")
                    horizontal = false;
                else if (QSettings().value("viewMode") == "SpotView" && QSettings().value("viewDirection") == "Right")
                    horizontal = true;
                else if (QSettings().value("viewMode") == "SpotView" && QSettings().value("viewDirection") == "Return")
                    horizontal = true;
                else
                    horizontal = true;

                if (horizontal)
                    nextPos = QPointF(nextPos.x() + rectChildren.width() + spaceBetween, nextPos.y()); //horizontal
                else
                    nextPos = QPointF(nextPos.x(), nextPos.y() + rectChildren.height() + spaceBetween); //vertical

                if (QSettings().value("viewMode") == "SpotView" && QSettings().value("viewDirection") == "Return")
                {
                    AGMediaFileRectItem *mediaFile = (AGMediaFileRectItem *)childItem;
//                    qDebug()<<"CR"<<mediaFile->itemToString()<<nextPos.x() + rectChildren.width() + spaceBetween<<visibleArea;

                    if (nextPos.x() + rectChildren.width() + spaceBetween > this->width())
                    {
                        nextPos = QPointF(parentItem->boundingRect().width() + spaceBetween, nextPos.y() + heightUntilNow + spaceBetween); //carriage return
                        heightUntilNow = 0;
                    }
                }
            }
            else if (childMediaType == "Clip")
                nextPos = QPointF(nextPos.x() + rectChildren.width() + spaceBetween, nextPos.y()); //horizontal
            else if (childMediaType == "Tag")
            {
                nextPos = QPointF(nextPos.x(), nextPos.y() + childItem->boundingRect().height()); //vertical next tag below previous tag

                QGraphicsTextItem *tagItem = (QGraphicsTextItem *)childItem;
                QGraphicsRectItem *clipItem = (QGraphicsRectItem *)parentItem;
                tagItem->setTextWidth(clipItem->rect().width());
    //            qDebug()<<"tagItemtextWidth"<<tagItem->textWidth();


    //            qDebug()<<"nexttag"<<itemToString(childItem)<<nextPos;
            }

            if (childMediaType == "Clip")
            {
                lastClipNextPos = nextPos;
            }
        }
        alternator = !alternator;
        heightUntilNow = qMax(heightUntilNow, rectChildren.height());

    } //foreach childitem

    if (parentMediaType == "MediaFile" && parentItemType == "Base") //childs have been arranged now
    {
        AGViewRectItem *parentViewItem = (AGViewRectItem *)parentItem;

//        qDebug()<<"checkSpotView"<<itemToString(parentRectItem)<<viewMode<<parentRectItem->rect()<<nextPos;
        if (QSettings().value("viewMode").toString() == "SpotView") //set size dependent on clips
        {
            parentViewItem->setRect(QRectF(parentViewItem->rect().x(), parentViewItem->rect().y(), qMax(parentViewItem->rect().width(), lastClipNextPos.x()), parentViewItem->rect().height()));
            //if coming from timelineview,. the childrenboundingrect is still to big...
        }
        else //timelineView: set size independent from clips
        {
            parentViewItem->setRect(QRectF(parentViewItem->rect().x(), parentViewItem->rect().y(), qMax(parentViewItem->rect().width(), mediaWidth), parentViewItem->rect().height()));
        }

        //reallign all poly's and check if parentitem has clips
//        if (mediaDuration != 0)
        {
            foreach (QGraphicsItem *item, scene->items())
            {
                if (item->focusProxy() == parentItem && item->data(itemTypeIndex).toString() == "Base" && item->data(mediaTypeIndex).toString() == "Clip") //find the clips
                {
                    AGClipRectItem *clipItem = (AGClipRectItem *)item;
                    clipItem->drawPoly(); //must be after foreach childItem to get sizes right
                }
            }
        }
    }

    {
        //adjust the width of text, lines and waves
        QGraphicsVideoItem *playerItem = nullptr;
        QGraphicsRectItem *progressLineItem = nullptr;

        foreach (QGraphicsItem *childItem, parentItem->childItems())
        {
            if (childItem->data(itemTypeIndex).toString().contains("SubPlayer"))
            {
                playerItem = (QGraphicsVideoItem *)childItem;
            }
            else if (childItem->data(itemTypeIndex).toString() == "SubName")
            {
                QGraphicsTextItem *textItem = (QGraphicsTextItem *)childItem;
                textItem->setTextWidth(parentItem->boundingRect().width() * 0.9);
            }
            else if (childItem->data(itemTypeIndex).toString() == "SubLog")
            {
                QGraphicsTextItem *textItem = (QGraphicsTextItem *)childItem;
                textItem->setTextWidth(parentItem->boundingRect().width() * 0.9);
            }
            else if (childItem->data(itemTypeIndex) == "SubDurationLine")
            {
                qreal scaleFactor = mediaFileScaleFactor;
                AGViewRectItem *parentViewItem = (AGViewRectItem *)parentItem;
                QGraphicsRectItem *childViewItem = (QGraphicsRectItem *)childItem;

//                qDebug()<<"SubDurationLine"<<parentViewItem->itemToString();
                if (childItem->data(mediaTypeIndex) == "Clip" || childItem->data(mediaTypeIndex) == "Folder"  ||
                        (childItem->data(mediaTypeIndex) == "MediaFile" && parentViewItem->parentRectItem->fileInfo.fileName() == "Export"))
                    scaleFactor = clipScaleFactor;

                childViewItem->setRect(0,0, childViewItem->data(mediaDurationIndex).toInt()  * scaleFactor, 5);
            }
            else if (childItem->data(itemTypeIndex) == "SubProgressLine")
            {
                progressLineItem = (QGraphicsRectItem *)childItem;
            }
            else if (childItem->data(itemTypeIndex) == "SubWave")
            {
                QGraphicsPathItem *pathItem = (QGraphicsPathItem *)childItem;
                QGraphicsRectItem *mediaItem = (QGraphicsRectItem *)pathItem->parentItem();
                QPainterPath painterPath = pathItem->path();
                QPainterPath newPainterPath;
                newPainterPath.moveTo(0,0);
                for (int i=0; i < painterPath.elementCount(); i++)
                {
                    QPainterPath::Element element = painterPath.elementAt(i);
                    newPainterPath.lineTo(qreal(i) / painterPath.elementCount() * mediaItem->rect().width(), element.y);
                }
                pathItem->setPath(newPainterPath);
            }
        }

        //postprocess as both a player and a progressline needed to be found
        if (playerItem != nullptr && progressLineItem != nullptr)
        {
            QMediaPlayer *m_player = (QMediaPlayer *)playerItem->mediaObject();

            if (m_player != nullptr)
            {
//                progressLineItem->setLine(QLineF(progressLineItem->pen().width() / 2.0, 0, qMax(progressLineItem->parentItem()->boundingRect().width() * m_player->position() / m_player->duration() - progressLineItem->pen().width() / 2.0, progressLineItem->pen().width() * 1.5), 0));

                progressLineItem->setRect(QRectF(0,0, progressLineItem->parentItem()->boundingRect().width() * m_player->position() / m_player->duration(), 10));


                double minX = parentItem->boundingRect().height() * 0.1;
                double maxX = parentItem->boundingRect().width() - playerItem->boundingRect().width() - minX;
                playerItem->setPos(qMin(qMax(parentItem->boundingRect().width() * m_player->position() / m_player->duration() - playerItem->boundingRect().width() / 2.0, minX), maxX), playerItem->pos().y());
            }
        }
    }

    if (parentMediaType == "Clip")
    {
        return parentItem->boundingRect(); //not the poly
    }
    else //all other
    {
        return parentItem->boundingRect()|parentItem->childrenBoundingRect();
    }

    //    parentItem->setToolTip(parentItem->toolTip() + QString(" %1 %2 %3 %4").arg(QString::number(r.x()), QString::number(r.y()), QString::number(r.width()), QString::number(r.height()/63.75)));
}

//https://stackoverflow.com/questions/19113532/qgraphicsview-zooming-in-and-out-under-mouse-position-using-mouse-wheel
void AGView::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers() & Qt::ControlModifier)
    {
        // Do a wheel-based zoom about the cursor position
        double angle = event->angleDelta().y();
        double factor = qPow(1.0015, angle);

        auto targetViewportPos = event->pos();
        auto targetScenePos = mapToScene(event->pos());

        scale(factor, factor);
//        qDebug()<<"scale"<<factor;
        centerOn(targetScenePos);
        QPointF deltaViewportPos = targetViewportPos - QPointF(viewport()->width() / 2.0, viewport()->height() / 2.0);
        QPointF viewportCenter = mapFromScene(targetScenePos) - deltaViewportPos;
        centerOn(mapToScene(viewportCenter.toPoint()));
    }
    else
        QGraphicsView::wheelEvent(event); //scroll with wheel
}

void AGView::setZoom(int value)
{
    scale(value/100.0, value / 100.0);
}

//https://forum.qt.io/topic/82015/how-to-move-qgraphicsscene-by-dragging-with-mouse/3
void AGView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        rightMousePressed = true;
        _panStartX = event->x();
        _panStartY = event->y();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
    }
    else if (event->button() == Qt::RightButton)
    {
        onItemRightClicked(event->pos());
        event->accept();
    }
    else
        QGraphicsView::mousePressEvent(event); //scroll with wheel
}

void AGView::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        rightMousePressed = false;
        setCursor(Qt::ArrowCursor);
        event->accept();
//        return;
    }
//    event->ignore();
    else
        QGraphicsView::mouseReleaseEvent(event); //scroll with wheel
}

void AGView::mouseMoveEvent(QMouseEvent *event)
{
//    QMediaPlayer *m_player = qobject_cast<QMediaPlayer *>(sender());
//    qDebug()<<"AGView::mouseMoveEvent"<<event<<scene->itemAt(event->localPos(), QTransform());
    if (rightMousePressed)
    {
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - (event->x() - _panStartX));
        verticalScrollBar()->setValue(verticalScrollBar()->value() - (event->y() - _panStartY));
        _panStartX = event->x();
        _panStartY = event->y();
        event->accept();
        return;
    }
    else
        QGraphicsView::mouseMoveEvent(event); //scroll with wheel
//    event->ignore();

}

void AGView::onSearchTextChanged(QString text)
{
    searchText = text;

    if (searchText != "")
        filtering = true;

    if (rootItem == nullptr)
        return;

    foreach (QGraphicsItem *mediaItem, scene->items())
    {
        if (mediaItem->data(mediaTypeIndex).toString() == "MediaFile" && mediaItem->data(itemTypeIndex).toString() == "Base") // && item->data(itemTypeIndex).toString() == "Base"
        {
            filterItem(mediaItem);
        }
    }

    if (searchText == "")
        filtering = false;

    arrangeItems(nullptr);
}

void AGView::filterItem(QGraphicsItem *item)
{
    if (!filtering)
        return;

    AGMediaFileRectItem *mediaItem = (AGMediaFileRectItem *)item;

    bool foundMediaFile = false;
    if (searchText == "" || mediaItem->fileInfo.fileName().contains(searchText, Qt::CaseInsensitive))
        foundMediaFile = true;

    foreach (QGraphicsItem *clipItem, scene->items())
    {
        if (clipItem->focusProxy() == mediaItem && clipItem->data(itemTypeIndex).toString() == "Base" && clipItem->data(mediaTypeIndex).toString() == "Clip") //find the clips
        {
            bool foundTag = false;;
            foreach (QGraphicsItem *item, clipItem->childItems())
            {
                if (item->data(mediaTypeIndex) == "Tag")
                {
                    AGTagTextItem *tagItem = (AGTagTextItem *)item;
                    if (searchText == "" || tagItem->tagName.contains(searchText, Qt::CaseInsensitive))
                        foundTag = true;
                }
            }

            if (foundTag)
                foundMediaFile = true;

            clipItem->setData(excludedInFilter, !foundTag);
        }
    }

    mediaItem->setData(excludedInFilter, !foundMediaFile);
}

void AGView::setMediaScaleAndArrange(qreal scaleFactor)
{
    this->mediaFileScaleFactor = scaleFactor / 60000.0;

    if (rootItem == nullptr)
        return;

    arrangeItems(nullptr);
}

void AGView::setClipScaleAndArrange(qreal scaleFactor)
{
    this->clipScaleFactor = scaleFactor / 60000.0;

    if (rootItem == nullptr)
        return;

//    assignCreateDates();
    arrangeItems(nullptr);
}

void AGView::setOrderBy(QString orderBy)
{
    if (rootItem == nullptr)
        return;

    this->orderBy = orderBy;

//    assignCreateDates();
    arrangeItems(nullptr);
}
