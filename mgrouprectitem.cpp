#include "mgrouprectitem.h"

#include <QGraphicsEffect>
#include <QGraphicsProxyWidget>
#include <QGraphicsWidget>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QSlider>

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
    folderItem->groups << this;

    QPen pen(Qt::transparent);
    setPen(pen);

    setRect(QRectF(0, 0, 200 * 9.0 / 16.0, 200 * 9.0 / 16.0));

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

    if (fileInfo.fileName() != "Project")
    {
    QSlider *audioLevelSlider = new QSlider();

    audioLevelSlider->setMaximum(100);
    audioLevelSlider->setSingleStep(10);
//    audioLevelSlider->setOrientation(Qt::Horizontal);
    audioLevelSlider->setTickPosition(QSlider::TicksBelow);
    audioLevelSlider->setTickInterval(10);
    if (QSettings().value(fileInfo.fileName() + "AudioLevelSlider").toInt() != audioLevelSlider->value())
        audioLevelSlider->setValue(QSettings().value(fileInfo.fileName() + "AudioLevelSlider").toInt());

//    audioLevelSlider->setMinimumHeight(this->rect().height());

    connect(audioLevelSlider, &QSlider::valueChanged, [=] (int value)
    {
        setTextItem(QTime(), QTime());

        if (value != QSettings().value(fileInfo.fileName() + "AudioLevelSlider"))
        {
            QSettings().setValue(fileInfo.fileName() + "AudioLevelSlider", value);
            QSettings().sync();
        }

        foreach(AGMediaFileRectItem *mediaFile, mediaFiles)
        {
            if (mediaFile->m_player != nullptr)
                mediaFile->m_player->setVolume(value);
        }
    });
//      btnuser->setGeometry(this->rect().toRect());
//      btnuser->setText("Test User");
//      QGraphicsProxyWidget *proxy = scene()->addWidget(btnuser);

      audioLevelSliderProxy = new QGraphicsProxyWidget(this); // parent can be NULL
      audioLevelSliderProxy->setWidget(audioLevelSlider);

      audioLevelSliderProxy->setData(mediaTypeIndex, mediaType);
      audioLevelSliderProxy->setData(itemTypeIndex, "SubAudioLevelSlider");

      audioLevelSlider->setMinimumHeight(this->boundingRect().height() * 0.6);
      audioLevelSlider->setMaximumHeight(this->boundingRect().height() * 0.6);

      audioLevelSlider->setStyleSheet("background-color: rgba(0,0,0,0)");
    }

      setTextItem(QTime(), QTime());
}

void MGroupRectItem::setTextItem(QTime time, QTime totalTime)
{
    if (subLogItem == nullptr)
    {
        newSubLogItem();
    }

    if (subLogItem != nullptr)
    {
        if (time != QTime())
            AGViewRectItem::setTextItem(time, totalTime);
        else
        {
            int foundFiles = 0;
            int foundClips = 0;
            int foundFileDuration = 0;
            int foundClipDuration = 0;

            foreach (AGMediaFileRectItem *mediaItem, mediaFiles)
            {
                if (!mediaItem->data(excludedInFilter).toBool())
                {
                    foundFiles++;
                    foundFileDuration += mediaItem->duration;
                }
            }

            if (timelineGroupItem != nullptr)
            {
                foreach (AGClipRectItem *clipItem, timelineGroupItem->clips)
                {
                    if (!clipItem->data(excludedInFilter).toBool())
                    {
                        foundClips++;
                        foundClipDuration += clipItem->duration;
                    }
                }
            }

            duration = foundClipDuration - (foundClips - 1) * AGlobal().frames_to_msec(QSettings().value("transitionTime").toInt());
//            qDebug()<<__func__<<"MGroupRectItem"<<fileInfo.fileName()<<duration<<time<<totalTime;

            if (audioLevelSliderProxy != nullptr)
            {
                QSlider *audioLevelSlider = (QSlider *)audioLevelSliderProxy->widget();
                if (QSettings().value(fileInfo.fileName() + "AudioLevelSlider").toInt() != audioLevelSlider->value())
                    audioLevelSlider->setValue(QSettings().value(fileInfo.fileName() + "AudioLevelSlider").toInt());

                subLogItem->setHtml(tr("<p>%1</p><p><small><i>Audio %2%</i></small></p><p><small><i>%3 %4</i></small></p><p><small><i>%5 %6</i></small></p><p><small><i>%7</i></small></p>").arg(
                                        fileInfo.fileName(),
                                        QString::number(audioLevelSlider->value()),
                                        QString::number(foundFiles) + " file"  + (foundFiles>1?"s":""),
                                        QTime::fromMSecsSinceStartOfDay(foundFileDuration).toString(),
                                        QString::number(foundClips) + " clip"  + (foundFiles>1?"s":""),
                                        QTime::fromMSecsSinceStartOfDay(duration).toString(),
                                        lastOutputString));
            }
            else
                subLogItem->setHtml(tr("<p>%1</p><p><small><i>%2 %3</i></small></p><p><small><i>%4</i></small></p>").arg(
                                        fileInfo.fileName(),
                                        QString::number(foundFiles) + " file"  + (foundFiles>1?"s":""),
                                        QTime::fromMSecsSinceStartOfDay(foundFileDuration).toString(),
                                        lastOutputString));
        }
    }
}

void MGroupRectItem::onItemRightClicked(QPoint pos)
{
    fileContextMenu->clear();

    AGViewRectItem::onItemRightClicked(pos);

    QPointF map = scene()->views().first()->mapToGlobal(QPoint(pos.x()+10, pos.y()));
    fileContextMenu->popup(QPoint(map.x(), map.y()));
}
