#include "aglobal.h"
#include "agviewrectitem.h"
#include "agmediafilerectitem.h"
#include "agview.h"
#include "agfolderrectitem.h"
#include "agcliprectitem.h"
#include "agtagtextitem.h"
#include "mgrouprectitem.h"
#include "mtimelinegrouprectitem.h"

#include <QGraphicsItem>

#include <QDebug>
#include <QGraphicsVideoItem>
#include <QKeyEvent>
#include <QTimeLine>
#include <QScrollBar>
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
#include <QMessageBox>
#include <QTimer>
#include <QGraphicsProxyWidget>
#include <QSlider>
#include <QPushButton>

AGView::AGView(QWidget *parent)
    : QGraphicsView(parent)
{
    scene = new QGraphicsScene(this);
    setScene(scene);

    setAlignment(Qt::AlignTop|Qt::AlignLeft);


    connect(scene, &QGraphicsScene::selectionChanged, this, &AGView::onSelectionChanged);

//    QPixmap bgPix(":/images/karton.jpg");
//    setBackgroundBrush(bgPix);

}

AGView::~AGView()
{
}

void AGView::clearAll()
{
//    qDebug()<<"AGView::clearAll";
    stopAndDeletePlayers();

    scene->clear();

    scale(1,1);

    undoList.clear();
    undoIndex = -1;
    undoSavePoint = -1;

    horizontalScrollBar()->setValue( 0 );
    verticalScrollBar()->setValue( 0 );
}

void AGView::stopAndDeletePlayers(QFileInfo fileInfo)
{
    QUrl deletedPlayerUrl = QUrl();

    if (playInDialog)
    {
        if (dialogMediaPlayer != nullptr)
        {
            if (fileInfo.absoluteFilePath() == "" || dialogMediaPlayer->media().request().url().path().contains(fileInfo.absoluteFilePath()))
            {
                qDebug()<<"Delete dialogMediaPlayer"<<dialogMediaPlayer->media().request().url().path();

                deletedPlayerUrl = dialogMediaPlayer->media().request().url().path();
                delete dialogMediaPlayer;
                dialogMediaPlayer = nullptr;

                if (dialogVideoWidget != nullptr)
                {
                    qDebug()<<"Delete dialogVideoWidget"<<deletedPlayerUrl.toString();
                    delete dialogVideoWidget;
                    dialogVideoWidget = nullptr;
                }
                if (playerDialog != nullptr)
                    playerDialog->close();
            }
        }
    }
    //playInDialog first as it kills also the player reference of the mediaitem

    foreach (QGraphicsItem *item, scene->items())
    {
        if (item->data(mediaTypeIndex).toString() == "MediaFile" && item->data(itemTypeIndex).toString() == "Base")
        {
            AGMediaFileRectItem *mediaItem = (AGMediaFileRectItem *)item;

            if (fileInfo.absoluteFilePath() == "" || mediaItem->fileInfo == fileInfo)
            {
                if (mediaItem->m_player != nullptr)
                {
                    QGraphicsItem *parentItem;
                    if (AGlobal().videoExtensions.contains(fileInfo.suffix(), Qt::CaseInsensitive))
                        parentItem = mediaItem->pictureItem;
                    else
                        parentItem = mediaItem;

                    mediaItem->playButtonProxy->setParentItem(parentItem);
                    mediaItem->playButtonProxy->setScale(1.0 / parentItem->scale());
//                    mediaItem->playButtonProxy->setPos(QPointF(parentItem->boundingRect().width() * 0.5 - mediaItem->playButtonProxy->boundingRect().width() * 0.5, parentItem->boundingRect().height() * 0.5 - mediaItem->playButtonProxy->boundingRect().height() * 0.5));

                    QPushButton *playButton = (QPushButton *)mediaItem->playButtonProxy->widget();
                    playButton->setIcon(qApp->style()->standardIcon(QStyle::SP_MediaPlay));

                    if (!deletedPlayerUrl.toString().contains(mediaItem->fileInfo.absoluteFilePath())) //already deleted above
                    {
//                        qDebug()<<"Delete mediaFile->m_player"<<mediaItem->fileInfo.absoluteFilePath();
                        delete mediaItem->m_player;
                    }
                    mediaItem->m_player = nullptr;
                }

                if (mediaItem->playerItem != nullptr)
                {
//                    qDebug()<<"Delete mediaFile->playerItem"<<mediaItem->fileInfo.absoluteFilePath();
                    delete mediaItem->playerItem;
                    mediaItem->playerItem = nullptr;
                }

                if (mediaItem->progressSliderProxy != nullptr)
                {
                    delete mediaItem->progressSliderProxy;
                    mediaItem->progressSliderProxy = nullptr;
                }

                if (mediaItem->subLogItem != nullptr) //now used for counter
                {
                    mediaItem->lastOutputString = "";
                    mediaItem->setTextItem(QTime(), QTime());
                }
            }
        }
    }
//    arrangeItems(nullptr, "");
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

void AGView::onAddItem(bool changed, QString parentName, QString mediaType, QFileInfo fileInfo, int duration, int clipIn, int clipOut, QString tag)
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
    AGMediaFileRectItem *mediaItem = nullptr;
    //find the parentitem

    //loop over all base items
    foreach (QGraphicsItem *item, scene->items())
    {
        if (item->data(itemTypeIndex).toString() == "Base")
        {
            //fileInfo of Tag is in AGTagTextItem instead of AGViewRectItem
            QFileInfo itemFileInfo;
            if (item->data(mediaTypeIndex).toString() == "Tag")
                itemFileInfo = ((AGTagTextItem *)item)->fileInfo;
            else
                itemFileInfo = ((AGViewRectItem *)item)->fileInfo;

            //if clip added then find the corresponding mediafile
            if (mediaType == "Clip" && item->data(mediaTypeIndex).toString() == "MediaFile" && fileInfo.absolutePath() == itemFileInfo.absolutePath() && fileInfo.completeBaseName() == itemFileInfo.completeBaseName())
            {
//                qDebug()<<"Clip mediaItem"<<item->data(mediaTypeIndex)<<parentMediaType<<parentName<<fileInfo.fileName()<<itemFileInfo.fileName();
                mediaItem = (AGMediaFileRectItem *)item;
                if (fileInfo.fileName() != itemFileInfo.fileName())
                {
//                    qDebug()<<"Rename"<<mediaType<<fileName<<viewFileInfo.fileName();
                    fileInfo.setFile(itemFileInfo.absoluteFilePath());//adjust the fileName to the filename of the corresponding mediaFile
                }
            }
            if (mediaType == "Tag" && item->data(mediaTypeIndex).toString() == "Clip" && itemFileInfo.absolutePath() == fileInfo.absolutePath() && itemFileInfo.completeBaseName() == fileInfo.completeBaseName())
            {
                if (fileInfo.fileName() != itemFileInfo.fileName())
                {
                    fileInfo.setFile(itemFileInfo.absoluteFilePath());//adjust the fileName to the filename of the corresponding mediaFile
                }
            }

            AGClipRectItem *clipItem = (AGClipRectItem *)item;

//            qDebug()<<"AGView::addItem"<<itemFileInfo.absoluteFilePath()<<fileInfo.absoluteFilePath()<<item->data(mediaTypeIndex).toString() << parentMediaType;
            if (item->data(mediaTypeIndex).toString() == parentMediaType)
            {
                if (mediaType == "Folder" && itemFileInfo.absolutePath() + "/" + itemFileInfo.fileName() == fileInfo.absolutePath())
                    parentItem = item;
                else if (mediaType == "FileGroup" && itemFileInfo.absolutePath() + "/" + itemFileInfo.fileName() == fileInfo.absolutePath())
                    parentItem = item;
                else if (mediaType == "TimelineGroup" && itemFileInfo.absolutePath() == fileInfo.absolutePath() && itemFileInfo.fileName() == parentName)
                    parentItem = item;
                else if (mediaType == "MediaFile" && itemFileInfo.absolutePath() == fileInfo.absolutePath() && itemFileInfo.fileName() == parentName)
                    parentItem = item;
                else if (mediaType == "Clip" && itemFileInfo.absolutePath() == fileInfo.absolutePath() && itemFileInfo.completeBaseName() == fileInfo.completeBaseName()) //MediaFile (do not match extension as clips can be called with .srt)
                    parentItem = item;
                else if (mediaType == "Clip" && itemFileInfo.absolutePath() == fileInfo.absolutePath() && itemFileInfo.fileName() == parentName) //Timeline
                    parentItem = item;
                else if (mediaType == "Tag" && itemFileInfo.absolutePath() == fileInfo.absolutePath() && itemFileInfo.completeBaseName() == fileInfo.completeBaseName() && clipItem->clipIn ==  clipIn)
                    parentItem = item;
            }
        }
    }

//    if (parentItem == nullptr)
//        qDebug()<<"AGView::addItem"<<(QThread::currentThread() == qApp->thread()?"Main":"Thread")<<parentName<<mediaType<<fileInfo.absoluteFilePath()<<parentMediaType<<parentItem;

    if (QThread::currentThread() != qApp->thread())
        qDebug()<<"AGView::addItem thread problem"<<parentName<<mediaType<<fileInfo.absolutePath() + "/"<<fileInfo.fileName()<<parentMediaType<<parentItem;

    QGraphicsItem *childItem = nullptr;

    if (mediaType == "Folder")
    {
        AGFolderRectItem *folderItem = new AGFolderRectItem(parentItem, fileInfo);
        folderItem->setZValue(scene->items().count());
        folderItem->updateToolTip(); //only for zValue

        childItem = folderItem;
    }
    else if (mediaType == "TimelineGroup")
    {
        MTimelineGroupRectItem *rectItem = new MTimelineGroupRectItem(parentItem, fileInfo);
        rectItem->setZValue(scene->items().count());
        rectItem->updateToolTip(); //only for zValue

        childItem = rectItem;
    }
    else if (mediaType == "FileGroup")
    {
        MGroupRectItem *rectItem = new MGroupRectItem(parentItem, fileInfo);
        rectItem->setZValue(scene->items().count());
        rectItem->updateToolTip();


        childItem = rectItem;
    }
    else if (mediaType == "MediaFile")
    {
        AGMediaFileRectItem *mediaItem = new AGMediaFileRectItem(parentItem, fileInfo, duration);

//        mediaItem->setFlag(QGraphicsItem::ItemIsSelectable);

//        connect(mediaItem, &AGMediaFileRectItem::propertyCopy, ui->propertyTreeView, &APropertyTreeView::onPropertyCopy);
//        connect(mediaItem, &AGMediaFileRectItem::exportClips, this, &MainWindow::onExportClips);

        connect(mediaItem, &AGMediaFileRectItem::addItem, this, &AGView::onAddItem);
        connect(mediaItem, &AGMediaFileRectItem::addUndo, this, &AGView::onAddUndo);
        connect(mediaItem, &AGMediaFileRectItem::fileWatch, this, &AGView::fileWatch);

        mediaItem->updateToolTip();

//        connect(mediaItem, &AGClipRectangleItem::agItemChanged, this, &AGView::onClipItemChanged);
//        connect(mediaItem, &AGClipRectangleItem::agMouseReleased, this, &AGView::onClipMouseReleased);
        connect(mediaItem, &AGMediaFileRectItem::hoverPositionChanged, this, &AGView::onHoverPositionChanged);

        connect(mediaItem, &AGMediaFileRectItem::showInStatusBar, this, &AGView::showInStatusBar);

        childItem = mediaItem;
        //https://doc.qt.io/qt-5/qtdatavisualization-audiolevels-example.html

        arrangeItems(nullptr, mediaType);
    }
    else if (mediaType == "Clip")
    {
        if (mediaItem == nullptr) //workaround in case srt file found without a mediafile
            return;

        AGClipRectItem *clipItem = new AGClipRectItem(parentItem, mediaItem, fileInfo, duration, clipIn, clipOut);
//        qDebug()<<"Clip"<<fileInfo.fileName()<<clipIn<<clipItem->mediaItem->data(mediaTypeIndex)<<clipItem->mediaItem->data(itemTypeIndex);

        connect(clipItem, &AGClipRectItem::addItem, this, &AGView::onAddItem);
        connect(clipItem, &AGClipRectItem::addUndo, this, &AGView::onAddUndo);

//        connect(clipItem, &AGViewRectItem::agItemChanged, this, &AGView::onClipItemChanged);
//        connect(clipItem, &AGViewRectItem::agMouseReleased, this, &AGView::onClipMouseReleased);
        connect(clipItem, &AGClipRectItem::hoverPositionChanged, this, &AGView::onHoverPositionChanged);
        connect(clipItem, &AGClipRectItem::deleteItem, this, &AGView::onDeleteItem);

        childItem = clipItem;

        arrangeItems(nullptr, mediaType); //in case of new clip added later

        onAddUndo(changed, "Create", "Clip", clipItem);
    }
    else if (mediaType == "Tag")
    {
        if (parentItem == nullptr) //workaround in case srt file found without a mediafile
            return;

        AGTagTextItem *tagItem = new AGTagTextItem(parentItem, fileInfo, tag);
//        AGClipRectItem *clipItem = (AGClipRectItem *)parentItem;

        connect(tagItem, &AGTagTextItem::deleteItem, this, &AGView::onDeleteItem);
        connect(tagItem, &AGTagTextItem::hoverPositionChanged, this, &AGView::onHoverPositionChanged);

        arrangeItems(tagItem->clipItem->mediaItem, mediaType); //in case of new clip added later

        onAddUndo(changed, "Create", "Tag", tagItem);

        childItem = tagItem;
    }

//    childItem->setFlag(QGraphicsItem::ItemIsMovable, true);
    childItem->setFlag(QGraphicsItem::ItemIsSelectable, true);

    if (parentItem == nullptr)
    {
        scene->addItem(childItem);
        rootItem = childItem;
        rootItem->setSelected(true);
    }

    if (false)//if (mediaType == "Folder" || mediaType == "FileGroup" || mediaType == "MediaFile")
    {
        QGraphicsTextItem *subNameItem;
        subNameItem = new QGraphicsTextItem(childItem);
        if (QSettings().value("theme") == "Black")
            subNameItem->setDefaultTextColor(Qt::white);
        else
            subNameItem->setDefaultTextColor(Qt::black);
        subNameItem->setPlainText(fileInfo.fileName());

        subNameItem->setData(mediaTypeIndex, mediaType);
        subNameItem->setData(itemTypeIndex, "SubNameItem");

        subNameItem->setPos(childItem->boundingRect().height() * 0.1, childItem->boundingRect().height() ); //pos need to set here as arrangeitem not called here

        subNameItem->setTextWidth(childItem->boundingRect().width() * 0.8);
    }

    //position new item at the bottom (arrangeitem will put it right
//    childItem->setPos(QPointF(0, rootItem->boundingRect().height()));
}

