#include "agcliprectitem.h"
#include "aglobal.h"
#include "agtagtextitem.h"

#include "agview.h" //for the constants

#include <QDir>
#include <QStyle>

AGClipRectItem::AGClipRectItem(QGraphicsItem *parent, AGMediaFileRectItem *mediaItem, QFileInfo fileInfo, int duration, int clipIn, int clipOut) :
    AGViewRectItem(parent, fileInfo)
{
//    changed = true;
    this->mediaType = "Clip";
    this->itemType = "Base";

    this->clipIn = clipIn;
    this->clipOut = clipOut;
    this->duration = duration;

    this->mediaItem = mediaItem;
    this->mediaItem->clips << this;
    this->timelineGroupItem = (AGViewRectItem *)mediaItem->focusProxy()->focusProxy();
    this->timelineGroupItem->clips<<this;
//    qDebug()<<"AGClipRectItem addclip"<<this->timelineGroupItem->fileInfo.fileName()<<this->timelineGroupItem->clips.count();
    setFocusProxy(mediaItem);

    int alpha = 125;
    if (AGlobal().audioExtensions.contains(fileInfo.suffix(), Qt::CaseInsensitive))
        setBrush(QColor(0, 128, 0, alpha)); //darkgreen
    else if (AGlobal().imageExtensions.contains(fileInfo.suffix(), Qt::CaseInsensitive))
        setBrush(QColor(218, 130, 42, alpha));
    else if (AGlobal().projectExtensions.contains(fileInfo.suffix(), Qt::CaseInsensitive))
        setBrush(QColor(130, 218, 42, alpha));
    else
        setBrush(QColor(42, 130, 218, alpha)); //#2a82da blue-ish

    setItemProperties("Clip", "Base", duration, QSize());

    QGraphicsRectItem *durationLine = new QGraphicsRectItem(this);

    durationLine->setBrush(Qt::darkGreen);

//    setItemProperties(durationLine, "Clip", "SubDurationLine", folderName, fileName, duration, QSize(), clipIn, clipOut);

    durationLine->setData(itemTypeIndex, "SubDurationLine");
    durationLine->setData(mediaTypeIndex, "Clip");

    durationLine->setData(folderNameIndex, fileInfo.absolutePath());
    durationLine->setData(fileNameIndex, fileInfo.fileName());

//    durationLine->setData(mediaDurationIndex, duration);
    durationLine->setData(mediaWithIndex, 0);
    durationLine->setData(mediaHeightIndex, 0);
}

AGClipRectItem::~AGClipRectItem()
{
//    this->mediaItem->clips.rem
}

QString AGClipRectItem::itemToString()
{
    return AGViewRectItem::itemToString() + " " + QString::number(clipIn) + " " + QString::number(clipOut);// + " " + QString::number(item->zValue());
}

void AGClipRectItem::processAction(QString action)
{
//    qDebug()<<"AGClipRectItem::processAction"<<action;
    if (action == "actionIn")
    {
        if (mediaItem->m_player->position() > clipOut)
            mediaItem->processAction(action); //add a new clip in mediaItem
        else
        {
            emit addUndo(true, "Update", "Clip", this, "clipIn", QString::number(clipIn), QString::number(mediaItem->m_player->position()));
            clipIn = mediaItem->m_player->position();
            duration = clipOut - clipIn;
            setData(mediaDurationIndex, duration);
        }

    }
    else if (action == "actionOut")
    {
        emit addUndo(true, "Update", "Clip", this, "clipOut", QString::number(clipOut), QString::number(mediaItem->m_player->position()));
        clipOut = mediaItem->m_player->position();
        duration = clipOut - clipIn;
//        qDebug()<<"AGClipRectItem processAction"<<action<<duration<<clipIn<<clipOut;
        setData(mediaDurationIndex, duration);
    }
    else if (action.contains("Key_"))
    {
//        qDebug()<<"ClipItem Key"<<action;

        //if key is * to ****: check if exists * and replace, otherwise add. if 0* then remove tag
        //if key is ✔: check if exists then remove (toggle), otherwise add
        //else if keybuffer not empty: check if (old) keybuffer exists (always the case...)
        //  if key is \r: empty keybuffer (tag will be red)
        //  if key is \b: remove last character from tag. If tag is empty, remove tag
        //  else update tag by adding key and update keybuffer
        //else if empty keybuffer: add key

        if (action == "Key_\r") //return
            keyBuffer = "";
        else
        {
            //loop over tags
            QString searchTag = "";
            if (action.contains("Key_*"))
                searchTag = "*";
            else if (action == "Key_✔")
                searchTag = "✔";
            else if (keyBuffer != "")
                searchTag = keyBuffer;

            bool found = false;
            if (searchTag != "")
            {
                foreach (AGTagTextItem *tagItem, tags)
                {
                    if ((action.contains("Key_*") && tagItem->tagName.contains(searchTag)) || tagItem->tagName == searchTag) //contains because of *(*(*(*(*)))))
                    {
                        found = true;

                        if (action == "Key_✔")
                            emit deleteItem(true, "Tag", fileInfo, clipIn, "✔");
                        else if (action == "Key_\b" && tagItem->tagName.length() == 1)
                            emit deleteItem(true, "Tag", fileInfo, clipIn, tagItem->tagName);
                        else if (action == "Key_*0")
                            emit deleteItem(true, "Tag", fileInfo, clipIn, tagItem->tagName);
                        else
                        {
                            QString oldValue = tagItem->tagName;
                            if (action == "Key_\b")
                            {
                                tagItem->tagName = tagItem->tagName.left(tagItem->tagName.length()-1);
                                keyBuffer = tagItem->tagName;
                            }
                            else if (action.contains("Key_*"))
                                tagItem->tagName = action.mid(4);
                            else
                            {
                                tagItem->tagName += action.mid(4);
                                keyBuffer = tagItem->tagName;
                            }

                            emit addUndo(true, "Update", "Tag", tagItem, "tagName", oldValue, keyBuffer);

                            tagItem->setHtml(QString("<div align=\"center\">") + tagItem->tagName + "</div>");

                        }
                    }
                }
            }

            if (!found  && !action.contains("Key_\b"))
            {
                //add tag
                if (!action.contains("Key_*") && action != "Key_✔")
                    keyBuffer += action.mid(4);
                emit addItem(true, "Clip", "Tag", fileInfo, duration, clipIn, clipOut, action.mid(4));
            }
        }
    }
    else
    {
        mediaItem->processAction(action);
    }
}

