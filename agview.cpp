#include "aglobal.h"
#include "agview.h"

#include <QGraphicsItem>

#include <QDebug>
#include <QGraphicsVideoItem>
#include <QMediaService>
#include <QMediaMetaData>
#include <QTimer>
#include <QKeyEvent>
#include <QTimeLine>
#include <QScrollBar>
#include <QGraphicsBlurEffect>
#include <QTime>

#include <qmath.h>

AGView::AGView(QWidget *parent)
    : QGraphicsView(parent)
{
    scene = new QGraphicsScene(this);
    setScene(scene);

    connect(scene, &QGraphicsScene::selectionChanged, this, &AGView::onSelectionChanged);

    QPixmap bgPix(":/images/karton.jpg");

    setBackgroundBrush(bgPix);
}

AGView::~AGView()
{
    qDebug()<<"GView::~AGView"; //not called yet!

    clearAll();
}

void AGView::clearAll()
{
    //stop all players
    foreach (QGraphicsItem *item, scene->items())
    {
        if (item->data(mediaTypeIndex).toString() == "MediaFile" && item->data(itemTypeIndex).toString() == "Base")
        {
            QGraphicsVideoItem *vidItem = nullptr;
            foreach (QGraphicsItem *childItem, item->childItems())
            {
                if (childItem->data(itemTypeIndex).toString().contains("SubVid"))
                    vidItem = (QGraphicsVideoItem *)childItem;

            }
            if (vidItem != nullptr)
            {
                QMediaPlayer *m_player = (QMediaPlayer *)vidItem->mediaObject();

                qDebug()<<"Player stopped"<<itemToString(item);

                m_player->stop();

            }
        }
    }

    scene->clear();
}

void AGView::onSelectionChanged()
{
    foreach (QGraphicsItem *item, scene->selectedItems())
        qDebug()<<"AGView::onSelectionChanged()"<<itemToString(item);

    if (scene->selectedItems().count() == 1)
    {
        emit itemSelected(scene->selectedItems().first());
//        playMedia(scene->selectedItems().first());
    }
}

void AGView::playMedia(QGraphicsItem *mediaItem)
{
    if (mediaItem->data(mediaTypeIndex).toString() == "MediaFile")
    {
        if (mediaItem->data(fileNameIndex).toString().toLower().contains(".mp"))
        {
            //find videoscreen
            //if not then create
            QGraphicsVideoItem *vidItem = nullptr;
            foreach (QGraphicsItem *childItem, mediaItem->childItems())
            {
                if (childItem->data(itemTypeIndex).toString().contains("SubVid"))
                    vidItem = (QGraphicsVideoItem *)childItem;
            }

            if (vidItem != nullptr)
            {
                vidItem = (QGraphicsVideoItem *)vidItem;
                QMediaPlayer *m_player = (QMediaPlayer *)vidItem->mediaObject();

                qDebug()<<"vid found"<<itemToString(vidItem)<<vidItem->mediaObject();

                if (m_player->state() != QMediaPlayer::PlayingState)
                    m_player->play();
                else
                    m_player->pause();

            }
            else
            {
                qDebug()<<"no vid found";

                vidItem = new QGraphicsVideoItem(mediaItem);
                vidItem->setSize(QSize(200 * 0.8, 200 * 9 / 16 * 0.8));
                vidItem->setPos(mediaItem->boundingRect().height() * 0.1, mediaItem->boundingRect().height() * 0.1);

                QMediaPlayer *m_player = new QMediaPlayer();
                connect(m_player, &QMediaPlayer::mediaStatusChanged, this, &AGView::onMediaStatusChanged);
                connect(m_player, QOverload<>::of(&QMediaPlayer::metaDataChanged), this, &AGView::onMetaDataChanged);
                connect(m_player, &QMediaPlayer::metaDataAvailableChanged, this, &AGView::onMetaDataAvailableChanged);
                connect(m_player, &QMediaPlayer::positionChanged, this, &AGView::onPositionChanged);

                m_player->setVideoOutput(vidItem);

                QString folderName = mediaItem->data(folderNameIndex).toString();
                QString fileName = mediaItem->data(fileNameIndex).toString();

                QTimer::singleShot(1000, this, [m_player, folderName, fileName]()->void
                {
                    m_player->setMedia(QUrl::fromLocalFile(folderName + fileName));
                });
                setItemProperties(vidItem, mediaItem->data(mediaTypeIndex).toString(), "SubVid", folderName, fileName, mediaItem->data(mediaDurationIndex).toInt());

            }
        }
    }
}

QString AGView::itemToString(QGraphicsItem *item)
{
    return item->data(folderNameIndex).toString() + " " + item->data(fileNameIndex).toString() + " " + item->data(itemTypeIndex).toString() + " " + item->data(mediaTypeIndex).toString() + " " + item->data(clipTagIndex).toString();
}