void AGView::onAddUndo(bool changed, QString action, QString mediaType, QGraphicsItem *item, QString property, QString oldValue, QString newValue)
{
    QString outputString = "AGView::onAddUndo " + QString::number(changed) + " " + action + " " + mediaType;

    if (mediaType == "Tag")
    {
        AGTagTextItem *tagItem = (AGTagTextItem *)item;
        outputString += " " + tagItem->fileInfo.fileName() + " " + QString::number(tagItem->clipItem->clipIn) + " " + tagItem->tagName;
    }
    else if (mediaType == "Clip")
    {
        AGClipRectItem *clipItem = (AGClipRectItem *)item;
        outputString += " " + clipItem->fileInfo.fileName() + " " + QString::number(clipItem->clipIn);
    }

    if (property != "")
        outputString += " " + property + " = " + oldValue + "->" + newValue;

    UndoStruct undoStruct;
    undoStruct.action = action;
    undoStruct.mediaType = mediaType;
    undoStruct.item = item;
    undoStruct.property = property;
    undoStruct.oldValue = oldValue;
    undoStruct.newValue = newValue;
//    undoStruct.changed = changed;

    undoIndex++;
    undoList.insert(undoIndex, undoStruct);

    if (!changed)
        undoSavePoint = undoIndex;

//    undoList<<undoStruct;
//    undoIndex = undoList.count() - 1;

//    qDebug()<<__func__<<outputString;
} //onAddUndo