void AGClipRectItem::onItemRightClicked(QPoint pos)
{
    QGraphicsView *view = scene()->views().first();

    fileContextMenu->clear();

    fileContextMenu->addAction(new QAction("Zoom to item",fileContextMenu));
    connect(fileContextMenu->actions().last(), &QAction::triggered, [=]()
    {
        view->fitInView(boundingRect()|childrenBoundingRect(), Qt::KeepAspectRatio);
    });
    fileContextMenu->actions().last()->setToolTip(tr("<p><b>%1</b></p>"
                                        "<p><i>Zooms in to %2 and it's details</i></p>"
                                              ).arg(fileContextMenu->actions().last()->text(), fileInfo.fileName()));

    fileContextMenu->addSeparator();

    fileContextMenu->addAction(new QAction("Delete clip",fileContextMenu));
    fileContextMenu->actions().last()->setIcon(qApp->style()->standardIcon(QStyle::SP_TrashIcon));
    connect(fileContextMenu->actions().last(), &QAction::triggered, [=]()
    {
        emit deleteItem(true, "Clip", fileInfo, clipIn);
    });

        fileContextMenu->addAction(new QAction("Archive clips",fileContextMenu));
        fileContextMenu->actions().last()->setIcon(qApp->style()->standardIcon(QStyle::SP_TrashIcon));
        connect(fileContextMenu->actions().last(), &QAction::triggered, [=]()
        {
            AGProcessAndThread *process = new AGProcessAndThread(this);
            process->command("Archive clips", [=]
            {
                QString recycleFolder = fileInfo.absolutePath() + "/" + "MSKRecycleBin/";

                bool success = true;

                QDir dir(recycleFolder);
                if (!dir.exists())
                {
                    success = dir.mkpath(".");
                    if (success)
                        process->addProcessLog("output", tr("%1 created").arg(recycleFolder));
                }

                if (success)
                {
                    //first supporting files
                    if (success)
                    {
                        //srt file
                        QString srtFileName = fileInfo.completeBaseName() + ".srt";
                        QFile *file = new QFile(fileInfo.absolutePath() + "/" + srtFileName);
                        if (file->exists())
                        {
                            recursiveFileRenameCopyIfExists(recycleFolder, srtFileName);
                            success = file->rename(recycleFolder + srtFileName);
                            if (success)
                            {
//                                qDebug()<<"Undo - archive srt"<<srtFileName <<recycleFolder;
                                emit addUndo(false, "Archive", "Clips", this->mediaItem);
                                process->addProcessLog("output", tr("%1 moved to recycle folder").arg(srtFileName));
                            }
                        }

                        //txt file
                        if (success)
                        {
                            srtFileName = fileInfo.completeBaseName() + ".txt";
                            file = new QFile(fileInfo.absolutePath() + "/" + srtFileName);
                            if (file->exists())
                            {
                                recursiveFileRenameCopyIfExists(recycleFolder, srtFileName);
                                success = file->rename(recycleFolder + srtFileName);
                                if (success)
                                    process->addProcessLog("output", tr("%1 moved to recycle folder").arg(srtFileName));
                            }
                        }
                        else
                             process->addProcessLog("error", QString("-3, could not rename to " + recycleFolder + srtFileName));
                    }
                    else
                         process->addProcessLog("error", QString("-2, could not rename to " + recycleFolder + fileInfo.fileName()));

               }
               else
                  process->addProcessLog("error", QString("-1, could not create folder " + recycleFolder));

            });
            process->start();

        });
        fileContextMenu->actions().last()->setToolTip(tr("<p><b>%1</b></p>"
                                                         "<p><i>Move the clips of %2 (.srt file) and supporting files (.txt) to the Media Sidekick recycle bin folder</i></p>"
                                                               ).arg(fileContextMenu->actions().last()->text(), fileInfo.fileName()));

    QPointF map = view->mapToGlobal(QPoint(pos.x()+10, pos.y()));
    fileContextMenu->popup(QPoint(map.x(), map.y()));
}