void AGView::updateToolTip(QGraphicsItem *item)
{
    item->setToolTip(QString("<p><b>%1</b></p>"
                                        "<p><i>%2</i></p>"
                                        "<ul>"
                             "<li><b>Foldername</b>: %3</li>"
                             "<li><b>Filename</b>: %4</li>"
                              "<li><b>Duration</b>: %5 (%6)</li>"
                              "<li><b>Size</b>: %7 * %8</li>"
                              "<li><b>Clip in</b>: %9</li>"
                             "<li><b>Clip out</b>: %10</li>"
                             "<li><b>Tag</b>: %11</li>"
                                        "</ul>").arg(item->data(mediaTypeIndex).toString(), item->data(itemTypeIndex).toString(), item->data(folderNameIndex).toString(), item->data(fileNameIndex).toString()
                                                     , AGlobal().msec_to_time(item->data(mediaDurationIndex).toInt()), QString::number(item->data(mediaDurationIndex).toInt())
                                                     , QString::number(item->data(mediaWithIndex).toInt()), QString::number(item->data(mediaHeightIndex).toInt())
                                                     ).arg(AGlobal().msec_to_time(item->data(clipInIndex).toInt()), AGlobal().msec_to_time(item->data(clipOutIndex).toInt()), item->data(clipTagIndex).toString()));
}

void AGView::setItemProperties(QGraphicsItem *parentItem, QString mediaType, QString itemType, QString folderName, QString fileName, int duration, QSize mediaSize, int clipIn, int clipOut, QString tag)
{
    parentItem->setData(itemTypeIndex, itemType);
    parentItem->setData(folderNameIndex, folderName);
    parentItem->setData(fileNameIndex, fileName);
    parentItem->setData(mediaTypeIndex, mediaType);

    parentItem->setData(mediaDurationIndex, duration);
    parentItem->setData(mediaWithIndex, mediaSize.width());
    parentItem->setData(mediaHeightIndex, mediaSize.height());

    parentItem->setData(clipInIndex, clipIn);
    parentItem->setData(clipOutIndex, clipOut);
    parentItem->setData(clipTagIndex, tag);

    updateToolTip(parentItem);
}

QGraphicsItem * AGView::addItem(QGraphicsItem *parentItem, QString mediaType, QString folderName, QString fileName, int duration, int clipIn, int clipOut, QString tag)
{
    QGraphicsItem *childItem = nullptr;

    if (mediaType == "Folder")
    {
        QGraphicsRectItem *rectItem = new QGraphicsRectItem(parentItem);

        rectItem->setRect(QRectF(0, 0, 200 * 9 / 16, 200 * 9 / 16));
        rectItem->setBrush(QBrush("#FF4500")); //orange

        setItemProperties(rectItem, mediaType, "Base", folderName, fileName, duration);

        childItem = rectItem;
    }
    else if (mediaType.contains("Group"))
    {
        QGraphicsRectItem *rectItem = new QGraphicsRectItem(parentItem);

        if (mediaType == "TimelineGroup")
            rectItem->setRect(QRectF(0, 0, 5, 175));
        else //fileGroup
            rectItem->setRect(QRectF(0, 0, 75, 200 * 9 / 16));
        rectItem->setBrush(QBrush(Qt::blue));

        setItemProperties(rectItem, mediaType, "Base", folderName, fileName, duration);

        childItem = rectItem;
    }
    else if (mediaType == "MediaFile")// && fileName.toLower().contains(".mp4")
    {
        QGraphicsRectItem *mediaItem = new QGraphicsRectItem(parentItem);
        mediaItem->setRect(QRectF(0, 0, qMax(duration/100.0,200.0), 200 * 9 / 16));
        if (parentItem->data(fileNameIndex).toString() == "Export")
            mediaItem->setRect(QRectF(0, 0, qMax(duration/100.0,200.0), 200 * 9 / 16)); //scale same as clips for export mediafiles
        else
            mediaItem->setRect(QRectF(0, 0, qMax(duration/500.0,200.0), 200 * 9 / 16)); //scale 5 times smaller

        mediaItem->setBrush(QBrush(Qt::darkGreen));

//        connect(mediaItem, &AGClipRectangleItem::agItemChanged, this, &AGView::onClipItemChanged);
//        connect(mediaItem, &AGClipRectangleItem::agMouseReleased, this, &AGView::onClipMouseReleased);
//        connect(mediaItem, &AGClipRectangleItem::clipPositionChanged, this, &AGView::onClipPositionChanged);

        setItemProperties(mediaItem, mediaType, "Base", folderName, fileName, duration);

        if (fileName.toLower().contains(".mp3"))
        {
            QGraphicsVideoItem *childVideoItem = new QGraphicsVideoItem(mediaItem);
            childVideoItem->setSize(QSize(200 * 0.8, 200 * 9 / 16 * 0.8));

            QMediaPlayer *m_player = new QMediaPlayer();
            connect(m_player, &QMediaPlayer::mediaStatusChanged, this, &AGView::onMediaStatusChanged);
            connect(m_player, QOverload<>::of(&QMediaPlayer::metaDataChanged), this, &AGView::onMetaDataChanged);
            connect(m_player, &QMediaPlayer::metaDataAvailableChanged, this, &AGView::onMetaDataAvailableChanged);
            connect(m_player, &QMediaPlayer::positionChanged, this, &AGView::onPositionChanged);

            m_player->setVideoOutput(childVideoItem);

            QTimer::singleShot(1000, this, [m_player, folderName, fileName]()->void
            {
                m_player->setMedia(QUrl::fromLocalFile(folderName + fileName));
            });

            setItemProperties(childVideoItem, mediaType, "SubVid", folderName, fileName, duration);
        }

        childItem = mediaItem;

        //https://doc.qt.io/qt-5/qtdatavisualization-audiolevels-example.html
    }
    else if (mediaType == "Clip")
    {
        QGraphicsRectItem *clipItem = new QGraphicsRectItem(parentItem);
        clipItem->setRect(QRectF(0, 0, duration/100.0, 100));
        clipItem->setBrush(QBrush(Qt::red));
        clipItem->setFocusProxy(parentItem);

//        connect(clipItem, &AGClipRectangleItem::agItemChanged, this, &AGView::onClipItemChanged);
//        connect(clipItem, &AGClipRectangleItem::agMouseReleased, this, &AGView::onClipMouseReleased);
//        connect(clipItem, &AGClipRectangleItem::clipPositionChanged, this, &AGView::onClipPositionChanged);
//        QGraphicsBlurEffect *bef = new QGraphicsBlurEffect();
//        QGraphicsDropShadowEffect *bef = new QGraphicsDropShadowEffect();

        setItemProperties(clipItem, mediaType, "Base", folderName, fileName, duration, QSize(), clipIn, clipOut);

        childItem = clipItem;
    }
    else if (mediaType == "Tag")
    {
        QGraphicsTextItem *tagItem = new QGraphicsTextItem(parentItem);
        tagItem->setDefaultTextColor(Qt::white);
        tagItem->setPlainText(tag);
        setItemProperties(tagItem, mediaType, "Base", folderName, fileName, duration, QSize(), clipIn, clipOut, tag);
        tagItem->setFlag(QGraphicsItem::ItemIsMovable, true); //to put in bin

        childItem = tagItem;
    }

//    childItem->setFlag(QGraphicsItem::ItemIsMovable, true);
    childItem->setFlag(QGraphicsItem::ItemIsSelectable, true);

    if (parentItem == nullptr)
    {
        scene->addItem(childItem);
        rootItem = childItem;
    }

    if (mediaType == "Folder" || mediaType == "FileGroup" || (mediaType == "MediaFile" && fileName.toLower().contains(".mp3")))
    {
        QGraphicsTextItem *childTextItem;
        childTextItem = new QGraphicsTextItem(childItem);
        childTextItem->setDefaultTextColor(Qt::white);
        childTextItem->setPlainText(fileName);
//        childTextItem->setFlag(QGraphicsItem::ItemIsMovable, true);
//        childTextItem->setFlag(QGraphicsItem::ItemIsSelectable, true);

        setItemProperties(childTextItem, mediaType, "SubTxt", folderName, fileName, duration);

        childTextItem->setTextWidth(childItem->boundingRect().width() * 0.8);

//        return childTextItem;
    }


    return childItem;
}