void AGView::undoOrRedo(QString undoOrRedo)
{
    if (undoOrRedo=="Redo")
        undoIndex++;

//    qDebug()<<"AGView::undoOrRedo"<<undoIndex<<undoList.count();

    if (undoIndex >= undoList.count() || undoIndex < 0)
    {
        if (undoOrRedo=="Redo")
            undoIndex--;
        return;
    }

    UndoStruct undoStruct = undoList.at(undoIndex);
//    qDebug()<<"AGView::undoOrRedo"<<undoOrRedo<<undoIndex<<undoSavePoint<<undoStruct.action<<undoStruct.mediaType<<undoStruct.item->data(fileNameIndex).toString()<<undoStruct.property<<undoStruct.oldValue<<undoStruct.newValue;

    if (undoStruct.action == "Create" || undoStruct.action == "Delete")
    {
        if ((undoStruct.action == "Create" && undoOrRedo == "Undo") || (undoStruct.action == "Delete" && undoOrRedo == "Redo")) //then delete
        {
            if (undoStruct.mediaType == "Clip")
            {
                AGClipRectItem *clipItem = (AGClipRectItem *)undoStruct.item;
                clipItem->mediaItem->clips.removeOne(clipItem);
                clipItem->mediaItem->groupItem->timelineGroupItem->clips.removeOne(clipItem);
            }
            else if (undoStruct.mediaType == "Tag")
            {
                AGTagTextItem *tagItem = (AGTagTextItem *)undoStruct.item;
                tagItem->clipItem->tags.removeOne(tagItem);
            }

            scene->removeItem(undoStruct.item);
        }
        else //add
        {
            scene->addItem(undoStruct.item);

            if (undoStruct.mediaType == "Clip")
            {
                AGClipRectItem *clipItem = (AGClipRectItem *)undoStruct.item;
                undoStruct.item->setParentItem(clipItem->mediaItem);
                clipItem->mediaItem->clips << clipItem;
            }
            else if (undoStruct.mediaType == "Tag")
            {
                AGTagTextItem *tagItem = (AGTagTextItem *)undoStruct.item;
                undoStruct.item->setParentItem(tagItem->clipItem);
                tagItem->clipItem->tags<<tagItem;
            }
        }
    }
    else if (undoStruct.action == "Update") //then update old
    {
        if (undoStruct.mediaType == "Clip")
        {
            AGClipRectItem *clipItem = (AGClipRectItem *)undoStruct.item;

            if (undoOrRedo == "Undo")
            {
                if (undoStruct.property == "clipIn")
                    clipItem->clipIn = undoStruct.oldValue.toInt();
                else
                    clipItem->clipOut = undoStruct.oldValue.toInt();
            }
            else
            {
                if (undoStruct.property == "clipIn")
                    clipItem->clipIn= undoStruct.newValue.toInt();
                else
                    clipItem->clipOut = undoStruct.newValue.toInt();
            }

            clipItem->duration = clipItem->clipOut - clipItem->clipIn;
            clipItem->updateToolTip();
        }
        else if (undoStruct.mediaType == "Tag")
        {
            AGTagTextItem *tagItem = (AGTagTextItem *)undoStruct.item;

            if (undoStruct.property == "tagName")
            {
                if (undoOrRedo == "Undo")
                    tagItem->tagName = undoStruct.oldValue;
                else
                    tagItem->tagName = undoStruct.newValue;

                tagItem->setHtml(tagItem->tagName);
            }
        }
    }

    if (undoOrRedo=="Undo")
        undoIndex--;

    arrangeItems(nullptr, __func__);

} //undoOrRedo

void AGView::updateChangedColors(bool debugOn)
{
    for (int i=0; i< undoList.count();i++)
    {
        bool toBeSaved = (i > qMin(undoSavePoint, undoIndex) && i <= qMax(undoSavePoint, undoIndex));
        UndoStruct undoStruct = undoList[i];
        if (undoStruct.mediaType == "Clip")
        {
            AGClipRectItem *clipItem = (AGClipRectItem *)undoStruct.item;

            if (debugOn)
                qDebug()<<__func__<<i<<undoIndex<<undoSavePoint<<toBeSaved<<undoStruct.action<<undoStruct.mediaType<<clipItem->fileInfo.fileName()<<clipItem->clipIn<<undoStruct.property<<undoStruct.oldValue<<undoStruct.newValue;
            if (toBeSaved)
            {
                QColor oldColor = clipItem->mediaItem->brush().color();
                if (undoStruct.action == "Create")
                    clipItem->setBrush(QColor(oldColor.red(), oldColor.green(), oldColor.blue(), 200));
                else
                    clipItem->setBrush(QColor(oldColor.red(), oldColor.green(), oldColor.blue(), 200));

                clipItem->mediaItem->setBrush(QColor(oldColor.red(), oldColor.green(), oldColor.blue(), 200));
            }
        }
        else if (undoStruct.mediaType == "Tag")
        {
            AGTagTextItem *tagItem = (AGTagTextItem *)undoStruct.item;
            if (debugOn)
                qDebug()<<__func__<<i<<undoIndex<<undoSavePoint<<toBeSaved<<undoStruct.action<<undoStruct.mediaType<<tagItem->fileInfo.fileName()<<tagItem->clipItem->clipIn<<tagItem->tagName<<undoStruct.property<<undoStruct.oldValue<<undoStruct.newValue;

            if (toBeSaved)
            {
//                tagItem->setHtml(QString("<div color: lightblue;>") + tagItem->tagName + "*</div>");
                tagItem->setHtml(QString("<div style='background-color: rgba(42, 130, 218, 0.5);'>") + tagItem->tagName + "</div>");//blue-ish #2a82da  style='background-color: rgba(42, 130, 218, 0.5);'

                QColor oldColor = tagItem->clipItem->mediaItem->brush().color();
                tagItem->clipItem->setBrush(QColor(oldColor.red(), oldColor.green(), oldColor.blue(), 200));
                tagItem->clipItem->mediaItem->setBrush(QColor(oldColor.red(), oldColor.green(), oldColor.blue(), 200));
            }
            else
                tagItem->setHtml(tagItem->tagName);
        }
        else
        {
            if (debugOn)
                qDebug()<<__func__<<i<<undoIndex<<undoSavePoint<<toBeSaved<<undoStruct.action<<undoStruct.mediaType<<"undoStruct.item->data(fileNameIndex).toString()"<<undoStruct.property<<undoStruct.oldValue<<undoStruct.newValue;
        }
    }
}

void AGView::saveModels()
{
//    qDebug()<<"AGView::saveModels"<<undoSavePoint << undoIndex<<undoList.count();
    //find mediaClips with changed items
    QList<AGMediaFileRectItem *> mediaItems;

    for (int indexOf = 0; indexOf < undoList.count();indexOf++)
    {
        bool toBeSaved = (indexOf > qMin(undoSavePoint, undoIndex) && indexOf <= qMax(undoSavePoint, undoIndex));

        if (toBeSaved)
        {
            UndoStruct undoStruct = undoList[indexOf];

//            qDebug()<<"AGView::saveModels"<<indexOf<<undoIndex<<undoSavePoint<<toBeSaved<<undoStruct.action<<undoStruct.mediaType<<undoStruct.item->data(fileNameIndex).toString()<<undoStruct.property<<undoStruct.oldValue<<undoStruct.newValue;

            AGMediaFileRectItem *mediaItem = nullptr;
            if (undoStruct.mediaType == "Clip")
            {
                AGClipRectItem *clipItem = (AGClipRectItem *)undoStruct.item;
                mediaItem = clipItem->mediaItem;
            }
            else if (undoStruct.mediaType == "Tag")
            {
                AGTagTextItem *tagItem = (AGTagTextItem *)undoStruct.item;
                mediaItem = tagItem->clipItem->mediaItem;
            }
            else
                qDebug()<<"AGView::saveModels unsupported mediatype"<<undoStruct.mediaType;

            if (mediaItem != nullptr && !mediaItems.contains(mediaItem))
                mediaItems <<mediaItem;

//            undoStruct.changed = false;

//            undoList[indexOf] = undoStruct;
        }
    }
    undoSavePoint = undoIndex;

    //    showUndoList(); //causes crash if called in saveModel
    //save the srt files of these mediafiles

    foreach (AGMediaFileRectItem *mediaItem, mediaItems)
        this->saveModel(mediaItem);

    //update the changed value in undo.

    arrangeItems(nullptr, __func__);

//    qDebug()<<"saveModels done";

}

void AGView::saveModel(AGMediaFileRectItem *mediaItem)
{
    QString srtFileName = mediaItem->fileInfo.completeBaseName() + ".srt";

//    qDebug()<<"AGView::saveModel"<<mediaItem->itemToString()<<mediaItem->clips.count();

    if (mediaItem->clips.count() > 0) //clips exists with changes
    {
//        qDebug()<<"AClipsTableView::saveModel"<<clipsItemModel->rowCount()<<fileName<<changeCount<<inMap.count();

        emit fileWatch(mediaItem->fileInfo.absolutePath() + "/" + srtFileName, false);

        int clipPerFileCounter = 1;

        QFile file;
        file.setFileName(mediaItem->fileInfo.absolutePath() + "/" + srtFileName);
        file.open(QIODevice::WriteOnly);

        QStringList streamList;
        foreach (AGClipRectItem *clipItem, mediaItem->clips)
        {
            QStringList tags;
            int stars = -1;
            bool alike = false;
            foreach (AGTagTextItem *tagItem, clipItem->tags)
            {
                if (tagItem->tagName.contains("*"))
                    stars = tagItem->tagName.length();
                else if (tagItem->tagName.contains("✔"))
                    alike = true;
                else
                    tags << tagItem->tagName;
            }

            QString srtContentString = "";
            srtContentString += "<o>" + QString::number(clipPerFileCounter) + "</o>";
            if (stars != -1)
                srtContentString += "<r>" + QString::number(stars) + "</r>";
            if (alike)
                srtContentString += "<a>true</a>";
//                srtContentString += "<h>" +  "</h>";
            if (tags.count() > 0)
                srtContentString += "<t>" + tags.join(";") + "</t>";

            streamList << QString::number(clipPerFileCounter);
            streamList << QTime::fromMSecsSinceStartOfDay(clipItem->clipIn).toString("HH:mm:ss.zzz") + " --> " + QTime::fromMSecsSinceStartOfDay(clipItem->clipOut).toString("HH:mm:ss.zzz");
            streamList << srtContentString;
            streamList << "";
            clipPerFileCounter++;

        } //foreach clip

        file.write(streamList.join("\n").toUtf8());
        file.close();

//        qDebug()<<"saving";

        QTimer::singleShot(500, this, [=]()->void //first constructor needs to finish
        {
            emit fileWatch(mediaItem->fileInfo.absolutePath() + "/" + srtFileName, true, false);
        });

//        qDebug()<<"save done";

    }
    else
    {
        QFile file;
        file.setFileName(mediaItem->fileInfo.absolutePath() + "/" + srtFileName);
        if (file.exists())
           file.remove();
    }
}