QGraphicsItem * AGClipRectItem::drawPoly()
{
//    prepareGeometryChange();

//    qDebug()<<"AGView::drawPoly"<<itemToString(clipItem);
    QGraphicsPolygonItem *polyItem = nullptr;

    //find poly
    foreach (QGraphicsItem *item, childItems())
    {
        if (item->data(itemTypeIndex) == "poly")
            polyItem = (QGraphicsPolygonItem *)item;
    }

    bool shouldDraw = true;
    if (parentItem() != focusProxy()) //if clip part of timeline
    {
        //if mediafile not in same FileGroup as clip then do not draw (in case of filtering where the clip is in parking and the mediafile not)
        QGraphicsItem *clipFileGroup = parentItem()->parentItem();
        QGraphicsItem *mediaFileGroup = focusProxy()->parentItem();

        if (clipFileGroup != mediaFileGroup)
        {
            shouldDraw = false;
        }
    }

    if (shouldDraw)
    {
        if (polyItem == nullptr)
        {
//            if (fileInfo.fileName().contains("Blindfold"))
//                qDebug()<<"new poly" << itemToString(clipItem);
//            QBrush brush;
//            brush.setColor(Qt::lightGray);
//            brush.setStyle(Qt::SolidPattern);

            QPen pen(Qt::transparent);

            polyItem = new QGraphicsPolygonItem(this);
            polyItem->setPen(pen);
            QColor color = Qt::lightGray;
            color.setAlpha(127);
            polyItem->setBrush(color);
//            setItemProperties(polyItem, "Poly", "poly", data(mediaDurationIndex).toInt());

            polyItem->setData(mediaTypeIndex, "Poly");
            polyItem->setData(itemTypeIndex, "poly");

//            polyItem->setData(mediaDurationIndex, data(mediaDurationIndex).toInt());
            polyItem->setData(mediaWithIndex, 0);
            polyItem->setData(mediaHeightIndex, 0);
        }

        if (polyItem != nullptr) //should always be the case here
        {
            QGraphicsRectItem *parentMediaFile = (QGraphicsRectItem *)focusProxy(); //parent of the clip
//            if (fileInfo.fileName().contains("Blindfold"))
//                qDebug()<<"draw poly" << itemToString(clipItem) << itemToString(parentMediaFile);
            if (parentMediaFile != nullptr) //should always be the case
            {
                int duration = parentMediaFile->data(mediaDurationIndex).toInt();

//                        qDebug()<<"AGView::drawPoly inout"<<clipIn<<clipOut<<duration<<itemToString(parentMediaFile);

                QPointF parentPointLeft = mapFromItem(parentMediaFile, QPoint(duration==0?0:parentMediaFile->rect().width() * clipIn / duration, parentMediaFile->rect().height()));
                QPointF parentPointRight = mapFromItem(parentMediaFile, QPoint(duration==0?0:parentMediaFile->rect().width() * clipOut / duration, parentMediaFile->rect().height()));

//                if (fileInfo.fileName().contains("Blindfold"))
//                    qDebug()<<"  "<<parentMediaFile->rect()<<clipIn<<duration<<parentPointLeft<<QPoint(duration==0?0:parentMediaFile->rect().width() * clipIn / duration, parentMediaFile->rect().height());

                //all points relative to clip
                QPolygonF polyGon;
                polyGon.append(QPointF(parentPointLeft.x(), parentPointLeft.y()+1));
                polyGon.append(QPointF(-1,-1));
                polyGon.append(QPointF(boundingRect().width(),-1));
                polyGon.append(QPointF(parentPointRight.x(), parentPointRight.y()+1));
                polyItem->setPolygon(polyGon);

//                QGraphicsOpacityEffect *bef = new QGraphicsOpacityEffect();
//                polyItem->setGraphicsEffect(bef); //make it significantly slower
            }
        }

    }
    else //should not draw
    {
        if (polyItem != nullptr)
        {
            delete polyItem;
            polyItem = nullptr;
        }
    }

    return polyItem;
}