void AGView::onClipItemChanged(AGClipRectangleItem *clipItem)
{
    drawPoly(clipItem);
}

void AGView::onClipMouseReleased(AGClipRectangleItem *clipItem)
{
    //update clipIn and out
    updateToolTip(clipItem);
    drawPoly(clipItem);
    arrangeItems(nullptr);
}

void AGView::onClipPositionChanged(AGClipRectangleItem *clipItem, int progress)
{
    QGraphicsItem *mediaItem = nullptr;
    if (clipItem->data(mediaTypeIndex).toString() == "Clip")
        mediaItem = clipItem->focusProxy();
    else if (clipItem->data(mediaTypeIndex).toString() == "MediaFile")
        mediaItem = clipItem;

//    qDebug()<<"AGView::onClipPositionChanged"<<itemToString(mediaItem)<<itemToString(clipItem)<<duration;

    QGraphicsItem *vidItem = nullptr;
    QGraphicsItem *pixItem = nullptr;
    QGraphicsLineItem *lineItem = nullptr;

    if (mediaItem != nullptr)
        foreach (QGraphicsItem *childItem, mediaItem->childItems())
        {
            if (childItem->data(itemTypeIndex).toString().contains("SubVid"))
                vidItem = childItem;
            if (childItem->data(itemTypeIndex).toString().contains("SubPix"))
                pixItem = childItem;
            if (childItem->data(itemTypeIndex).toString().contains("SubLine"))
                lineItem = (QGraphicsLineItem *)childItem;

        }

    if (vidItem != nullptr)
    {
        QGraphicsVideoItem *childVideoItem = (QGraphicsVideoItem *)vidItem;
        QMediaPlayer *m_player = (QMediaPlayer *)childVideoItem->mediaObject();

//        qDebug()<<"vid found"<<itemToString(vidItem)<<childVideoItem->mediaObject();

        m_player->pause();
        m_player->setPosition(m_player->duration() * progress);

//        if (m_player->state() != QMediaPlayer::PlayingState)
//            m_player->play();
//        else
//            m_player->pause();

        if (lineItem != nullptr)
            lineItem->setLine(QLineF(lineItem->pen().width()/2, 0, qMax(lineItem->parentItem()->boundingRect().width() * progress / m_player->duration() - lineItem->pen().width()/2.0,lineItem->pen().width()/2.0), 0));

//            lineItem->setLine(QLineF(0, 0, lineItem->parentItem()->boundingRect().width() * progress / m_player->duration(), 0));
    }



}