void AGView::onDeleteItem(bool changed, QString mediaType, QFileInfo fileInfo, int clipIn, QString tagName)
{
//    qDebug()<<"AGView::onDeleteItem"<<(QThread::currentThread() == qApp->thread()?"Main":"Thread")<<fileInfo.absoluteFilePath()<<mediaType;

    //if (mediaType is clip then find the original extension!) //return value?

    if (QThread::currentThread() != qApp->thread())
        qDebug()<<"AGMediaFileRectItem::onDeleteItem thread problem"<<fileInfo.fileName();

    foreach (QGraphicsItem *item, scene->items())
    {
        if (item->data(itemTypeIndex) == "Base" && item->data(mediaTypeIndex) == mediaType)
        {
            bool matchOnFile = false;
            if (mediaType == "Tag") //ignore suffix .srt
            {
                AGTagTextItem *tagItem = (AGTagTextItem *)item;
                matchOnFile = tagItem->fileInfo.absolutePath() == fileInfo.absolutePath() && tagItem->fileInfo.completeBaseName() == fileInfo.completeBaseName();

                if (clipIn != -1 && tagName != "")
                {
                    matchOnFile = matchOnFile && tagItem->clipItem->clipIn == clipIn && tagName == tagItem->tagName;

                    if (matchOnFile)
                    {
                        onAddUndo(changed, "Delete", "Tag", tagItem);
                        /*bool removeSuccess = */tagItem->clipItem->tags.removeOne(tagItem);
//                        qDebug()<<"remove tagItem" <<fileInfo.fileName()<<clipIn<<tagName<<removeSuccess;
                    }
                }
            }
            else if (mediaType == "Clip") //ignore suffix .srt
            {
                AGClipRectItem *clipItem = (AGClipRectItem *)item;
                matchOnFile = clipItem->fileInfo.absolutePath() == fileInfo.absolutePath() && clipItem->fileInfo.completeBaseName() == fileInfo.completeBaseName();

//                qDebug()<<"delete clip"<<matchOnFile<<clipItem->fileInfo.absolutePath() << fileInfo.absolutePath() << clipItem->fileInfo.completeBaseName() << fileInfo.completeBaseName()<<clipIn;

                if (clipIn != -1) //to delete all clips
                {
                    matchOnFile = matchOnFile && clipItem->clipIn == clipIn;
                }

                if (matchOnFile)
                {
                    onAddUndo(changed, "Delete", "Clip", clipItem, "Including tags(tbd)", QString::number(clipItem->tags.count()));
                    /*bool removeSuccess = */clipItem->mediaItem->clips.removeOne(clipItem);
                    clipItem->mediaItem->groupItem->timelineGroupItem->clips.removeOne(clipItem);
//                    qDebug()<<"remove clipItem" <<fileInfo.fileName()<<clipIn << removeSuccess;
                }
            }
            else if (mediaType == "MediaFile")
            {
                AGMediaFileRectItem *mediaFile = (AGMediaFileRectItem *)item;
                matchOnFile = mediaFile->fileInfo.absolutePath() == fileInfo.absolutePath() && mediaFile->fileInfo.fileName() == fileInfo.fileName();

                if (matchOnFile)
                {
                    mediaFile->groupItem->mediaFiles.removeOne(mediaFile);
//                    qDebug()<<"remove mediaFile" <<fileInfo.fileName();
                }
            }

            if (matchOnFile)
            {
                if (mediaType == "MediaFile")
                    stopAndDeletePlayers(fileInfo);

//                qDebug()<<"  Item"<<item->data(fileNameIndex).toString();
                scene->removeItem(item);
//                delete item; //do not delete items as undo can bring them back

                arrangeItems(nullptr, __func__);
            }
        }
    }
}

void AGView::setThemeColors(QColor color)
{
    foreach (QGraphicsItem *item, scene->items())
    {
        if (item->data(itemTypeIndex).toString() == "SubNameItem" || item->data(itemTypeIndex).toString() == "SubLogItem" || item->data(mediaTypeIndex).toString() == "Tag")
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

        folderItem->onItemRightClicked(pos);
    }
    else if (item->data(mediaTypeIndex) == "FileGroup")
    {
        MGroupRectItem *groupItem = (MGroupRectItem *)item;

        groupItem->onItemRightClicked(pos);
    }
    else if (item->data(mediaTypeIndex) == "MediaFile")
    {
        AGMediaFileRectItem *mediaFileItem = (AGMediaFileRectItem *)item;

        mediaFileItem->onItemRightClicked(pos);
    }
    else if (item->data(mediaTypeIndex) == "Clip")
    {
        AGClipRectItem *clipRectItem = (AGClipRectItem *)item;

        clipRectItem->onItemRightClicked(pos);
    }
    else if (item->data(mediaTypeIndex) == "Tag")
    {
        AGTagTextItem *tagItem = (AGTagTextItem *)item;

        tagItem->onItemRightClicked(pos);
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
        stopAndDeletePlayers(); //delete progresslines
        playInDialog = false;
    }

    if (!playInDialog && checked) //from initem to dialog player
    {
        stopAndDeletePlayers();
        playInDialog = true;
    }
}

