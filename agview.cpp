#include "aglobal.h"
#include "agmediarectangleitem.h"
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
#include <QSettings>

#include <qmath.h>

AGView::AGView(QWidget *parent)
    : QGraphicsView(parent)
{
    scene = new QGraphicsScene(this);
    setScene(scene);

    connect(scene, &QGraphicsScene::selectionChanged, this, &AGView::onSelectionChanged);

//    QPixmap bgPix(":/images/karton.jpg");
//    setBackgroundBrush(bgPix);

    viewMode = "fileView"; //default
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
            QGraphicsVideoItem *playerItem = nullptr;
            foreach (QGraphicsItem *childItem, item->childItems())
            {
                if (childItem->data(itemTypeIndex).toString().contains("SubPlayer"))
                    playerItem = (QGraphicsVideoItem *)childItem;

            }
            if (playerItem != nullptr)
            {
                QMediaPlayer *m_player = (QMediaPlayer *)playerItem->mediaObject();

                qDebug()<<"Player stopped"<<itemToString(item);

                m_player->stop();

            }
        }
    }

    scene->clear();
}

void AGView::onSelectionChanged()
{
//    foreach (QGraphicsItem *item, scene->selectedItems())
//        qDebug()<<"AGView::onSelectionChanged()"<<itemToString(item);

    if (scene->selectedItems().count() == 1)
    {
        playMedia(scene->selectedItems().first()); //this first to get m_player in calling itemSelected
        emit itemSelected(scene->selectedItems().first()); //triggers MainWindow::onGraphicsItemSelected
    }
}