void AGView::onMediaLoaded(QString folderName, QString fileName, QImage image, int duration, QSize mediaSize)
{
//    qDebug()<<"AGView::onMediaLoaded"<<folderName<<fileName<<duration<<mediaSize;

    QGraphicsItem *mediaItem = nullptr;
    foreach (QGraphicsItem *item, scene->items())
    {
        if (item->data(folderNameIndex) == folderName && item->data(fileNameIndex) == fileName && item->data(itemTypeIndex) == "Base")
        {
            mediaItem = item;
//            break;
        }
    }

    if (mediaItem != nullptr)
    {
        QGraphicsPixmapItem *pixItem = nullptr;

        setItemProperties(mediaItem, "MediaFile", "Base", folderName, fileName,  duration, mediaSize);

        foreach (QGraphicsItem *childItem, mediaItem->childItems())
        {
            if (childItem->data(itemTypeIndex).toString().contains("SubPix"))
                pixItem = (QGraphicsPixmapItem *)childItem;
        }

        if (image != QImage())
        {
            if (pixItem == nullptr)
                pixItem = new QGraphicsPixmapItem(mediaItem);

            QPixmap pixmap = QPixmap::fromImage(image);//.scaled(QSize(200,200 * myImage.height() / myImage.width()))
            pixItem->setPixmap(pixmap);
            pixItem->setScale(200.0 * 9.0 / 16.0 / image.height() * 0.8);

            setItemProperties(pixItem, "MediaFile", "SubPix", folderName, fileName, duration, mediaSize);
        }

        arrangeItems(mediaItem->parentItem()); //arrange the folder / foldergroup
    }
}

QGraphicsItem * AGView::drawPoly(QGraphicsItem *parentItem)
{
    QGraphicsPolygonItem *polyItem = nullptr;

    //find poly
    foreach (QGraphicsItem *item, parentItem->childItems())
    {
        if (item->data(itemTypeIndex) == "poly")
            polyItem = (QGraphicsPolygonItem *)item;
    }

    if (polyItem == nullptr)
    {
        QBrush brush;
        brush.setColor(Qt::lightGray);
        brush.setStyle(Qt::SolidPattern);
        QPen pen(Qt::transparent);

        polyItem = new QGraphicsPolygonItem(parentItem);
        polyItem->setPen(pen);
        polyItem->setBrush(brush);
    //    QGraphicsPolygonItem *pItem = scene->addPolygon(polyGon, pen, brush);
//        polyItem->setParentItem(parentItem);
        setItemProperties(polyItem, "Poly", "poly", parentItem->data(folderNameIndex).toString(), parentItem->data(fileNameIndex).toString(), parentItem->data(mediaDurationIndex).toInt());
    }

    if (polyItem != nullptr) //should always be the case here
    {
        QGraphicsItem *parentMediaFile = parentItem->focusProxy(); //parent of the clip
        if (parentMediaFile != nullptr) //should always be the case
        {
//                    QPointF parentPoint = parentItem->mapToItem(parentMediaFile, QPoint(parentMediaFile->boundingRect().width()*0.2, parentMediaFile->boundingRect().height()));

                    int clipIn = parentItem->data(clipInIndex).toInt();
                    int clipOut = parentItem->data(clipOutIndex).toInt();
                    int duration = parentMediaFile->data(mediaDurationIndex).toInt();

//                    qDebug()<<"AGView::drawPoly inout"<<clipIn<<clipOut<<duration;

                    QPointF parentPoint1 = polyItem->mapFromItem(parentMediaFile, QPoint(duration==0?0:parentMediaFile->boundingRect().width()*clipIn/duration, parentMediaFile->boundingRect().height()));
                    QPointF parentPoint2 = polyItem->mapFromItem(parentMediaFile, QPoint(duration==0?0:parentMediaFile->boundingRect().width()*clipOut/duration, parentMediaFile->boundingRect().height()));

//                    qDebug()<<"AGView::drawPoly parents"<<parentItem->data(itemTypeIndex).toString() << parentItem->data(mediaTypeIndex).toString() << parentItem->data(fileNameIndex).toString()<<parentMediaFile->data(itemTypeIndex).toString() << parentMediaFile->data(mediaTypeIndex).toString() << parentMediaFile->data(fileNameIndex).toString()<<parentMediaFile->boundingRect()<<parentPoint1;

//                    qDebug()<<"AGView::drawPoly pItem"<<polyItem->data(itemTypeIndex).toString() << polyItem->data(mediaTypeIndex).toString() << polyItem->data(fileNameIndex).toString()<<polyItem->polygon()<<parentPoint1;

                    QPolygonF polyGon;
                    polyGon.append(QPointF(parentPoint1.x(), parentPoint1.y()+1));
                    polyGon.append(QPointF(-1,-1));
                    polyGon.append(QPointF(parentItem->boundingRect().width(),-1));
                    polyGon.append(QPointF(parentPoint2.x(), parentPoint2.y()+1));
                    polyItem->setPolygon(polyGon);

                    QGraphicsOpacityEffect *bef = new QGraphicsOpacityEffect();
                    polyItem->setGraphicsEffect(bef); //make it significantly slower


//                    qDebug()<<"AGView::drawPoly polygon"<<polyItem->polygon();
        }
    }
    return polyItem;
}