void AGView::onHoverPositionChanged(QGraphicsRectItem *rectItem, int progress)
{
    if (rectItem->data(itemTypeIndex) != "Base")
        return;

    AGMediaFileRectItem *mediaItem = nullptr;
    if (rectItem->data(mediaTypeIndex).toString() == "Clip")
    {
        AGClipRectItem *clipItem = (AGClipRectItem *)rectItem;
        mediaItem = clipItem->mediaItem;

        //if selection is already this clip or a clip of this mediafile do nothing
    }
    else if (rectItem->data(mediaTypeIndex).toString() == "MediaFile")
    {
        mediaItem = (AGMediaFileRectItem *)rectItem;

        //if selection is already this mediafile or a clip of this mediafile do nothing
        QGraphicsItem *firstItem = scene->selectedItems().first();

        AGMediaFileRectItem *firstMediaItem = nullptr;
        if (firstItem->data(mediaTypeIndex) == "Clip")
        {
            AGClipRectItem *clipItem = (AGClipRectItem *)firstItem;
            firstMediaItem = clipItem->mediaItem;
        }
        if (firstItem->data(mediaTypeIndex) == "MediaItem")
        {
            firstMediaItem = (AGMediaFileRectItem *)firstItem;
        }

        if (scene->selectedItems().count() == 0 || firstMediaItem != mediaItem)
        {
            scene->clearSelection();
            mediaItem->setSelected(true);
        }
    }

//    qDebug()<<"AGView::onHoverPositionChanged"<<mediaItem->itemToString()<<progress;

    int duration = 0;
    if (!playInDialog)
    {
        if (mediaItem->m_player == nullptr)
        {
            mediaItem->processAction("actionPlay_Pause");
        }
        else
        {
            mediaItem->m_player->setPosition(progress);
            duration = mediaItem->m_player->duration();
        }
    }
    else
    {
        if (dialogMediaPlayer == nullptr)
        {
            mediaItem->processAction("actionPlay_Pause");
        }
        else
        {
            dialogMediaPlayer->setPosition(progress);
            duration = dialogMediaPlayer->duration();
        }
    }
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

//        arrangeItems(mediaItem->parentItem(), __func__); //arrange the folder / foldergroup
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
            AGMediaFileRectItem *parentMediaItem = (AGMediaFileRectItem *)parentItem;
            filterItem(parentItem);

//            QGraphicsItem *parkingFileGroup = parentItem->focusProxy()->parentItem()->focusProxy();
            MGroupRectItem *parkingFileGroup = parentMediaItem->groupItem->folderItem->parkingGroupItem;
            if (parentItem->data(excludedInFilter).toBool()) //filtered
            {
                //MediaFile: move to parking
                parentItem->setParentItem(parkingFileGroup);

                //clips
                if (QSettings().value("viewMode").toString() == "SpotView") //Clips: move to mediafile
                {
                    foreach (AGClipRectItem *clipItem, parentMediaItem->clips)
                    {
                        clipItem->setParentItem(clipItem->mediaItem);
                    }
                }
                else //move to parking timeline
                {
                    foreach (AGClipRectItem *clipItem, parentMediaItem->clips)
                    {
                        clipItem->setParentItem(parkingFileGroup->timelineGroupItem);
                    }
                }
            }
            else //not filtered
            {
                if (QSettings().value("viewMode").toString() == "SpotView")
                    parentMediaItem->setParentItem(parentMediaItem->groupItem);
                else //TimelineView
                {
//                    bool clipsFound = false;
//                    foreach (AGClipRectItem *clipItem, parentMediaItem->clips)
//                    {
//                        clipsFound = true;
//                    }

                    //if no clips then parking
                    if (parentMediaItem->clips.count() > 0)
                        parentMediaItem->setParentItem(parentMediaItem->groupItem);
                    else
                        parentMediaItem->setParentItem(parkingFileGroup);
                }

                //clips
                if (QSettings().value("viewMode").toString() == "SpotView") //Clips: move to mediafile
                {
                    foreach (AGClipRectItem *clipItem, parentMediaItem->clips)
                    {
                        if (clipItem->data(excludedInFilter).toBool()) //clip filtered
                            clipItem->setParentItem(parkingFileGroup->timelineGroupItem); //parking timeline
                        else
                            clipItem->setParentItem(clipItem->mediaItem);
                    }
                }
                else //move to FileGroup timeline
                {
                    foreach (AGClipRectItem *clipItem, parentMediaItem->clips)
                    {
                        if (clipItem->data(excludedInFilter).toBool()) //clip filtered
                            clipItem->setParentItem(parkingFileGroup->timelineGroupItem); //parking timeline
                        else
                            clipItem->setParentItem(parentMediaItem->groupItem->timelineGroupItem);
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

    arrangeItems(nullptr, __func__);

    if (QSettings().value("viewMode") == "TimelineView") //otherwise artifacts, but very strange!
        arrangeItems(nullptr, __func__);

    scale(1.000001, 1.000001); //workaround to remove artifacts (poly's) (causes hoverMoveEvents??? so do not use in arrangeItems!)
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
//        qDebug()<<"AGView::AssignCreateDates"<<mediaItem->fileInfo.fileName()<<orderBy<<mediaItem->data(createDateIndex).toString();

        QString createDateString = "";
        if (orderBy == "Date")
        {
            createDateString = mediaItem->data(createDateIndex).toString();
//            qDebug()<<"createDateString1"<<createDateString;

            //temporary, will be replaced by new propertyload
            if (createDateString == "")
            {
                createDateString = mediaItem->exiftoolPropertyMap["CreateDate"].value;
//                if (createDateString == "0000:00:00 00:00:00")
//                    createDateString = "";

                if (createDateString != "")
                {
//                    qDebug()<<"AGView::assignCreateDates"<<mediaItem->fileInfo.fileName()<<createDateString;
                    mediaItem->setData(createDateIndex, createDateString);
                }
            }
        } //order by date

        if (createDateString != "") //assigned by date
        {
            QDateTime createDate = QDateTime::fromString(createDateString.left(19), "yyyy:MM:dd HH:mm:ss");
            mediaItem->setZValue(createDate.toSecsSinceEpoch());
//            qDebug()<<"AssignCreateDates"<<mediaItem->fileInfo.absolutePath() + "/" + mediaItem->fileInfo.completeBaseName().toLower()<<createDateString<<createDate<<mediaItem->zValue();
        }
        else //order by name
        {
            zValueMediaFile += 10000;
            mediaItem->setZValue(zValueMediaFile);
//            qDebug()<<"AssignCreateDates"<<mediaItem->fileInfo.absolutePath() + "/" + mediaItem->fileInfo.completeBaseName().toLower()<<zValueMediaFile<<mediaItem->zValue();
        }

        mediaItem->updateToolTip();

        foreach (AGClipRectItem *clipItem, mediaItem->clips)
        {
            clipItem->setZValue(mediaItem->zValue() + clipItem->clipIn / 1000.0); //add secs
            clipItem->updateToolTip();
        }
    }
}

QRectF AGView::arrangeItems(QGraphicsItem *parentItem, QString caller)
{
    //part 1: initialisation: reParent, assignCreateDates, visibility
    if (parentItem == nullptr)
    {
        parentItem = rootItem;
        reParent(rootItem);
        assignCreateDates();
    }

    int spaceBetween = 10;

    QString parentMediaType = parentItem->data(mediaTypeIndex).toString();
    QString parentItemType = parentItem->data(itemTypeIndex).toString();
    AGMediaFileRectItem *parentMediaItem = nullptr;
    qreal aspectRatio = 9.0 / 16.0;

    //set groups and folders without descendants invisible
    if ((parentMediaType.contains("Group") || parentMediaType == "Folder") && parentItemType == "Base") //|| parentMediaType == "TimelineGroup"
    {
        if (noFileOrClipDescendants(parentItem) && parentItem != rootItem)
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
        parentMediaItem = (AGMediaFileRectItem *)parentItem;

        if (parentMediaItem->ffmpegPropertyMap["Width"].value != "")
        {
            aspectRatio = parentMediaItem->ffmpegPropertyMap["Height"].value.toDouble() / parentMediaItem->ffmpegPropertyMap["Width"].value.toDouble();
//            qDebug()<<__func__<<"aspectRatio"<<aspectRatio << parentMediaItem->ffmpegMetaValueMap["Height"].value.toDouble() << parentMediaItem->ffmpegMetaValueMap["Width"].value.toDouble();
        }

        //set rect proportional to duration
        if (parentMediaItem->groupItem->fileInfo.fileName() == "Export")
            parentMediaItem->setRect(QRectF(0, 0, qMax(parentMediaItem->duration * clipScaleFactor, mediaHeight / aspectRatio), mediaHeight));
        else
            parentMediaItem->setRect(QRectF(0, 0, qMax(parentMediaItem->duration * mediaFileScaleFactor, mediaHeight / aspectRatio), mediaHeight));
    }
    else if (parentMediaType.contains("Clip") && parentItemType == "Base")
    {
        //set rect proportional to duration
        AGClipRectItem *clipItem = (AGClipRectItem *)parentItem;

        qreal speed = 1;
        foreach (AGTagTextItem *tagItem, clipItem->tags)
        {
            if (tagItem->tagName.contains("speed", Qt::CaseInsensitive))
            {
                qreal foundSpeed = tagItem->tagName.mid(5).toDouble();
                if (foundSpeed != 0)
                    speed = foundSpeed;
            }
        }

        clipItem->duration = (clipItem->clipOut - clipItem->clipIn) / speed;

        clipItem->setRect(QRectF(0, 0, qMax(clipItem->duration * clipScaleFactor, 0.0), 100));
//        qDebug()<<__func__<<"clip setrect"<<clipItem->fileInfo.fileName()<<clipItem->clipIn<<clipItem->rect();
    }

    //set the children start position
    QPointF nextPos = QPointF(0, 0);// parentItem->boundingRect().height());// + spaceBetween
    QPointF lastClipNextPos = nextPos;

    bool alternator = false;
    bool firstChild = true;

//    if (parentItem == rootItem)
//    if (caller != "")
//        qDebug()<<"arrangeItems"<<caller<<(QThread::currentThread() == qApp->thread()?"Main":"Thread")<<parentMediaType<<parentItemType<<"parentItem->data(fileNameIndex).toString()";

    QGraphicsProxyWidget* pRect = dynamic_cast<QGraphicsProxyWidget*>(parentItem);

    if (pRect != nullptr)
    {
//        qDebug()<<__func__<<pRect->data(mediaTypeIndex)<<pRect->data(itemTypeIndex);
    }

    if (QThread::currentThread() != qApp->thread())
        qDebug()<<"arrangeItems thread problem"<<parentItemType<<parentItem<<parentMediaType<<thread();

    //part 2: nextpos and arrange each child recursivelt

    qreal heightUntilNow = 0;
    foreach (QGraphicsItem *childItem, parentItem->childItems())
    {
        QString childMediaType = childItem->data(mediaTypeIndex).toString();
        QString childItemType = childItem->data(itemTypeIndex).toString();

        //part 2.2: recursive
        QRectF rectChildren = arrangeItems(childItem, "");

//        qDebug()<<"  child"<< childMediaType << childItemType <<childItem->data(fileNameIndex).toString() <<childItem->parentItem()->data(mediaTypeIndex).toString()<<childItem->parentItem()->data(itemTypeIndex).toString()<<childItem->parentItem()->data(fileNameIndex).toString()<<nextPos;

        //part 2.1: calculating nextPos for current child
        if (childItemType == "Base")
        {
            if (childMediaType == "TimelineGroup")
                nextPos = QPointF(parentItem->boundingRect().width() + spaceBetween, mediaHeight + spaceBetween); //parentItem->boundingRect().height()
            else if (childMediaType == "FileGroup")
            {
                MGroupRectItem *groupItem = (MGroupRectItem *)childItem;
                groupItem->setTextItem(QTime(), QTime());
                nextPos = QPointF(parentItem->boundingRect().width() + spaceBetween, nextPos.y()); //horizontally
            }
            else if (childMediaType == "Folder")
            {
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
//                AGClipRectItem *clipItem = (AGClipRectItem *)childItem;
//                qDebug()<<__func__<<"clip nextPos first"<<clipItem->fileInfo.fileName()<<clipItem->clipIn<<nextPos;

                firstChild = false;
                alternator = false;
            }
            else if (childMediaType == "Tag" && firstChild)
            {
                nextPos = QPointF(0, 0); //first tag next to clip (parent)
//                qDebug()<<"firsttag"<<itemToString(childItem)<<nextPos;

                firstChild = false;
            }

            if (childMediaType == "MediaFile") //first and notfirst
            {
                AGMediaFileRectItem *mediaItem = (AGMediaFileRectItem *)childItem;

                QColor oldColor = mediaItem->brush().color();
                mediaItem->setBrush(QColor(oldColor.red(), oldColor.green(), oldColor.blue(), 60));

                AGViewRectItem *parentViewRect = (AGViewRectItem *)parentItem;
                if ((QSettings().value("viewMode") == "SpotView" && QSettings().value("viewDirection") == "Return") || (parentViewRect->fileInfo.fileName() == "Export") || (parentViewRect->fileInfo.fileName() == "Project") || (parentViewRect->fileInfo.fileName() == "Parking"))
                {
    //                QRect viewport_rect(0, 0, viewport()->width(), viewport()->height());
    //                QRectF visible_scene_rect = mapToScene(viewport_rect).boundingRect();

                    //if not leftest and wider then viewport
                    if (nextPos.x() > parentItem->boundingRect().width() + spaceBetween && nextPos.x() + rectChildren.width() + spaceBetween > viewport()->width() - 150)
                    {
    //                        qDebug()<<"CR"<<childItem->data(fileNameIndex).toString()<<nextPos.x() + rectChildren.width() + spaceBetween<<this->width()<<viewport()->width();
                        //        qDebug()<<"heightUntilNow"<<childItemType<<childMediaType<<childItem->data(folderNameIndex).toString()<<childItem->data(fileNameIndex).toString()<<heightUntilNow<<rectChildren.height();

                        nextPos = QPointF(parentItem->boundingRect().width() + spaceBetween, nextPos.y() + heightUntilNow + spaceBetween); //carriage return
                        heightUntilNow = rectChildren.height();
                    }
                    else
                        heightUntilNow = qMax(heightUntilNow, rectChildren.height());
                }
            }
            else if (childMediaType == "Clip") //first and notfirst: find the clipposition based on the position on the mediaitem timeline
            {
                AGClipRectItem *clipItem = (AGClipRectItem *)childItem;
                QColor oldColor = clipItem->brush().color();
                clipItem->setBrush(QColor(oldColor.red(), oldColor.green(), oldColor.blue(), 125));

//                AGClipRectItem *clipItem = (AGClipRectItem *)childItem;
                if (parentMediaType == "MediaFile" && clipItem->mediaItem->duration != 0) //not timeline
                {
                    //position of clipIn on MediaItem
    //                qDebug()<<"position of clipIn on MediaItem"<<clipItem->clipIn<<clipItem->clipOut<<clipItem->duration<<parentItem->boundingRect().width()<<childItem->boundingRect().width()<<nextPos.x();

                    //childItem->boundingRect().width() only known after setRect in recursive call of arrangeItems
                    nextPos = QPointF(qMax(clipItem->mediaItem->duration==0?0:(parentItem->boundingRect().width() * (clipItem->clipIn + clipItem->clipOut) / 2.0 / clipItem->mediaItem->duration - childItem->boundingRect().width() / 2.0), nextPos.x()), nextPos.y());

    //                qDebug()<<__func__<<"clip nextPos all"<<clipItem->fileInfo.fileName()<<clipItem->clipIn<<nextPos;
                }
            }
        }
        else if (childItemType == "SubDurationLine")
        {
            nextPos = QPointF(0, 0);
        }
        else if (childItemType == "SubProgressSlider")
        {
            nextPos = QPointF(0, parentItem->boundingRect().height() - childItem->boundingRect().height());
        }
        else if (childItemType == "SubAudioLevelSlider")
        {
            nextPos = QPointF(parentItem->boundingRect().width() * 0.7, parentItem->boundingRect().height() * 0.2);
        }
        else if (childItemType.contains("SubNameItem")) //other subs (subpic and subtxt and subplayer)
        {
            nextPos = QPointF(parentItem->boundingRect().height() * 0.1, parentItem->boundingRect().height() );
        }
        else if (childItemType.contains("SubLogItem")) //other subs (subpic and subtxt and subplayer)
        {
            nextPos = QPointF(parentItem->boundingRect().height() * 0.1, parentItem->boundingRect().height() );//parentItem->boundingRect().height() - childItem->boundingRect().height() );
        }
        else if (childItemType.contains("SubWave")) //other subs (subpic and subtxt and subplayer)
        {
            nextPos = QPointF(0, 0);
        }
        else if (childItemType.contains("SubPlayButton"))
        {
//            playButtonProxy
            nextPos = QPointF(parentItem->boundingRect().width() * 0.5 - childItem->boundingRect().width() * 0.5 * childItem->scale(), parentItem->boundingRect().height() * 0.5 - childItem->boundingRect().height() * 0.5 * childItem->scale());
            //parent is pictureItem or playeritem
//            nextPos = QPointF(parentItem->boundingRect().width() * parentItem->scale() * 1.2 - childItem->boundingRect().width() / 2, parentItem->boundingRect().height() * 1.2  * parentItem->scale() - childItem->boundingRect().height() / 2);
//            nextPos = QPointF(parentItem->boundingRect().width() * parentItem->scale(), parentItem->boundingRect().height() );
//            qDebug()<<__func__<<nextPos<<"parentItem->boundingRect()"<<parentItem->data(mediaTypeIndex).toString()<<parentItem->data(itemTypeIndex).toString()<<parentItem->boundingRect().width() * parentItem->scale()<<parentItem->boundingRect().height() * parentItem->scale();
        }
        else if (childItemType.contains("Sub")) //other subs (subpic and subtxt and subplayer)
        {
            nextPos = QPointF(parentItem->boundingRect().height() * 0.1, parentItem->boundingRect().height() * 0.1);
        }
        else //"poly"
            nextPos = QPointF(0,0);

        childItem->setPos(nextPos);

//        qDebug()<<"AGView::arrangeItems"<<rectChildren<<parentItem->toolTip()<<childItem->toolTip();

        //part 2.3: calculating nextPos for next child (using rectChildren or childItem->boundingRect
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
//                AGClipRectItem *clipItem = (AGClipRectItem *)childItem;
//                qDebug()<<__func__<<"clip nextPos nextchild timeline"<<clipItem->fileInfo.fileName()<<clipItem->clipIn<<nextPos;
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
            }
            else if (childMediaType == "Clip")
            {
                nextPos = QPointF(nextPos.x() + rectChildren.width() + spaceBetween, nextPos.y()); //horizontal
//                AGClipRectItem *clipItem = (AGClipRectItem *)childItem;
//                qDebug()<<__func__<<"clip nextPos nextchild not timeline"<<clipItem->fileInfo.fileName()<<nextPos;
            }
            else if (childMediaType == "Tag")
            {
                //if outside vertical bounds and clip has enough horizontal space... then (kind of carriage) return

                if (nextPos.x() + 2 * childItem->boundingRect().width() > parentItem->boundingRect().width())
                    nextPos = QPointF(0, nextPos.y() + childItem->boundingRect().height());
                else
                    nextPos = QPointF(nextPos.x() + childItem->boundingRect().width(), nextPos.y()); //vertical next tag below previous tag

//                AGTagTextItem *tagItem = (AGTagTextItem *)childItem;
//                QGraphicsRectItem *clipItem = (QGraphicsRectItem *)parentItem;

//                if (nextPos.y() + childItem->boundingRect().height() > parentItem->boundingRect().height())
//                    nextPos = QPointF(nextPos.x() + childItem->boundingRect().width(), 0);
//                else
//                    nextPos = QPointF(nextPos.x(), nextPos.y() + childItem->boundingRect().height()); //vertical next tag below previous tag

//                tagItem->setTextWidth(clipItem->rect().width());
    //            qDebug()<<"tagItemtextWidth"<<tagItem->textWidth();


    //            qDebug()<<"nexttag"<<itemToString(childItem)<<nextPos;
            }

            if (childMediaType == "Clip")
            {
                lastClipNextPos = nextPos;
            }
        } //if base
        alternator = !alternator;

    } //foreach childitem

    //part 3: setRect and drawPoly
    if (parentMediaType == "MediaFile" && parentItemType == "Base") //childs have been arranged now
    {
//        qDebug()<<"checkSpotView"<<itemToString(parentRectItem)<<viewMode<<parentRectItem->rect()<<nextPos;
        if (QSettings().value("viewMode").toString() == "SpotView") //set size dependent on clips
        {
            parentMediaItem->setRect(QRectF(parentMediaItem->rect().x(), parentMediaItem->rect().y(), qMax(parentMediaItem->rect().width(), lastClipNextPos.x()), parentMediaItem->rect().height()));
            //if coming from timelineview,. the childrenboundingrect is still to big...
        }
        else //timelineView: set size independent from clips
        {
            parentMediaItem->setRect(QRectF(parentMediaItem->rect().x(), parentMediaItem->rect().y(), qMax(parentMediaItem->rect().width(), mediaHeight / aspectRatio), parentMediaItem->rect().height()));
        }

        //reallign all poly's and check if parentitem has clips
//        if (mediaDuration != 0)
        {
            foreach (AGClipRectItem *clipItem, parentMediaItem->clips)
            {
                clipItem->drawPoly(); //must be after foreach childItem to get sizes right
            }
        }
    }

    //part 4: adjust the width of text, lines and waves
    {
        QGraphicsVideoItem *playerItem = nullptr;
        QGraphicsProxyWidget *progressSliderProxy = nullptr;

        foreach (QGraphicsItem *childItem, parentItem->childItems())
        {
            if (childItem->data(itemTypeIndex).toString().contains("SubPlayer"))
            {
                playerItem = (QGraphicsVideoItem *)childItem;
            }
            else if (childItem->data(itemTypeIndex).toString() == "SubNameItem")
            {
                QGraphicsTextItem *subNameItem = (QGraphicsTextItem *)childItem;
                subNameItem->setTextWidth(parentItem->boundingRect().width() * 0.9);
            }
            else if (childItem->data(itemTypeIndex).toString() == "SubLogItem")
            {
                QGraphicsTextItem *subLogItem = (QGraphicsTextItem *)childItem;
                subLogItem->setTextWidth(parentItem->boundingRect().width() * 0.9);
            }
            else if (childItem->data(itemTypeIndex) == "SubDurationLine")
            {
                qreal scaleFactor = mediaFileScaleFactor;
                AGViewRectItem *parentViewItem = (AGViewRectItem *)parentItem; //mediafile or clipFile
                QGraphicsRectItem *durationLine = (QGraphicsRectItem *)childItem;

//                qDebug()<<"SubDurationLine"<<childViewItem->data(fileNameIndex).toString()<<parentViewItem->duration;
                if (childItem->data(mediaTypeIndex) == "Clip" || childItem->data(mediaTypeIndex) == "Folder"  ||
                        (childItem->data(mediaTypeIndex) == "MediaFile" && parentViewItem->parentRectItem->fileInfo.fileName() == "Export"))
                    scaleFactor = clipScaleFactor;

                durationLine->setRect(0,0, parentViewItem->duration * scaleFactor, 5);
            }
//            else if (childItem->data(itemTypeIndex) == "SubProgressLine")
//            {
//                progressRectItem = (QGraphicsRectItem *)childItem;
//            }
            else if (childItem->data(itemTypeIndex) == "SubProgressSlider")
            {
                progressSliderProxy = (QGraphicsProxyWidget *)childItem;
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
        if (playerItem != nullptr && progressSliderProxy != nullptr)
        {
            QMediaPlayer *m_player = (QMediaPlayer *)playerItem->mediaObject();

            QSlider *progressSlider = (QSlider *)progressSliderProxy->widget();
            if (m_player != nullptr)
            {
                progressSlider->setValue(m_player->position());

                double minX = parentItem->boundingRect().height() * 0.1;
                double maxX = parentItem->boundingRect().width() - playerItem->boundingRect().width() - minX;
                playerItem->setPos(qMin(qMax(parentItem->boundingRect().width() * m_player->position() / m_player->duration() - playerItem->boundingRect().width() / 2.0, minX), maxX), playerItem->pos().y());
            }
            progressSlider->setMinimumWidth(parentItem->boundingRect().width());
            progressSlider->setMaximumWidth(parentItem->boundingRect().width());
        }
    }

    if (parentItem == rootItem) //
    {
        updateChangedColors(false);
        AGFolderRectItem *folderItem = (AGFolderRectItem *)parentItem;
        folderItem->setTextItem(QTime(), QTime());
    }

    //part 5: set boundingrect
    if (parentMediaType == "Clip")
    {
        return parentItem->boundingRect(); //not the poly
    }
    else if (parentItemType == "SubPicture")
    {
//        qDebug()<<"SubPicture"<<parentItem->boundingRect();
        return parentItem->parentItem()->boundingRect(); //not the original size of the image...
    }
    else //all other
    {
        return parentItem->boundingRect()|parentItem->childrenBoundingRect();
    }

    //    parentItem->setToolTip(parentItem->toolTip() + QString(" %1 %2 %3 %4").arg(QString::number(r.x()), QString::number(r.y()), QString::number(r.width()), QString::number(r.height()/63.75)));

} //arrangeItems

//https://stackoverflow.com/questions/19113532/qgraphicsview-zooming-in-and-out-under-mouse-position-using-mouse-wheel
void AGView::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers() & Qt::ControlModifier)
    {
        // zoom
        const ViewportAnchor anchor = transformationAnchor();
        setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
        int angle = event->angleDelta().y();
        qreal factor;
        if (angle > 0) {
            factor = 1.1;
        } else {
            factor = 0.9;
        }
        scale(factor, factor);
        setTransformationAnchor(anchor);
    }
    else
        QGraphicsView::wheelEvent(event); //scroll with wheel
}

