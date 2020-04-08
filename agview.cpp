#include "aglobal.h"
#include "agmediarectangleitem.h"
#include "agview.h"

#include <QGraphicsItem>

#include <QDebug>
#include <QGraphicsVideoItem>
#include <QMediaService>
#include <QMediaMetaData>
#include <QKeyEvent>
#include <QTimeLine>
#include <QScrollBar>
#include <QGraphicsBlurEffect>
#include <QTime>
#include <QSettings>
#include <QAudioProbe>
#include <QDialog>
#include <QVBoxLayout>
#include <QTimer>

#include <qmath.h>

AGView::AGView(QWidget *parent)
    : QGraphicsView(parent)
{
    scene = new QGraphicsScene(this);
    setScene(scene);

    connect(scene, &QGraphicsScene::selectionChanged, this, &AGView::onSelectionChanged);

    pseudoCreateDate = QDateTime::fromString("1970-01-02", "yyyy-MM-dd");

//    QPixmap bgPix(":/images/karton.jpg");
//    setBackgroundBrush(bgPix);

}

AGView::~AGView()
{
//    qDebug()<<"AGView::~AGView"; //not called yet!

    clearAll();
}

void AGView::clearAll()
{
    if (!playInDialog)
        stopAndDeleteAllPlayers();
    else
    {
        if (dialogMediaPlayer != nullptr)
            dialogMediaPlayer->stop();
    }

    scene->clear();
}

void AGView::stopAndDeleteAllPlayers()
{
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

//                qDebug()<<"Player stopped"<<itemToString(item);

                m_player->stop();
                delete m_player;

                delete playerItem;

            }
        }
    }
}

void AGView::onSelectionChanged()
{
//    foreach (QGraphicsItem *item, scene->selectedItems())
//        qDebug()<<"AGView::onSelectionChanged()"<<itemToString(item);

    if (scene->selectedItems().count() == 1)
    {
        emit itemSelected(scene->selectedItems().first()); //triggers MainWindow::onGraphicsItemSelected
    }
}

QString AGView::itemToString(QGraphicsItem *item)
{
    return item->data(folderNameIndex).toString() + " " + item->data(fileNameIndex).toString() + " " + item->data(itemTypeIndex).toString() + " " + item->data(mediaTypeIndex).toString() + " " + item->data(clipTagIndex).toString();
}