void AGView::onMediaStatusChanged(QMediaPlayer::MediaStatus status)//
{
    QMediaPlayer *m_player = qobject_cast<QMediaPlayer *>(sender());

//    qDebug()<<"AGView::onMediaStatusChanged"<<status<<m_player->metaData(QMediaMetaData::Title).toString()<<m_player->media().canonicalUrl()<<m_player->error()<<m_player->errorString();

    //    if (status == QMediaPlayer::BufferedMedia)
//    {
//        m_player->setPosition(0);
//    }

    if (status == QMediaPlayer::LoadedMedia)
    {
        if (m_player->media().canonicalUrl().toString().toLower().contains(".mp4"))
        {
//            m_player->setNotifyInterval(AGlobal().frames_to_msec(1));

            m_player->play();
#ifdef Q_OS_MACxxxx
        QSize s1 = size();
        QSize s2 = s1 + QSize(1, 1);
        resize(s2);// enlarge by one pixel
        resize(s1);// return to original size
#endif

            m_player->setMuted(true);
        }
        else
        {
//            if (m_player->state() != QMediaPlayer::PlayingState)
//                m_player->play();
//            else
//                m_player->pause();

        }

    }
    else if (status == QMediaPlayer::EndOfMedia)
    {
//        m_player->setPosition(m_player->duration() / 2);
//        m_player->pause();
    }
}

void AGView::onMetaDataChanged()
{
    QMediaPlayer *m_player = qobject_cast<QMediaPlayer *>(sender());

    QString folderFileName = m_player->media().canonicalUrl().toString().replace("file:///", "").replace("file:", "");
    int lastIndexOf = folderFileName.lastIndexOf("/");
    QString folderName = folderFileName.left(lastIndexOf + 1);
    QString fileName = folderFileName.mid(lastIndexOf + 1);
//    qDebug()<<"AGView::onMetaDataChanged"<<folderName<<fileName<<m_player->metaData(QMediaMetaData::Duration).toString()<<metadatalist.count();

    QImage image = QImage();
    if (fileName.toLower().contains("mp3"))
        image = QImage(":/musicnote.png");
    onMediaLoaded(folderName, fileName, image, m_player->metaData(QMediaMetaData::Duration).toInt(), QSize());

    //to do: for mp3, Author, Title and AlbumTitle if available

    foreach (QString metadata_key, m_player->availableMetaData())
    {
        QVariant var_data = m_player->metaData(metadata_key);
//        qDebug() <<"AGView::onMetaDataChanged" << metadata_key << var_data.toString();
    }
}

void AGView::onMetaDataAvailableChanged(bool available)
{
    QMediaPlayer *m_player = qobject_cast<QMediaPlayer *>(sender());
//    qDebug()<<"AGView::onMetaDataAvailableChanged"<<available<<m_player->media().canonicalUrl()<<m_player->metaData(QMediaMetaData::Duration).toString();
}

void AGView::onPositionChanged(int progress)
{
    QMediaPlayer *m_player = qobject_cast<QMediaPlayer *>(sender());
    m_player->setProperty("test", "hi");

    QString folderFileName = m_player->media().canonicalUrl().toString().replace("file:///", "").replace("file:", "");
    int lastIndexOf = folderFileName.lastIndexOf("/");
    QString folderName = folderFileName.left(lastIndexOf + 1);
    QString fileName = folderFileName.mid(lastIndexOf + 1);

//    qDebug()<<"AGView::onPositionChanged"<<fileName<<progress<<m_player->duration();

//    foreach (QGraphicsItem *item, scene->items())
//    {
//        if (item->data(folderNameIndex) == folderName && item->data(fileNameIndex) == fileName && item->data(itemTypeIndex) == "SubLine")
//        {
//            QGraphicsLineItem *lineItem = (QGraphicsLineItem *)item;

//            if (lineItem != nullptr)
//            {
//                lineItem->setLine(QLineF(lineItem->pen().width()/2, 0, qMax(lineItem->parentItem()->boundingRect().width() * progress / m_player->duration() - lineItem->pen().width()/2.0,lineItem->pen().width()/2.0), 0));
//            }
//        }
//    }

    QGraphicsItem *mediaItem = nullptr;
    foreach (QGraphicsItem *item, scene->items())
    {
        if (item->data(folderNameIndex) == folderName && item->data(fileNameIndex) == fileName && item->data(itemTypeIndex) == "Base")
        {
            mediaItem = item;
//            break;
        }
    }

    if (mediaItem != nullptr)
    {
//        QGraphicsItem *vidItem = nullptr;
//        QGraphicsPixmapItem *pixItem = nullptr;
        QGraphicsLineItem *lineItem = nullptr;

        foreach (QGraphicsItem *childItem, mediaItem->childItems())
        {
//            if (childItem->data(itemTypeIndex).toString().contains("SubVid"))
//                vidItem = childItem;
//            if (childItem->data(itemTypeIndex).toString().contains("SubPix"))
//                pixItem = (QGraphicsPixmapItem *)childItem;
            if (childItem->data(itemTypeIndex).toString().contains("SubLine"))
                lineItem = (QGraphicsLineItem *)childItem;
        }

        if (mediaItem->data(mediaDurationIndex).toInt() != 0)
        {
            if (lineItem == nullptr)
            {
                lineItem = new QGraphicsLineItem(mediaItem);
                QPen pen;
                pen.setWidth(10);
                lineItem->setPen(pen);

//                arrangeItems(mediaItem);

                setItemProperties(lineItem, "MediaFile", "SubLine", folderName, fileName, mediaItem->data(mediaDurationIndex).toInt(), QSize(mediaItem->data(mediaWithIndex).toInt(), mediaItem->data(mediaHeightIndex).toInt()));
            }

//            lineItem->setLine(QLineF(lineItem->pen().width()/2, 0, lineItem->parentItem()->boundingRect().width() - lineItem->pen().width()/2.0, 0));
            lineItem->setLine(QLineF(lineItem->pen().width()/2, 0, qMax(lineItem->parentItem()->boundingRect().width() * progress / m_player->duration() - lineItem->pen().width()/2.0,lineItem->pen().width()/2.0), 0));

        }
    }
}