void AGView::setZoom(int value)
{
//    qDebug()<<"AGView::setZoom"<<value;
    scale(value/100.0, value / 100.0);
}

//https://forum.qt.io/topic/82015/how-to-move-qgraphicsscene-by-dragging-with-mouse/3
void AGView::mousePressEvent(QMouseEvent *event)
{
    QGraphicsItem *itemAtScreenPos = itemAt(event->pos());

    if (event->button() == Qt::LeftButton)
    {
        if (itemAtScreenPos == nullptr || !itemAtScreenPos->data(itemTypeIndex).toString().contains("Slider"))
        {
            panEnabled = true;
            panStartX = event->x();
            panStartY = event->y();
            setCursor(Qt::ClosedHandCursor);
            event->accept();
        }

        if (itemAtScreenPos != nullptr)
        {
            if (itemAtScreenPos->data(itemTypeIndex) == "SubPlayButton" || itemAtScreenPos->data(itemTypeIndex) == "SubProgressSlider")
                itemAtScreenPos = itemAtScreenPos->parentItem();
//            qDebug()<<__func__<<"setSelected"<<itemAtScreenPos->data(mediaTypeIndex)<<itemAtScreenPos->data(itemTypeIndex);
            scene->clearSelection();
            itemAtScreenPos->setSelected(true);
//            QGraphicsView::mousePressEvent(event); //to send event to pictureItems
        }
    }
    else if (event->button() == Qt::RightButton)
    {
//        qDebug()<<__func__<<"RightButton"<<event->pos();
        if (itemAtScreenPos->data(itemTypeIndex) != "SubPlayButton" && itemAtScreenPos->data(itemTypeIndex) != "SubProgressSlider")
        {
            onItemRightClicked(event->pos());
            event->accept();
        }
    }
//    else
        QGraphicsView::mousePressEvent(event); //scroll with wheel
}

