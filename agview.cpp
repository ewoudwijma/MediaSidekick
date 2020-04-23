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
#include <QProcess>
#include <QDir>
#include <QDesktopServices>
#include <QGroupBox>
#include <QLabel>
#include <QMenu>

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
//    qDebug()<<"AGView::clearAll"<<mediaFilesMap.count();
    if (!playInDialog)
        stopAndDeleteAllPlayers();
    else
    {
        if (dialogMediaPlayer != nullptr)
            dialogMediaPlayer->stop();
    }

    scene->clear();

    mediaFilesMap.clear();
//    foreach (AMediaStruct mediaStruct, mediaFilesMap) //alphabetically ordered! (instead of scene->items)
//    {
//        mediaStruct.mediaItem = nullptr;
//    }
}

void AGView::stopAndDeleteAllPlayers()
{
    foreach (QGraphicsItem *item, scene->items())
    {
        if (item->data(mediaTypeIndex).toString() == "MediaFile" && item->data(itemTypeIndex).toString() == "Base")
        {
            QGraphicsVideoItem *playerItem = nullptr;
            QGraphicsLineItem *progressLineItem = nullptr;
            foreach (QGraphicsItem *childItem, item->childItems())
            {
                if (childItem->data(itemTypeIndex).toString().contains("SubPlayer"))
                    playerItem = (QGraphicsVideoItem *)childItem;
                if (childItem->data(itemTypeIndex).toString().contains("SubProgressLine"))
                    progressLineItem = (QGraphicsLineItem *)childItem;
            }

            if (playerItem != nullptr)
            {
                QMediaPlayer *m_player = (QMediaPlayer *)playerItem->mediaObject();

//                qDebug()<<"Player stopped"<<itemToString(item);

                m_player->stop();
                delete m_player;

                delete playerItem;
            }
            if (progressLineItem != nullptr)
            {
                delete progressLineItem;
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

QString AGView::itemToString(QGraphicsItem *item)
{
    return item->data(folderNameIndex).toString() + " " + item->data(fileNameIndex).toString() + " " + item->data(itemTypeIndex).toString() + " " + item->data(mediaTypeIndex).toString() + " " + item->data(clipTagIndex).toString() + " " + QString::number(item->zValue());
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
    {
        tooltipText += tr("<p><b>Folder %1</b></p>"
                       "<p><i>Folder with its subfolders and media files. Only <b>folders containing mediafiles</b> are shown</i></p>"
                          "<p><b>Properties</p>"
                       "<ul>"
            "<li><b>Foldername</b>: %1</li>"
                          "</ul>").arg(tooltipItem->data(folderNameIndex).toString() + tooltipItem->data(fileNameIndex).toString());
        tooltipText += tr("<p><b>Help</p>"
                    "<ul>"
                      "<li><b>Actions</b>: Right click</li>"
                          "<li><b>ACVC recycle bin</b>: not shown. Right click / Open in explorer will show the ACVC recycle bin</li>"
                      "</ul>"
        );
    }
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
                          "<p><b>Properties</p>"
                       "<ul>"
            "<li><b>Foldername</b>: %1</li>"
                          "<li><b>Filename</b>: %2</li>"
                          "<li><b>Create date</b>: %3</li>").arg(tooltipItem->data(folderNameIndex).toString(), tooltipItem->data(fileNameIndex).toString(), tooltipItem->data(createDateIndex).toString());

        if (tooltipItem->data(mediaDurationIndex).toInt() != 0)
        {
            tooltipText += tr("<li><b>Duration</b>: %1 (%2 s)</li>").arg(AGlobal().msec_to_time(tooltipItem->data(mediaDurationIndex).toInt()), QString::number(tooltipItem->data(mediaDurationIndex).toInt() / 1000.0));
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
            "<p><b>Help</p>"
            "<ul>"
                          "<li><b>Actions</b>: Right click</li>"
                          "<li><b>Duration line</b>: Red or green line above. Allows comparison of duration of media files and clips</li>"
                          "<li><b>Progress line</b>: Red line below</li>"
                          "<li><b>Change playing position</b>: %2Hover over %1 (start playing first)</li>"
                          "</ul>"
            ).arg(tooltipItem->data(mediaTypeIndex).toString(), commandControl);

        tooltipText += tr("<p><b>Debug</p>""<ul>");
        tooltipText += tr("<li><b>Parent</b>: %1</li>").arg(itemToString(tooltipItem->parentItem()));
        tooltipText += tr("<li><b>Proxy</b>: %1</li>").arg(itemToString(tooltipItem->focusProxy()));
        tooltipText += tr("<li><b>zValue</b>: %1</li>").arg(QString::number(tooltipItem->zValue()));
        tooltipText += tr("<li><b>filtered</b>: %1</li></ul>").arg(tooltipItem->data(excludedInFilter).toBool());

    }
    else if (tooltipItem->data(mediaTypeIndex) == "Clip")
    {
        tooltipText += tr("<p><b>Clip from %6 to %7</b></p>"
                       "<ul>"
                          "<li><b>Foldername</b>: %1</li>"
            "<li><b>Filename</b>: %2</li>"
                          "<li><b>Create date</b>: %3</li>"
             "<li><b>Duration</b>: %4 (%5 s)</li>"
                          "<li><b>In and out</b>: %6 - %7</li>"
                       "</ul>").arg(tooltipItem->data(folderNameIndex).toString(), tooltipItem->data(fileNameIndex).toString()
                                    , tooltipItem->data(createDateIndex).toString()
                                    , AGlobal().msec_to_time(tooltipItem->data(mediaDurationIndex).toInt()), QString::number(tooltipItem->data(mediaDurationIndex).toInt() / 1000.0)
                                    , AGlobal().msec_to_time(tooltipItem->data(clipInIndex).toInt()), AGlobal().msec_to_time(tooltipItem->data(clipOutIndex).toInt())
                         );

        tooltipText += tr("</ul>"
            "<p><b>Help</p>"
            "<ul>"
                          "<li><b>Actions</b>: Right click</li>"
                          "<li><b>Change playing position</b>: %2Hover over %1 (start playing first)</li>"
                          "</ul>"
            ).arg(tooltipItem->data(mediaTypeIndex).toString(), commandControl);

        tooltipText += tr("<p><b>Debug</p>""<ul>");
        tooltipText += tr("<li><b>Parent</b>: %1</li>").arg(itemToString(tooltipItem->parentItem()));
        tooltipText += tr("<li><b>Proxy</b>: %1</li>").arg(itemToString(tooltipItem->focusProxy()));
        tooltipText += tr("<li><b>zValue</b>: %1</li>").arg(QString::number(qint64(tooltipItem->zValue())));
        tooltipText += tr("<li><b>filtered</b>: %1</li></ul>").arg(tooltipItem->data(excludedInFilter).toBool());

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
            QString itemFileNameWithoutExtension = item->data(fileNameIndex).toString().left(item->data(fileNameIndex).toString().lastIndexOf("."));

            //if clip added then find the corresponding mediafile
            if (mediaType == "Clip" && item->data(mediaTypeIndex).toString() == "MediaFile" && item->data(folderNameIndex).toString() == folderName && itemFileNameWithoutExtension == fileNameWithoutExtension)
            {
                proxyItem = item;
                if (fileName != item->data(fileNameIndex).toString())
                {
//                    qDebug()<<"Rename"<<mediaType<<fileName<<item->data(fileNameIndex).toString();
                    fileName = item->data(fileNameIndex).toString();//adjust the fileName to the filename of the corresponding mediaFile
                }
            }
            if (mediaType == "Tag" && item->data(mediaTypeIndex).toString() == "Clip" && item->data(folderNameIndex).toString() == folderName && itemFileNameWithoutExtension == fileNameWithoutExtension)
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
                else if (mediaType == "Clip" && item->data(folderNameIndex).toString() == folderName && itemFileNameWithoutExtension == fileNameWithoutExtension) //MediaFile (do not match extension as clips can be called with .srt)
                    parentItem = item;
//                    qDebug()<<"Parent"<<mediaType<<folderName<<fileName<<itemToString(parentItem);
                else if (mediaType == "Clip" && item->data(folderNameIndex).toString() == folderName && item->data(fileNameIndex).toString() == parentName) //Timeline
                    parentItem = item;
                else if (mediaType == "Tag" && item->data(folderNameIndex).toString() == folderName && itemFileNameWithoutExtension == fileNameWithoutExtension && item->data(clipInIndex).toInt() ==  clipIn)
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
        AGMediaRectangleItem *folderItem = new AGMediaRectangleItem(parentItem);

        QPen pen(Qt::transparent);
        folderItem->setPen(pen);

        folderItem->setRect(QRectF(0, 0, 200 * 9 / 16 + 4, 200 * 9 / 16)); //+2 is minimum to get dd-mm-yyyy not wrapped (+2 extra to be sure)

        setItemProperties(folderItem, mediaType, "Base", folderName, fileName, duration);

        QGraphicsPixmapItem *pictureItem = new QGraphicsPixmapItem(folderItem);
        QImage image = QImage(":/images/ACVCFolder.png");
        QPixmap pixmap = QPixmap::fromImage(image);//.scaled(QSize(200,200 * myImage.height() / myImage.width()))
        pictureItem->setPixmap(pixmap);
        if (image.height() != 0)
            pictureItem->setScale(200.0 * 9.0 / 16.0 / image.height() * 0.8);
        setItemProperties(pictureItem, mediaType, "SubPicture", folderName, fileName, duration);

        childItem = folderItem;
    }
    else if (mediaType.contains("Group"))
    {
        QGraphicsRectItem *rectItem = new QGraphicsRectItem(parentItem);

        QString newFileName = fileName;
        if (mediaType == "TimelineGroup")
        {
            newFileName = parentItem->data(fileNameIndex).toString(); //filename of timelinegroup is Video/Audio etc.
            rectItem->setRect(QRectF(0, 0, 0, 200)); //invisible
            rectItem->setBrush(Qt::red);

            parentItem->setFocusProxy(rectItem); //FileGroupItem->focusProxy == TimelineGroup
        }
        else //fileGroup
        {
            rectItem->setRect(QRectF(0, 0, 200 * 9 / 16, 200 * 9 / 16));

            QGraphicsPixmapItem *pictureItem = new QGraphicsPixmapItem(rectItem);
            QImage image = QImage(":/images/ACVCFolder.png");
            QPixmap pixmap = QPixmap::fromImage(image);//.scaled(QSize(200,200 * myImage.height() / myImage.width()))
            pictureItem->setPixmap(pixmap);
            if (image.height() != 0)
                pictureItem->setScale(200.0 * 9.0 / 16.0 / image.height() * 0.8);
            setItemProperties(pictureItem, mediaType, "SubPicture", folderName, fileName, duration);

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

        QBrush brush;
        brush.setColor(Qt::red);

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

        mediaItem->setFocusProxy(parentItem);

        setItemProperties(mediaItem, mediaType, "Base", folderName, fileName, duration, QSize(), 0, duration);

        assignCreateDates(mediaItem); //in case new item created

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
            if (image.height() != 0)
                pictureItem->setScale(mediaWidth * 9.0 / 16.0 / image.height() * 0.8);
            setItemProperties(pictureItem, mediaType, "SubPicture", folderName, fileName, duration);
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

//        qDebug()<<"setFocusProxy"<<itemToString(clipItem)<<itemToString(parentItem)<<itemToString(proxyItem);

        QGraphicsLineItem *durationLine = new QGraphicsLineItem(clipItem);

        QPen pen;
        pen.setWidth(5);
        pen.setBrush(Qt::darkGreen);

        durationLine->setPen(pen);

        setItemProperties(durationLine, mediaType, "SubDurationLine", folderName, fileName, duration, QSize(), clipIn, clipOut);

        setItemProperties(clipItem, mediaType, "Base", folderName, fileName, duration, QSize(), clipIn, clipOut);

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

        if (AGlobal().audioExtensions.contains(extension))
            tagItem->setHtml(QString("<div align=\"center\">") + tag + "</div>");//#008000 darkgreen style='background-color: rgba(0, 128, 0, 0.5);'
        else
            tagItem->setHtml(QString("<div align=\"center\">") + tag + "</div>");//blue-ish #2a82da  style='background-color: rgba(42, 130, 218, 0.5);'
//        tagItem->setPlainText(tag);

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

void AGView::assignCreateDates(QGraphicsItem *mediaItem)
{
    QString folderName = mediaItem->data(folderNameIndex).toString();
    QString fileName = mediaItem->data(fileNameIndex).toString();
    QString createDateString = mediaItem->data(createDateIndex).toString();

    //check if this file has a createdate
    //if not, see if a createdate can be found in mediaFilesMap (added by trim, remux etc.)
    //if not, see if a createdate can be found in properties

    //if not, take all the files without createdate and reorder them based on the filename

    QString fileNameWithoutExtensionLow = fileName.left(fileName.lastIndexOf(".")).toLower();

    if (mediaFilesMap[folderName + fileNameWithoutExtensionLow].mediaItem == nullptr)
    {
//        qDebug()<<"AGView::assignCreateDates2.1 new"<<itemToString(mediaItem)<<createDateString;
        mediaFilesMap[folderName + fileNameWithoutExtensionLow].mediaItem = mediaItem;
        mediaFilesMap[folderName + fileNameWithoutExtensionLow].folderName = folderName;
        mediaFilesMap[folderName + fileNameWithoutExtensionLow].fileName = fileName;
    }
    else
//        qDebug()<<"AGView::assignCreateDates2.1 existing"<<itemToString(mediaItem)<<createDateString;

    if (createDateString == "")
    {
        createDateString = mediaFilesMap[folderName + fileNameWithoutExtensionLow].createDateString;

//        qDebug()<<"AGView::assignCreateDates2.2 from mediaFilesMap"<<fileName<<createDateString;

        if (createDateString == "")
        {
            QVariant  *createDatePointer = new QVariant();
            emit getPropertyValue(folderName + fileName, "CreateDate", createDatePointer);
            createDateString = createDatePointer->toString();
            if (createDateString == "0000:00:00 00:00:00")
                createDateString = "";

            if (createDateString != "")
                mediaFilesMap[folderName + fileNameWithoutExtensionLow].createDateString = createDateString;
        }

        if (createDateString != "")
        {
//            qDebug()<<"AGView::assignCreateDates2.3 from properties"<<fileName<<createDateString;

            QDateTime createDate = QDateTime::fromString(createDateString, "yyyy:MM:dd HH:mm:ss");

            mediaItem->setData(createDateIndex, createDateString);
            mediaItem->setZValue(createDate.toSecsSinceEpoch());
            updateToolTip(mediaItem); //to set createDate

        }
    }

    if (createDateString != "") //update clips
    {
//        qDebug()<<"AGView::assignCreateDates2.4 update clips"<<fileName<<createDateString<<mediaFilesMap.count();

        QDateTime createDate = QDateTime::fromString(createDateString, "yyyy:MM:dd HH:mm:ss");
        foreach (QGraphicsItem *clipItem, scene->items())
        {
            if (clipItem->focusProxy() == mediaItem && clipItem->data(itemTypeIndex).toString() == "Base" && clipItem->data(mediaTypeIndex).toString() == "Clip") //find the clips
            {
                QDateTime clipCreateDate = createDate.addMSecs(clipItem->data(clipInIndex).toInt());

//                qDebug()<<"  "<<itemToString(clipItem)<<clipCreateDate.toString("yyyy:MM:dd HH:mm:ss");

                clipItem->setData(createDateIndex, clipCreateDate.toString("yyyy:MM:dd HH:mm:ss"));
                clipItem->setZValue(clipCreateDate.toSecsSinceEpoch());
                updateToolTip(clipItem);
            }
        }

    }

    if (createDateString == "") //update all empties
    {
//        qDebug()<<"AGView::assignCreateDates2.5 update no createdate"<<fileName<<createDateString<<mediaFilesMap.count();

        qreal zValueMediaFile = 10000;
        foreach (AMediaStruct mediaStruct, mediaFilesMap) //alphabetically ordered! (instead of scene->items)
        {
            if (mediaStruct.createDateString == "")
            {
                if (mediaStruct.mediaItem != nullptr)
                {
//                    qDebug()<<"   "<<mediaStruct.fileName<<mediaStruct.mediaItem->data(fileNameIndex).toString()<<itemToString(mediaStruct.mediaItem)<<zValue;

                    mediaStruct.mediaItem->setZValue(zValueMediaFile);
                    updateToolTip(mediaStruct.mediaItem); //to set createDate

                    foreach (QGraphicsItem *clipItem, scene->items())
                    {
                        if (clipItem->focusProxy() == mediaStruct.mediaItem && clipItem->data(itemTypeIndex).toString() == "Base" && clipItem->data(mediaTypeIndex).toString() == "Clip") //find the clips
                        {
                            clipItem->setZValue(zValueMediaFile + clipItem->data(clipInIndex).toInt() / 1000.0); //add secs
                            updateToolTip(clipItem);
                        }
                    }

                    zValueMediaFile += 10000;
                }
            }
        }
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
            QString fileNameWithoutExtension = fileName.left(fileName.lastIndexOf("."));

            QString itemFileNameWithoutExtension = item->data(fileNameIndex).toString().left(item->data(fileNameIndex).toString().lastIndexOf("."));

            matchOnFile = itemFileNameWithoutExtension == fileNameWithoutExtension;
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

    mediaFilesMap[folderName + fileName.left(fileName.lastIndexOf(".")).toLower()].mediaItem = nullptr;

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

void AGView::onItemRightClicked(QPoint pos)
{
    QGraphicsItem *itemAtScreenPos = itemAt(pos);

    QGraphicsRectItem *mediaItem = nullptr;
    //as rectItem stays the same if rightclick pressed twice, we look at the item at the screen position.
    if (itemAtScreenPos == nullptr)
        return;
    else
    {
        if (itemAtScreenPos->data(itemTypeIndex).toString().contains("Sub"))
            mediaItem = (QGraphicsRectItem *)itemAtScreenPos->parentItem();
        else //Base
            mediaItem = (QGraphicsRectItem *)itemAtScreenPos;
    }
//    qDebug()<<"AGView::onItemRightClicked itemAtScreenPos"<<pos<<mapFromGlobal(pos)<<itemToString(itemAtScreenPos)<<itemToString(mediaItem);

    QString folderName = mediaItem->data(folderNameIndex).toString();
    QString fileName = mediaItem->data(fileNameIndex).toString();

    QString fileNameLow = fileName.toLower();
    int lastIndexOf = fileNameLow.lastIndexOf(".");
    QString extension = fileNameLow.mid(lastIndexOf + 1);

    fileContextMenu = new QMenu(this);
    setContextMenuPolicy(Qt::CustomContextMenu);
    fileContextMenu->setToolTipsVisible(true);

//    fileContextMenu->eventFilter()

    QPalette pal = fileContextMenu->palette();
    QColor menuColor;
    if (QSettings().value("theme") == "Black")
    {
        menuColor = QColor(41, 42, 45);//80,80,80);
        fileContextMenu->setStyleSheet(R"(
                                   QMenu::separator {
                                    background-color: darkgray;
                                   }
                                       QMenu {
                                        border: 1px solid darkgray;
                                       }

                                 )");
    }
    else
        menuColor = pal.window().color();

    pal.setColor(QPalette::Base, menuColor);
    pal.setColor(QPalette::Window, menuColor);
    fileContextMenu->setPalette(pal);

    QString commandControl = "Ctrl-";
#ifdef Q_OS_MAC
    commandControl = "⌘-";
#endif

//    qDebug()<<"AGView::onItemRightClicked"<<folderName<<fileName<<screenPos<<itemToString(mediaItem);

    if (mediaItem->data(mediaTypeIndex) == "Folder")
    {
        fileContextMenu->addAction(new QAction("Zoom to item",fileContextMenu));
        connect(fileContextMenu->actions().last(), &QAction::triggered, [=]()
        {
            fitInView(mediaItem->boundingRect()|mediaItem->childrenBoundingRect(), Qt::KeepAspectRatio);
        });
        fileContextMenu->actions().last()->setToolTip(tr("<p><b>%1</b></p>"
                                            "<p><i>Zooms in to %2 and it's details</i></p>"
                                                  ).arg(fileContextMenu->actions().last()->text(), fileName));

        fileContextMenu->addSeparator();

        fileContextMenu->addAction(new QAction("Export",fileContextMenu));
        fileContextMenu->actions().last()->setIcon(style()->standardIcon(QStyle::SP_DirOpenIcon));
        connect(fileContextMenu->actions().last(), &QAction::triggered, [=]()
        {
            QDesktopServices::openUrl( QUrl::fromLocalFile( folderName + fileName) );
        });

        fileContextMenu->actions().last()->setToolTip(tr("<p><b>%1</b></p>"
                                            "<p><i>To be done (%2)</i></p>"
                                                  ).arg(fileContextMenu->actions().last()->text(), fileName)); //not effective!

        fileContextMenu->addSeparator();

        fileContextMenu->addAction(new QAction("Open in explorer",fileContextMenu));
        fileContextMenu->actions().last()->setIcon(style()->standardIcon(QStyle::SP_DirOpenIcon));
        connect(fileContextMenu->actions().last(), &QAction::triggered, [=]()
        {
            QDesktopServices::openUrl( QUrl::fromLocalFile( folderName + fileName) );
        });

        fileContextMenu->actions().last()->setToolTip(tr("<p><b>%1</b></p>"
                                            "<p><i>Shows the current file or folder %2 in the explorer of your computer</i></p>"
                                                  ).arg(fileContextMenu->actions().last()->text(), fileName)); //not effective!

        fileContextMenu->popup(mapToGlobal(QPoint(pos.x()+10, pos.y())));
    }
    else if (mediaItem->data(mediaTypeIndex) == "MediaFile")
    {
        QMediaPlayer *m_player = nullptr;
        if (!playInDialog)
        {
            QGraphicsVideoItem *playerItem = nullptr;
            foreach (QGraphicsItem *childItem, mediaItem->childItems())
            {
                if (childItem->data(itemTypeIndex).toString().contains("SubPlayer"))
                    playerItem = (QGraphicsVideoItem *)childItem;
            }
        //    qDebug()<<"playerItem"<<playerItem;
            if (playerItem != nullptr)
                m_player = (QMediaPlayer *)playerItem->mediaObject();
        }
        else
            m_player = dialogMediaPlayer;

    //    setfileContextMenuPolicy(Qt::ActionsfileContextMenu);
    //    QColor darkColorAlt = QColor(45,90,45);
    //    QPalette palette = fileContextMenu->palette();
    //    palette.setColor(QPalette::Background, darkColorAlt);

        if (m_player != nullptr)
        {
            fileContextMenu->addAction(new QAction("Frame back",fileContextMenu));
            fileContextMenu->actions().last()->setToolTip(tr("<p><b>Go to next or previous frame</b></p>"
                                              "<p><i>Go to next or previous frame</i></p>"
                                              ));
            fileContextMenu->actions().last()->setIcon(style()->standardIcon(QStyle::SP_MediaSeekBackward));
            connect(fileContextMenu->actions().last(), &QAction::triggered, [=]()
            {
                onRewind(m_player);
            });
        }

        if (AGlobal().videoExtensions.contains(extension) || AGlobal().audioExtensions.contains(extension))
        {
            fileContextMenu->addAction(new QAction("Play/Pause",fileContextMenu));
            fileContextMenu->actions().last()->setToolTip(tr("<p><b>Play or pause</b></p>"
                                                             "<p><i>Play or pause the video</i></p>"
                                                             ));
            fileContextMenu->actions().last()->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
            fileContextMenu->actions().last()->setShortcut(QKeySequence(tr("Ctrl+P")));

            connect(fileContextMenu->actions().last(), &QAction::triggered, [=]()
            {
                initPlayer(mediaItem);
            }); //onPlayVideoButton(m_player);
        }

        if (m_player != nullptr)
        {
            fileContextMenu->addAction(new QAction("Frame forward",fileContextMenu));
            fileContextMenu->actions().last()->setToolTip(tr("<p><b>Go to next or previous clip</b></p>"
                                                                                                   "<p><i>Go to next or previous clip</i></p>"
                                                                                                   "<ul>"
                                                                                                   "<li>Shortcut: %1up and %1down</li>"
                                                                                                   "</ul>").arg(commandControl));
            fileContextMenu->actions().last()->setIcon(style()->standardIcon(QStyle::SP_MediaSeekForward));
            connect(fileContextMenu->actions().last(), &QAction::triggered, [=]()
            {
                onFastForward(m_player);
            });

            fileContextMenu->addAction(new QAction("Stop",fileContextMenu));
            fileContextMenu->actions().last()->setToolTip(tr("<p><b>Stop the video</b></p>"
                                                             "<p><i>Stop the video</i></p>"
                                                             ));
            fileContextMenu->actions().last()->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
            connect(fileContextMenu->actions().last(), &QAction::triggered, [=]()
            {
                onStop(m_player);
            });

            fileContextMenu->addAction(new QAction("Mute",fileContextMenu));
            fileContextMenu->actions().last()->setToolTip(tr("<p><b>Mute or unmute</b></p>"
                                                             "<p><i>Mute or unmute sound (toggle)</i></p>"
                                                             ));
            fileContextMenu->actions().last()->setIcon(style()->standardIcon(QStyle::SP_MediaVolume));
            connect(fileContextMenu->actions().last(), &QAction::triggered, [=]()
            {
                onMuteVideoButton(m_player);
            });

            fileContextMenu->addAction(new QAction("Speed",fileContextMenu));
            fileContextMenu->actions().last()->setToolTip(tr("<p><b>Speed</b></p>"
                                                                        "<p><i>Change the play speed of the video</i></p>"
                                                                        "<ul>"
                                                                        "<li>Supported speeds are depending on the media file codec installed on your computer</li>"
                                                                        "</ul>"));
            fileContextMenu->actions().last()->setIcon(style()->standardIcon(QStyle::SP_MediaVolume));
            connect(fileContextMenu->actions().last(), &QAction::triggered, [=]()
            {
                const QString &arg1 = "1x";
                double playbackRate = arg1.left(arg1.lastIndexOf("x")).toDouble();
                if (arg1.indexOf(" fps")  > 0)
                    playbackRate = arg1.left(arg1.lastIndexOf(" fps")).toDouble() / QSettings().value("frameRate").toInt();
                onSetPlaybackRate(m_player, playbackRate);
            });
        }

        fileContextMenu->addSeparator();

        fileContextMenu->addAction(new QAction("Zoom to item",fileContextMenu));
        connect(fileContextMenu->actions().last(), &QAction::triggered, [=]()
        {
            fitInView(mediaItem->boundingRect()|mediaItem->childrenBoundingRect(), Qt::KeepAspectRatio);
        });
        fileContextMenu->actions().last()->setToolTip(tr("<p><b>%1</b></p>"
                                            "<p><i>Zooms in to %2 and it's details</i></p>"
                                                  ).arg(fileContextMenu->actions().last()->text(), fileName));

        if (AGlobal().videoExtensions.contains(extension) || AGlobal().audioExtensions.contains(extension))
        {
            fileContextMenu->addSeparator();

            fileContextMenu->addAction(new QAction("Trim " + fileName,fileContextMenu));
            connect(fileContextMenu->actions().last(), &QAction::triggered, [=]()
            {
                AJobParams jobParams;
                jobParams.action = "Trim";

                QStandardItem *parentItem = jobTreeView->createJob(jobParams, nullptr , nullptr);

                QStandardItem *currentItem = nullptr;

                bool trimDone = false;

                {
                    QString folderFileNameLow = fileName.toLower();
                    int lastIndexOf = folderFileNameLow.lastIndexOf(".");
                    QString extension = folderFileNameLow.mid(lastIndexOf + 1);

                    if (!AGlobal().projectExtensions.contains(extension))
                    {
    //                     QStandardItem *parentItem2 = nullptr;
                        emit trimAll(parentItem, currentItem, folderName, fileName, false);
    //                     qDebug()<<"AFilesTreeView::onTrimAll"<<fileName<<parentItem;
                        trimDone = true;
                    }
                }

    //            if (trimDone)
    //            {
    //                emit loadClips(parentItem);

    //                emit loadProperties(parentItem);
    //            }

            });
            fileContextMenu->actions().last()->setToolTip(tr("<p><b>%1</b></p>"
                                                             "<p><i>Create new video file(s) for each clip of %2. </i></p>"
                                                             "<ul>"
                                                             "<li><b>Clips and properties</b>: Clips and properties are copied to the new file(s)</li>"
                                                             "<li><b>Source file</b>: New file(s) will be created. To remove %2: Right click archive</li>"
                                                                   "</ul>").arg(fileContextMenu->actions().last()->text(), fileName));
        } //trim

        fileContextMenu->addSeparator();

        fileContextMenu->addAction(new QAction("Archive file",fileContextMenu));
        connect(fileContextMenu->actions().last(), &QAction::triggered, [=]()
        {
            //remove player
            stopAndDeleteAllPlayers();

            AJobParams jobParams;
            jobParams.thisObject = this;
            jobParams.action = "Archive Files";

            QStandardItem *parentItem = jobTreeView->createJob(jobParams, nullptr , nullptr);

//                emit releaseMedia(folderName, fileName);
             emit moveFilesToACVCRecycleBin(parentItem, folderName, fileName);

//            emit loadClips(parentItem);
//            emit loadProperties(parentItem);

        });
        fileContextMenu->actions().last()->setToolTip(tr("<p><b>%1</b></p>"
                                                         "<p><i>Move %2 and its supporting files (clips / .srt files) to the ACVC recycle bin folder. If files do already exist in the recycle bin, these files are renamed first, with BU (Backup) added to their name</i></p>"
                                                               ).arg(fileContextMenu->actions().last()->text(), fileName));

        if (AGlobal().videoExtensions.contains(extension) || AGlobal().audioExtensions.contains(extension) || AGlobal().imageExtensions.contains(extension))
        {
            fileContextMenu->addAction(new QAction("Archive clips",fileContextMenu));
            connect(fileContextMenu->actions().last(), &QAction::triggered, [=]()
            {
            });
            fileContextMenu->actions().last()->setToolTip(tr("<p><b>%1</b></p>"
                                                             "<p><i>Move the clips of %2 (.srt file) to the ACVC recycle bin folder</i></p>"
                                                                   ).arg(fileContextMenu->actions().last()->text(), fileName));
        }

        if (AGlobal().videoExtensions.contains(extension))
        {
            fileContextMenu->addSeparator();

            fileContextMenu->addAction(new QAction("Remux to Mp4 / yuv420",fileContextMenu));
            connect(fileContextMenu->actions().last(), &QAction::triggered, [=]()
            {
                AJobParams jobParams;
                jobParams.thisObject = this;
                jobParams.action = "Remux";

                QStandardItem *parentItem = jobTreeView->createJob(jobParams, nullptr , nullptr);

                QStandardItem *currentItem = nullptr;

                {
                    QString sourceFolderFileName = folderName + fileName;

                    QString targetFolderFileName;
                    int lastIndex = fileName.lastIndexOf(".");
                    if (lastIndex > -1)
                        targetFolderFileName = folderName + fileName.left(lastIndex) + "RM.mp4";

                #ifdef Q_OS_WIN
                    sourceFolderFileName = sourceFolderFileName.replace("/", "\\");
                    targetFolderFileName = targetFolderFileName.replace("/", "\\");
                #endif

//                    qDebug()<<"AFilesTreeView::onRemux"<<folderName<<fileName<<mediaItem->data(mediaDurationIndex).toInt();

                    AJobParams jobParams;
                    jobParams.parentItem = parentItem;
                    jobParams.folderName = folderName;
                    jobParams.fileName = fileName;
                    jobParams.action = "Remux";
                    jobParams.command = "ffmpeg -y -i \"" + sourceFolderFileName + "\" -pix_fmt yuv420p -y \"" + targetFolderFileName + "\""; //-map_metadata 0  -loglevel +verbose
                    jobParams.parameters["exportFolderFileName"] = targetFolderFileName;
                    jobParams.parameters["totalDuration"] = QString::number(mediaItem->data(mediaDurationIndex).toInt());
                    jobParams.parameters["durationMultiplier"] = QString::number(2);

                    //causes right ordering of mediafile after add item and before mediaLoaded
                    mediaFilesMap[folderName + fileName.left(lastIndex).toLower() + "rm"].createDateString = mediaItem->data(createDateIndex).toString();
                    mediaFilesMap[folderName + fileName.left(lastIndex).toLower() + "rm"].folderName = folderName;
                    mediaFilesMap[folderName + fileName.left(lastIndex).toLower() + "rm"].fileName = fileName.left(lastIndex) + "RM.mp4";

                    currentItem = jobTreeView->createJob(jobParams, nullptr, nullptr);

                    copyClips(currentItem, folderName, fileName, fileName.left(fileName.lastIndexOf(".")) + "RM.mp4");

                    emit propertyCopy(currentItem, folderName, fileName, folderName, fileName.left(fileName.lastIndexOf(".")) + "RM.mp4");

    //                emit releaseMedia(folderName, fileName);
    //                onMoveFilesToACVCRecycleBin(currentItem, folderName, fileName);
                }

    //            emit loadClips(parentItem);

    //            emit loadProperties(parentItem);
            });
            fileContextMenu->actions().last()->setToolTip(tr("<p><b>%1</b></p>"
                                                             "<p><i>Remux %2 into a mp4 container and convert video to yuv420 color format</i></p>"
                                                             "<ul>"
                                                             "<li><b>mp4 container</b>: enabling property updates</li>"
                                                             "<li><b>yuv420 color format</b>: enabling Wideview conversion</li>"
                                                             "<li><b>Usage example</b>: <a href=\"https://www.fatshark.com/\">Fatshark</a> DVR</li>"
                                                             "<li>%1 can be time consuming. See Jobs tab for progress.</li>"
                                                             "<li><b>Source file</b>: A new file will be created. To remove %2: Right click archive</li>"
                                                                   "</ul>").arg(fileContextMenu->actions().last()->text(), fileName));

            fileContextMenu->addAction(new QAction("Create wideview (16:9) video",fileContextMenu));
            connect(fileContextMenu->actions().last(), &QAction::triggered, [=]()
            {
                AJobParams jobParams;
                jobParams.action = "Wideview";

                QStandardItem *parentItem = jobTreeView->createJob(jobParams, nullptr , nullptr);

                QStandardItem *currentItem = nullptr;

                {
                    derperView = new ADerperView();

    //                connect(this, &AGView::stopThreadProcess, derperView, &ADerperView::onStopThreadProcess);

                    AJobParams jobParams;
                    jobParams.thisObject = this;
                    jobParams.parentItem = parentItem;
                    jobParams.folderName = folderName;
                    jobParams.fileName = fileName;
                    jobParams.action = "Wideview";
                    jobParams.parameters["totalDuration"] = QString::number(mediaItem->data(mediaDurationIndex).toInt());
                    jobParams.parameters["durationMultiplier"] = QString::number(0.6);
                    jobParams.parameters["targetFileName"] = fileName.left(fileName.lastIndexOf(".")) + "WV.mp4";

                    //causes right ordering of mediafile after add item and before mediaLoaded
                    mediaFilesMap[folderName + fileName.left(fileName.lastIndexOf(".")).toLower() + "wv"].createDateString = mediaItem->data(createDateIndex).toString();
                    mediaFilesMap[folderName + fileName.left(fileName.lastIndexOf(".")).toLower() + "wv"].folderName = folderName;
                    mediaFilesMap[folderName + fileName.left(fileName.lastIndexOf(".")).toLower() + "wv"].fileName = jobParams.parameters["targetFileName"];

                    currentItem = jobTreeView->createJob(jobParams, [] (AJobParams jobParams)
                    {
                        AGView *filesTreeView = qobject_cast<AGView *>(jobParams.thisObject);

                            connect(filesTreeView->derperView, &ADerperView::processOutput, [=](QString output)
                            {
           //                         qDebug() << "AFilesTreeView::processOutput" <<jobParams.parameters["totalDuration"] << output<<jobParams.currentIndex<<jobParams.currentIndex.data();
                                emit filesTreeView->jobAddLog(jobParams, output);
                            });

                            emit filesTreeView->jobAddLog(jobParams, "===================");
                            emit filesTreeView->jobAddLog(jobParams, "WideView by Derperview, Derperview by Banelle: https://github.com/banelle/derperview");
                            emit filesTreeView->jobAddLog(jobParams, "Perform non-linear stretch of 4:3 video to make it 16:9.");
                            emit filesTreeView->jobAddLog(jobParams, "See also Derperview - A Command Line Superview Alternative: https://intofpv.com/t-derperview-a-command-line-superview-alternative");
                            emit filesTreeView->jobAddLog(jobParams, "===================");
                            emit filesTreeView->jobAddLog(jobParams, "ACVC uses unmodified Derperview sourcecode and embedded it in the Qt and ACVC job handling structure.");
                            emit filesTreeView->jobAddLog(jobParams, "ACVC added 'Remux to MP4/Yuv420' to prepare videocontent for Wideview conversion");
                            emit filesTreeView->jobAddLog(jobParams, "===================");

                        return filesTreeView->derperView->Go((jobParams.folderName + jobParams.fileName).toUtf8().constData(), (jobParams.folderName + jobParams.parameters["targetFileName"]).toUtf8().constData(), 1);

                    }, nullptr);

                    copyClips(currentItem, folderName, fileName, fileName.left(fileName.lastIndexOf(".")) + "WV.mp4");

                    emit propertyCopy(currentItem, folderName, fileName, folderName, fileName.left(fileName.lastIndexOf(".")) + "WV.mp4");

    //                emit releaseMedia(folderName, fileName);
    //                emit moveFilesToACVCRecycleBin(currentItem, folderName, fileName);
                } //for all files

    //            emit loadClips(parentItem);

    //            emit loadProperties(parentItem);

    //            emit derperviewCompleted("");

            });
            fileContextMenu->actions().last()->setToolTip(tr("<p><b>%1 by Derperview</b></p>"
                                                             "<p><i>Perform non-linear stretch of 4:3 video to make it 16:9</i></p>"
                                                             "<ul>"
                                                             "<li><b>Derperview</b>: Courtesy of Banelle to use the Derperview stretching algorithm (see About).</li>"
                                                             "<li><b>yuv420</b>: %2 needs to have 4:3 aspect ratio and yuv420 color format. Use <i>Remux to mp4/yuv420</i> if needed.</li>"
                                                             "<li>%1 can be time consuming. See Jobs tab for progress.</li>"
                                                             "<li><b>Source file</b>: A new file will be created. To remove %2: Right click archive</li>"
                                                                   "</ul>").arg(fileContextMenu->actions().last()->text(), fileName));

        }

        fileContextMenu->addSeparator();

        fileContextMenu->addAction(new QAction("Open in explorer",fileContextMenu));
        fileContextMenu->actions().last()->setIcon(style()->standardIcon(QStyle::SP_DirOpenIcon));
        connect(fileContextMenu->actions().last(), &QAction::triggered, [=]() {
#ifdef Q_OS_MAC
    QStringList args;
    args << "-e";
    args << "tell application \"Finder\"";
    args << "-e";
    args << "activate";
    args << "-e";
    args << "select POSIX file \""+folderName + fileName+"\"";
    args << "-e";
    args << "end tell";
    QProcess::startDetached("osascript", args);
#endif

#ifdef Q_OS_WIN
    QStringList args;
    args << "/select," << QDir::toNativeSeparators(folderName + fileName);
    QProcess::startDetached("explorer", args);
#endif

    });

        fileContextMenu->actions().last()->setToolTip(tr("<p><b>%1</b></p>"
                                            "<p><i>Shows the current file or folder %2 in the explorer of your computer</i></p>"
                                                  ).arg(fileContextMenu->actions().last()->text(), fileName)); //not effective!


        fileContextMenu->addAction(new QAction("Open in default application",fileContextMenu));
        fileContextMenu->actions().last()->setIcon(QIcon(QPixmap::fromImage(QImage(":/images/shotcut-logo-320x320.png"))));
        connect(fileContextMenu->actions().last(), &QAction::triggered, [=]()
        {
            QDesktopServices::openUrl( QUrl::fromLocalFile( folderName + fileName) );
        });

        if (!AGlobal().projectExtensions.contains(extension))
        {
            fileContextMenu->addSeparator();

            fileContextMenu->addAction(new QAction("Properties",fileContextMenu));
            fileContextMenu->actions().last()->setIcon(QIcon(QPixmap::fromImage(QImage(":/acvc.ico"))));
            connect(fileContextMenu->actions().last(), &QAction::triggered, [=]()
            {
                QDialog * propertiesDialog = new QDialog(this);
                propertiesDialog->setWindowTitle("ACVC Properties");
            #ifdef Q_OS_MAC
                propertiesDialog->setWindowFlag(Qt::WindowStaysOnTopHint); //needed for MAC / OSX
            #endif

                QRect savedGeometry = QSettings().value("Geometry").toRect();
                savedGeometry.setX(savedGeometry.x() + savedGeometry.width()/4);
                savedGeometry.setY(savedGeometry.y() + savedGeometry.height()/4);
                savedGeometry.setWidth(savedGeometry.width()/2);
                savedGeometry.setHeight(savedGeometry.height()/2);
                propertiesDialog->setGeometry(savedGeometry);

                QVBoxLayout *mainLayout = new QVBoxLayout(propertiesDialog);

                if (m_player != nullptr && m_player->availableMetaData().count() > 0)
                {
                    QGroupBox *metadataGroupBox = new QGroupBox(propertiesDialog);
                    metadataGroupBox->setTitle("Properties by QMediaPlayer");
                    mainLayout->addWidget(metadataGroupBox);
                    QVBoxLayout *metadataLayout = new QVBoxLayout;
                    metadataGroupBox->setLayout(metadataLayout);

                    foreach (QString metadata_key, m_player->availableMetaData())
                    {
                        QLabel *metaDataLabel = new QLabel(metadataGroupBox);
            //                qDebug()<<"Metadata"<<metadata_key<<m_player->metaData(metadata_key);
                        QVariant meta = m_player->metaData(metadata_key);
                        if (meta.toSize() != QSize())
                            metaDataLabel->setText(metadata_key + ": " + QString::number( meta.toSize().width()) + " x " + QString::number( meta.toSize().height()));
                        else
                            metaDataLabel->setText(metadata_key + ": " + meta.toString());
                        metadataLayout->addWidget(metaDataLabel);
                    }
                }

                QStringList ffMpegMetaList = mediaItem->data(ffMpegMetaIndex).toString().split(";");

                if (ffMpegMetaList.count() > 0 && ffMpegMetaList.first() != "")
                {
                    QGroupBox *ffMpegMetaGroupBox = new QGroupBox(propertiesDialog);
                    ffMpegMetaGroupBox->setTitle("Properties by FFMpeg");
                    mainLayout->addWidget(ffMpegMetaGroupBox);
                    QVBoxLayout *ffMpegMetaLayout = new QVBoxLayout;
                    ffMpegMetaGroupBox->setLayout(ffMpegMetaLayout);

                    foreach (QString keyValuePair, ffMpegMetaList)
                    {
                        QLabel *ffMpegMetaLabel = new QLabel(ffMpegMetaGroupBox);
                        ffMpegMetaLabel->setText(keyValuePair);
                        ffMpegMetaLayout->addWidget(ffMpegMetaLabel);
                    }
                }

                QGroupBox *parentPropGroupBox = new QGroupBox(propertiesDialog);
                parentPropGroupBox->setTitle("Properties by Exiftool");
                mainLayout->addWidget(parentPropGroupBox);
                QVBoxLayout *parentPropLayout = new QVBoxLayout;
                parentPropGroupBox->setLayout(parentPropLayout);

    //            int fileColumnNr = -1;
    //            for(int col = 0; col < ui->propertyTreeView->propertyItemModel->columnCount(); col++)
    //            {
    //              if (ui->propertyTreeView->propertyItemModel->headerData(col, Qt::Horizontal).toString() == folderName + fileName)
    //              {
    //                  fileColumnNr = col;
    //              }
    //            }
    //        //    qDebug()<<"APropertyTreeView::onSetPropertyValue"<<fileName<<fileColumnNr<<propertyName<<propertyItemModel->rowCount();

    //            if (fileColumnNr != -1)
    //            {
    //                //get row/ item value
    //                for(int rowIndex = 0; rowIndex < ui->propertyTreeView->propertyItemModel->rowCount(); rowIndex++)
    //                {
    //                    QModelIndex topLevelIndex = ui->propertyTreeView->propertyItemModel->index(rowIndex,propertyIndex);

    //                    if (ui->propertyTreeView->propertyItemModel->rowCount(topLevelIndex) > 0)
    //                    {

    //                        bool first = true;
    //                        QGroupBox *childPropGroupBox = new QGroupBox(parentPropGroupBox);
    //                        QVBoxLayout *childPropLayout = new QVBoxLayout;

    //                        for (int childRowIndex = 0; childRowIndex < ui->propertyTreeView->propertyItemModel->rowCount(topLevelIndex); childRowIndex++)
    //                        {
    //                            QModelIndex sublevelIndex = ui->propertyTreeView->propertyItemModel->index(childRowIndex,propertyIndex, topLevelIndex);
    //                            QString propValue = ui->propertyTreeView->propertyItemModel->index(childRowIndex, fileColumnNr, topLevelIndex).data().toString();

    //                            if (propValue != "")
    //                            {
    //                                if (first)
    //                                {
    //                                    childPropGroupBox->setTitle(topLevelIndex.data().toString());
    //                                    parentPropLayout->addWidget(childPropGroupBox);
    //                                    childPropGroupBox->setLayout(childPropLayout);

    //                                    first = false;
    //                                }

    //                                QLabel *metaDataLabel = new QLabel(childPropGroupBox);
    //                                metaDataLabel->setText(sublevelIndex.data().toString() + ": " + propValue);
    //                                childPropLayout->addWidget(metaDataLabel);
    //                            }
    //                        }
    //                    }
    //                }
    //            }


                propertiesDialog->show();

                fileContextMenu->actions().last()->setToolTip(tr("<p><b>Properties</b></p>"
                                                "<p><i>Show properties for the currently selected media item</i></p>"
                                                      "<ul>"
                                                      "<li><b>Properties by FFMpeg</b>: Temporary available to compare with Exiftool. Available for Videos FFMpeg tool shows average framerate and framerate. Last one is the framerate if no drops are present</li>"
                                                      "<li><b>Properties by QMediaplayer</b>: Temporary available to compare with Exiftool. Available for Audio and Videos after they started playing</li>"
                                                      "<li><b>Properties by Exiftool</b>: See also property tab in classical mode. Properties are also edited there</li>"
                                                      "</ul>"));
            });

        }

        fileContextMenu->popup(mapToGlobal(QPoint(pos.x()+10, pos.y())));
    }
    else if (mediaItem->data(mediaTypeIndex) == "Clip")
    {
        fileContextMenu->addAction(new QAction("Zoom to item",fileContextMenu));
        connect(fileContextMenu->actions().last(), &QAction::triggered, [=]()
        {
            fitInView(mediaItem->boundingRect()|mediaItem->childrenBoundingRect(), Qt::KeepAspectRatio);
        });
        fileContextMenu->actions().last()->setToolTip(tr("<p><b>%1</b></p>"
                                            "<p><i>Zooms in to %2 and it's details</i></p>"
                                                  ).arg(fileContextMenu->actions().last()->text(), fileName));

        fileContextMenu->addSeparator();

        fileContextMenu->addAction(new QAction("Delete",fileContextMenu));
        connect(fileContextMenu->actions().last(), &QAction::triggered, [=]()
        {
            fitInView(mediaItem->boundingRect()|mediaItem->childrenBoundingRect(), Qt::KeepAspectRatio);
        });
        fileContextMenu->actions().last()->setToolTip(tr("<p><b>%1</b></p>"
                                            "<p><i>Zooms in to %2 and it's details</i></p>"
                                                  ).arg(fileContextMenu->actions().last()->text(), fileName));

        fileContextMenu->addSeparator();

        fileContextMenu->popup(mapToGlobal(QPoint(pos.x()+10, pos.y())));
    }
    else if (mediaItem->data(mediaTypeIndex) == "Tag")
    {
        fileContextMenu->addAction(new QAction("Zoom to item",fileContextMenu));
        connect(fileContextMenu->actions().last(), &QAction::triggered, [=]()
        {
            fitInView(mediaItem->boundingRect()|mediaItem->childrenBoundingRect(), Qt::KeepAspectRatio);
        });
        fileContextMenu->actions().last()->setToolTip(tr("<p><b>%1</b></p>"
                                            "<p><i>Zooms in to %2 and it's details</i></p>"
                                                  ).arg(fileContextMenu->actions().last()->text(), fileName));

        fileContextMenu->popup(mapToGlobal(QPoint(pos.x()+10, pos.y())));
    }
}

void AGView::initPlayer(QGraphicsRectItem *mediaItem)
{
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
                playerItem->setSize(QSize(mediaWidth * 0.8, mediaWidth * 9.0 / 16.0 * 0.8));
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

    int duration = 0;
    if (!playInDialog)
    {
        if (playerItem != nullptr)
        {
            QMediaPlayer *m_player = (QMediaPlayer *)playerItem->mediaObject();

            m_player->setPosition(progress);

            duration = m_player->duration();
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

    if (progressLineItem != nullptr && duration != 0)
        progressLineItem->setLine(QLineF(progressLineItem->pen().width() / 2.0, 0, qMax(progressLineItem->parentItem()->boundingRect().width() * progress / duration - progressLineItem->pen().width() / 2.0, progressLineItem->pen().width() * 1.5), 0));
}

void AGView::onMediaLoaded(QString folderName, QString fileName, QImage image, int duration, QSize mediaSize, QString ffmpegMeta, QList<int> samples)
{
    //can be called multiple times in case a file is created (directory new) and take some time to be written (file update)

//    qDebug()<<"AGView::onMediaLoaded"<<folderName<<fileName<<duration<<mediaSize;//<<ffmpegMeta <<propertyPointer->toString()

    QGraphicsItem *mediaItem = nullptr;
    foreach (QGraphicsItem *item, scene->items())
    {
        if (item->data(folderNameIndex) == folderName && item->data(fileNameIndex) == fileName && item->data(itemTypeIndex) == "Base" && item->data(mediaTypeIndex) == "MediaFile")
            mediaItem = item;
    }

    if (mediaItem != nullptr)
    {
        assignCreateDates(mediaItem);

        if (ffmpegMeta != "") //when called by onMetaDataChanged (from QMediaPlayer)
            mediaItem->setData(ffMpegMetaIndex, ffmpegMeta);

        if (duration != -1)
            setItemProperties(mediaItem, "MediaFile", "Base", folderName, fileName,  duration, mediaSize); //including updateToolTip
        else
            updateToolTip(mediaItem);

        QGraphicsPixmapItem *pictureItem = nullptr;
        QGraphicsLineItem *durationLine = nullptr;
        QGraphicsVideoItem *playerItem = nullptr;

        foreach (QGraphicsItem *childItem, mediaItem->childItems())
        {
            if (childItem->data(itemTypeIndex).toString().contains("SubPicture"))
                pictureItem = (QGraphicsPixmapItem *)childItem;
            if (childItem->data(itemTypeIndex).toString().contains("SubDurationLine"))
                durationLine = (QGraphicsLineItem *)childItem;
            if (childItem->data(itemTypeIndex).toString().contains("SubPlayer"))
                playerItem = (QGraphicsVideoItem *)childItem;
        }

        if (image != QImage())
        {
            if (pictureItem == nullptr)
                pictureItem = new QGraphicsPixmapItem(mediaItem);

            QPixmap pixmap = QPixmap::fromImage(image);//.scaled(QSize(200,200 * myImage.height() / myImage.width()))
            pictureItem->setPixmap(pixmap);
            if (image.height() != 0)
                pictureItem->setScale(mediaWidth * 9.0 / 16.0 / image.height() * 0.8);

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
//                qDebug()<<"sample"<<sample;
                painterPath.lineTo(qreal(counter) / samples.count() * duration * mediaFileScaleFactor, sample * mediaWidth * 9.0 / 16.0 / 100.0);//(1.0 * progressInMSeconds / durationInMSeconds) * durationInMSeconds / 100.0
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

                QPen pen;
                pen.setWidth(5);
                if (mediaItem->parentItem()->data(fileNameIndex) == "Export")
                    pen.setBrush(Qt::darkGreen);
                else
                    pen.setBrush(Qt::red);

                durationLine->setPen(pen);

                setItemProperties(durationLine, "MediaFile", "SubDurationLine", folderName, fileName, duration, mediaSize);
            }

//            if (playerItem != nullptr) //update duration from QMediaPlayer (needed for audio files)
//                setItemProperties(playerItem, "MediaFile", "SubPlayer", folderName, fileName, duration, mediaSize);
        }

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
//            if (clipItem->data(fileNameIndex).toString().contains("Blindfold"))
//                qDebug()<<"new poly" << itemToString(clipItem);
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
//            if (clipItem->data(fileNameIndex).toString().contains("Blindfold"))
//                qDebug()<<"draw poly" << itemToString(clipItem) << itemToString(parentMediaFile);
            if (parentMediaFile != nullptr) //should always be the case
            {
                int clipIn = clipItem->data(clipInIndex).toInt();
                int clipOut = clipItem->data(clipOutIndex).toInt();
                int duration = parentMediaFile->data(mediaDurationIndex).toInt();

//                        qDebug()<<"AGView::drawPoly inout"<<clipIn<<clipOut<<duration<<itemToString(parentMediaFile);

                QPointF parentPointLeft = clipItem->mapFromItem(parentMediaFile, QPoint(duration==0?0:parentMediaFile->rect().width() * clipIn / duration, parentMediaFile->rect().height()));
                QPointF parentPointRight = clipItem->mapFromItem(parentMediaFile, QPoint(duration==0?0:parentMediaFile->rect().width() * clipOut / duration, parentMediaFile->rect().height()));

//                if (clipItem->data(fileNameIndex).toString().contains("Blindfold"))
//                    qDebug()<<"  "<<parentMediaFile->rect()<<clipIn<<duration<<parentPointLeft<<QPoint(duration==0?0:parentMediaFile->rect().width() * clipIn / duration, parentMediaFile->rect().height());

                //all points relative to clip
                QPolygonF polyGon;
                polyGon.append(QPointF(parentPointLeft.x(), parentPointLeft.y()+1));
                polyGon.append(QPointF(-1,-1));
                polyGon.append(QPointF(clipItem->boundingRect().width(),-1));
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
            if (childItem->data(itemTypeIndex).toString().contains("SubProgressLine"))
                progressLineItem = (QGraphicsLineItem *)childItem;
        }

        //create progressLineItem when position changed, not when mediaLoaded
        if (progressLineItem == nullptr)
        {
            progressLineItem = new QGraphicsLineItem(mediaItem);

            QPen pen;
            pen.setWidth(10);
            pen.setBrush(Qt::red);
            progressLineItem->setPen(pen);

            setItemProperties(progressLineItem, "MediaFile", "SubProgressLine", folderName, fileName, m_player->duration(), QSize());
            progressLineItem->setPos(0, mediaItem->boundingRect().height() - progressLineItem->pen().width() / 2.0 ); //pos need to set here as arrangeitem not called here
        }

        //update progressLine (as arrangeitem not called here)
        if (progressLineItem != nullptr && m_player->duration() != 0) // subProgressline already created in mediaLoaded
            progressLineItem->setLine(QLineF(progressLineItem->pen().width() / 2.0, 0, qMax(progressLineItem->parentItem()->boundingRect().width() * progress / m_player->duration() - progressLineItem->pen().width() / 2.0, progressLineItem->pen().width() * 1.5), 0));

        if (playerItem != nullptr && m_player->duration() != 0) //move video to current position
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
//    qDebug()<<"AGView::onStop"<<m_player->media().request().url();
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

    int spaceBetween = 10;

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
        QGraphicsRectItem *mediaFileItem = (QGraphicsRectItem *)parentItem;

        if (parentItem->parentItem()->data(fileNameIndex).toString() == "Export")
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
                    //position of clipIn on MediaItem
                    int clipIn = childItem->data(clipInIndex).toInt();
                    int clipOut = childItem->data(clipOutIndex).toInt();
    //                qDebug()<<"position of clipIn on MediaItem"<<clipIn<<clipOut<<duration;
                    nextPos = QPointF(qMax(mediaDuration==0?0:(parentItem->boundingRect().width() * (clipIn+clipOut) / 2.0 / mediaDuration - childItem->boundingRect().width() / 2.0), nextPos.x()), nextPos.y());
                }
            }
        }
        else if (childItemType == "SubDurationLine")
        {
            nextPos = QPointF(0, 0);
        }
        else if (childItemType == "SubProgressLine")
        {
            QGraphicsLineItem *progressLineItem = (QGraphicsLineItem *)childItem;

//            qDebug()<<"SubProgressLine"<<itemToString(parentItem)<<itemToString(childItem)<<parentItem->boundingRect();

            nextPos = QPointF(0, parentItem->boundingRect().height() - progressLineItem->pen().width() / 2.0);
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
            parentRectItem->setRect(QRectF(parentRectItem->rect().x(), parentRectItem->rect().y(), qMax(parentRectItem->rect().width(), mediaWidth), parentRectItem->rect().height()));
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
    }

    {
        //adjust the width of text, lines and waves
        QGraphicsVideoItem *playerItem = nullptr;
        QGraphicsLineItem *progressLineItem = nullptr;

        foreach (QGraphicsItem *childItem, parentItem->childItems())
        {
            QGraphicsLineItem *lineItem = (QGraphicsLineItem *)childItem;

            qreal scaleFactor = mediaFileScaleFactor;
            if (childItem->data(mediaTypeIndex) == "Clip" || childItem->data(mediaTypeIndex) == "Folder"  || (childItem->data(mediaTypeIndex) == "MediaFile" && childItem->parentItem()->parentItem()->data(fileNameIndex) == "Export"))
                scaleFactor = clipScaleFactor;

            if (childItem->data(itemTypeIndex).toString().contains("SubPlayer"))
            {
                playerItem = (QGraphicsVideoItem *)childItem;
            }
            else if (childItem->data(itemTypeIndex).toString() == "SubName")
            {
                QGraphicsTextItem *textItem = (QGraphicsTextItem *)childItem;
                textItem->setTextWidth(parentItem->boundingRect().width() * 0.9);
            }
            else if (childItem->data(itemTypeIndex) == "SubDurationLine")
            {
                lineItem->setLine(QLineF(lineItem->pen().width() / 2.0, 0, lineItem->data(mediaDurationIndex).toInt()  * scaleFactor - lineItem->pen().width() / 2.0, 0));
            }
            else if (childItem->data(itemTypeIndex) == "SubProgressLine")
            {
                progressLineItem = lineItem;
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
                progressLineItem->setLine(QLineF(progressLineItem->pen().width() / 2.0, 0, qMax(progressLineItem->parentItem()->boundingRect().width() * m_player->position() / m_player->duration() - progressLineItem->pen().width() / 2.0, progressLineItem->pen().width() * 1.5), 0));

            double minX = parentItem->boundingRect().height() * 0.1;
            double maxX = parentItem->boundingRect().width() - playerItem->boundingRect().width() - minX;
            playerItem->setPos(qMin(qMax(parentItem->boundingRect().width() * m_player->position() / m_player->duration() - playerItem->boundingRect().width() / 2.0, minX), maxX), playerItem->pos().y());
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

void AGView::mouseReleaseEvent(QMouseEvent *event){
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

void AGView::filterItem(QGraphicsItem *mediaItem)
{
    if (!filtering)
        return;

    bool foundMediaFile = false;
    if (searchText == "" || mediaItem->data(fileNameIndex).toString().contains(searchText, Qt::CaseInsensitive))
        foundMediaFile = true;

    foreach (QGraphicsItem *clipItem, scene->items())
    {
        if (clipItem->focusProxy() == mediaItem && clipItem->data(itemTypeIndex).toString() == "Base" && clipItem->data(mediaTypeIndex).toString() == "Clip") //find the clips
        {
            bool foundTag = false;;
            foreach (QGraphicsItem *tagItem, clipItem->childItems())
            {
                if (searchText == "" || (tagItem->data(mediaTypeIndex) == "Tag" && tagItem->data(clipTagIndex).toString().contains(searchText, Qt::CaseInsensitive)))
                    foundTag = true;
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

    arrangeItems(nullptr);
}