void AGView::folderScan(QGraphicsItem *parentItem, QString mode)
{
    foreach (QGraphicsItem *childItem, parentItem->childItems())
    {
        if (mode == "fileView")
        {
            if (childItem->data(mediaTypeIndex).toString() == "TimelineGroup" && childItem->data(itemTypeIndex).toString() == "Base")
            {
                //find all timelines
                foreach (QGraphicsItem *clipItem, childItem->childItems())
                {
                    foreach (QGraphicsItem *mediaFileItem, childItem->parentItem()->childItems())
                    {
//                        qDebug()<<"check for match and reparent"<<clipItem->toolTip()<<mediaFileItem->toolTip();
                        if (mediaFileItem->data(mediaTypeIndex).toString() == "MediaFile" && mediaFileItem->data(itemTypeIndex).toString() == "Base")
                        {
                            //check for match and reparent
                            if (mediaFileItem->data(fileNameIndex).toString() == clipItem->data(fileNameIndex).toString())
                                clipItem->setParentItem(mediaFileItem);
                        }
                    }
                }
            }
        }
        else //timelineView
        {
            if (childItem->data(mediaTypeIndex).toString() == "FileGroup" && childItem->data(itemTypeIndex).toString() == "Base")
            {
                //find clips
                foreach (QGraphicsItem *timelineGroupItem, childItem->childItems())
                {
                    if (timelineGroupItem->data(mediaTypeIndex).toString() == "TimelineGroup" && timelineGroupItem->data(itemTypeIndex).toString() == "Base")
                    {
                        //we found the destination

                        //find all timelines
                        foreach (QGraphicsItem *mediaFileItem, childItem->childItems())
                        {
                                foreach (QGraphicsItem *clipItem, mediaFileItem->childItems())
                                {
                                    if (!clipItem->data(itemTypeIndex).toString().contains("Sub"))
                                        clipItem->setParentItem(timelineGroupItem);
                                }
                        }
                    }
                }
            }
        }
        folderScan(childItem, mode);
    }
}

void AGView::onFileView()
{
    //clips as childs of file
    //find the timeline of each folder
    //find the files of each folder
    //get the clips of the timeline and reparent to the files

    qDebug()<<"AGView::onFileView";

    folderScan(rootItem, "fileView");

    arrangeItems(nullptr);

//    foreach (QGraphicsItem *item, scene->items())
//    {
//        if (item->data(mediaTypeIndex) == "Clip" && item->data(itemTypeIndex) == "Base")
//        {
//            drawPoly(item);
//        }
//    }
}

void AGView::onTimelineView()
{
    qDebug()<<"AGView::onTimelineView";

    //clips as childs of timeline group
    folderScan(rootItem, "timelineView");
    arrangeItems(nullptr);

//    foreach (QGraphicsItem *item, scene->items())
//    {
//        if (item->data(mediaTypeIndex) == "Clip" && item->data(itemTypeIndex) == "Base")
//        {
//            drawPoly(item);
//        }
//    }
}

void AGView::onCreateClip()
{
    //selectedclip at selectedposition
    QGraphicsItem *selectedItem = nullptr;
    if (scene->selectedItems().count() == 1)
        selectedItem = scene->selectedItems().first();

    if (selectedItem != nullptr)
    {
        //find current position
        qDebug()<<"Find position of"<<itemToString(selectedItem);
        //check if it fits within current clips
        //add clip
        addItem(selectedItem, "Clip", selectedItem->data(folderNameIndex).toString(), selectedItem->data(fileNameIndex).toString(), 3000, 2000, 5000);
        arrangeItems(nullptr);
    }

}
void AGView::onPlayVideoButton()
{
    if (scene->selectedItems().count() == 1)
        playMedia(scene->selectedItems().first());

}

void AGView::onMuteVideoButton()
{
    if (scene->selectedItems().count() == 1)
    {
        QGraphicsItem *mediaItem = scene->selectedItems().first();

        if (mediaItem->data(mediaTypeIndex).toString() == "MediaFile")
        {
            if (mediaItem->data(fileNameIndex).toString().toLower().contains(".mp"))
            {
                //find videoscreen
                //if not then create
                QGraphicsVideoItem *vidItem = nullptr;
                foreach (QGraphicsItem *childItem, mediaItem->childItems())
                {
                    if (childItem->data(itemTypeIndex).toString().contains("SubVid"))
                        vidItem = (QGraphicsVideoItem *)childItem;
                }

                if (vidItem != nullptr)
                {
                    vidItem = (QGraphicsVideoItem *)vidItem;
                    QMediaPlayer *m_player = (QMediaPlayer *)vidItem->mediaObject();

                    m_player->setMuted(!m_player->isMuted());
                }
            }
        }
    }
}