void AGView::processAction(QString action)
{
//    qDebug()<<"AGView::processAction"<<action;

//    https://shotcut.org/howtos/keyboard-shortcuts/
//    https://makeawebsitehub.com/adobe-creative-cloud-cheat-sheet/

//    qDebug()<<"AGView::processAction"<<action;

    setFocus(Qt::OtherFocusReason);

    QGraphicsItem *item;

    if (scene->selectedItems().count() == 0)
        item = rootItem;
    else
        item = scene->selectedItems().first();

    if (action == "actionZoom_In")
    {
        scale(1.1, 1.1);
        emit showInStatusBar("Zoom In", 3000);
    }
    else if (action == "actionZoom_Out")
    {
        scale(0.9, 0.9);
        emit showInStatusBar("Zoom Out", 3000);
    }
    else if (action.contains("Top_Folder"))
    {
//        qDebug()<<action;
        scene->clearSelection();
        rootItem->setSelected(true);
        centerOn(rootItem);
    }
    else if (action.contains("Export"))
    {
        AGFolderRectItem *folderItem = (AGFolderRectItem *)rootItem;

        folderItem->processAction(action);
    }
    else if (action.contains("Item_")) //up down left right
    {
        QString localAction;

        if (item->data(mediaTypeIndex) == "MediaFile" || item->data(mediaTypeIndex) == "Clip")
        {
            if (action.contains("Up"))
                localAction = "Left";
            else if (action.contains("Down"))
                localAction = "Right";
            else if (action.contains("Left"))
                localAction = "Up";
            else if (action.contains("Right"))
                localAction = "Down";
        }
        else
            localAction = action;

        QStringList comment;
        //find previous / next sibling
        QGraphicsItem *newItem = nullptr;
        if (item->parentItem() != nullptr && (localAction.contains("Up") || localAction.contains("Down")))
        {
            for (int i=0; i < item->parentItem()->childItems().count(); i++)
            {
                QGraphicsItem *childItem = item->parentItem()->childItems()[i];

                if (childItem == item)
                {
                    comment << tr("try Sibling %1").arg(QString::number(i));

                    QGraphicsItem *siblingItem =  nullptr;
                    QGraphicsItem *previousSiblingItem =  nullptr;
                    QGraphicsItem *nextSiblingItem =  nullptr;

                    //find next sibling of same type
                    for (int j = 0; j < item->parentItem()->childItems().count(); j++)
                    {
                        QGraphicsItem *childItem2 = item->parentItem()->childItems()[j];

                        if (childItem2->data(itemTypeIndex) == childItem->data(itemTypeIndex) && childItem2->data(mediaTypeIndex) == childItem->data(mediaTypeIndex) && childItem2->isVisible())
                        {
                            if (j < i)
                                previousSiblingItem = childItem2;

                            if (nextSiblingItem == nullptr && j > i)
                                nextSiblingItem = childItem2;
                        }
                    }

                    if (localAction.contains("Down") && nextSiblingItem != nullptr)
                        siblingItem = nextSiblingItem;
                    if (localAction.contains("Up") && previousSiblingItem != nullptr)
                        siblingItem = previousSiblingItem;

                    comment << tr("try Sibling %1").arg("siblingItem->data(fileNameIndex).toString()");

                    if (siblingItem != nullptr && siblingItem->isVisible() && newItem == nullptr)
                    {
                        newItem = siblingItem;
                        comment << "found Sibling";
                    }
                }
            }
        }

        if (newItem == nullptr && localAction.contains("Right")) // find child for right
        {
            foreach (QGraphicsItem *childItem, item->childItems())
            {
                if (childItem->data(itemTypeIndex) == "Base" && childItem->isVisible())
                {
                    if (newItem == nullptr)
                    {
                        newItem = childItem;
                        comment << "found Child";
                    }
                }
            }
        }

        if (newItem == nullptr && (localAction.contains("Left") || localAction.contains("Up")) && item->parentItem() != nullptr) //find parent for left
        {
            newItem = item->parentItem();
            comment << "Parent";
        }

        if (newItem != nullptr)
        {
//            qDebug()<<action<<comment.join(",")<<item->data(mediaTypeIndex).toString()<<item->data(fileNameIndex).toString()<<newItem->data(mediaTypeIndex).toString()<<newItem->data(fileNameIndex).toString();
            scene->clearSelection();
            newItem->setSelected(true);
            centerOn(newItem);
        }
    }
    else if (scene->selectedItems().count() == 1)
    {
        if (item->data(mediaTypeIndex) == "MediaFile")
        {
            AGMediaFileRectItem *mediaItem = (AGMediaFileRectItem *)item;
            mediaItem->processAction(action);
            arrangeItems(nullptr, __func__ );
        }
        else if (item->data(mediaTypeIndex) == "Clip")
        {
            AGClipRectItem *clipItem = (AGClipRectItem *)item;
            clipItem->processAction(action);
            arrangeItems(nullptr, __func__ );
        }
    }
    else
        emit showInStatusBar(tr("No item selected (%1)").arg(QString::number(scene->selectedItems().count())), 3000);
}