void AGView::updateToolTip(QGraphicsItem *item)
{
    QString tooltipText = "";

    QString commandControl = "Ctrl-";
#ifdef Q_OS_MAC
    commandControl = "⌘-";
#endif


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
        tooltipText += tr("<p><b>Filegroup %1</b></p>"
                       "<p><i>Grouping of %1 files in folder %2</i></p>"
                          "<ul>"
                          "<li><b>Sorting</b>: Files are sorted by their <u>creation date</u>. Files without creation date are shown first, in alphabetic order.</li>"
                          ).arg(tooltipItem->data(fileNameIndex).toString(), tooltipItem->data(folderNameIndex).toString());
        if (tooltipItem->data(fileNameIndex).toString() == "Video")
            tooltipText += tr("<li><b>Supported Video files</b>: %1</li>"
                              "</ul>").arg(AGlobal().videoExtensions.join(","));
        else if (tooltipItem->data(fileNameIndex).toString() == "Audio")
            tooltipText += tr("<li><b>Supported Audio files</b>: %1</li>"
                              "</ul>").arg(AGlobal().audioExtensions.join(","));
        else if (tooltipItem->data(fileNameIndex).toString() == "Image")
            tooltipText += tr("<li><b>Supported Image files</b>: %1</li>"
                              "</ul>").arg(AGlobal().imageExtensions.join(","));
        else if (tooltipItem->data(fileNameIndex).toString() == "Export")
            tooltipText += tr("<li><b>Export files</b>: Videos generated by ACVC (lossless or encode). Can be openened by common video players, uploaded to social media etc.</li>"
                              "<li><b>Supported Export methods</b>: %1</li>"
                              "<li><b>Supported Export files</b>: %2</li>"
                              "</ul>").arg(AGlobal().exportMethods.join(","), AGlobal().exportExtensions.join(","));
        else if (tooltipItem->data(fileNameIndex).toString() == "Project")
            tooltipText += tr("<li><b>Project files</b>: generated by ACVC. Can be openened by video editors. Currently Shotcut and Premiere.</li>"
                              "<li><b>Supported Export methods</b>: %1</li>"
                              "<li><b>Supported Export files</b>: %2</li>"
                              "</ul>").arg(AGlobal().exportMethods.join(","), AGlobal().projectExtensions.join(","));
        else if (tooltipItem->data(fileNameIndex).toString() == "Parking")
            tooltipText += tr("<li><b>Parking</b>: Mediafiles or clips which do not match the filter criteria are moved here (see the search field).</li>"
                              "<li><b>Timeline view</b>: Mediafiles without clips are moved to the parking (In spotview they are shown in the video and audio group).</li>"
                              "</ul>");
        else
            tooltipText += tr("<li><b>Group</b>: %2</li>"
                              "</ul>").arg(tooltipItem->data(fileNameIndex).toString());
    }
    else if (tooltipItem->data(mediaTypeIndex) == "TimelineGroup")
    {
        tooltipText += tr("<p><b>Timeline group of %1 of folder %2</b></p>"
                       "<p><i>Showing all clips next to each other</i></p>"
                          "<ul>"
                          "<li><b>Sorting</b>: Currently sorted in order of mediafiles and within a mediafile, sorted chronologically</li>"
                          "<li><b>Transition</b>: If transition defined (%3) then clips overlapping.</li>"
                          ).arg(tooltipItem->parentItem()->data(fileNameIndex).toString(), tooltipItem->data(folderNameIndex).toString(), AGlobal().frames_to_time(QSettings().value("transitionTime").toInt()));
    }
    else if (tooltipItem->data(mediaTypeIndex) == "MediaFile")
    {
        tooltipText += tr("<p><b>Media file %2</b></p>"
                       "<p><i></i></p>"
                       "<ul>"
            "<li><b>Foldername</b>: %1</li>").arg(tooltipItem->data(folderNameIndex).toString(), tooltipItem->data(fileNameIndex).toString());

        if (tooltipItem->data(mediaDurationIndex).toInt() != 0)
        {
            tooltipText += tr("<li><b>Duration</b>: %1 (%2 s)</li>").arg(AGlobal().msec_to_time(tooltipItem->data(mediaDurationIndex).toInt()), QString::number(tooltipItem->data(mediaDurationIndex).toInt() / 1000.0));
            tooltipText += tr("<li><b>Duration line</b>: Red line above. 1 cm is about 30 seconds (unscaled, depending on display)</li>");
            tooltipText += tr("<li><b>Progress line</b>: Red line below</li>");
        }

        if (tooltipItem->data(mediaWithIndex).toInt() != -1)
            tooltipText += tr("<li><b>Size</b>: %1 * %2</li>").arg(QString::number(tooltipItem->data(mediaWithIndex).toInt()), QString::number(tooltipItem->data(mediaHeightIndex).toInt()));

        QStringList exiftoolProperties = tooltipItem->data(exifToolMetaIndex).toString().split(",");
        if (exiftoolProperties.count() != 0)
        {
            foreach (QString exiftoolProperty, exiftoolProperties) {
                QStringList keyValue = exiftoolProperty.split(" = ");
                if (keyValue.count() == 2)
                    tooltipText += tr("<li><b>%1</b>: %2</li>").arg(keyValue[0],keyValue[1]);
            }
        }

        tooltipText += tr("</ul>"
            "<p><b>Actions</p>"
            "<ul>"
                          "<li><b>Start playing</b>: Click %1 to start playing (background color will change to another tint of orange-ish)</li>"
                          "<ul>"
                          "<li><b>Play / pause</b>: Click %1 to toggle play and pause</li>"
                          "<li><b>Change playing position</b>: %2Hover over %1</li>"
                          "</ul>"
            "<li><b>More actions and details</b>: Click on audio or video and detailed actions and properties are shown in right pane</li>"
            "</ul>").arg(tooltipItem->data(mediaTypeIndex).toString(), commandControl);

        tooltipText += tr("<li><b>Parent</b>: %1</li>").arg(itemToString(tooltipItem->parentItem()));
        tooltipText += tr("<li><b>Proxy</b>: %1</li>").arg(itemToString(tooltipItem->focusProxy()));
        tooltipText += tr("<li><b>zValue</b>: %1</li>").arg(QString::number(tooltipItem->zValue()));
        tooltipText += tr("<li><b>filtered</b>: %1</li>").arg(tooltipItem->data(excludedInFilter).toBool());

    }
    else if (tooltipItem->data(mediaTypeIndex) == "Clip")
    {
        tooltipText += tr("<p><b>Clip from %4 to %5</b></p>"
                       "<ul>"
            "<li><b>Filename</b>: %1</li>"
             "<li><b>Duration</b>: %2 (%3 s)</li>"
                       "</ul>").arg(tooltipItem->data(fileNameIndex).toString()
                                    , AGlobal().msec_to_time(tooltipItem->data(mediaDurationIndex).toInt()), QString::number(tooltipItem->data(mediaDurationIndex).toInt() / 1000.0)
                                    , AGlobal().msec_to_time(tooltipItem->data(clipInIndex).toInt()), AGlobal().msec_to_time(tooltipItem->data(clipOutIndex).toInt())
                         );

        tooltipText += tr("</ul>"
            "<p><b>Actions</p>"
            "<ul>"
                          "<li><b>Start playing</b>: Click %1 to start playing (background color will change to another tint of orange-ish)</li>"
                          "<ul>"
                          "<li><b>Play / pause</b>: Click %1 to toggle play and pause</li>"
                          "<li><b>Change playing position</b>: %2Hover over %1</li>"
                          "</ul>"
            "<li><b>More actions and details</b>: Click on audio or video and detailed actions and properties are shown in right pane</li>"
            "</ul>").arg(tooltipItem->data(mediaTypeIndex).toString(), commandControl);

        tooltipText += tr("<li><b>Parent</b>: %1</li>").arg(itemToString(tooltipItem->parentItem()));
        tooltipText += tr("<li><b>Proxy</b>: %1</li>").arg(itemToString(tooltipItem->focusProxy()));
        tooltipText += tr("<li><b>zValue</b>: %1</li>").arg(QString::number(tooltipItem->zValue()));
        tooltipText += tr("<li><b>filtered</b>: %1</li>").arg(tooltipItem->data(excludedInFilter).toBool());

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

void AGView::addItem(QString parentName, QString mediaType, QString folderName, QString fileName, int duration, int clipIn, int clipOut, QString tag)
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

    int lastIndexOf = fileName.lastIndexOf(".");
    QString fileNameWithoutExtension = fileName.left(lastIndexOf); //for .srt files as they have videname as fileNameIndex value

    QGraphicsItem *parentItem = nullptr;
    QGraphicsItem *proxyItem = nullptr;
    //find the parentitem

    foreach (QGraphicsItem *item, scene->items())
    {
        if (item->data(itemTypeIndex).toString() == "Base")
        {
            //if clip added then find the corresponding mediafile
            if (mediaType == "Clip" && item->data(mediaTypeIndex).toString() == "MediaFile" && item->data(folderNameIndex).toString() == folderName && item->data(fileNameIndex).toString().contains(fileNameWithoutExtension))
            {
                proxyItem = item;
                if (fileName != item->data(fileNameIndex).toString())
                {
//                    qDebug()<<"Rename"<<mediaType<<fileName<<item->data(fileNameIndex).toString();
                    fileName = item->data(fileNameIndex).toString();//adjust the fileName to the filename of the corresponding mediaFile
                }
            }
            if (mediaType == "Tag" && item->data(mediaTypeIndex).toString() == "Clip" && item->data(folderNameIndex).toString() == folderName && item->data(fileNameIndex).toString().contains(fileNameWithoutExtension))
            {
                if (fileName != item->data(fileNameIndex).toString())
                    fileName = item->data(fileNameIndex).toString();//adjust the fileName to the filename of the corresponding mediaFile
            }

            if (item->data(mediaTypeIndex).toString() == parentMediaType)
            {
                if (mediaType == "Folder" && item->data(folderNameIndex).toString() + item->data(fileNameIndex).toString() + "/" == folderName)
                    parentItem = item;
                else if (mediaType == "FileGroup" && item->data(folderNameIndex).toString() + item->data(fileNameIndex).toString() + "/" == folderName)
                    parentItem = item;
                else if (mediaType == "TimelineGroup" && item->data(folderNameIndex).toString() == folderName && item->data(fileNameIndex).toString() == parentName)
                    parentItem = item;
                else if (mediaType == "MediaFile" && item->data(folderNameIndex).toString() == folderName  && item->data(fileNameIndex).toString() == parentName)
                    parentItem = item;
                else if (mediaType == "Clip" && item->data(folderNameIndex).toString() == folderName && item->data(fileNameIndex).toString().contains(fileNameWithoutExtension)) //MediaFile (do not match extension as clips can be called with .srt)
                    parentItem = item;
//                    qDebug()<<"Parent"<<mediaType<<folderName<<fileName<<itemToString(parentItem);
                else if (mediaType == "Clip" && item->data(folderNameIndex).toString() == folderName && item->data(fileNameIndex).toString() == parentName) //Timeline
                    parentItem = item;
                else if (mediaType == "Tag" && item->data(folderNameIndex).toString() == folderName && item->data(fileNameIndex).toString().contains(fileNameWithoutExtension) && item->data(clipInIndex).toInt() ==  clipIn)
                    parentItem = item;
            }
        }
    }
    QString fileNameLow = fileName.toLower();
    QString extension = fileNameLow.mid(lastIndexOf + 1);

//    if (parentItem == nullptr)
//        qDebug()<<"AGView::addItem"<<parentName<<mediaType<<folderName<<fileName<<parentMediaType<<itemToString(parentItem);

    QGraphicsItem *childItem = nullptr;

    if (mediaType == "Folder")
    {
        QGraphicsRectItem *rectItem = new QGraphicsRectItem(parentItem);

        QPen pen(Qt::transparent);
        rectItem->setPen(pen);

        rectItem->setRect(QRectF(0, 0, 200 * 9 / 16 + 4, 200 * 9 / 16)); //+2 is minimum to get dd-mm-yyyy not wrapped (+2 extra to be sure)
//        rectItem->setBrush(Qt::darkYellow);

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

        QString newFileName = fileName;
        if (mediaType == "TimelineGroup")
        {
            newFileName = parentItem->data(fileNameIndex).toString(); //filename of timelinegroup is Video/Audio etc.
            rectItem->setRect(QRectF(0, 0, 0, 225));

            parentItem->setFocusProxy(rectItem); //FileGroupItem->focusProxy == TimelineGroup

        }
        else //fileGroup
        {
            rectItem->setRect(QRectF(0, 0, 200 * 9 / 16, 200 * 9 / 16));

            QGraphicsPixmapItem *pictureItem = new QGraphicsPixmapItem(rectItem);
            QImage image = QImage(":/images/ACVCFolder.png");
            QPixmap pixmap = QPixmap::fromImage(image);//.scaled(QSize(200,200 * myImage.height() / myImage.width()))
            pictureItem->setPixmap(pixmap);
            pictureItem->setScale(200.0 * 9.0 / 16.0 / image.height() * 0.8);
            setItemProperties(pictureItem, "MediaFile", "SubPicture", folderName, fileName, duration);


            QGraphicsColorizeEffect *bef = new QGraphicsColorizeEffect();
            if (fileName == "Audio")
                bef->setColor(Qt::darkGreen);
            else if (fileName == "Image")
                bef->setColor(Qt::yellow);
            else if (fileName == "Project")
                bef->setColor(Qt::cyan);
            else if (fileName == "Parking")
                bef->setColor(Qt::red);
            else //video
                bef->setColor(Qt::blue);
            pictureItem->setGraphicsEffect(bef); //make it significantly slower

            if (fileName == "Parking")
                parentItem->setFocusProxy(rectItem); //Folder->focusProxy == FileGroupParking
        }
//        rectItem->setBrush(Qt::darkCyan);
        QPen pen(Qt::transparent);
        rectItem->setPen(pen);

        setItemProperties(rectItem, mediaType, "Base", folderName, newFileName, duration);

        childItem = rectItem;
    }
    else if (mediaType == "MediaFile")
    {
        AGMediaRectangleItem *mediaItem = new AGMediaRectangleItem(parentItem);
        if (parentItem->data(fileNameIndex).toString() == "Export")
            mediaItem->setRect(QRectF(0, 0, qMax(duration  * scaleFactor, 200.0), 200 * 9 / 16)); //scale same as clips for export mediafiles
        else
            mediaItem->setRect(QRectF(0, 0, qMax(duration  * scaleFactor / 5.0, 200.0), 200 * 9 / 16)); //scale 5 times smaller

        QBrush brush;
        brush.setColor(Qt::red);
//        brush.setStyle(Qt::LinearGradientPattern);
//        brush.set

//        mediaItem->setBrush(Qt::red); //QBrush("#FF4500")orange
//        mediaItem->setOpacity(10);
//        QGraphicsDropShadowEffect *bef = new QGraphicsDropShadowEffect();
//        mediaItem->setGraphicsEffect(bef); //make it significantly slower

        int alpha = 60;
        if (AGlobal().audioExtensions.contains(extension))
            mediaItem->setBrush(QColor(0, 128, 0, alpha)); //darkgreen
        else if (AGlobal().imageExtensions.contains(extension))
            mediaItem->setBrush(QColor(218, 130, 42, alpha));
        else if (AGlobal().projectExtensions.contains(extension))
            mediaItem->setBrush(QColor(130, 218, 42, alpha));
        else
            mediaItem->setBrush(QColor(42, 130, 218, alpha)); //#2a82da blue-ish

//        connect(mediaItem, &AGClipRectangleItem::agItemChanged, this, &AGView::onClipItemChanged);
//        connect(mediaItem, &AGClipRectangleItem::agMouseReleased, this, &AGView::onClipMouseReleased);
        connect(mediaItem, &AGMediaRectangleItem::clipPositionChanged, this, &AGView::onClipPositionChanged);
        connect(mediaItem, &AGMediaRectangleItem::itemClicked, this, &AGView::onItemClicked);

        mediaItem->setFocusProxy(parentItem);

        setItemProperties(mediaItem, mediaType, "Base", folderName, fileName, duration, QSize(), 0, duration);

        if (AGlobal().projectExtensions.contains(extension))
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
        if (parentItem == nullptr) //workaround in case srt file found without a mediafile
            return;

        AGMediaRectangleItem *clipItem = new AGMediaRectangleItem(parentItem);
        clipItem->setRect(QRectF(0, 0, duration  * scaleFactor, 100));

        int alpha = 125;
        if (AGlobal().audioExtensions.contains(extension))
            clipItem->setBrush(QColor(0, 128, 0, alpha)); //darkgreen
        else if (AGlobal().imageExtensions.contains(extension))
            clipItem->setBrush(QColor(218, 130, 42, alpha));
        else if (AGlobal().projectExtensions.contains(extension))
            clipItem->setBrush(QColor(130, 218, 42, alpha));
        else
            clipItem->setBrush(QColor(42, 130, 218, alpha)); //#2a82da blue-ish

        clipItem->setFocusProxy(proxyItem);

//        connect(clipItem, &AGMediaRectangleItem::agItemChanged, this, &AGView::onClipItemChanged);
//        connect(clipItem, &AGMediaRectangleItem::agMouseReleased, this, &AGView::onClipMouseReleased);
        connect(clipItem, &AGMediaRectangleItem::clipPositionChanged, this, &AGView::onClipPositionChanged);
        connect(clipItem, &AGMediaRectangleItem::itemClicked, this, &AGView::onItemClicked);

        setItemProperties(clipItem, mediaType, "Base", folderName, fileName, duration, QSize(), clipIn, clipOut);
//        qDebug()<<"setFocusProxy"<<itemToString(clipItem)<<itemToString(parentItem)<<itemToString(proxyItem);

        QGraphicsLineItem *durationLine = new QGraphicsLineItem(clipItem);

//        QBrush brush;
//        brush.setColor(Qt::red);
//        brush.setStyle(Qt::SolidPattern);

        QPen pen;
        pen.setWidth(5);
        pen.setBrush(Qt::red);

        durationLine->setPen(pen);

        durationLine->setLine(QLineF(durationLine->pen().width() / 2.0, 0, duration  * scaleFactor / 5.0, 0));
//            durationLine->setLine(QLineF(durationLine->pen().width()/2, 0, qMax(durationLine->parentItem()->boundingRect().width() - durationLine->pen().width()/2.0,durationLine->pen().width()/2.0), 0));

        setItemProperties(durationLine, mediaType, "SubDurationLine", folderName, fileName, duration, QSize(), clipIn, clipOut);

        QDateTime createDate = QDateTime::fromString(proxyItem->data(createDateIndex).toString(), "yyyy:MM:dd HH:mm:ss");

//        qDebug()<<"setZValue"<<createDate<<itemToString(proxyItem);

        clipItem->setZValue(createDate.addMSecs(clipItem->data(clipInIndex).toInt()).toMSecsSinceEpoch());

        childItem = clipItem;

    }
    else if (mediaType == "Tag")
    {
        if (parentItem == nullptr) //workaround in case srt file found without a mediafile
            return;

        QGraphicsTextItem *tagItem = new QGraphicsTextItem(parentItem);
        if (tag == "*" || tag == "**" || tag == "***" || tag == "****" || tag == "*****")
            tagItem->setDefaultTextColor(Qt::yellow);
        else if (QSettings().value("theme") == "Black")
            tagItem->setDefaultTextColor(Qt::white);
        else
            tagItem->setDefaultTextColor(Qt::black);

//        if (AGlobal().audioExtensions.contains(extension))
//            tagItem->setHtml(QString("<div style='background-color: rgba(0, 128, 0, 0.5);'>") + tag + "</div>");//#008000 darkgreen
//        else
//            tagItem->setHtml(QString("<div style='background-color: rgba(42, 130, 218, 0.5);'>") + tag + "</div>");//blue-ish #2a82da
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
        emit itemSelected(rootItem);
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

        setItemProperties(childTextItem, mediaType, "SubName", folderName, fileName, duration);

        childTextItem->setTextWidth(childItem->boundingRect().width() * 0.8);

    }

    //position new item at the bottom (arrangeitem will put it right
//    childItem->setPos(QPointF(0, rootItem->boundingRect().height()));

    //    arrangeItems(nullptr);

    if (AGlobal().projectExtensions.contains(extension))
    {
        loadMediaCompleted ++;

        emit mediaLoaded(folderName, fileName);
    }
}