void AGView::playMedia(QGraphicsItem *mediaItem)
{
    if (mediaItem->data(mediaTypeIndex).toString() == "MediaFile")
    {
        QString fileNameLow = mediaItem->data(fileNameIndex).toString().toLower();
        int lastIndexOf = fileNameLow.lastIndexOf(".");
        QString extension = fileNameLow.mid(lastIndexOf + 1);

        if (AGlobal().videoExtensions.contains(extension) || AGlobal().audioExtensions.contains(extension))
//        if (mediaItem->data(fileNameIndex).toString().toLower().contains(".mp"))
        {
            //find videoscreen
            //if not then create
            QGraphicsVideoItem *playerItem = nullptr;
            QGraphicsPixmapItem *pictureItem = nullptr;
            foreach (QGraphicsItem *childItem, mediaItem->childItems())
            {
                if (childItem->data(itemTypeIndex).toString().contains("SubPlayer"))
                    playerItem = (QGraphicsVideoItem *)childItem;
                if (childItem->data(itemTypeIndex).toString().contains("SubPicture"))
                    pictureItem = (QGraphicsPixmapItem *)childItem;
            }

            if (playerItem == nullptr)
            {
                playerItem = new QGraphicsVideoItem(mediaItem);
                playerItem->setSize(QSize(200 * 0.8, 200 * 9 / 16 * 0.8));
                playerItem->setPos(mediaItem->boundingRect().height() * 0.1, mediaItem->boundingRect().height() * 0.1);

                QMediaPlayer *m_player = new QMediaPlayer();
                connect(m_player, &QMediaPlayer::mediaStatusChanged, this, &AGView::onMediaStatusChanged);
                connect(m_player, QOverload<>::of(&QMediaPlayer::metaDataChanged), this, &AGView::onMetaDataChanged);
                connect(m_player, &QMediaPlayer::metaDataAvailableChanged, this, &AGView::onMetaDataAvailableChanged);
                connect(m_player, &QMediaPlayer::positionChanged, this, &AGView::onPositionChanged);

                m_player->setVideoOutput(playerItem);

                QString folderName = mediaItem->data(folderNameIndex).toString();
                QString fileName = mediaItem->data(fileNameIndex).toString();

//                QTimer::singleShot(1000, this, [m_player, folderName, fileName]()->void
//                {
                    m_player->setMedia(QUrl::fromLocalFile(folderName + fileName));
//                });
                setItemProperties(playerItem, mediaItem->data(mediaTypeIndex).toString(), "SubPlayer", folderName, fileName, mediaItem->data(mediaDurationIndex).toInt());
            }

            if (AGlobal().videoExtensions.contains(extension) && pictureItem != nullptr) //remove picture as video is taking over
            {
                scene->removeItem(pictureItem);
                delete pictureItem;
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
    QString tooltipText = "";

    QGraphicsItem *tooltipItem = item;
    if (item->data(itemTypeIndex).toString().contains("Sub"))
        tooltipItem = item->parentItem();

    if (tooltipItem->data(mediaTypeIndex) == "Folder")
        tooltipText += tr("<p><b>Folder %1</b></p>"
                       "<p><i>Folder with its subfolders and media files</i></p>"
                          "<ul>"
                          "<li>Only <b>folders containing mediafiles</b> are shown</li>"
                          "<li>The <b>ACVC recycle bin</b> is not shown</li>"
                          "<li><b>Open in Explorer</b>: Right panel: Click to see folder with files and subfolders, including the recycle bin.</li>"
                          "</ul>").arg(tooltipItem->data(folderNameIndex).toString() + tooltipItem->data(fileNameIndex).toString());
    else if (tooltipItem->data(mediaTypeIndex) == "FileGroup")
    {
        tooltipText += tr("<p><b>File group %1</b></p>"
                       "<p><i>Grouping of %1 files in folder %2</i></p>"
                          "<ul>"
                          ).arg(tooltipItem->data(fileNameIndex).toString(), tooltipItem->data(folderNameIndex).toString());
        if (tooltipItem->data(fileNameIndex).toString() == "Video")
            tooltipText += tr("<li><b>Group</b>: Supported Video files: %1</li>"
                              "</ul>").arg(AGlobal().videoExtensions.join(","));
        else if (tooltipItem->data(fileNameIndex).toString() == "Audio")
            tooltipText += tr("<li><b>Group</b>: Supported Audio files: %1</li>"
                              "</ul>").arg(AGlobal().audioExtensions.join(","));
        else if (tooltipItem->data(fileNameIndex).toString() == "Image")
            tooltipText += tr("<li><b>Group</b>: Supported Image files: %1</li>"
                              "</ul>").arg(AGlobal().imageExtensions.join(","));
        else if (tooltipItem->data(fileNameIndex).toString() == "Export")
            tooltipText += tr("<li><i>Export files are videos generated by ACVC, can be openen by common video players, uploaded to social media etc.</i></li>"
                              "<li><b>Supported Export methods</b>: %1</li>"
                              "<li><b>Supported Export files</b>: %2</li>"
                              "</ul>").arg(AGlobal().exportMethods.join(","), AGlobal().exportExtensions.join(","));
        else if (tooltipItem->data(fileNameIndex).toString() == "Project")
            tooltipText += tr("<li><i>Project files are generated by ACVC and can be openened by video editors</i></li>"
                              "<li><b>Supported Export methods</b>: %1</li>"
                              "<li><b>Supported Export files</b>: %2</li>"
                              "</ul>").arg(AGlobal().exportMethods.join(","), AGlobal().projectExtensions.join(","));
        else
            tooltipText += tr("<li><b>Group</b>: %2</li>"
                              "</ul>").arg(tooltipItem->data(fileNameIndex).toString());
    }
    else if (tooltipItem->data(mediaTypeIndex) == "MediaFile")
    {
        tooltipText += tr("<p><b>Media file %2</b></p>"
                       "<p><i></i></p>"
                       "<ul>"
            "<li><b>Foldername</b>: %1</li>"
             "<li><b>Duration</b>: %3 (%4 s)</li>").arg(tooltipItem->data(folderNameIndex).toString(), tooltipItem->data(fileNameIndex).toString()
                                                      , AGlobal().msec_to_time(tooltipItem->data(mediaDurationIndex).toInt()), QString::number(tooltipItem->data(mediaDurationIndex).toInt() / 1000.0));

        if (tooltipItem->data(mediaWithIndex).toInt() != -1)
            tooltipText += tr("<li><b>Size</b>: %5 * %6</li>").arg(QString::number(tooltipItem->data(mediaWithIndex).toInt()), QString::number(tooltipItem->data(mediaHeightIndex).toInt()));

        tooltipText += tr("<li><b>Duration line</b>: Red line above. 1 cm is about 30 seconds (unscaled, depending on display)</li>");
        tooltipText += tr("<li><b>Progress line</b>: Red line below</li>");

        tooltipText += tr("</ul>"
            "<p><b>Actions</p>"
            "<ul>"
            "<li><b>Play / pause</b>: Click on item and Sound or Video will be loaded and start playing. Click again will toggle play and pause</li>"
            "<li><b>More actions and details</b>: Click on media file and detailed actions and properties will be shown in right pane</li>"
            "<li><b>Change playing position</b>: Hover over progress line (red line below)</li>"
            "</ul>");
    }
    else if (tooltipItem->data(mediaTypeIndex) == "Clip")
    {
        tooltipText += tr("<p><b>Clip from %4 to %5</b></p>"
                       "<ul>"
            "<li><b>Filename</b>: %1</li>"
             "<li><b>Duration</b>: %2 (%3 s)</li>"
                       "</ul>"
            "<p><b>Actions</p>"
            "<ul>"
                          "<li><b>Play / pause</b>: Click on item and Sound or Video will be loaded and start playing. Click again will toggle play and pause</li>"
                          "<li><b>Change playing position</b>: Hover over progress line (red line below), change position within the clip start- and endpoint</li>"
            "</ul>").arg(tooltipItem->data(fileNameIndex).toString()
                                    , AGlobal().msec_to_time(tooltipItem->data(mediaDurationIndex).toInt()), QString::number(tooltipItem->data(mediaDurationIndex).toInt() / 1000.0)
                                    , AGlobal().msec_to_time(tooltipItem->data(clipInIndex).toInt()), AGlobal().msec_to_time(tooltipItem->data(clipOutIndex).toInt()));

    }
    else if (tooltipItem->data(mediaTypeIndex) == "Tag")
    {
        tooltipText += tr("<p><b>Rating, Tags and alike %1</b></p>"
                       "<p><i>Rating from * to *****, alike indicator or tag</i></p>"
                       "<ul>"
                       "</ul>"
            "<p><b>Actions</p>"
            "<ul>"
                          "<li><b>Remove</b>: Drag to bin (in next version of ACVC)</li>"
            "</ul>").arg(tooltipItem->data(clipTagIndex).toString());
    }

    if (tooltipText != "")
        item->setToolTip(tooltipText);
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

void AGView::addItem(QString parentFileName, QString mediaType, QString folderName, QString fileName, int duration, int clipIn, int clipOut, QString tag)
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
        parentMediaType = "MediaFile";
    else if (mediaType == "Tag")
        parentMediaType = "Clip";

    QGraphicsItem *parentItem = nullptr;
    foreach (QGraphicsItem *item, scene->items())
    {
        if (item->data(mediaTypeIndex).toString() == parentMediaType && item->data(itemTypeIndex).toString() == "Base")
        {
        if (mediaType == "Folder" && item->data(folderNameIndex).toString() + item->data(fileNameIndex).toString() + "/" == folderName)
                parentItem = item;
        else if (mediaType == "FileGroup" && item->data(folderNameIndex).toString() + item->data(fileNameIndex).toString() + "/" == folderName)
                parentItem = item;
        else if (mediaType == "TimelineGroup" && item->data(folderNameIndex).toString() == folderName && item->data(fileNameIndex).toString() == parentFileName)
                parentItem = item;
        else if (mediaType == "MediaFile" && item->data(folderNameIndex).toString() == folderName  && item->data(fileNameIndex).toString() ==  parentFileName)
                parentItem = item;
        else if (mediaType == "Clip" && item->data(folderNameIndex).toString() == folderName && item->data(fileNameIndex).toString() ==  fileName)
                parentItem = item;
        else if (mediaType == "Tag" && item->data(folderNameIndex).toString() == folderName && item->data(fileNameIndex).toString() ==  fileName && item->data(clipInIndex).toInt() ==  clipIn)
                parentItem = item;
        }
    }

//    if (parentItem != parentItem2)

//    qDebug()<<"AGView::addItem"<<parentFileName<<mediaType<<folderName<<fileName<<parentMediaType<<itemToString(parentItem);

//    if (parentItem2 != nullptr)
//        parentItem = parentItem2;

    QGraphicsItem *childItem = nullptr;

    QString fileNameLow = fileName.toLower();
    int lastIndexOf = fileNameLow.lastIndexOf(".");
    QString extension = fileNameLow.mid(lastIndexOf + 1);

    if (mediaType == "Folder")
    {
        QGraphicsRectItem *rectItem = new QGraphicsRectItem(parentItem);

        rectItem->setRect(QRectF(0, 0, 200 * 9 / 16, 200 * 9 / 16));
        rectItem->setBrush(Qt::darkYellow); //orange

        setItemProperties(rectItem, mediaType, "Base", folderName, fileName, duration);

        QGraphicsPixmapItem *pictureItem = new QGraphicsPixmapItem(rectItem);
        QImage image = QImage(":/images/ACVCFolder.png");
        QPixmap pixmap = QPixmap::fromImage(image);//.scaled(QSize(200,200 * myImage.height() / myImage.width()))
        pictureItem->setPixmap(pixmap);
        pictureItem->setScale(200.0 * 9.0 / 16.0 / image.height() * 0.8);
        setItemProperties(pictureItem, "MediaFile", "SubPicture", folderName, fileName, duration);

        childItem = rectItem;
    }
    else if (mediaType.contains("Group"))
    {
        QGraphicsRectItem *rectItem = new QGraphicsRectItem(parentItem);

        if (mediaType == "TimelineGroup")
            rectItem->setRect(QRectF(0, 0, 5, 225));
        else //fileGroup
        {
            rectItem->setRect(QRectF(0, 0, 75, 200 * 9 / 16));

            QGraphicsPixmapItem *pictureItem = new QGraphicsPixmapItem(rectItem);
            QImage image = QImage(":/acvc.ico");
            QPixmap pixmap = QPixmap::fromImage(image);//.scaled(QSize(200,200 * myImage.height() / myImage.width()))
            pictureItem->setPixmap(pixmap);
            pictureItem->setScale(200.0 * 9.0 / 16.0 / image.height() * 0.5);
            setItemProperties(pictureItem, "MediaFile", "SubPicture", folderName, fileName, duration);
        }
        rectItem->setBrush(QBrush(Qt::darkCyan));

        setItemProperties(rectItem, mediaType, "Base", folderName, fileName, duration);

        childItem = rectItem;
    }
    else if (mediaType == "MediaFile")
    {
        AGMediaRectangleItem *mediaItem = new AGMediaRectangleItem(parentItem);
        mediaItem->setRect(QRectF(0, 0, qMax(duration/100.0,200.0), 200 * 9 / 16));
        if (parentItem->data(fileNameIndex).toString() == "Export")
            mediaItem->setRect(QRectF(0, 0, qMax(duration/100.0,200.0), 200 * 9 / 16)); //scale same as clips for export mediafiles
        else
            mediaItem->setRect(QRectF(0, 0, qMax(duration/500.0,200.0), 200 * 9 / 16)); //scale 5 times smaller

        mediaItem->setBrush(QBrush(QBrush("#FF4500"))); //orange

//        connect(mediaItem, &AGClipRectangleItem::agItemChanged, this, &AGView::onClipItemChanged);
//        connect(mediaItem, &AGClipRectangleItem::agMouseReleased, this, &AGView::onClipMouseReleased);
        connect(mediaItem, &AGMediaRectangleItem::clipPositionChanged, this, &AGView::onClipPositionChanged);
        connect(mediaItem, &AGMediaRectangleItem::itemClicked, this, &AGView::onItemClicked);

        setItemProperties(mediaItem, mediaType, "Base", folderName, fileName, duration, QSize(), 0, duration);

        if (AGlobal().audioExtensions.contains(extension))
        {
            QGraphicsVideoItem *childVideoItem = new QGraphicsVideoItem(mediaItem);
            childVideoItem->setSize(QSize(200 * 0.8, 200 * 9 / 16 * 0.8));

            QMediaPlayer *m_player = new QMediaPlayer();
            connect(m_player, &QMediaPlayer::mediaStatusChanged, this, &AGView::onMediaStatusChanged);
            connect(m_player, QOverload<>::of(&QMediaPlayer::metaDataChanged), this, &AGView::onMetaDataChanged);
            connect(m_player, &QMediaPlayer::metaDataAvailableChanged, this, &AGView::onMetaDataAvailableChanged);
            connect(m_player, &QMediaPlayer::positionChanged, this, &AGView::onPositionChanged);

            m_player->setVideoOutput(childVideoItem);

//            QTimer::singleShot(1000, this, [m_player, folderName, fileName]()->void
//            {
                m_player->setMedia(QUrl::fromLocalFile(folderName + fileName));
//            });

            setItemProperties(childVideoItem, mediaType, "SubPlayer", folderName, fileName, duration);
        }
        else if (AGlobal().projectExtensions.contains(extension))
        {
            //add an image
            QGraphicsPixmapItem *pictureItem = new QGraphicsPixmapItem(mediaItem);
            QImage image;
            if (extension == "mlt")
                image = QImage(":/images/shotcut-logo-320x320.png");
            else if (extension == "xml")
                image = QImage(":/images/adobepremiereicon.png");
            else
                image = QImage(":/acvc.ico");
            QPixmap pixmap = QPixmap::fromImage(image);//.scaled(QSize(200,200 * myImage.height() / myImage.width()))
            pictureItem->setPixmap(pixmap);
            pictureItem->setScale(200.0 * 9.0 / 16.0 / image.height() * 0.8);
            setItemProperties(pictureItem, "MediaFile", "SubPicture", folderName, fileName, duration);
        }
        //else loadMedia/mediaLoaded sets an image for video and image and selectItem sets a video player for video

        childItem = mediaItem;

        //https://doc.qt.io/qt-5/qtdatavisualization-audiolevels-example.html
    }
    else if (mediaType == "Clip")
    {
        AGMediaRectangleItem *clipItem = new AGMediaRectangleItem(parentItem);
        clipItem->setRect(QRectF(0, 0, duration/100.0, 100));
        if (AGlobal().audioExtensions.contains(extension))
            clipItem->setBrush(Qt::darkGreen);
        else
            clipItem->setBrush(QBrush(QColor(42, 130, 218)));
        clipItem->setFocusProxy(parentItem);

//        connect(clipItem, &AGMediaRectangleItem::agItemChanged, this, &AGView::onClipItemChanged);
//        connect(clipItem, &AGMediaRectangleItem::agMouseReleased, this, &AGView::onClipMouseReleased);
        connect(clipItem, &AGMediaRectangleItem::clipPositionChanged, this, &AGView::onClipPositionChanged);
        connect(clipItem, &AGMediaRectangleItem::itemClicked, this, &AGView::onItemClicked);

//        QGraphicsBlurEffect *bef = new QGraphicsBlurEffect();
//        QGraphicsDropShadowEffect *bef = new QGraphicsDropShadowEffect();

        setItemProperties(clipItem, mediaType, "Base", folderName, fileName, duration, QSize(), clipIn, clipOut);

        QGraphicsLineItem *durationLine = new QGraphicsLineItem(clipItem);

        QBrush brush;
        brush.setColor(Qt::red);
        brush.setStyle(Qt::SolidPattern);

        QPen pen;
        pen.setWidth(5);
        pen.setBrush(brush);

        durationLine->setPen(pen);

        durationLine->setLine(QLineF(durationLine->pen().width()/2, 0, duration/500.0, 0));
//            durationLine->setLine(QLineF(durationLine->pen().width()/2, 0, qMax(durationLine->parentItem()->boundingRect().width() - durationLine->pen().width()/2.0,durationLine->pen().width()/2.0), 0));

        setItemProperties(durationLine, mediaType, "SubDurationLine", folderName, fileName, duration, QSize(), clipIn, clipOut);


        childItem = clipItem;
    }
    else if (mediaType == "Tag")
    {
        QGraphicsTextItem *tagItem = new QGraphicsTextItem(parentItem);
        if (QSettings().value("theme") == "Black")
            tagItem->setDefaultTextColor(Qt::white);
        else
            tagItem->setDefaultTextColor(Qt::black);

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

//    if (mediaType == "Folder" || mediaType == "FileGroup" || (mediaType == "MediaFile" && AGlobal().audioExtensions.contains(extension)))
    if (mediaType != "Clip" && mediaType != "Tag" && mediaType != "TimelineGroup") //all the others get text
    {
        QGraphicsTextItem *childTextItem;
        childTextItem = new QGraphicsTextItem(childItem);
        if (QSettings().value("theme") == "Black")
            childTextItem->setDefaultTextColor(Qt::white);
        else
            childTextItem->setDefaultTextColor(Qt::black);
        childTextItem->setPlainText(fileName);
        childTextItem->setZValue(-100);
//        childTextItem->setFlag(QGraphicsItem::ItemIsMovable, true);
//        childTextItem->setFlag(QGraphicsItem::ItemIsSelectable, true);

        setItemProperties(childTextItem, mediaType, "SubName", folderName, fileName, duration);

        childTextItem->setTextWidth(childItem->boundingRect().width() * 0.8);

//        return childTextItem;
    }

    //position new item at the bottom (arrangeitem will put it right
//    childItem->setPos(QPointF(0, rootItem->boundingRect().height()));
    arrangeItems(nullptr);

}

void AGView::setTextItemsColor(QColor color)
{
    foreach (QGraphicsItem *item, scene->items())
    {
        if (item->data(itemTypeIndex).toString() == "SubName" || item->data(mediaTypeIndex).toString() == "Tag")
        {
            QGraphicsTextItem *textItem = (QGraphicsTextItem *)item;
            textItem->setDefaultTextColor(color);
        }
    }
}

void AGView::onClipItemChanged(QGraphicsItem *clipItem)
{
    drawPoly(clipItem);
}

void AGView::onClipMouseReleased(QGraphicsItem *clipItem)
{
    //update clipIn and out
    updateToolTip(clipItem);
    drawPoly(clipItem);
    arrangeItems(nullptr);
}

void AGView::onItemClicked(QGraphicsItem *item)
{
//    qDebug()<<"AGView::onItemClicked"<<itemToString(item);
    QGraphicsItem *mediaItem = item;

    if (item->data(mediaTypeIndex) == "Clip")
    {
        mediaItem = item->focusProxy();
        playMedia(mediaItem);//load if not already done
    }
    else
        mediaItem = item;

    {
        QString fileNameLow = mediaItem->data(fileNameIndex).toString().toLower();
        int lastIndexOf = fileNameLow.lastIndexOf(".");
        QString extension = fileNameLow.mid(lastIndexOf + 1);

        if (AGlobal().videoExtensions.contains(extension) || AGlobal().audioExtensions.contains(extension))
//        if (mediaItem->data(fileNameIndex).toString().toLower().contains(".mp"))
        {
            //find videoscreen
            //if not then create
            QGraphicsVideoItem *playerItem = nullptr;
            foreach (QGraphicsItem *childItem, mediaItem->childItems())
            {
                if (childItem->data(itemTypeIndex).toString().contains("SubPlayer"))
                    playerItem = (QGraphicsVideoItem *)childItem;
            }

            if (playerItem != nullptr)
            {
                playerItem = (QGraphicsVideoItem *)playerItem;
                QMediaPlayer *m_player = (QMediaPlayer *)playerItem->mediaObject();

                if (m_player->state() != QMediaPlayer::PlayingState)
                    m_player->play();
                else
                    m_player->pause();
            }
        }
    }

}

void AGView::onClipPositionChanged(QGraphicsItem *clipItem, int progress)
{
    QGraphicsItem *mediaItem = nullptr;
    if (clipItem->data(mediaTypeIndex).toString() == "Clip")
        mediaItem = clipItem->focusProxy();
    else if (clipItem->data(mediaTypeIndex).toString() == "MediaFile")
        mediaItem = clipItem;

    QGraphicsItem *playerItem = nullptr;
    QGraphicsLineItem *progressLineItem = nullptr;

    if (mediaItem != nullptr)
        foreach (QGraphicsItem *childItem, mediaItem->childItems())
        {
            if (childItem->data(itemTypeIndex).toString().contains("SubPlayer"))
                playerItem = childItem;
            if (childItem->data(itemTypeIndex).toString().contains("SubProgressLine"))
                progressLineItem = (QGraphicsLineItem *)childItem;
        }

//    qDebug()<<"AGView::onClipPositionChanged"<<itemToString(mediaItem)<<itemToString(clipItem)<<itemToString(progressLineItem)<<progress;

    if (playerItem != nullptr)
    {
        QGraphicsVideoItem *childVideoItem = (QGraphicsVideoItem *)playerItem;
        QMediaPlayer *m_player = (QMediaPlayer *)childVideoItem->mediaObject();

//        qDebug()<<"vid found"<<itemToString(playerItem)<<childVideoItem->mediaObject();

//        m_player->pause();
        m_player->setPosition(progress);

        if (progressLineItem != nullptr)
            progressLineItem->setLine(QLineF(progressLineItem->pen().width()/2, 0, qMax(progressLineItem->parentItem()->boundingRect().width() * progress / m_player->duration() - progressLineItem->pen().width()/2.0,progressLineItem->pen().width()/2.0), 0));

//            lineItem->setLine(QLineF(0, 0, lineItem->parentItem()->boundingRect().width() * progress / m_player->duration(), 0));
    }
}

void AGView::onMediaLoaded(QString folderName, QString fileName, QImage image, int duration, QSize mediaSize, QString ffmpegMeta)
{
    QGraphicsItem *mediaItem = nullptr;
    foreach (QGraphicsItem *item, scene->items())
    {
        if (item->data(folderNameIndex) == folderName && item->data(fileNameIndex) == fileName && item->data(itemTypeIndex) == "Base")
        {
            mediaItem = item;
//            break;
        }
    }

//    qDebug()<<"AGView::onMediaLoaded"<<folderName<<fileName<<duration<<mediaSize<<ffmpegMeta<<itemToString(mediaItem);

    if (mediaItem != nullptr)
    {
        setItemProperties(mediaItem, "MediaFile", "Base", folderName, fileName,  duration, mediaSize);
        if (ffmpegMeta != "") //when called by onMetaDataChanged (from QMediaPlayer)
            mediaItem->setData(ffMpegMetaIndex, ffmpegMeta);

        QGraphicsPixmapItem *pictureItem = nullptr;
        QGraphicsLineItem *durationLine = nullptr;
        QGraphicsLineItem *progressLineItem = nullptr;
        QGraphicsVideoItem *playerItem = nullptr;


        foreach (QGraphicsItem *childItem, mediaItem->childItems())
        {
            if (childItem->data(itemTypeIndex).toString().contains("SubPicture"))
                pictureItem = (QGraphicsPixmapItem *)childItem;
            if (childItem->data(itemTypeIndex).toString().contains("SubDurationLine"))
                durationLine = (QGraphicsLineItem *)childItem;
            if (childItem->data(itemTypeIndex).toString().contains("SubProgressline"))
                progressLineItem = (QGraphicsLineItem *)childItem;
            if (childItem->data(itemTypeIndex).toString().contains("SubPlayer"))
                playerItem = (QGraphicsVideoItem *)childItem;
        }

        if (image != QImage())
        {
            if (pictureItem == nullptr)
                pictureItem = new QGraphicsPixmapItem(mediaItem);

            QPixmap pixmap = QPixmap::fromImage(image);//.scaled(QSize(200,200 * myImage.height() / myImage.width()))
            pictureItem->setPixmap(pixmap);
            pictureItem->setScale(200.0 * 9.0 / 16.0 / image.height() * 0.8);

            setItemProperties(pictureItem, "MediaFile", "SubPicture", folderName, fileName, duration, mediaSize);
        }

        if (duration != 0)
        {
            if (durationLine == nullptr)
            {
                durationLine = new QGraphicsLineItem(mediaItem);

                QBrush brush;
                brush.setColor(Qt::red);
                brush.setStyle(Qt::SolidPattern);

                QPen pen;
                pen.setWidth(5);
                pen.setBrush(brush);

                durationLine->setPen(pen);

                setItemProperties(durationLine, "MediaFile", "SubDurationLine", folderName, fileName, duration, mediaSize);
            }

            durationLine->setLine(QLineF(durationLine->pen().width()/2, 0, duration/500 - durationLine->pen().width()/2.0, 0));
    //            durationLine->setLine(QLineF(durationLine->pen().width()/2, 0, qMax(durationLine->parentItem()->boundingRect().width() - durationLine->pen().width()/2.0,durationLine->pen().width()/2.0), 0));

            if (progressLineItem == nullptr)
            {
                progressLineItem = new QGraphicsLineItem(mediaItem);

                QBrush brush;
                brush.setColor(Qt::red);
                brush.setStyle(Qt::SolidPattern);

                QPen pen;
                pen.setWidth(10);
                pen.setBrush(brush);
                progressLineItem->setPen(pen);

                setItemProperties(progressLineItem, "MediaFile", "SubProgressline", folderName, fileName, duration, mediaSize);
                progressLineItem->setLine(QLineF(progressLineItem->pen().width()/2, 0, progressLineItem->pen().width()/2, 0));
            }

//            lineItem->setLine(QLineF(lineItem->pen().width()/2, 0, lineItem->parentItem()->boundingRect().width() - lineItem->pen().width()/2.0, 0));
//            progressLineItem->setLine(QLineF(progressLineItem->pen().width()/2, 0, qMax(progressLineItem->parentItem()->boundingRect().width() * progress / m_player->duration() - progressLineItem->pen().width()/2.0,progressLineItem->pen().width()/2.0), 0));

            if (playerItem != nullptr) //update duration from QMediaPlayer (needed for audio files)
                setItemProperties(playerItem, "MediaFile", "SubPlayer", folderName, fileName, duration, mediaSize);
        }

//        qDebug()<<"arrangeItems"<<itemToString(mediaItem->parentItem());
        arrangeItems(mediaItem->parentItem()); //arrange the folder / foldergroup

        loadMediaCompleted ++;

        emit mediaLoaded(folderName, fileName, image, duration, mediaSize, ffmpegMeta);

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
//        polyItem->setPen(pen);
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


void AGView::onMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    QMediaPlayer *m_player = qobject_cast<QMediaPlayer *>(sender());

//    qDebug()<<"AGView::onMediaStatusChanged"<<status<<m_player->metaData(QMediaMetaData::Title).toString()<<m_player->media().canonicalUrl()<<m_player->error()<<m_player->errorString();

    if (status == QMediaPlayer::LoadedMedia)
    {
        m_player->setNotifyInterval(AGlobal().frames_to_msec(1));

        QString folderFileNameLow = m_player->media().canonicalUrl().toString().toLower();
        int lastIndexOf = folderFileNameLow.lastIndexOf(".");
        QString extension = folderFileNameLow.mid(lastIndexOf + 1);

        if (AGlobal().videoExtensions.contains(extension))
        {

            m_player->play();
//            m_player->pause();
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
//        m_player->play();
        m_player->setPosition(0);
        m_player->pause();
//        onPositionChanged(0);
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
    QString fileNameLow = fileName.toLower();
    lastIndexOf = fileNameLow.lastIndexOf(".");
    QString extension = fileNameLow.mid(lastIndexOf + 1);

    if (AGlobal().audioExtensions.contains(extension))
        image = QImage(":/musicnote.png");
    onMediaLoaded(folderName, fileName, image, m_player->metaData(QMediaMetaData::Duration).toInt());

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

    QGraphicsItem *mediaItem = nullptr;
    foreach (QGraphicsItem *item, scene->items())
    {
        if (item->data(folderNameIndex) == folderName && item->data(fileNameIndex) == fileName && item->data(itemTypeIndex) == "Base")
        {
            mediaItem = item;
//            break;
        }
    }

    if (mediaItem != nullptr  && m_player->duration() != 0)
    {
        QGraphicsVideoItem *playerItem = nullptr;
        QGraphicsLineItem *progressLineItem = nullptr;

        foreach (QGraphicsItem *childItem, mediaItem->childItems())
        {
            if (childItem->data(itemTypeIndex).toString().contains("SubPlayer"))
                playerItem = (QGraphicsVideoItem *)childItem;
            if (childItem->data(itemTypeIndex).toString().contains("SubProgressline"))
                progressLineItem = (QGraphicsLineItem *)childItem;
        }

        if (progressLineItem != nullptr) // subProgressline already created in mediaLoaded
        {

//            lineItem->setLine(QLineF(lineItem->pen().width()/2, 0, lineItem->parentItem()->boundingRect().width() - lineItem->pen().width()/2.0, 0));
            progressLineItem->setLine(QLineF(progressLineItem->pen().width()/2, 0, qMax(progressLineItem->parentItem()->boundingRect().width() * progress / m_player->duration() - progressLineItem->pen().width()/2.0,progressLineItem->pen().width()/2.0), 0));
        }

        if (playerItem != nullptr) //move video to current position
        {
            double minX = mediaItem->boundingRect().height() * 0.1;
            double maxX = mediaItem->boundingRect().width() - playerItem->boundingRect().width() - minX;
            playerItem->setPos(qMin(qMax(mediaItem->boundingRect().width() * progress / m_player->duration() - playerItem->boundingRect().width() / 2.0, minX), maxX), playerItem->pos().y());

        }
    }
}

void AGView::folderScan(QGraphicsItem *parentItem, QString mode, QString prefix)
{
    viewMode = mode;
    foreach (QGraphicsItem *childItem, parentItem->childItems())
    {
//        if (childItem->data(itemTypeIndex) == "Base")
//            qDebug()<<"AGView::folderScan" + prefix<<itemToString(childItem);
        if (viewMode == "fileView")
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
        folderScan(childItem, mode, prefix + "  ");
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

        addItem("Media", "Clip", selectedItem->data(folderNameIndex).toString(), selectedItem->data(fileNameIndex).toString(), 3000, 2000, 5000);
        //selectedItem
        arrangeItems(nullptr);
    }

}
void AGView::onPlayVideoButton(QMediaPlayer *m_player)
{
    if (m_player->state() != QMediaPlayer::PlayingState)
        m_player->play();
    else
        m_player->pause();


    return;
//    if (scene->selectedItems().count() == 1)
//        playMedia(scene->selectedItems().first());
    if (scene->selectedItems().count() == 1)
    {
        QGraphicsItem *mediaItem = scene->selectedItems().first();

        onItemClicked(mediaItem);
    }
}

void AGView::onMuteVideoButton(QMediaPlayer *m_player)
{
    m_player->setMuted(!m_player->isMuted());

    return;

    if (scene->selectedItems().count() == 1)
    {
        QGraphicsItem *mediaItem = scene->selectedItems().first();

        if (mediaItem->data(mediaTypeIndex).toString() == "MediaFile")
        {
            QString fileNameLow = mediaItem->data(fileNameIndex).toString().toLower();
            int lastIndexOf = fileNameLow.lastIndexOf(".");
            QString extension = fileNameLow.mid(lastIndexOf + 1);

            if (AGlobal().videoExtensions.contains(extension) || AGlobal().audioExtensions.contains(extension))
//            if (mediaItem->data(fileNameIndex).toString().toLower().contains(".mp"))
            {
                //find videoscreen
                //if not then create
                QGraphicsVideoItem *playerItem = nullptr;
                foreach (QGraphicsItem *childItem, mediaItem->childItems())
                {
                    if (childItem->data(itemTypeIndex).toString().contains("SubPlayer"))
                        playerItem = (QGraphicsVideoItem *)childItem;
                }

                if (playerItem != nullptr)
                {
                    playerItem = (QGraphicsVideoItem *)playerItem;
                    QMediaPlayer *m_player = (QMediaPlayer *)playerItem->mediaObject();

                    m_player->setMuted(!m_player->isMuted());
                }
            }
        }
    }
}

void AGView::onFastForward(QMediaPlayer *m_player)
{
    if (m_player->state() != QMediaPlayer::PausedState)
        m_player->pause();
    if (m_player->position() < m_player->duration() - AGlobal().frames_to_msec(1))
        m_player->setPosition(m_player->position() + AGlobal().frames_to_msec(1));
//    qDebug()<<"onNextKeyframeButtonClicked"<<m_player->position();
}

void AGView::onRewind(QMediaPlayer *m_player)
{
    if (m_player->state() != QMediaPlayer::PausedState)
        m_player->pause();
    if (m_player->position() > AGlobal().frames_to_msec(1))
        m_player->setPosition(m_player->position() - AGlobal().frames_to_msec(1));
//    qDebug()<<"onPreviousKeyframeButtonClicked"<<m_player->position();
}

void AGView::onSkipNext(QMediaPlayer *m_player)
{
//    int *prevRow = new int();
//    int *nextRow = new int();
//    int *relativeProgress = new int();
//    m_scrubber->progressToRow("V", int(m_player->position()), prevRow, nextRow, relativeProgress);

//    if (*nextRow == -1)
//        *nextRow = lastHighlightedRow;

//    int *relativeProgressl = new int();

//    if (*prevRow == *nextRow)
//        m_scrubber->rowToPosition("V", *nextRow+1, relativeProgressl);
//    else
//        m_scrubber->rowToPosition("V", *nextRow, relativeProgressl);

////    qDebug()<<"AVideoWidget::skipNext"<<m_player->position()<<*nextRow<<*relativeProgress<<*relativeProgressl;

//    if (*relativeProgressl != -1)
//    {
//        m_player->setPosition(*relativeProgressl);
//    }
}

void AGView::onSkipPrevious(QMediaPlayer *m_player)
{
//    int *prevRow = new int();
//    int *nextRow = new int();
//    int *relativeProgress = new int();
//    m_scrubber->progressToRow("V", m_player->position(), prevRow, nextRow, relativeProgress);

//    if (*prevRow == -1)
//        *prevRow = lastHighlightedRow;

//    int *relativeProgressl = new int();

//    if (*prevRow == *nextRow)
//        m_scrubber->rowToPosition("V", *prevRow-1, relativeProgressl);
//    else
//        m_scrubber->rowToPosition("V", *prevRow, relativeProgressl);

////    qDebug()<<"AVideoWidget::skipPrevious"<<m_player->position()<<*prevRow<<*relativeProgress<<*relativeProgressl;

//    if (*relativeProgressl != -1)
//    {
//        m_player->setPosition(*relativeProgressl);
//    }
}

void AGView::onStop(QMediaPlayer *m_player)
{
    qDebug()<<"AVideoWidget::onStop"<<m_player->state();

    m_player->stop();
}

void AGView::onMute(QMediaPlayer *m_player)
{
    m_player->setMuted(!m_player->isMuted());
}

void AGView::onSetSourceVideoVolume(QMediaPlayer *m_player, int volume)
{
//    sourceVideoVolume = volume;

//    QString fileNameLow = selectedFileName.toLower();
//    int lastIndexOf = fileNameLow.lastIndexOf(".");
//    QString extension = fileNameLow.mid(lastIndexOf + 1);

//    bool exportFileFound = false;
//    foreach (QString exportMethod, AGlobal().exportMethods)
//        if (fileNameLow.contains(exportMethod))
//            exportFileFound = true;

//    if (!(AGlobal().audioExtensions.contains(extension) || exportFileFound))
//        m_player->setVolume(volume);
}

void AGView::onSetPlaybackRate(QMediaPlayer *m_player, qreal rate)
{
    m_player->setPlaybackRate(rate);
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
    QPointF lastClipNextPos = nextPos;

    int mediaDuration = parentItem->data(mediaDurationIndex).toInt();

    bool alternator = false;
    bool firstClip = true;

//    if (parentItem == rootItem)
//        qDebug()<<"arrangeItems"<<parentItemType<<itemToString(parentItem)<<parentMediaType;

    foreach (QGraphicsItem *childItem, parentItem->childItems())
    {
//        qDebug()<<"childItem before"<<itemToString(childItem)<<nextPos;
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
                    nextPos = QPointF(parentItem->boundingRect().width() + spaceBetween, 100); //timelineView mode: first clip next to timelineGroup (parent)
                else //mediaFile
                {
                    nextPos = QPointF(0, parentItem->boundingRect().height() + spaceBetween + 100); //spotView mode: first clip below mediafile (parent)
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
        else if (childItemType == "SubProgressline") //progressline
        {
            QGraphicsLineItem *lineItem = (QGraphicsLineItem *)childItem;

//            qDebug()<<"SubProgressline"<<itemToString(parentItem)<<itemToString(childItem)<<parentItem->boundingRect();

            nextPos = QPointF(0, parentItem->boundingRect().height() - lineItem->pen().width()/ 2.0 );
        }
        else if (childItemType == "SubDurationLine") //progressline
        {
//            QGraphicsLineItem *lineItem = (QGraphicsLineItem *)childItem;

            nextPos = QPointF(0, 0 );
        }
        else if (childItemType.contains("SubName")) //other subs (subpic and subtxt and subplayer)
        {
            nextPos = QPointF(parentItem->boundingRect().height() * 0.1, parentItem->boundingRect().height() );
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
        if (childMediaType == "Folder" || childMediaType.contains("Group"))
            nextPos = QPointF(nextPos.x(), nextPos.y() + rectChildren.height() + spaceBetween); //vertical
        else if (childMediaType == "Clip" && parentMediaType == "TimelineGroup")
        {
            int transitionTimeFrames = QSettings().value("transitionTime").toInt();
            int frameRate = QSettings().value("frameRate").toInt();
            double transitionTimeMSec = 1000 * transitionTimeFrames / frameRate;
            nextPos = QPointF(nextPos.x() + rectChildren.width() - transitionTimeMSec / 100, nextPos.y() + (alternator?-25:25)); //horizontal alternating //
        }
        else if (childMediaType == "Clip" || childMediaType == "MediaFile")
            nextPos = QPointF(nextPos.x() + rectChildren.width() + spaceBetween, nextPos.y()); //horizontal
        else if (childMediaType == "Tag")
        {
            nextPos = QPointF(nextPos.x(), nextPos.y() + childItem->boundingRect().height()); //vertical next tag below previous tag
//            qDebug()<<"nexttag"<<itemToString(childItem)<<nextPos;
        }

        if (childMediaType == "Clip")
            lastClipNextPos = nextPos;

        alternator = !alternator;
        }
    } //foreach

    if (parentMediaType == "MediaFile" && parentItemType == "Base") //childs have been arranged now
    {
//        qDebug()<<"checkSpotView"<<itemToString(parentRectItem)<<viewMode<<parentRectItem->rect()<<nextPos;
        if (viewMode == "fileView")
            parentRectItem->setRect(QRectF(parentRectItem->rect().x(), parentRectItem->rect().y(), qMax(parentRectItem->rect().width(), lastClipNextPos.x()), parentRectItem->rect().height()));
            //if coming from timelineview,. the childrenboundingrect is still to big...
        else //timelineView
            parentRectItem->setRect(QRectF(parentRectItem->rect().x(), parentRectItem->rect().y(), qMax(200.0, parentRectItem->rect().width()), parentRectItem->rect().height()));
//            parentRectItem->setRect(QRectF(parentRectItem->rect().x(), parentRectItem->rect().y(), 200, parentRectItem->rect().height()));

        foreach (QGraphicsItem *childItem, parentItem->childItems())
        {

            if (childItem->data(itemTypeIndex).toString() == "SubName")
            {
                QGraphicsTextItem *textItem = (QGraphicsTextItem *)childItem;
                textItem->setTextWidth(parentItem->boundingRect().width() * 0.9);
            }
        }

        //reallign all poly's
        if (mediaDuration != 0)
        {
            foreach (QGraphicsItem *childItem, scene->items())
            {
                if (childItem->focusProxy() == parentItem) //find the clips
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