void AGView::mouseReleaseEvent(QMouseEvent *event)
{
//    qDebug()<<"AGView::mouseReleaseEvent"<<event;
    if (event->button() == Qt::LeftButton)
    {
        panEnabled = false;
        setCursor(Qt::ArrowCursor);
        event->accept();
//        return;
    }
//    event->ignore();
//    else
        QGraphicsView::mouseReleaseEvent(event); //scroll with wheel
}

void AGView::mouseMoveEvent(QMouseEvent *event)
{
//    QMediaPlayer *m_player = qobject_cast<QMediaPlayer *>(sender());
//    qDebug()<<"AGView::mouseMoveEvent"<<event<<scene->itemAt(event->localPos(), QTransform());
    if (panEnabled)
    {
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - (event->x() - panStartX));
        verticalScrollBar()->setValue(verticalScrollBar()->value() - (event->y() - panStartY));
        panStartX = event->x();
        panStartY = event->y();
        event->accept();
//        return;
    }
//    else
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
        if (mediaItem->data(mediaTypeIndex).toString() == "MediaFile" && mediaItem->data(itemTypeIndex).toString() == "Base")
        {
            filterItem(mediaItem);
        }
    }

    if (searchText == "")
        filtering = false;

    arrangeItems(nullptr, __func__);

    scale(1.000001, 1.000001); //workaround to remove artifacts (poly's) (causes hoverMoveEvents??? so do not use in arrangeItems!)
}

void AGView::filterItem(QGraphicsItem *item)
{
//    if (!filtering)
//        return;

    AGMediaFileRectItem *mediaItem = (AGMediaFileRectItem *)item;

    bool foundMediaFile = false;
    if (searchText == "" || mediaItem->fileInfo.fileName().contains(searchText, Qt::CaseInsensitive))
        foundMediaFile = true;

    foreach (AGClipRectItem *clipItem, mediaItem->clips)
    {
        {
            bool foundTag = searchText == "";
            foreach (AGTagTextItem *tagItem, clipItem->tags)
            {
                if (searchText == "" || tagItem->tagName.contains(searchText, Qt::CaseInsensitive))
                    foundTag = true;
            }

            if (foundTag)
                foundMediaFile = true;

            clipItem->setData(excludedInFilter, !foundTag);
//            qDebug()<<__func__<<clipItem->fileInfo.fileName()<<clipItem->clipIn<<foundTag;
        }
    }

    mediaItem->setData(excludedInFilter, !foundMediaFile);
}

void AGView::setMediaScaleAndArrange(qreal scaleFactor)
{
    this->mediaFileScaleFactor = scaleFactor / 60000.0;

    if (rootItem == nullptr)
        return;

    arrangeItems(nullptr, __func__);
}

void AGView::setClipScaleAndArrange(qreal scaleFactor)
{
    this->clipScaleFactor = scaleFactor / 60000.0;

    if (rootItem == nullptr)
        return;

//    assignCreateDates();
    arrangeItems(nullptr, __func__);
}

void AGView::setOrderBy(QString orderBy)
{
    this->orderBy = orderBy;

    if (rootItem == nullptr)
        return;

//    qDebug()<<"AGView::setOrderBy"<<orderBy;

//    assignCreateDates();
    arrangeItems(nullptr, __func__);

    if (QSettings().value("viewMode") == "TimelineView") //otherwise artifacts, but very strange!
        arrangeItems(nullptr, __func__);

    scale(1.000001, 1.000001); //workaround to remove artifacts (poly's) (causes hoverMoveEvents??? so do not use in arrangeItems!)
}

void AGView::keyPressEvent(QKeyEvent *event)
{
    if (event->modifiers() == 0) //no control or shift
    {
        if (event->text() != "")
        {
//            qDebug()<<"AGView::keyPressEvent"<<event<<event->text();
            processAction("Key_" + event->text());
        }
    }
    QGraphicsView::keyPressEvent(event);

}

void AGView::onArrangeItems()
{
    arrangeItems(nullptr, __func__);
}