void AGView::deleteItem(QString mediaType, QString folderName, QString fileName)
{
//    qDebug()<<"AGView::deleteItem"<<folderName<<fileName<<mediaType;

    //if (mediaType is clip then find the original extension!) //return value?

    bool deletedItems = false;
    foreach (QGraphicsItem *item, scene->items())
    {
        bool matchOnFile;
        if (mediaType == "Clip")
        {
            int lastIndex = fileName.lastIndexOf(".");
            QString fileNameWithoutExtension = fileName.left(lastIndex);

            matchOnFile = item->data(fileNameIndex).toString().contains(fileNameWithoutExtension);
        }
        else
            matchOnFile = item->data(fileNameIndex) == fileName;

        if (item->data(folderNameIndex) == folderName && matchOnFile && item->data(itemTypeIndex) == "Base" && item->data(mediaTypeIndex) == mediaType)
        {
            if (!playInDialog)
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

    //                qDebug()<<"Player stopped"<<itemToString(item);

                    m_player->stop();
                    delete m_player;
                }
            }

//            qDebug()<<"  Item"<<itemToString(item)<<item;
//            scene->removeItem(item);
            delete item;
            deletedItems = true;

        }
    }

    if (deletedItems)
        arrangeItems(nullptr);
}

void AGView::setThemeColors(QColor color)
{
    foreach (QGraphicsItem *item, scene->items())
    {
        if (item->data(itemTypeIndex).toString() == "SubName" || item->data(mediaTypeIndex).toString() == "Tag")
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

void AGView::onItemClicked(QGraphicsRectItem *rectItem)
{
//    qDebug()<<"AGView::onItemClicked"<<itemToString(rectItem);

    QGraphicsRectItem *mediaItem = rectItem;
    if (rectItem->data(mediaTypeIndex) == "Clip")
    {
        mediaItem = (QGraphicsRectItem *)rectItem->focusProxy();
//        playMedia(mediaItem);//load if not already done
    }
    else
        mediaItem = rectItem;

    QString fileNameLow = mediaItem->data(fileNameIndex).toString().toLower();
    int lastIndexOf = fileNameLow.lastIndexOf(".");
    QString extension = fileNameLow.mid(lastIndexOf + 1);

    if (AGlobal().videoExtensions.contains(extension) || AGlobal().audioExtensions.contains(extension))
    {
        QString folderName = mediaItem->data(folderNameIndex).toString();
        QString fileName = mediaItem->data(fileNameIndex).toString();

        if (!playInDialog)
        {
            //find videoscreen
            QGraphicsVideoItem *playerItem = nullptr;
            foreach (QGraphicsItem *childItem, mediaItem->childItems())
            {
                if (childItem->data(itemTypeIndex).toString().contains("SubPlayer"))
                    playerItem = (QGraphicsVideoItem *)childItem;
            }

            if (playerItem == nullptr)
            {
//                mediaItem->setBrush(Qt::darkRed);

                playerItem = new QGraphicsVideoItem(mediaItem);
                playerItem->setSize(QSize(200 * 0.8, 200 * 9 / 16 * 0.8));
                playerItem->setPos(mediaItem->boundingRect().height() * 0.1, mediaItem->boundingRect().height() * 0.1);

                QMediaPlayer *m_player = new QMediaPlayer();
                connect(m_player, &QMediaPlayer::mediaStatusChanged, this, &AGView::onMediaStatusChanged);
                connect(m_player, QOverload<>::of(&QMediaPlayer::metaDataChanged), this, &AGView::onMetaDataChanged);
                connect(m_player, &QMediaPlayer::metaDataAvailableChanged, this, &AGView::onMetaDataAvailableChanged);
                connect(m_player, &QMediaPlayer::positionChanged, this, &AGView::onPositionChanged);
                m_player->setVideoOutput(playerItem);

                m_player->setMuted(AGlobal().videoExtensions.contains(extension));

                m_player->setMedia(QUrl::fromLocalFile(folderName + fileName));

                setItemProperties(playerItem, mediaItem->data(mediaTypeIndex).toString(), "SubPlayer", folderName, fileName, mediaItem->data(mediaDurationIndex).toInt());
            }
            else
            {
                QMediaPlayer *m_player = (QMediaPlayer *)playerItem->mediaObject();

                if (m_player->state() != QMediaPlayer::PlayingState)
                    m_player->play();
                else
                    m_player->pause();
            }
        }
        else  //playInDialog
        {
            if (playerDialog == nullptr)
            {
                playerDialog = new QDialog(this);
                playerDialog->setWindowTitle("ACVC Media player");
            #ifdef Q_OS_MAC
                playerDialog->setWindowFlag(Qt::WindowStaysOnTopHint); //needed for MAC / OSX
            #endif

                QRect savedGeometry = QSettings().value("Geometry").toRect();
                savedGeometry.setX(savedGeometry.x() + savedGeometry.width()/4);
                savedGeometry.setY(savedGeometry.y() + savedGeometry.height()/4);
                savedGeometry.setWidth(savedGeometry.width()/2);
                savedGeometry.setHeight(savedGeometry.height()/2);
                playerDialog->setGeometry(savedGeometry);

                dialogVideoWidget = new QVideoWidget(playerDialog);
                dialogMediaPlayer = new QMediaPlayer();
                connect(dialogMediaPlayer, &QMediaPlayer::mediaStatusChanged, this, &AGView::onMediaStatusChanged);
                connect(dialogMediaPlayer, QOverload<>::of(&QMediaPlayer::metaDataChanged), this, &AGView::onMetaDataChanged);
                connect(dialogMediaPlayer, &QMediaPlayer::metaDataAvailableChanged, this, &AGView::onMetaDataAvailableChanged);
                connect(dialogMediaPlayer, &QMediaPlayer::positionChanged, this, &AGView::onPositionChanged);
                dialogMediaPlayer->setVideoOutput(dialogVideoWidget);

                QVBoxLayout *m_pDialogLayout = new QVBoxLayout(this);

                m_pDialogLayout->addWidget(dialogVideoWidget);

                playerDialog->setLayout(m_pDialogLayout);

                connect(playerDialog, &QDialog::finished, this, &AGView::onPlayerDialogFinished);

                playerDialog->show();
            }

            if (!dialogMediaPlayer->media().request().url().toString().contains(folderName + fileName))
            {
                dialogMediaPlayer->setMuted(AGlobal().videoExtensions.contains(extension));

                dialogMediaPlayer->setMedia(QUrl::fromLocalFile(folderName + fileName));
            }
            else
            {
                if (dialogMediaPlayer->state() != QMediaPlayer::PlayingState)
                    dialogMediaPlayer->play();
                else
                    dialogMediaPlayer->pause();
            }
        }
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
    if (playInDialog && !checked) //from dialog to initem player
    {
//        dialogMediaPlayer->pause();
        if (playerDialog != nullptr)
            playerDialog->close();
        playInDialog = false;
    }
    if (!playInDialog && checked) //from initem to dialog player
    {
        //close all players
        stopAndDeleteAllPlayers();
        playInDialog = true;
//        playMedia((QGraphicsRectItem *)scene->selectedItems().first());
    }
}

void AGView::onClipPositionChanged(QGraphicsRectItem *rectItem, int progress)
{
    QGraphicsRectItem *mediaItem = nullptr;
    if (rectItem->data(mediaTypeIndex).toString() == "Clip")
        mediaItem = (QGraphicsRectItem *)rectItem->focusProxy();
    else if (rectItem->data(mediaTypeIndex).toString() == "MediaFile")
        mediaItem = rectItem;

    QGraphicsVideoItem *playerItem = nullptr;
    QGraphicsLineItem *progressLineItem = nullptr;

    if (mediaItem != nullptr)
        foreach (QGraphicsItem *childItem, mediaItem->childItems())
        {
            if (childItem->data(itemTypeIndex).toString().contains("SubPlayer"))
                playerItem = (QGraphicsVideoItem *)childItem;
            if (childItem->data(itemTypeIndex).toString().contains("SubProgressLine"))
                progressLineItem = (QGraphicsLineItem *)childItem;
        }

//    qDebug()<<"AGView::onClipPositionChanged"<<itemToString(mediaItem)<<itemToString(rectItem)<<itemToString(progressLineItem)<<progress;

    int duration;
    if (!playInDialog)
    {
        if (playerItem != nullptr)
        {
            QMediaPlayer *m_player = (QMediaPlayer *)playerItem->mediaObject();

            m_player->setPosition(progress);

            duration = m_player->duration();

    //            lineItem->setLine(QLineF(0, 0, lineItem->parentItem()->boundingRect().width() * progress / m_player->duration(), 0));
        }
    }
    else
    {
        dialogMediaPlayer->setPosition(progress);
        duration = dialogMediaPlayer->duration();
    }

    if (progressLineItem != nullptr)
        progressLineItem->setLine(QLineF(progressLineItem->pen().width()/2.0, 0, qMax(progressLineItem->parentItem()->boundingRect().width() * progress / duration - progressLineItem->pen().width() / 2.0, progressLineItem->pen().width() * 1.5), 0));
}

void AGView::onMediaLoaded(QString folderName, QString fileName, QImage image, int duration, QSize mediaSize, QString ffmpegMeta, QList<int> samples)
{
//    qDebug()<<"AGView::onMediaLoaded"<<folderName<<fileName<<duration<<mediaSize;//<<ffmpegMeta <<propertyPointer->toString()
    QGraphicsItem *mediaItem = nullptr;
    foreach (QGraphicsItem *item, scene->items())
    {
        if (item->data(folderNameIndex) == folderName && item->data(fileNameIndex) == fileName && item->data(itemTypeIndex) == "Base" && item->data(mediaTypeIndex) == "MediaFile")
            mediaItem = item;
    }

    if (mediaItem != nullptr)
    {
        QVariant  *createDatePointer = new QVariant();
        emit getPropertyValue(folderName + fileName, "CreateDate", createDatePointer);
        QDateTime createDate = QDateTime::fromString(createDatePointer->toString(), "yyyy:MM:dd HH:mm:ss");
        if (createDate.toSecsSinceEpoch() == -3600)
        {
            createDate = pseudoCreateDate;
            pseudoCreateDate = pseudoCreateDate.addDays(1);
        }

        mediaItem->setData(exifToolMetaIndex, "CreateDate = " + createDate.toString("yyyy:MM:dd HH:mm:ss") + ",SecsSinceEpoch = " + QString::number(createDate.toSecsSinceEpoch()));
        mediaItem->setData(createDateIndex, createDate.toString("yyyy:MM:dd HH:mm:ss"));

        mediaItem->setZValue(createDate.toSecsSinceEpoch());

        //for each clip
        foreach (QGraphicsItem *clipItem, scene->items())
        {
            if (clipItem->focusProxy() == mediaItem && clipItem->data(itemTypeIndex).toString() == "Base" && clipItem->data(mediaTypeIndex).toString() == "Clip") //find the clips
            {
                clipItem->setZValue(createDate.addMSecs(clipItem->data(clipInIndex).toInt()).toMSecsSinceEpoch());
            }
        }

        if (ffmpegMeta != "") //when called by onMetaDataChanged (from QMediaPlayer)
            mediaItem->setData(ffMpegMetaIndex, ffmpegMeta);

        if (duration != -1)
            setItemProperties(mediaItem, "MediaFile", "Base", folderName, fileName,  duration, mediaSize);

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

        QString fileNameLow = fileName;
        int lastIndexOf = fileNameLow.lastIndexOf(".");
        QString extension = fileNameLow.mid(lastIndexOf + 1);

        if (AGlobal().audioExtensions.contains(extension))
        {
            QPainterPath painterPath;
            painterPath.moveTo(0, 0);

            int counter = 0;
            foreach (int sample, samples)
            {
                painterPath.lineTo(qreal(counter) / samples.count() * duration * scaleFactor / 5.0, sample);//(1.0 * progressInMSeconds / durationInMSeconds) * durationInMSeconds / 100.0
                counter++;
            }


            QGraphicsPathItem *pathItem = new QGraphicsPathItem(mediaItem);
            pathItem->setPath(painterPath);
            setItemProperties(pathItem, "MediaFile", "SubWave", folderName, fileName, duration, mediaSize);
        }

        if (duration > 0)
        {
            if (durationLine == nullptr)
            {
                durationLine = new QGraphicsLineItem(mediaItem);

//                QBrush brush;
//                brush.setColor(Qt::red);
//                brush.setStyle(Qt::SolidPattern);

                QPen pen;
                pen.setWidth(5);
                pen.setBrush(Qt::red);

                durationLine->setPen(pen);

                setItemProperties(durationLine, "MediaFile", "SubDurationLine", folderName, fileName, duration, mediaSize);
            }

            durationLine->setLine(QLineF(durationLine->pen().width() / 2.0, 0, duration  * scaleFactor / 5.0 - durationLine->pen().width() / 2.0, 0));
    //            durationLine->setLine(QLineF(durationLine->pen().width()/2, 0, qMax(durationLine->parentItem()->boundingRect().width() - durationLine->pen().width()/2.0,durationLine->pen().width()/2.0), 0));

            if (playerItem != nullptr) //update duration from QMediaPlayer (needed for audio files)
                setItemProperties(playerItem, "MediaFile", "SubPlayer", folderName, fileName, duration, mediaSize);
        }

//        qDebug()<<"arrangeItems"<<itemToString(mediaItem->parentItem());
        arrangeItems(mediaItem->parentItem()); //arrange the folder / foldergroup

        loadMediaCompleted ++;

        emit mediaLoaded(folderName, fileName, image, duration, mediaSize, ffmpegMeta, samples);

    } //mediaItem != nullptr
}

QGraphicsItem * AGView::drawPoly(QGraphicsItem *clipItem)
{
//    qDebug()<<"AGView::drawPoly"<<itemToString(clipItem);
    QGraphicsPolygonItem *polyItem = nullptr;

    //find poly
    foreach (QGraphicsItem *item, clipItem->childItems())
    {
        if (item->data(itemTypeIndex) == "poly")
            polyItem = (QGraphicsPolygonItem *)item;
    }

    bool shouldDraw = true;
    if (clipItem->parentItem() != clipItem->focusProxy()) //if clip part of timeline
    {
        //if mediafile not in same FileGroup as clip then do not draw (in case of filtering where the clip is in parking and the mediafile not)
        QGraphicsItem *clipFileGroup = clipItem->parentItem()->parentItem();
        QGraphicsItem *mediaFileGroup = clipItem->focusProxy()->parentItem();

        if (clipFileGroup != mediaFileGroup)
        {
            shouldDraw = false;
        }
    }

    if (shouldDraw)
    {
        if (polyItem == nullptr)
        {
//            QBrush brush;
//            brush.setColor(Qt::lightGray);
//            brush.setStyle(Qt::SolidPattern);

            QPen pen(Qt::transparent);

            polyItem = new QGraphicsPolygonItem(clipItem);
            polyItem->setPen(pen);
            QColor color = Qt::lightGray;
            color.setAlpha(127);
            polyItem->setBrush(color);
            setItemProperties(polyItem, "Poly", "poly", clipItem->data(folderNameIndex).toString(), clipItem->data(fileNameIndex).toString(), clipItem->data(mediaDurationIndex).toInt());
        }

        if (polyItem != nullptr) //should always be the case here
        {
            QGraphicsRectItem *parentMediaFile = (QGraphicsRectItem *)clipItem->focusProxy(); //parent of the clip
            if (parentMediaFile != nullptr) //should always be the case
            {
                int clipIn = clipItem->data(clipInIndex).toInt();
                int clipOut = clipItem->data(clipOutIndex).toInt();
                int duration = parentMediaFile->data(mediaDurationIndex).toInt();

//                        qDebug()<<"AGView::drawPoly inout"<<clipIn<<clipOut<<duration<<itemToString(parentMediaFile);

                QPointF parentPoint1 = polyItem->mapFromItem(parentMediaFile, QPoint(duration==0?0:parentMediaFile->rect().width()*clipIn/duration, parentMediaFile->rect().height()));
                QPointF parentPoint2 = polyItem->mapFromItem(parentMediaFile, QPoint(duration==0?0:parentMediaFile->rect().width()*clipOut/duration, parentMediaFile->rect().height()));

//                    qDebug()<<"AGView::drawPoly parents"<<parentItem->data(itemTypeIndex).toString() << parentItem->data(mediaTypeIndex).toString() << parentItem->data(fileNameIndex).toString()<<parentMediaFile->data(itemTypeIndex).toString() << parentMediaFile->data(mediaTypeIndex).toString() << parentMediaFile->data(fileNameIndex).toString()<<parentMediaFile->boundingRect()<<parentPoint1;

//                    qDebug()<<"AGView::drawPoly pItem"<<polyItem->data(itemTypeIndex).toString() << polyItem->data(mediaTypeIndex).toString() << polyItem->data(fileNameIndex).toString()<<polyItem->polygon()<<parentPoint1;

                QPolygonF polyGon;
                polyGon.append(QPointF(parentPoint1.x(), parentPoint1.y()+1));
                polyGon.append(QPointF(-1,-1));
                polyGon.append(QPointF(clipItem->boundingRect().width(),-1));
                polyGon.append(QPointF(parentPoint2.x(), parentPoint2.y()+1));
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


void AGView::onMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    QMediaPlayer *m_player = qobject_cast<QMediaPlayer *>(sender());

//    qDebug()<<"AGView::onMediaStatusChanged"<<status<<m_player->metaData(QMediaMetaData::Title).toString()<<m_player->media().request().url()<<m_player->error()<<m_player->errorString();

    //find mediafile
//    foreach (QGraphicsItem *item, scene->items())
//    {
//        if (item->data(mediaTypeIndex).toString() == "MediaFile" && item->data(itemTypeIndex).toString() == "SubPlayer")
//        {
//            QGraphicsVideoItem *playerItem = (QGraphicsVideoItem *)item;

//            if ((QMediaPlayer *)playerItem->mediaObject() == m_player)
//            {
//                QGraphicsRectItem *mediaItem = (QGraphicsRectItem *)playerItem->parentItem();
////                mediaItem->setBrush(QBrush()); //orange-ish
//            }
//        }
//    }

    if (status == QMediaPlayer::LoadedMedia)
    {

        m_player->setNotifyInterval(AGlobal().frames_to_msec(1));

        QString folderFileNameLow = m_player->media().request().url().toString().toLower();
        int lastIndexOf = folderFileNameLow.lastIndexOf(".");
        QString extension = folderFileNameLow.mid(lastIndexOf + 1);

        if (AGlobal().videoExtensions.contains(extension))
        {

            if (m_player->state() != QMediaPlayer::PlayingState)
                m_player->play();
            else
                m_player->pause();
#ifdef Q_OS_MAC
        QSize s1 = size();
        QSize s2 = s1 + QSize(1, 1);
        resize(s2);// enlarge by one pixel
        resize(s1);// return to original size
#endif

        }
        else if (AGlobal().audioExtensions.contains(extension))
        {
            if (m_player->state() != QMediaPlayer::PlayingState)
                m_player->play();
            else
                m_player->pause();
        }

    }
//    else if (status == QMediaPlayer::EndOfMedia)
//    {
////        m_player->play();
//        m_player->setPosition(0);
//        m_player->pause();
////        onPositionChanged(0);
//    }
}

//class TimelineWaveform : public QQuickPaintedItem
//{
//    Q_OBJECT
//    Q_PROPERTY(QVariant levels MEMBER m_audioLevels NOTIFY propertyChanged)
//    Q_PROPERTY(QColor fillColor MEMBER m_color NOTIFY propertyChanged)
//    Q_PROPERTY(int inPoint MEMBER m_inPoint NOTIFY inPointChanged)
//    Q_PROPERTY(int outPoint MEMBER m_outPoint NOTIFY outPointChanged)

//public:
//    TimelineWaveform()
//    {
//        setAntialiasing(QPainter::Antialiasing);
//        connect(this, SIGNAL(propertyChanged()), this, SLOT(update()));
//    }

//    void paint(QPainter *painter)
//    {
//        QVariantList data = m_audioLevels.toList();
//        if (data.isEmpty())
//            return;

//        // In and out points are # frames at current fps,
//        // but audio levels are created at 25 fps.
//        // Scale in and out point to 25 fps.
//        const int inPoint = qRound(m_inPoint / MLT.profile().fps() * 25.0);
//        const int outPoint = qRound(m_outPoint / MLT.profile().fps() * 25.0);
//        const qreal indicesPrPixel = qreal(outPoint - inPoint) / width();

//        QPainterPath path;
//        path.moveTo(-1, height());
//        int i = 0;
//        for (; i < width(); ++i)
//        {
//            int idx = inPoint + int(i * indicesPrPixel);
//            if (idx + 1 >= data.length())
//                break;
//            qreal level = qMax(data.at(idx).toReal(), data.at(idx + 1).toReal()) / 256;
//            path.lineTo(i, height() - level * height());
//        }
//        path.lineTo(i, height());
//        painter->fillPath(path, m_color.lighter());

//        QPen pen(painter->pen());
//        pen.setColor(m_color.darker());
//        painter->strokePath(path, pen);
//    }

//signals:
//    void propertyChanged();
//    void inPointChanged();
//    void outPointChanged();

//private:
//    QVariant m_audioLevels;
//    int m_inPoint;
//    int m_outPoint;
//    QColor m_color;
//};

void AGView::onMetaDataChanged()
{
    QMediaPlayer *m_player = qobject_cast<QMediaPlayer *>(sender());

    QString folderFileName = m_player->media().request().url().toString();
#ifdef Q_OS_WIN
    folderFileName = folderFileName.replace("file:///", "");
#else
    folderFileName = folderFileName.replace("file://", ""); //on MAX / OSX foldername should be /users... not users...
#endif
    folderFileName = folderFileName.replace("file:", ""); //for network folders?

    int lastIndexOf = folderFileName.lastIndexOf("/");
    QString folderName = folderFileName.left(lastIndexOf + 1);
    QString fileName = folderFileName.mid(lastIndexOf + 1);
//    qDebug()<<"AGView::onMetaDataChanged"<<folderName<<fileName<<m_player->metaData(QMediaMetaData::Duration).toString()<<metadatalist.count();

    QImage image = QImage();
    QString fileNameLow = fileName.toLower();
    lastIndexOf = fileNameLow.lastIndexOf(".");
    QString extension = fileNameLow.mid(lastIndexOf + 1);

    if (AGlobal().audioExtensions.contains(extension))
    {
//        QAudioProbe *probe = new QAudioProbe();
//            probe->setSource(m_player);
//            connect(probe, SIGNAL(audioBufferProbed(QAudioBuffer)), this, SLOT(onAudioBufferProbed(QAudioBuffer)));

//        image = QImage(":/musicnote.png");

//        int channels = 2;
//        int n = 100; //playtime

//        QVariantList levels;

////        const char* key[2] = { "meta.media.audio_level.0", "meta.media.audio_level.1"};

//        for (int i = 0; i < n;i++)
//        for (int channel = 0; channel < channels; channel++)
//            // Convert real to uint for caching as image.
//            // Scale by 0.9 because values may exceed 1.0 to indicate clipping.
////            levels << 256 * qMin(frame->get_double(key[channel]) * 0.9, 1.0);
//            levels << QRandomGenerator::global()->bounded(256);//256 * i/n * 0.9; random

//        int count = levels.size();
//        QImage image2((count + 3) / 4 / channels, channels, QImage::Format_ARGB32);
//        n = image2.width() * image2.height();
//        for (int i = 0; i < n; i ++) {
//            QRgb p;
//            if ((4*i + 3) < count) {
//                p = qRgba(levels.at(4*i).toInt(), levels.at(4*i+1).toInt(), levels.at(4*i+2).toInt(), levels.at(4*i+3).toInt());
//            } else {
//                int last = levels.last().toInt();
//                int r = (4*i+0) < count? levels.at(4*i+0).toInt() : last;
//                int g = (4*i+1) < count? levels.at(4*i+1).toInt() : last;
//                int b = (4*i+2) < count? levels.at(4*i+2).toInt() : last;
//                int a = last;
//                p = qRgba(r, g, b, a);
//            }
//            image2.setPixel(i / 2, i % channels, p);
//        }
////        if (image2.isNull()) {
////            QImage image2(1, 1, QImage::Format_ARGB32);
////        }
//        if (!image2.isNull())
//            image = image2;
    }
    //onMediaLoaded not needed anymore as loadMedia also do this
//    onMediaLoaded(folderName, fileName, image, m_player->metaData(QMediaMetaData::Duration).toInt());

    //to do: for mp3, Author, Title and AlbumTitle if available

    foreach (QString metadata_key, m_player->availableMetaData())
    {
        QVariant var_data = m_player->metaData(metadata_key);
//        qDebug() <<"AGView::onMetaDataChanged" << metadata_key << var_data.toString();
    }
}

void AGView::onMetaDataAvailableChanged(bool available)
{
//    QMediaPlayer *m_player = qobject_cast<QMediaPlayer *>(sender());
//    qDebug()<<"AGView::onMetaDataAvailableChanged"<<available<<m_player->media().request().url()<<m_player->metaData(QMediaMetaData::Duration).toString();
}

void AGView::onPositionChanged(int progress)
{
    QMediaPlayer *m_player = qobject_cast<QMediaPlayer *>(sender());
    m_player->setProperty("test", "hi");

    QString folderFileName = m_player->media().request().url().toString();
#ifdef Q_OS_WIN
    folderFileName = folderFileName.replace("file:///", "");
#else
    folderFileName = folderFileName.replace("file://", ""); //on MAX / OSX foldername should be /users... not users...
#endif
    folderFileName = folderFileName.replace("file:", ""); //for network folders?

    int lastIndexOf = folderFileName.lastIndexOf("/");
    QString folderName = folderFileName.left(lastIndexOf + 1);
    QString fileName = folderFileName.mid(lastIndexOf + 1);

//    qDebug()<<"AGView::onPositionChanged"<<fileName<<progress<<m_player->duration()<<m_player->media().request().url().toString();

    QGraphicsItem *mediaItem = nullptr;
    foreach (QGraphicsItem *item, scene->items())
    {
        if (item->data(folderNameIndex) == folderName && item->data(fileNameIndex) == fileName && item->data(itemTypeIndex) == "Base" && item->data(mediaTypeIndex) == "MediaFile")
            mediaItem = item;
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

        if (progressLineItem == nullptr)
        {
            progressLineItem = new QGraphicsLineItem(mediaItem);

            QPen pen;
            pen.setWidth(10);
            pen.setBrush(Qt::red);
            progressLineItem->setPen(pen);

            setItemProperties(progressLineItem, "MediaFile", "SubProgressline", folderName, fileName, 0, QSize());
//            progressLineItem->setLine(QLineF(progressLineItem->pen().width()/2.0, 0, progressLineItem->pen().width()*1.5, 0));
            progressLineItem->setPos(0, mediaItem->boundingRect().height() - progressLineItem->pen().width()/ 2.0 );
        }

        if (progressLineItem != nullptr) // subProgressline already created in mediaLoaded
        {

//            lineItem->setLine(QLineF(lineItem->pen().width()/2, 0, lineItem->parentItem()->boundingRect().width() - lineItem->pen().width()/2.0, 0));
            progressLineItem->setLine(QLineF(progressLineItem->pen().width()/2.0, 0, qMax(progressLineItem->parentItem()->boundingRect().width() * progress / m_player->duration() - progressLineItem->pen().width() / 2.0, progressLineItem->pen().width() * 1.5), 0));
        }

        if (playerItem != nullptr) //move video to current position
        {
            double minX = mediaItem->boundingRect().height() * 0.1;
            double maxX = mediaItem->boundingRect().width() - playerItem->boundingRect().width() - minX;
            playerItem->setPos(qMin(qMax(mediaItem->boundingRect().width() * progress / m_player->duration() - playerItem->boundingRect().width() / 2.0, minX), maxX), playerItem->pos().y());
        }
    }
}

void AGView::reParent(QGraphicsItem *parentItem, QString prefix)
{
    if (parentItem->data(itemTypeIndex) == "Base")
    {
//        qDebug()<<"AGView::reParent"<<prefix + itemToString(parentItem);
        if (parentItem->data(mediaTypeIndex) == "MediaFile" && parentItem->parentItem()->data(fileNameIndex).toString() != "Export" && parentItem->parentItem()->data(fileNameIndex).toString() != "Project")
        {
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
    QGraphicsItem *selectedItem = nullptr;
    if (scene->selectedItems().count() == 1)
        selectedItem = scene->selectedItems().first();

    if (selectedItem != nullptr)
    {
        //find current position
//        qDebug()<<"Find position of"<<itemToString(selectedItem);
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

    if (scene->selectedItems().count() == 1)
    {
        QGraphicsRectItem *mediaItem = (QGraphicsRectItem *)scene->selectedItems().first();

        onItemClicked(mediaItem);
    }
}

void AGView::onMuteVideoButton(QMediaPlayer *m_player)
{
    m_player->setMuted(!m_player->isMuted());
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
    qDebug()<<"AGView::onStop"<<m_player->media().request().url();
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
    {
        parentItem = rootItem;
        reParent(rootItem);
    }

    int spaceBetween = 5;

    QString parentMediaType = parentItem->data(mediaTypeIndex).toString();
    QString parentItemType = parentItem->data(itemTypeIndex).toString();

    QGraphicsRectItem *parentRectItem = (QGraphicsRectItem *)parentItem;
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
        QGraphicsRectItem *parentRectItem = (QGraphicsRectItem *)parentItem;

        if (parentItem->parentItem()->data(fileNameIndex).toString() == "Export")
            parentRectItem->setRect(QRectF(0, 0, qMax(mediaDuration  * scaleFactor, 200.0), 200 * 9 / 16));
        else
            parentRectItem->setRect(QRectF(0, 0, qMax(mediaDuration  * scaleFactor / 5.0, 200.0), 200 * 9 / 16));
    }

    //set the children start position
    QPointF nextPos = QPointF(0, parentItem->boundingRect().height() + spaceBetween);
    QPointF lastClipNextPos = nextPos;

    bool alternator = false;
    bool firstChild = true;

//    if (parentItem == rootItem)
//        qDebug()<<"arrangeItems"<<parentItemType<<itemToString(parentItem)<<parentMediaType<<QSettings().value("viewMode")<<QSettings().value("viewDirection");

    foreach (QGraphicsItem *childItem, parentItem->childItems())
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
                if (parentMediaType == "MediaFile")
                {
                    //position of clipIn on MediaItem
                    int clipIn = childItem->data(clipInIndex).toInt();
                    int clipOut = childItem->data(clipOutIndex).toInt();
    //                qDebug()<<"position of clipIn on MediaItem"<<clipIn<<clipOut<<duration;
                    nextPos = QPointF(qMax(mediaDuration==0?0:(parentItem->boundingRect().width() * (clipIn+clipOut)/ 2.0 / mediaDuration - childItem->boundingRect().width() / 2.0), nextPos.x()), nextPos.y());
                }
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
        if (childMediaType == "Folder" || childMediaType.contains("Group"))
            nextPos = QPointF(nextPos.x(), nextPos.y() + rectChildren.height() + spaceBetween); //vertical
        else if (childMediaType == "Clip" && parentMediaType == "TimelineGroup")
        {
            int transitionTimeFrames = QSettings().value("transitionTime").toInt();
            int frameRate = QSettings().value("frameRate").toInt();
            double transitionTimeMSec = 1000 * transitionTimeFrames / frameRate;
            nextPos = QPointF(nextPos.x() + rectChildren.width() - transitionTimeMSec  * scaleFactor, nextPos.y() + (alternator?-0:0)); //horizontal alternating //
        }
        else if (childMediaType == "MediaFile")
        {
            if (parentItem->data(fileNameIndex) == "Export" || (QSettings().value("viewMode") == "SpotView" && parentItem->data(fileNameIndex) != "Project" && QSettings().value("viewDirection") == "Down")) //project files horizontal
                nextPos = QPointF(nextPos.x(), nextPos.y() + rectChildren.height() + spaceBetween); //vertical
            else
                nextPos = QPointF(nextPos.x() + rectChildren.width() + spaceBetween, nextPos.y()); //horizontal
        }
        else if (childMediaType == "Clip")
            nextPos = QPointF(nextPos.x() + rectChildren.width() + spaceBetween, nextPos.y()); //horizontal
        else if (childMediaType == "Tag")
        {
            nextPos = QPointF(nextPos.x(), nextPos.y() + childItem->boundingRect().height()); //vertical next tag below previous tag
//            qDebug()<<"nexttag"<<itemToString(childItem)<<nextPos;
        }

        if (childMediaType == "Clip")
        {
            lastClipNextPos = nextPos;
        }

        alternator = !alternator;
    } //foreach childitem

    if (parentMediaType == "MediaFile" && parentItemType == "Base") //childs have been arranged now
    {
//        qDebug()<<"checkSpotView"<<itemToString(parentRectItem)<<viewMode<<parentRectItem->rect()<<nextPos;
        if (QSettings().value("viewMode").toString() == "SpotView") //set size dependent on clips
        {
            parentRectItem->setRect(QRectF(parentRectItem->rect().x(), parentRectItem->rect().y(), qMax(parentRectItem->rect().width(), lastClipNextPos.x()), parentRectItem->rect().height()));
            //if coming from timelineview,. the childrenboundingrect is still to big...
        }
        else //timelineView: set size independent from clips
        {
            parentRectItem->setRect(QRectF(parentRectItem->rect().x(), parentRectItem->rect().y(), qMax(parentRectItem->rect().width(), 200.0), parentRectItem->rect().height()));
        }

        //reallign all poly's and check if parentitem has clips
        if (mediaDuration != 0)
        {
            foreach (QGraphicsItem *clipItem, scene->items())
            {
                if (clipItem->focusProxy() == parentItem && clipItem->data(itemTypeIndex).toString() == "Base" && clipItem->data(mediaTypeIndex).toString() == "Clip") //find the clips
                        drawPoly(clipItem); //must be after foreach childItem to get sizes right
            }
        }

        //adjust the textwidth
        foreach (QGraphicsItem *childItem, parentItem->childItems())
        {
            if (childItem->data(itemTypeIndex).toString() == "SubName")
            {
                QGraphicsTextItem *textItem = (QGraphicsTextItem *)childItem;
                textItem->setTextWidth(parentItem->boundingRect().width() * 0.9);
            }
            //to do, adjust waveform width
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
                foundMediaFile = true;

            foreach (QGraphicsItem *clipItem, scene->items())
            {
                if (clipItem->focusProxy() == item && clipItem->data(itemTypeIndex).toString() == "Base" && clipItem->data(mediaTypeIndex).toString() == "Clip") //find the clips
                {
                    bool foundTag = false;;
                    foreach (QGraphicsItem *tagItem, clipItem->childItems())
                    {
                        if (text == "" || (tagItem->data(mediaTypeIndex) == "Tag" && tagItem->data(clipTagIndex).toString().contains(text, Qt::CaseInsensitive)))
                            foundTag = true;
                    }

                    if (foundTag)
                        foundMediaFile = true;

                    clipItem->setData(excludedInFilter, !foundTag);
                }
            }

            item->setData(excludedInFilter, !foundMediaFile);
        }
    }
//    reParent(rootItem);
    arrangeItems(nullptr);
}

void AGView::setScaleFactorAndArrange(qreal scaleFactor)
{
    this->scaleFactor = scaleFactor;

    //redraw lines
    foreach (QGraphicsItem *item, scene->items())
    {
        QGraphicsLineItem *lineItem = (QGraphicsLineItem *)item;

        if (item->data(itemTypeIndex) == "SubDurationLine")
        {
//            qDebug()<<"AGView::setScaleFactorAndArrange"<<itemToString(lineItem)<<scaleFactor;
            lineItem->setLine(QLineF(lineItem->pen().width() / 2.0, 0, lineItem->data(mediaDurationIndex).toInt()  * scaleFactor / 5.0 - lineItem->pen().width() / 2.0, 0));
        }
        else if (item->data(itemTypeIndex) == "SubProgressLine")
        {
//            qDebug()<<"AGView::setScaleFactorAndArrange"<<itemToString(lineItem)<<scaleFactor;
            lineItem->setLine(QLineF(lineItem->pen().width() / 2.0, 0, lineItem->data(mediaDurationIndex).toInt()  * scaleFactor / 5.0 - lineItem->pen().width() / 2.0, 0));
        }
        else if (item->data(itemTypeIndex) == "SubWave")
        {
            QGraphicsPathItem *pathItem = (QGraphicsPathItem *)item;
            QPainterPath painterPath = pathItem->path();
//            qDebug()<<"AGView::setScaleFactorAndArrange"<<itemToString(pathItem)<<scaleFactor<<pathItem->data(mediaDurationIndex).toInt()<<1.0 / painterPath.elementCount() * pathItem->data(mediaDurationIndex).toInt() * scaleFactor / 5.0;
            QPainterPath newPainterPath;
            newPainterPath.moveTo(0,0);
            for (int i=0; i < painterPath.elementCount(); i++)
            {
                QPainterPath::Element element = painterPath.elementAt(i);
                newPainterPath.lineTo(qreal(i) / painterPath.elementCount() * pathItem->data(mediaDurationIndex).toInt() * scaleFactor / 5.0, element.y);
            }
            pathItem->setPath(newPainterPath);
        }

    }

    arrangeItems(nullptr);
}