bool AGView::noFileOrClipDescendants(QGraphicsItem *parentItem)
{
    foreach (QGraphicsItem *childItem, parentItem->childItems())
    {
        if ((childItem->data(mediaTypeIndex).toString() == "Clip" || childItem->data(mediaTypeIndex).toString() == "MediaFile"))
            return false;
        bool x = noFileOrClipDescendants(childItem);
        if (!x)
            return false;
    }
    return true;
}

QRectF AGView::arrangeItems(QGraphicsItem *parentItem)
{
    if (parentItem == nullptr)
        parentItem = rootItem;

    int spaceBetween = 5;

    QString parentMediaType = parentItem->data(mediaTypeIndex).toString();
    QString parentItemType = parentItem->data(itemTypeIndex).toString();

    QGraphicsRectItem *parentRectItem = (QGraphicsRectItem *)parentItem;

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
    else if (parentMediaType.contains("MediaFile") && parentItemType == "Base")
    {
        int duration = parentItem->data(mediaDurationIndex).toInt();
        if (parentItem->parentItem()->data(fileNameIndex).toString() == "Export")
            parentRectItem->setRect(QRectF(0, 0, qMax(duration/100.0,200.0), 200 * 9 / 16));
        else
            parentRectItem->setRect(QRectF(0, 0, qMax(duration/500.0,200.0), 200 * 9 / 16));
    }

    //set the children start position
    QPointF nextPos = QPointF(0, parentItem->boundingRect().height() + spaceBetween);

    int mediaDuration = parentItem->data(mediaDurationIndex).toInt();

    bool alternator = false;
    bool firstClip = true;
    bool checkSpotView = false;

    if (parentItem == rootItem)
        qDebug()<<"arrangeItems"<<parentItemType<<itemToString(parentItem)<<parentMediaType;

    foreach (QGraphicsItem *childItem, parentItem->childItems())
    {
//        qDebug()<<"childItem"<<itemToString(childItem);
//        if (childItem->isVisible())
        {
        QString childMediaType = childItem->data(mediaTypeIndex).toString();
        QString childItemType = childItem->data(itemTypeIndex).toString();

        if (childItemType == "Base")
        {
            if ((childMediaType == "Folder" || childMediaType.contains("Group")))
            {
                if (childMediaType == "TimelineGroup")
                    nextPos = QPointF(parentItem->boundingRect().width() + spaceBetween, parentItem->boundingRect().height() + spaceBetween);
                else //FileGroup or Folder
                    nextPos = QPointF(parentItem->boundingRect().width() + spaceBetween, nextPos.y()); //horizontally
            }
            else if (childMediaType == "MediaFile" && firstClip)
            {
                nextPos = QPointF(parentItem->boundingRect().width() + spaceBetween, 0); // with of folder or group
                firstClip = false;
            }
            else if (childMediaType == "Clip" && firstClip)
            {
                if (parentMediaType == "TimelineGroup")
                    nextPos = QPointF(parentItem->boundingRect().width() + spaceBetween, 50); //timelineView mode: first clip next to timelineGroup (parent)
                else //mediaFile
                {
                    nextPos = QPointF(0, parentItem->boundingRect().height() + spaceBetween + 50); //spotView mode: first clip below mediafile (parent)
                    checkSpotView = true;
                }
                firstClip = false;
                alternator = false;
            }
            else if (childMediaType == "Tag" && firstClip)
            {
                nextPos = QPointF(0, 0); //first tag next to clip (parent)
//                qDebug()<<"firsttag"<<itemToString(childItem)<<nextPos;

                firstClip = false;
            }

            if (childMediaType == "Clip" && parentMediaType == "MediaFile") //first and notfirst: find the clipposition based on the position on the mediaitem timeline
            {
                //position of clipIn on MediaItem
                int clipIn = childItem->data(clipInIndex).toInt();
                int clipOut = childItem->data(clipOutIndex).toInt();
//                qDebug()<<"position of clipIn on MediaItem"<<clipIn<<clipOut<<duration;
                nextPos = QPointF(qMax(mediaDuration==0?0:(parentItem->boundingRect().width() * (clipIn+clipOut)/ 2.0 / mediaDuration - childItem->boundingRect().width() / 2.0), nextPos.x()), nextPos.y());
            }
        }
        else if (childItemType == "SubLine") //progressline
        {
            QGraphicsLineItem *lineItem = (QGraphicsLineItem *)childItem;

//            qDebug()<<"Sub on base"<<itemToString(childItem)<<nextPos<<lineItem->pen().width();

            nextPos = QPointF(0, parentItem->boundingRect().height() - lineItem->pen().width()/ 2.0 );
//            nextPos = QPointF(0, parentItem->boundingRect().height() );
        }
        else if (childItemType.contains("Sub")) //other subs (pic and txt)
        {
            nextPos = QPointF(parentItem->boundingRect().height() * 0.1, parentItem->boundingRect().height() * 0.1);
//                qDebug()<<"Sub on base"<<itemToString(childItem)<<nextPos;
        }
        else //"poly"
            nextPos = QPointF(0,0);

        childItem->setPos(nextPos);

        QRectF rectChildren = arrangeItems(childItem);

//        qDebug()<<"AGView::arrangeItems"<<rectChildren<<parentItem->toolTip()<<childItem->toolTip();

        //add rect of children
        if (childMediaType == "Folder" || childMediaType.contains("Group"))
            nextPos = QPointF(nextPos.x(), nextPos.y() + rectChildren.height() + spaceBetween); //vertical
        else if (childMediaType == "Clip" && parentMediaType == "TimelineGroup")
            nextPos = QPointF(nextPos.x() + rectChildren.width() - 10, nextPos.y() + (alternator?-25:25)); //horizontal alternating //
        else if (childMediaType == "Clip" || childMediaType == "MediaFile")
            nextPos = QPointF(nextPos.x() + rectChildren.width() + spaceBetween, nextPos.y()); //horizontal
        else if (childMediaType == "Tag")
        {
            nextPos = QPointF(nextPos.x(), nextPos.y() + childItem->boundingRect().height()); //vertical next tag below previous tag
//            qDebug()<<"nexttag"<<itemToString(childItem)<<nextPos;
        }

        alternator = !alternator;
        }
    }

    if (parentMediaType == "MediaFile" && parentItemType == "Base")
    {
//        if (checkSpotView)
//            parentRectItem->setRect(QRectF(parentRectItem->rect().x(), parentRectItem->rect().y(), qMax(parentRectItem->rect().width(), nextPos.x()), parentRectItem->rect().height()));
//        else
//            parentRectItem->setRect(QRectF(parentRectItem->rect().x(), parentRectItem->rect().y(), qMax(200.0, nextPos.x()), parentRectItem->rect().height()));
////            parentRectItem->setRect(QRectF(parentRectItem->rect().x(), parentRectItem->rect().y(), 200, parentRectItem->rect().height()));

        //reallign all poly's
        if (mediaDuration != 0)
        {
            foreach (QGraphicsItem *childItem, scene->items())
            {
                if (childItem->focusProxy() == parentItem)
                {
                    QString childMediaType = childItem->data(mediaTypeIndex).toString();
                    QString childItemType = childItem->data(itemTypeIndex).toString();

                    if (childItemType == "Base" && childMediaType == "Clip")
                    {
                        drawPoly(childItem);

                    }
                }
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
    if (event->modifiers() & Qt::ControlModifier)//ewi instead of ControlModifier
    {
        // Do a wheel-based zoom about the cursor position
        double angle = event->angleDelta().y();
        double factor = qPow(1.0015, angle);

        auto targetViewportPos = event->pos();
        auto targetScenePos = mapToScene(event->pos());

        scale(factor, factor);
        centerOn(targetScenePos);
        QPointF deltaViewportPos = targetViewportPos - QPointF(viewport()->width() / 2.0, viewport()->height() / 2.0);
        QPointF viewportCenter = mapFromScene(targetScenePos) - deltaViewportPos;
        centerOn(mapToScene(viewportCenter.toPoint()));

        return;
    }
    else
        QGraphicsView::wheelEvent(event); //scroll with wheel
}

//https://forum.qt.io/topic/82015/how-to-move-qgraphicsscene-by-dragging-with-mouse/3
void AGView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::RightButton) //instead of right
    {
        rightMousePressed = true;
        _panStartX = event->x();
        _panStartY = event->y();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }
    else
        QGraphicsView::mousePressEvent(event); //scroll with wheel
}

void AGView::mouseReleaseEvent(QMouseEvent *event){
    if (event->button() == Qt::RightButton)
    {
        rightMousePressed = false;
        setCursor(Qt::ArrowCursor);
        event->accept();
        return;
    }
//    event->ignore();
    else
        QGraphicsView::mouseReleaseEvent(event); //scroll with wheel
}

void AGView::mouseMoveEvent(QMouseEvent *event)
{
//    QMediaPlayer *m_player = qobject_cast<QMediaPlayer *>(sender());
//    qDebug()<<"AGView::mouseMoveEvent"<<event<<scene->itemAt(event->localPos(), QTransform())->data(fileNameIndex).toString();
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
    foreach (QGraphicsItem *item, scene->items())
    {
        if (item->data(mediaTypeIndex).toString() == "MediaFile" && item->data(itemTypeIndex).toString() == "Base") // && item->data(itemTypeIndex).toString() == "Base"
        {
            bool foundMediaFile = false;
            if (text == "" || item->data(fileNameIndex).toString().contains(text, Qt::CaseInsensitive))
            {
                foundMediaFile = true;
            }
            else
            {
                foreach (QGraphicsItem *clipItem, item->childItems())
                {
                    bool foundTag = false;;
                    foreach (QGraphicsItem *tagItem, clipItem->childItems())
                    {
                        if (tagItem->data(mediaTypeIndex) == "Tag" && tagItem->data(clipTagIndex).toString().contains(text, Qt::CaseInsensitive))
                            foundTag = true;
                    }

                    if (foundTag)
                        foundMediaFile = true;

                    //not working for some reason...
//                    if ((foundTag && !clipItem->isVisible()) || (!foundTag && clipItem->isVisible()))
//                    {
//                        clipItem->setVisible(foundTag);
//                        clipItem->setScale(foundTag?1:0);
//                    }
                }
            }

            if ((foundMediaFile && !item->isVisible()) || (!foundMediaFile && item->isVisible()))
            {
                item->setVisible(foundMediaFile);
                item->setScale(foundMediaFile?1:0);
            }


        }
    }
    arrangeItems(nullptr);
}