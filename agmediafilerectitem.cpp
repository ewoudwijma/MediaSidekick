#include "agcliprectitem.h"
#include "aglobal.h"
#include "agmediafilerectitem.h"
#include "agtagtextitem.h"
#include "agview.h"

#include <QBrush>
#include <QDesktopServices>
#include <QDialog>
#include <QDir>
#include <QGraphicsVideoItem>
#include <QLabel>
#include <QSettings>
#include <QTextBrowser>
#include <QVBoxLayout>
#include <QDateTime>

#include <QMediaService>
#include <QMediaMetaData>
#include <QTabWidget>
#include <QGroupBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QJsonDocument>
#include <QGraphicsProxyWidget>
#include <QPushButton>
#include <QSlider>
#include <QXmlStreamReader>
//#include <QAudioProbe>

#include <QStyle>

#include <QDebug>

#include <QMessageBox>

AGMediaFileRectItem::AGMediaFileRectItem(QGraphicsItem *parent, QFileInfo fileInfo, int duration):
    AGViewRectItem(parent, fileInfo)
{
    this->mediaType = "MediaFile";
    this->itemType = "Base";

    this->duration = duration;

    this->groupItem = (MGroupRectItem *)parent;
    this->groupItem->mediaFiles<<this;
//    setFocusProxy(parent);

//    qDebug()<<"AGMediaFileRectItem::AGMediaFileRectItem" <<fileInfo.fileName();

    QBrush brush;
    brush.setColor(Qt::red);

    int alpha = 60;
    if (AGlobal().audioExtensions.contains(fileInfo.suffix(), Qt::CaseInsensitive))
        setBrush(QColor(0, 128, 0, alpha)); //darkgreen
    else if (AGlobal().imageExtensions.contains(fileInfo.suffix(), Qt::CaseInsensitive))
        setBrush(QColor(218, 130, 42, alpha));
    else if (AGlobal().projectExtensions.contains(fileInfo.suffix(), Qt::CaseInsensitive))
        setBrush(QColor(130, 218, 42, alpha));
    else
        setBrush(QColor(42, 130, 218, alpha)); //#2a82da blue-ish

    setData(itemTypeIndex, itemType);
    setData(mediaTypeIndex, mediaType);

    //add an image
    pictureItem = new QGraphicsPixmapItem(this);
    pictureItem->setData(mediaTypeIndex, "MediaFile");
    pictureItem->setData(itemTypeIndex, "SubPicture");

    QImage image;
    if (AGlobal().projectExtensions.contains(fileInfo.suffix(), Qt::CaseInsensitive))
    {
        if (fileInfo.suffix().toLower() == "mlt")
            image = QImage(":/images/shotcut-logo-320x320.png");
        else if (fileInfo.suffix().toLower() == "xml")
            image = QImage(":/images/adobepremiereicon.png");
        else
            image = QImage(":/MediaSidekick.ico");
    }
    else
        image = QImage(":/images/testbeeld.png");

    QPixmap pixmap = QPixmap::fromImage(image);//.scaled(QSize(200,200 * myImage.height() / myImage.width()))
    pictureItem->setPixmap(pixmap);
    if (image.height() != 0)
        pictureItem->setScale(mediaDefaultHeight / image.height() * 0.8);

    setTextItem(QTime(), QTime()); //Create

    connect(this, &AGMediaFileRectItem::mediaLoaded, this, &AGMediaFileRectItem::onMediaLoaded); //to make sure it is run in the main thread

    onMediaFileChanged(); //load media and exiftool

    QPushButton *playButton = new QPushButton();

    playButton->setIcon(qApp->style()->standardIcon(QStyle::SP_MediaPlay));

    if (AGlobal().videoExtensions.contains(fileInfo.suffix(), Qt::CaseInsensitive))
        playButton->setStyleSheet("background-color: rgba(0,0,0,0);border: 0px;");

    connect(playButton, &QPushButton::clicked, [=] ()
    {
        processAction("actionPlay_Pause");
    });

    if (groupItem->fileInfo.fileName() != "Project")
    {
        playButtonProxy = new QGraphicsProxyWidget(this); // parent can be NULL
        playButtonProxy->setWidget(playButton);

        playButtonProxy->setData(mediaTypeIndex, mediaType);
        playButtonProxy->setData(itemTypeIndex, "SubPlayButton");
    }
}

AGMediaFileRectItem::~AGMediaFileRectItem()
{
    bool somethingKilled = false;
    foreach (AGProcessAndThread *process, processes)
    {
        if ((process->process != nullptr && process->process->state() != QProcess::NotRunning) || (process->jobThread != nullptr && process->jobThread->isRunning()))
        {
//            qDebug()<<"AGMediaFileRectItem::~AGMediaFileRectItem Killing process"<<fileInfo.fileName()<<process->name;//<<process->process<<process->jobThread;
            process->kill();
            somethingKilled = true;
        }
    }

    if (somethingKilled) //add a delay to let process killing do its evil work.
    {
        //https://stackoverflow.com/questions/3752742/how-do-i-create-a-pause-wait-function-using-qt
        //wait
        QTime dieTime= QTime::currentTime().addSecs(1);
        while (QTime::currentTime() < dieTime)
            QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    }
}

void AGMediaFileRectItem::setTextItem(QTime time, QTime totalTime)
{
    if (subLogItem == nullptr)
    {
        newSubLogItem();
    }
//    qDebug()<<__func__<<"setTextItem"<<fileInfo.fileName()<<(m_player!=nullptr?m_player->duration():0)<<duration<<(m_player!=nullptr?m_player->position():0)<<time<<totalTime;

    if (subLogItem != nullptr)
    {
        if (time == QTime())
        {
            int position = 0;
            if (m_player != nullptr)
                position = m_player->position();

            QString newString = "";
            if (fileInfo.lastModified().secsTo(QDateTime::currentDateTime()) < 3600 && qAbs(fileInfo.lastModified().secsTo(fileInfo.birthTime())) > 10) //not within 10 secs of birthTime (then new)
                newString = " Updated(" + QTime::fromMSecsSinceStartOfDay(fileInfo.lastModified().msecsTo(QDateTime::currentDateTime())).toString("mm:ss") + ")";
            else if (fileInfo.birthTime().secsTo(QDateTime::currentDateTime()) < 3600)
                newString = " New(" + QTime::fromMSecsSinceStartOfDay(fileInfo.birthTime().msecsTo(QDateTime::currentDateTime())).toString("mm:ss") + ")";

            if (duration != 0)
            {
                if (AGlobal().videoExtensions.contains(fileInfo.suffix(), Qt::CaseInsensitive))
                {
                    QString createDateString = exiftoolPropertyMap["CreateDate"].value;

                    if (createDateString.right(8) == "00:00:00")
                        createDateString = createDateString.left(10);

                    if (createDateString != "")
                        createDateString = " @ " + createDateString;

                    subLogItem->setHtml(tr("<p>%1<span style=\"color:red\";>%2</span></p><p><small><i>%3</i></small></p><p><small><i>%4</i></small></p><p><small><i>%5</i></small></p>").arg(
                                            fileInfo.fileName(), newString,
                                            QTime::fromMSecsSinceStartOfDay(position).toString() + " / " + QTime::fromMSecsSinceStartOfDay(duration).toString() + createDateString,
                                            ffmpegPropertyMap["Width"].value + " * " + ffmpegPropertyMap["Height"].value + " @ " + ffmpegPropertyMap["VideoFrameRate"].value + "fps",
                                            lastOutputString));
                }
                else if (AGlobal().audioExtensions.contains(fileInfo.suffix(), Qt::CaseInsensitive))
                {
                    QString audioParams = exiftoolPropertyMap["AudioBitrate"].value + " / " + exiftoolPropertyMap["SampleRate"].value + " / " + exiftoolPropertyMap["ChannelMode"].value + " / " + exiftoolPropertyMap["AudioChannels"].value;
                    subLogItem->setHtml(tr("<p>%1<span style=\"color:red\";>%2</span></p><p><small><i>%3</i></small></p><p><small><i>%4</i></small></p><p><small><i>%5</i></small></p>").arg(
                                            fileInfo.fileName(), newString,
                                            QTime::fromMSecsSinceStartOfDay(position).toString() + " / " + QTime::fromMSecsSinceStartOfDay(duration).toString(),
                                            audioParams,
                                            lastOutputString));
                }
                else
                    subLogItem->setHtml(tr("<p>%1<span style=\"color:red\";>%2</span></p><p><small><i>%3</i></small></p><p><small><i>%4</i></small></p>").arg(
                                            fileInfo.fileName(), newString,
                                            QTime::fromMSecsSinceStartOfDay(position).toString() + " / " + QTime::fromMSecsSinceStartOfDay(duration).toString(),
                                            lastOutputString));
            }
            else
                subLogItem->setHtml(tr("<p>%1<span style=\"color:red\";>%2</span></p><p><small><i>%3</i></small></p>").arg(
                                        fileInfo.fileName(), newString,
                                        lastOutputString));
        }
        else
            AGViewRectItem::setTextItem(time, totalTime);
    }
}

void AGMediaFileRectItem::onMediaLoaded(QFileInfo fileInfo, QImage image, int duration, QSize mediaSize, QList<int> samples)
{
    //main, private slot
//    qDebug()<<"AGMediaFileRectItem::onMediaLoaded" << fileInfo.fileName()<<duration<<mediaSize;

    if (QThread::currentThread() != qApp->thread())
        qDebug()<<"AGMediaFileRectItem::onMediaLoaded thread problem"<<fileInfo.fileName();

    if (duration != -1)
        this->duration = duration;

    if (image != QImage())
    {
        if (pictureItem == nullptr)
            pictureItem = new QGraphicsPixmapItem(this);

        QPixmap pixmap = QPixmap::fromImage(image);//.scaled(QSize(200,200 * myImage.height() / myImage.width()))
        pictureItem->setPixmap(pixmap);
        if (image.height() != 0)
            pictureItem->setScale(mediaDefaultHeight / image.height() * 0.8);

        pictureItem->setData(mediaTypeIndex, "MediaFile");
        pictureItem->setData(itemTypeIndex, "SubPicture");

        playButtonProxy->setParentItem(pictureItem);
        playButtonProxy->setScale(1.0 / pictureItem->scale());
//        playButtonProxy->setPos(QPointF(pictureItem->boundingRect().width() * 0.5 * pictureItem->scale() - playButtonProxy->boundingRect().width() * 0.5 * playButtonProxy->scale(), pictureItem->boundingRect().height() * 0.5 * pictureItem->scale() - playButtonProxy->boundingRect().height() * 0.5 * playButtonProxy->scale()));

//        qDebug()<<__func__<<"playButtonProxy"<<fileInfo.fileName()<<pictureItem->boundingRect()<<playButtonProxy->boundingRect()<<playButtonProxy->scale()<<playButtonProxy->pos();
    }

    if (AGlobal().audioExtensions.contains(fileInfo.suffix(), Qt::CaseInsensitive))
    {
        QPainterPath painterPath;
        painterPath.moveTo(0, 0);

        int counter = 0;
        foreach (int sample, samples)
        {
//                qDebug()<<"sample"<<sample;
            painterPath.lineTo(qreal(counter) / samples.count() * duration * 1, sample * mediaDefaultHeight / 100.0);//(1.0 * progressInMSeconds / durationInMSeconds) * durationInMSeconds / 100.0 mediaFileScaleFactor
            counter++;
        }

        QGraphicsPathItem *pathItem = new QGraphicsPathItem(this);
        pathItem->setPath(painterPath);

        pathItem->setData(mediaTypeIndex, "MediaFile");
        pathItem->setData(itemTypeIndex, "SubWave");

        delete pictureItem;
        pictureItem = nullptr;
    }

    if (duration > 0)
    {
        if (durationLine == nullptr)
        {
            durationLine = new QGraphicsRectItem(this);

            if (parentRectItem->fileInfo.fileName() == "Export")
                durationLine->setBrush(Qt::darkGreen);
            else
                durationLine->setBrush(Qt::red);

            durationLine->setData(mediaTypeIndex, "MediaFile");
            durationLine->setData(itemTypeIndex, "SubDurationLine");
        }

        setTextItem(QTime(), QTime());
    }

    AGView *view = (AGView *)scene()->views().first();
    view->arrangeItems(nullptr, __func__);
}

void AGMediaFileRectItem::onMediaFileChanged()
{
    //main
//    qDebug()<<"AGMediaFileRectItem::onMediaFileChanged"<<(QThread::currentThread() == qApp->thread()?"Main":"Thread")<<fileInfo.absoluteFilePath();

    if (QThread::currentThread() != qApp->thread())
        qDebug()<<"AGMediaFileRectItem::onMediaFileChanged thread problem"<<fileInfo.fileName();

//    if (AGlobal().videoExtensions.contains(fileInfo.suffix(), Qt::CaseInsensitive) || AGlobal().audioExtensions.contains(fileInfo.suffix(), Qt::CaseInsensitive) || AGlobal().imageExtensions.contains(fileInfo.suffix(), Qt::CaseInsensitive))
    {

        AGProcessAndThread *process1 = new AGProcessAndThread(this);
        process1->command("Load Media " + fileInfo.fileName(), [=]()
        {
            loadMedia(process1);
        });

        processes<<process1;
        connect(process1, &AGProcessAndThread::processOutput, this, &AGMediaFileRectItem::onProcessOutput);

        process1->start();

        AGProcessAndThread *process2 = new AGProcessAndThread(this);
        process2->command("Load Exiftool " + fileInfo.fileName(), "exiftool1206 -api largefilesupport=1 -s -c \"%02.6f\" \"" + fileInfo.absoluteFilePath() + "\"");

        processes<<process2;
        connect(process2, &AGProcessAndThread::processOutput, this, &AGMediaFileRectItem::onProcessOutput);
        connect(process2, &AGProcessAndThread::processOutput, [=] (QTime time, QTime totalTime, QString event, QString outputString)
        {

            //this is executed in the created thread!

            if (event == "finished")
            {
                if (process2->errorMessage == "")
                {
                    foreach (QString keyValuePair, process2->log)
                    {
                        QStringList keyValueList = keyValuePair.split(" : ");

                        if (keyValueList.count() > 1)
                        {
                            QString propertyName = keyValueList[0].trimmed();
                            QString valueString = keyValueList[1].trimmed();

                            QString categoryName;

                            QStringList toplevelPropertyNames;
                            toplevelPropertyNames << "001 - General" << "002 - Video" << "003 - Audio" << "004 - Location" << "005 - Camera" << "006 - Labels" << "007 - Status" << "008 - Media" << "009 - File" << "010 - Date" << "011 - Artists" << "012 - Keywords" << "013 - Ratings"<< "099 - Other";

                            if (propertyName == "CreateDate")//|| propertyName == "SuggestedName"
                            {
                                categoryName = "001 - General";

                                if (valueString == "0000:00:00 00:00:00")
                                    valueString = "";
                            }
                            else if (propertyName == "Directory")
                                categoryName = "007 - Status";
                            else if ( propertyName == "GPSLatitude" || propertyName == "GPSLongitude" || propertyName == "GPSAltitude")
                            {
                                categoryName = "004 - Location";
                                //change value for Geo data
                                QStringList positiveValues = QStringList() << " m Above Sea Level" << " N" << " E";
                                QStringList negativeValues = QStringList() << " m Below Sea Level" << " S" << " W";

                                foreach (QString value, positiveValues)
                                    if (valueString.contains(value))
                                        valueString = QString::number(valueString.left(valueString.length() - value.length()).toDouble());

                                foreach (QString value, negativeValues)
                                    if (valueString.contains(value))
                                        valueString = QString::number(-valueString.left(valueString.length() - value.length()).toDouble());

                                //init if not done
                                if (exiftoolPropertyMap["GeoCoordinate"].value == "")
                                    exiftoolPropertyMap["GeoCoordinate"].value = ";;";

                                QStringList geoStringList = exiftoolPropertyMap["GeoCoordinate"].value.split(";");
                                if (propertyName == "GPSLatitude")
                                    geoStringList[0] = valueString;
                                else if (propertyName == "GPSLongitude")
                                    geoStringList[1] = valueString;
                                else if (propertyName == "GPSAltitude")
                                    geoStringList[2] = valueString;

        //                        valueMap["GeoCoordinate"][folderFileName] = geoStringList.join(";"); //sets the value

                                propertyName = "GeoCoordinate";
                                valueString = geoStringList.join(";");
                            }
                            else if ( propertyName == "Make" || propertyName == "Model" )
                                categoryName = "005 - Camera";
                            else if (propertyName == "Artist" || propertyName == "Title")
                                categoryName = "006 - Labels";
                            else if (propertyName == "Rating")
                            {
                                categoryName = "006 - Labels";
                            }
                            else if (propertyName == "Keywords")
                            {
                                categoryName = "006 - Labels";
                                valueString = valueString.replace(",", ";");
        //                        qDebug()<<"Keywords assignment"<<fileInfo.absoluteFilePath()<<categoryName<<propertyName<<valueString;
        //                        valueString = exiftoolValueMap[propertyName].value.replace(",", ";"); //sets the value
        //                        exiftoolValueMap[propertyName].value = valueString;
                            }
                            else if (propertyName == "RatingPercent" || propertyName == "SharedUserRating") //video
                            {
            //                        qDebug()<<"SharedUserRating"<<valueString;
                                categoryName = "013 - Ratings";
                                if (valueString.toInt() == 1 ) //== 1)
                                    valueString = "1";
                                else if (valueString.toInt() ==25)// == 25)
                                    valueString = "2";
                                else if (valueString.toInt() ==50)//== 50)
                                    valueString = "3";
                                else if (valueString.toInt() ==75)// 75)
                                    valueString = "4";
                                else if (valueString.toInt() ==99)//== 99)
                                    valueString = "5";
                            }

                            else if (propertyName.contains("Rating"))
                                categoryName = "013 - Ratings";
                            else if (propertyName.contains("Keyword") || propertyName == "Category" || propertyName == "Comment" || propertyName == "Subject")
                                categoryName = "012 - Keywords";
                            else if ( propertyName == "Director" || propertyName == "Producer" || propertyName == "Publisher" || propertyName == "Creator" || propertyName == "Writer" || propertyName.contains("Author")|| propertyName == "ContentDistributor")
                                categoryName = "011 - Artists";
                            else if (propertyName == "Duration")
                            {
                                categoryName = "008 - Media";
                                valueString = valueString.replace(" (approx)", "");
                                QTime fileDurationTime = QTime::fromString(valueString,"h:mm:ss");
                                if (fileDurationTime == QTime())
                                {
                                    QString durationString = valueString;
                                    durationString = durationString.left(durationString.length() - 2); //remove " s"
                                    fileDurationTime = QTime::fromMSecsSinceStartOfDay(int(durationString.toDouble() * 1000.0));
                                }

                                if (fileDurationTime.msecsSinceStartOfDay() == 0)
                                    fileDurationTime = QTime::fromMSecsSinceStartOfDay(24 * 60 * 60 * 1000 - 1);

                                valueString = fileDurationTime.toString("HH:mm:ss.zzz");

        //                        qDebug()<<"exiftool Duration"<<valueString;
                            }
                            else // if (!editMode)
                            {
                                if (propertyName == "ImageWidth" || propertyName == "ImageHeight" || propertyName == "CompressorID" || propertyName == "ImageWidth" || propertyName == "ImageHeight" || propertyName == "VideoFrameRate" || propertyName == "BitDepth" )
                                    categoryName = "002 - Video";
                                else if (propertyName.contains("Audio") || propertyName == "SampleRate" || propertyName == "ChannelMode")
                                    categoryName = "003 - Audio";
                                else if (propertyName.contains("Duration") || propertyName.contains("Image") || propertyName.contains("Video") || propertyName.contains("Compressor") || propertyName == "TrackDuration"  || propertyName == "AvgBitrate" )
                                    categoryName = "008 - Media";
                                else if (propertyName.contains("File") || propertyName.contains("Directory"))
                                    categoryName = "009 - File";
                                else if (propertyName.contains("Date"))
                                    categoryName = "010 - Date";
                                else
                                    categoryName = "099 - Other";
                            }

        //                    QString propertyKey = QString::number(exiftoolPropertyMap[categoryName].count()).rightJustified(3, '0') + " - " + propertyName;
        //                    QString categoryKey = QString::number(exiftoolPropertyMap.count()).rightJustified(3, '0') + " - " + categoryName;

                            MMetaDataStruct metaDataStruct;
                            metaDataStruct.absoluteFilePath = fileInfo.absoluteFilePath();
                            metaDataStruct.categoryName = categoryName;
                            metaDataStruct.propertyName = propertyName;
                            metaDataStruct.propertySortOrder = QString::number(exiftoolCategoryProperyMap[categoryName].count()).rightJustified(3, '0');
                            metaDataStruct.value = valueString;

                            exiftoolCategoryProperyMap[categoryName][propertyName] = metaDataStruct;
                            exiftoolPropertyMap[propertyName] = metaDataStruct;
                        }
                    }
                }

                setTextItem(QTime(),QTime()); //assign exiftool value

    //            qDebug()<<process2->name + " finished"<<fileInfo.fileName();
            } //event == "finished"
        });

        process2->start();

    }
}

void AGMediaFileRectItem::onProcessOutput(QTime time, QTime totalTime, QString event, QString outputString)
{
    AGProcessAndThread *processAndThread = (AGProcessAndThread *)sender();

//    qDebug()<<"AGMediaFileRectItem::onProcessOutput"<<time<<totalTime<<event<<outputString;

//    if (subLogItem != nullptr)
//    {
//        subLogItem->setHtml(tr("<p>%1</p><p>%2</p><p><small><i>%3</i></small></p>").arg(fileInfo.fileName(), time.toString() + " / " + totalTime.toString(), outputString));
//    }
//    qDebug()<<"setOutput"<<subLogItem<<outputString<<this->thread();

    if (event == "finished")
        time = QTime::fromMSecsSinceStartOfDay(duration);

    lastOutputString = outputString;
//    setTextItem(time, totalTime); //done in onPositionChanged

    if (time != QTime() && duration != 0)
    {
        AGView *agView = (AGView *)scene()->views().first();

        if (!agView->playInDialog)
        {
            if (m_player != nullptr)
            {
//                qDebug()<<__func__<<"setPosition"<<time.msecsSinceStartOfDay();
                 m_player->setPosition(time.msecsSinceStartOfDay());
            }
        }
        else
        {
            if (agView->dialogMediaPlayer != nullptr)
            {
                agView->dialogMediaPlayer->setPosition(time.msecsSinceStartOfDay());
            }
        }
    }

    if (event == "finished")
    {
//        subLogItem->setHtml(tr("<small><i>%1</i></small>").arg(processAndThread->errorMessage));
        lastOutputString = processAndThread->errorMessage;
        setTextItem(QTime(), QTime());
    }
}

void AGMediaFileRectItem::processAction(QString action)
{
    if (m_player == nullptr)
        initPlayer(true);

//    qDebug()<<"AGMediaFileRectItem::processAction"<<fileInfo.fileName()<<action<<m_player->position();

    if (action == "actionIn")
    {
        emit addItem(true, "Timeline", "Clip", fileInfo, 3000, m_player->position(), m_player->position() + 3000);
    }
    else if (action == "actionPlay_Pause")
    {
        initPlayer(true);
    }
    else if (action == "actionPrevious_frame")
    {
        onRewind(m_player);
    }
    else if (action == "actionNext_frame")
    {
        onFastForward(m_player);
    }
    else if (action == "actionMute")
    {
        onMuteVideoButton(m_player);
    }
    else if (action == "actionSpeed_Up")
    {
        onSpeed_Up(m_player);
    }
    else if (action == "actionSpeed_Down")
    {
        onSpeed_Down(m_player);
    }
    else if (action == "actionVolume_Up")
    {
        onVolume_Up(m_player);
    }
    else if (action == "actionVolume_Down")
    {
        onVolume_Down(m_player);
    }
    else if (action.contains("Trim"))
    {
        if (m_player->state() != QMediaPlayer::PausedState)
            m_player->pause();

        foreach (AGClipRectItem *clipItem, clips)
        {
            QTime inTime = QTime::fromMSecsSinceStartOfDay(clipItem->clipIn);
            QTime outTime = QTime::fromMSecsSinceStartOfDay(clipItem->clipOut);

            int deltaIn = qMin(1000, inTime.msecsSinceStartOfDay());
            inTime = inTime.addMSecs(-deltaIn);
            int deltaOut = 1000;//fmin(1000, ui->videoWidget->m_player->duration() - outTime.msecsSinceStartOfDay());
            outTime = outTime.addMSecs(deltaOut);
            int clipDuration = outTime.msecsSinceStartOfDay() - inTime.msecsSinceStartOfDay();

            QString inTimeToMMSS;
            if (inTime.msecsSinceStartOfDay() < 60 * 60000)
                inTimeToMMSS = inTime.toString("mm-ss");
            else
                inTimeToMMSS = inTime.toString("HH-mm-ss");

            QString fileNameTarget = fileInfo.completeBaseName() + "+" + inTimeToMMSS + "." + fileInfo.suffix();

            QString sourceFolderFileName = fileInfo.absoluteFilePath();
            QString targetFolderFileName = fileInfo.absolutePath() + "/" + fileNameTarget;

        #ifdef Q_OS_WIN
            sourceFolderFileName = sourceFolderFileName.replace("/", "\\");
            targetFolderFileName = targetFolderFileName.replace("/", "\\");
        #endif

//                QString command = "ffmpeg -i \"" + sourceFolderFileName + "\" -ss " + inTime.toString("HH:mm:ss.zzz") + " -t " + QTime::fromMSecsSinceStartOfDay(clipDuration).toString("hh:mm:ss.zzz") + " -vcodec copy -acodec copy -y \"" + targetFolderFileName + "\""; // -map_metadata 0;
//                QString command = "ffmpeg -ss " + QString::number(inTime.msecsSinceStartOfDay()) + "ms -i \"" + sourceFolderFileName + "\" -t " + QString::number(clipDuration) + "ms -vcodec copy -acodec copy -y \"" + targetFolderFileName + "\""; // -map_metadata 0;


            QString command;
            if (action.contains("lossless"))
                command = "ffmpeg -ss " + inTime.toString("HH:mm:ss.zzz") + " -i \"" + sourceFolderFileName + "\" -t " + QTime::fromMSecsSinceStartOfDay(clipDuration).toString("hh:mm:ss.zzz") + " -vcodec copy -acodec copy -y \"" + targetFolderFileName + "\""; // -map_metadata 0;
            else //encoding
                command = "ffmpeg -ss " + inTime.toString("HH:mm:ss.zzz") + " -i \"" + sourceFolderFileName + "\" -t " + QTime::fromMSecsSinceStartOfDay(clipDuration).toString("hh:mm:ss.zzz") + " -y \"" + targetFolderFileName + "\""; // -map_metadata 0;

            AGProcessAndThread *process = new AGProcessAndThread(this);
            process->command("Trim " + fileInfo.fileName() + " " + QString::number(clipItem->clipIn), command, inTime.msecsSinceStartOfDay());

            processes<<process;
            connect(process, &AGProcessAndThread::processOutput, this, &AGMediaFileRectItem::onProcessOutput);
            connect(process, &AGProcessAndThread::processOutput, [=] (QTime time, QTime totalTime, QString event, QString outputString)
            {
                emit fileWatch(fileInfo.absolutePath() + "/" + fileNameTarget, false);

                if (event == "finished")
                {
                    emit fileWatch(fileInfo.absolutePath() + "/" + fileNameTarget, true, true);

                    if (process->errorMessage != "")
                        QMessageBox::information(this->scene()->views().first(), "Error " + process->name, process->errorMessage);
                    else
                    {
                        QString aLike = "false";
                        QString stars;
                        QStringList tags;
                        foreach (AGTagTextItem *tagItem, clipItem->tags)
                        {
                            if (tagItem->tagName.contains("*"))
                                stars = tagItem->tagName;
                            else if (tagItem->tagName.contains("✔"))
                                aLike = "true";
                            else if (tagItem->tagName != "")
                                tags << tagItem->tagName;
                        }

    //                        qDebug()<<"AGMediaFileRectItem::finished"<<event<<outputString<<aLike<<stars<<tags<<fileInfo.absolutePath() + "/" + fileNameTarget.left(fileNameTarget.lastIndexOf(".")) + ".srt";

                        //create srt file

                        QFile file(fileInfo.absolutePath() + "/" + fileNameTarget.left(fileNameTarget.lastIndexOf(".")) + ".srt");
                        if ( file.open(QIODevice::WriteOnly) )
                        {
                            QTextStream stream( &file );
            //                        int order = clipsTableView->clipsItemModel->index(row, orderAfterMovingIndex).data().toInt();

                            QString srtContentString = "";
            //                        srtContentString += "<o>" + QString::number(order) + "</o>"; //+1 for file trim, +2 for export
                            srtContentString += "<r>" + QString::number(stars.length()) + "</r>";
                            srtContentString += "<a>" + aLike + "</a>";
                            srtContentString += "<t>" + tags.join(";") + "</t>";

                            stream << 1 << endl;
                            stream << QTime::fromMSecsSinceStartOfDay(deltaIn).toString("HH:mm:ss.zzz") << " --> " << QTime::fromMSecsSinceStartOfDay(clipDuration - deltaOut).toString("HH:mm:ss.zzz") << endl;
                            stream << srtContentString << endl;
                            stream << endl;

                            file.close();
                        }
                    }
                }
            });

            process->start();

//                    qDebug()<<"AClipsTableView::onTrim before propertyCopy"<< folderName<<fileName<<fileNameTarget<<parentItem;
//                    emit propertyCopy(currentItem, folderName, fileName, folderName, fileNameTarget);
        }
    }
    else
        AGViewRectItem::processAction(action);

//    setSelected(true); //not because if triggered by clip, the clip should remain selected.
}

void AGMediaFileRectItem::onItemRightClicked(QPoint pos)
{
    AGView *agView = (AGView *)scene()->views().first();

    fileContextMenu->clear();

    QString commandControl = "Ctrl-";
#ifdef Q_OS_MAC
    commandControl = "⌘-";
#endif

    if (m_player != nullptr)
    {
        fileContextMenu->addAction(new QAction("Frame back",fileContextMenu));
        fileContextMenu->actions().last()->setToolTip(tr("<p><b>Go to next or previous frame</b></p>"
                                          "<p><i>Go to next or previous frame</i></p>"
                                          ));
        fileContextMenu->actions().last()->setIcon(qApp->style()->standardIcon(QStyle::SP_MediaSeekBackward));
        fileContextMenu->actions().last()->setShortcut(QKeySequence(tr("Ctrl+Left")));
        connect(fileContextMenu->actions().last(), &QAction::triggered, [=]()
        {
            processAction("actionPrevious_frame");
        });
    }

    if (AGlobal().videoExtensions.contains(fileInfo.suffix(), Qt::CaseInsensitive) || AGlobal().audioExtensions.contains(fileInfo.suffix(), Qt::CaseInsensitive))
    {
        fileContextMenu->addAction(new QAction("Play/Pause",fileContextMenu));
        fileContextMenu->actions().last()->setToolTip(tr("<p><b>Play or pause</b></p>"
                                                         "<p><i>Play or pause the video</i></p>"
                                                         ));
        fileContextMenu->actions().last()->setIcon(qApp->style()->standardIcon(QStyle::SP_MediaPlay));
        fileContextMenu->actions().last()->setShortcut(QKeySequence(tr("Space")));

        connect(fileContextMenu->actions().last(), &QAction::triggered, [=]()
        {
            processAction("actionPlay_Pause");
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
        fileContextMenu->actions().last()->setIcon(qApp->style()->standardIcon(QStyle::SP_MediaSeekForward));
        fileContextMenu->actions().last()->setShortcut(QKeySequence(tr("Ctrl+Right")));
        connect(fileContextMenu->actions().last(), &QAction::triggered, [=]()
        {
            processAction("actionNext_frame");
        });

        fileContextMenu->addAction(new QAction("Stop",fileContextMenu));
        fileContextMenu->actions().last()->setToolTip(tr("<p><b>Stop the video</b></p>"
                                                         "<p><i>Stop the video</i></p>"
                                                         ));
        fileContextMenu->actions().last()->setIcon(qApp->style()->standardIcon(QStyle::SP_MediaStop));
        connect(fileContextMenu->actions().last(), &QAction::triggered, [=]()
        {
            onStop(m_player);
        });

        fileContextMenu->addAction(new QAction("Mute",fileContextMenu));
        fileContextMenu->actions().last()->setToolTip(tr("<p><b>Mute or unmute</b></p>"
                                                         "<p><i>Mute or unmute sound (toggle)</i></p>"
                                                         ));
        fileContextMenu->actions().last()->setIcon(qApp->style()->standardIcon(QStyle::SP_MediaVolume));
        connect(fileContextMenu->actions().last(), &QAction::triggered, [=]()
        {
            processAction("actionMute");
        });

        fileContextMenu->addAction(new QAction("Speed Up",fileContextMenu));
        fileContextMenu->actions().last()->setToolTip(tr("<p><b>Speed</b></p>"
                                                                    "<p><i>Change the play speed of the video</i></p>"
                                                                    "<ul>"
                                                                    "<li>Supported speeds are depending on the media file codec installed on your computer</li>"
                                                                    "</ul>"));
        fileContextMenu->actions().last()->setIcon(qApp->style()->standardIcon(QStyle::SP_MediaVolume));
        fileContextMenu->actions().last()->setShortcut(QKeySequence(tr("Shift+Right")));
        connect(fileContextMenu->actions().last(), &QAction::triggered, [=]()
        {
            processAction("actionSpeed_Up");
        });

        fileContextMenu->addAction(new QAction("Speed Down",fileContextMenu));
        fileContextMenu->actions().last()->setToolTip(tr("<p><b>Speed</b></p>"
                                                                    "<p><i>Change the play speed of the video</i></p>"
                                                                    "<ul>"
                                                                    "<li>Supported speeds are depending on the media file codec installed on your computer</li>"
                                                                    "</ul>"));
        fileContextMenu->actions().last()->setIcon(qApp->style()->standardIcon(QStyle::SP_MediaVolume));
        fileContextMenu->actions().last()->setShortcut(QKeySequence(tr("Shift+Left")));
        connect(fileContextMenu->actions().last(), &QAction::triggered, [=]()
        {
            processAction("actionSpeed_Down");
        });


        fileContextMenu->addAction(new QAction("Volume Up",fileContextMenu));
        fileContextMenu->actions().last()->setToolTip(tr("<p><b>Speed</b></p>"
                                                                    "<p><i>Change the play speed of the video</i></p>"
                                                                    "<ul>"
                                                                    "<li>Supported speeds are depending on the media file codec installed on your computer</li>"
                                                                    "</ul>"));
        fileContextMenu->actions().last()->setIcon(qApp->style()->standardIcon(QStyle::SP_MediaVolume));
        fileContextMenu->actions().last()->setShortcut(QKeySequence(tr("+")));
        connect(fileContextMenu->actions().last(), &QAction::triggered, [=]()
        {
            processAction("actionVolume_Up");
        });

        fileContextMenu->addAction(new QAction("Volume Down",fileContextMenu));
        fileContextMenu->actions().last()->setToolTip(tr("<p><b>Speed</b></p>"
                                                                    "<p><i>Change the play speed of the video</i></p>"
                                                                    "<ul>"
                                                                    "<li>Supported speeds are depending on the media file codec installed on your computer</li>"
                                                                    "</ul>"));
        fileContextMenu->actions().last()->setIcon(qApp->style()->standardIcon(QStyle::SP_MediaVolume));
        fileContextMenu->actions().last()->setShortcut(QKeySequence(tr("-")));
        connect(fileContextMenu->actions().last(), &QAction::triggered, [=]()
        {
            processAction("actionVolume_Down");
        });
    }

    if (AGlobal().videoExtensions.contains(fileInfo.suffix(), Qt::CaseInsensitive) || AGlobal().audioExtensions.contains(fileInfo.suffix(), Qt::CaseInsensitive))
    {
        fileContextMenu->addSeparator();

        fileContextMenu->addAction(new QAction("Trim lossless", fileContextMenu));
        fileContextMenu->actions().last()->setIcon(QIcon(QPixmap::fromImage(QImage(":/Spinner.gif"))));
        connect(fileContextMenu->actions().last(), &QAction::triggered, [=]()
        {
            processAction("Trim lossless");
        });

        fileContextMenu->actions().last()->setToolTip(tr("<p><b>%1</b></p>"
                                                         "<p><i>Create new video file(s) for each clip of %2. </i></p>"
                                                         "<ul>"
                                                         "<li><b>Clips and properties</b>: Clips and properties are copied to the new file(s)</li>"
                                                         "<li><b>Remark</b>: New file(s) will be created. To remove %2: Right click archive</li>"
                                                               "</ul>").arg(fileContextMenu->actions().last()->text(), fileInfo.fileName()));

        fileContextMenu->addAction(new QAction("Trim encoding", fileContextMenu));
        fileContextMenu->actions().last()->setIcon(QIcon(QPixmap::fromImage(QImage(":/Spinner.gif"))));
        connect(fileContextMenu->actions().last(), &QAction::triggered, [=]()
        {
            processAction("Trim encoding");
        });
        fileContextMenu->actions().last()->setToolTip(tr("<p><b>%1</b></p>"
                                                         "<p><i>Use Trim Encoding instead of Trim Lossless if lossless encoding is not producing a correct file. </i></p>"
                                                         "<ul>"
                                                         "<li><b>Example</b>: e.g. lossless trim of ReelSteady Go files results in files with incorrect length and incorrect keyframes. Use Trim Encode in this case!</li>"
                                                               "</ul>").arg(fileContextMenu->actions().last()->text()));

        fileContextMenu->addAction(new QAction("Encode", fileContextMenu));
        fileContextMenu->actions().last()->setIcon(QIcon(QPixmap::fromImage(QImage(":/Spinner.gif"))));
        connect(fileContextMenu->actions().last(), &QAction::triggered, [=]()
        {
            initPlayer(false);

            QString targetFileName = fileInfo.completeBaseName() + "EN." + fileInfo.suffix();

            QStringList ffmpegFiles;
            QStringList ffmpegClips;
            QStringList ffmpegCombines;
            QStringList ffmpegMappings;

            ffmpegFiles << "-i \"" + fileInfo.absoluteFilePath() + "\"";

            int fileReference = 0;

            QString filterClip = tr("[%1:v]").arg(QString::number(fileReference));

            filterClip += tr("setpts=PTS-STARTPTS");

            filterClip += tr("[v%1]").arg(QString::number(fileReference));

            ffmpegClips << filterClip;

            filterClip = tr("[%1:a]").arg(QString::number(fileReference));

//                filterClip += tr("aformat=sample_fmts=fltp:sample_rates=44100:channel_layouts=stereo,asetpts=PTS-STARTPTS");
            filterClip += tr("asetpts=PTS-STARTPTS");

            filterClip += tr("[a%1]").arg(QString::number(fileReference));

            ffmpegClips << filterClip;

            ffmpegMappings << "-map [v0]";
            ffmpegMappings << "-map [a0]";
            ffmpegMappings << "-y \"" + fileInfo.absolutePath() + "/" + targetFileName + "\"";

            QString command = "ffmpeg " + ffmpegFiles.join(" ") + " -filter_complex \"" + ffmpegClips.join(";") + (ffmpegCombines.count()>0?";":"") + ffmpegCombines.join(";") + "\" " + ffmpegMappings.join(" ");

//                command = "ffmpeg " + ffmpegFiles.join(" ") + " -vcodec libx264" + " -y \"" + folderName + targetFileName + "\"";
//                        -ss 00:00:05.240 -to 00:00:08.360    \
//                        -vcodec libx264 -acodec libvo_aacenc \
//                        "Path\Do you want him1.flv"

//                command = "exiftool -api largefilesupport=1 -r -s -c \"%02.6f\" \"" + folderName + "\"";

//                command = "dir enc*";

//            qDebug()<<"ffmpeg cmd"<<command;

            //causes right ordering of mediafile after add item and before mediaLoaded

            AGProcessAndThread *process = new AGProcessAndThread(this);
            process->command("Encode " + fileInfo.fileName(), command);

            processes<<process;
            connect(process, &AGProcessAndThread::processOutput, this, &AGMediaFileRectItem::onProcessOutput);
            connect(process, &AGProcessAndThread::processOutput, [=] (QTime time, QTime totalTime, QString event, QString outputString)
            {
                emit fileWatch(fileInfo.absolutePath() + "/" + targetFileName, false);

                if (event == "finished")
                {
                    emit fileWatch(fileInfo.absolutePath() + "/" + targetFileName, true, true);
//                    QString targetFileName = fileInfo.completeBaseName() + "EN.mp4";

//                    qDebug()<<"Encode finished"<<fileInfo.absolutePath()<<fileInfo.completeBaseName()<<targetFileName;

                    if (process->errorMessage != "")
                        QMessageBox::information(this->scene()->views().first(), "Error " + process->name, process->errorMessage);
                    else
                    {
                        QFile file(fileInfo.absolutePath() + "/" + fileInfo.completeBaseName() + ".srt");
                        if (file.exists())
                           file.copy(fileInfo.absolutePath() + "/" + targetFileName.left(targetFileName.lastIndexOf(".")) + ".srt");

                        emit propertyCopy(nullptr, fileInfo.absolutePath() + "/", fileInfo.fileName(), fileInfo.absolutePath() + "/", targetFileName);
                    }
                }
            });

            process->start();

        });
        fileContextMenu->actions().last()->setToolTip(tr("<p><b>%1</b></p>"
                                                         "<p><i>"
                                                         " %2. </i></p>"
                                                         "<ul>"
                                                         "<li><b>Clips and properties</b>: Clips and properties are copied to the new file(s)</li>"
                                                         "<li><b>Source file</b>: New file(s) will be created. To remove %2: Right click archive</li>"
                                                               "</ul>").arg(fileContextMenu->actions().last()->text(), fileInfo.fileName()));
    } //audio or video

    if (AGlobal().videoExtensions.contains(fileInfo.suffix(), Qt::CaseInsensitive))
    {
        fileContextMenu->addSeparator();

        fileContextMenu->addAction(new QAction("Remux to Mp4 / yuv420 / 4:3",fileContextMenu));
        fileContextMenu->actions().last()->setIcon(QIcon(QPixmap::fromImage(QImage(":/Spinner.gif"))));

        connect(fileContextMenu->actions().last(), &QAction::triggered, [=]()
        {
            initPlayer(false);

            QString sourceFolderFileName = fileInfo.absoluteFilePath();

            QString targetFolderFileName = fileInfo.absolutePath() + "/" + fileInfo.completeBaseName() + "RM.mp4";

        #ifdef Q_OS_WIN
            sourceFolderFileName = sourceFolderFileName.replace("/", "\\");
            targetFolderFileName = targetFolderFileName.replace("/", "\\");
        #endif

            QString command = "ffmpeg -y -i \"" + sourceFolderFileName + "\" -filter:v \"pad=iw:iw*3/4:(ow-iw)/2:(oh-ih)/2\" -aspect 4:3 -pix_fmt yuv420p -y \"" + targetFolderFileName + "\""; //-map_metadata 0  -loglevel +verbose

            //causes right ordering of mediafile after add item and before mediaLoaded

            AGProcessAndThread *process = new AGProcessAndThread(this);
            process->command("Remux " + fileInfo.fileName(), command);
            processes<<process;
            connect(process, &AGProcessAndThread::processOutput, this, &AGMediaFileRectItem::onProcessOutput);
            connect(process, &AGProcessAndThread::processOutput, [=] (QTime time, QTime totalTime, QString event, QString outputString)
            {
                QString targetFileName = fileInfo.completeBaseName() + "RM.mp4";

                emit fileWatch(fileInfo.absolutePath() + "/" + targetFileName, false);

                if (event == "finished")
                {
                    emit fileWatch(fileInfo.absolutePath() + "/" + targetFileName, true, true);

                    if (process->errorMessage != "")
                        QMessageBox::information(this->scene()->views().first(), "Error " + process->name, process->errorMessage);
                    else
                    {
                        QFile file(fileInfo.absolutePath() + "/" + fileInfo.completeBaseName() + ".srt");
                        if (file.exists())
                           file.copy(fileInfo.absolutePath() + "/" + targetFileName.left(targetFileName.lastIndexOf(".")) + ".srt");

                        emit propertyCopy(nullptr, fileInfo.absolutePath() + "/", fileInfo.fileName(), fileInfo.absolutePath() + "/", fileInfo.completeBaseName() + "RM.mp4");
                    }
                }
            });

            process->start();
        });
        fileContextMenu->actions().last()->setToolTip(tr("<p><b>%1</b></p>"
                                                         "<p><i>Remux %2 into a mp4 container, convert video to yuv420 color format and pad to 4:3 ratio</i></p>"
                                                         "<ul>"
                                                         "<li><b>mp4 container</b>: enabling property updates</li>"
                                                         "<li><b>yuv420 color format and 4:3 ratio</b>: enabling Wideview conversion</li>"
                                                         "<li><b>Usage example</b>: <a href=\"https://www.fatshark.com/\">Fatshark</a> DVR</li>"
                                                         "<li>%1 can be time consuming. See Jobs for progress.</li>"
                                                         "<li><b>Source file</b>: A new file will be created. To remove %2: Right click archive</li>"
                                                               "</ul>").arg(fileContextMenu->actions().last()->text(), fileInfo.fileName()));

        fileContextMenu->addAction(new QAction("Create wideview (4:3 to 16:9)",fileContextMenu));
        fileContextMenu->actions().last()->setIcon(QIcon(QPixmap::fromImage(QImage(":/Spinner.gif"))));
        connect(fileContextMenu->actions().last(), &QAction::triggered, [=]()
        {
            initPlayer(false);

            derperView = new ADerperView();
            AGProcessAndThread *process = new AGProcessAndThread(this);
            process->command("Wideview " + fileInfo.fileName(), [=]()
            {
                connect(process, &AGProcessAndThread::stopThreadProcess, derperView, &ADerperView::onStopThreadProcess);

                connect(derperView, &ADerperView::processLog, process, &AGProcessAndThread::addProcessLog);

                derperView->processLog("output", "===================");
                derperView->processLog("output", "WideView by Derperview, Derperview by Banelle: https://github.com/banelle/derperview");
                derperView->processLog("output", "Perform non-linear stretch of 4:3 video to make it 16:9.");
                derperView->processLog("output", "See also Derperview - A Command Line Superview Alternative: https://intofpv.com/t-derperview-a-command-line-superview-alternative");
                derperView->processLog("output", "===================");
                derperView->processLog("output", "Media Sidekick uses unmodified Derperview sourcecode and embedded it in the Qt and Media Sidekick job handling structure.");
                derperView->processLog("output", "Use 'Remux to MP4/Yuv420' to prepare videocontent for Wideview conversion");
                derperView->processLog("output", "===================");

                derperView->Go((fileInfo.absoluteFilePath()).toUtf8().constData(), (fileInfo.absolutePath() + "/" + fileInfo.completeBaseName() + "WV.mp4").toUtf8().constData(), 1);
            });

            processes<<process;
            connect(process, &AGProcessAndThread::processOutput, this, &AGMediaFileRectItem::onProcessOutput);
            connect(process, &AGProcessAndThread::processOutput, [=] (QTime time, QTime totalTime, QString event, QString outputString)
            {
                //this is executed in the created thread!

                QString targetFileName = fileInfo.completeBaseName() + "WV.mp4";

                emit fileWatch(fileInfo.absolutePath() + "/" + targetFileName, false);

                if (event == "finished")
                {
                    emit fileWatch(fileInfo.absolutePath() + "/" + targetFileName, true, true);

                    if (process->errorMessage == "")
                    {
                        QFile file(fileInfo.absolutePath() + "/" + fileInfo.completeBaseName() + ".srt");
                        if (file.exists())
                           file.copy(fileInfo.absolutePath() + "/" + targetFileName.left(targetFileName.lastIndexOf(".")) + ".srt");

                        emit propertyCopy(nullptr, fileInfo.absolutePath() + "/", fileInfo.fileName(), fileInfo.absolutePath() + "/", fileInfo.completeBaseName() + "WV.mp4");
                    }
                    else
                        QMessageBox::information(agView, "Error " + process->name, process->errorMessage);
                }
            });

            process->start();
        });

        fileContextMenu->actions().last()->setToolTip(tr("<p><b>%1 by Derperview</b></p>"
                                                         "<p><i>Perform non-linear stretch of 4:3 video to make it 16:9</i></p>"
                                                         "<ul>"
                                                         "<li><b>Derperview</b>: Courtesy of Banelle to use the Derperview stretching algorithm (see About).</li>"
                                                         "<li><b>yuv420</b>: %2 needs to have 4:3 aspect ratio and yuv420 color format. Use <i>Remux to mp4/yuv420/4:3</i> if needed.</li>"
                                                         "<li>%1 can be time consuming. See Jobs for progress.</li>"
                                                         "<li><b>Source file</b>: A new file will be created. To remove %2: Right click archive</li>"
                                                               "</ul>").arg(fileContextMenu->actions().last()->text(), fileInfo.fileName()));

    }

    fileContextMenu->addSeparator();

    fileContextMenu->addAction(new QAction("Archive file",fileContextMenu));
    fileContextMenu->actions().last()->setIcon(qApp->style()->standardIcon(QStyle::SP_TrashIcon));
    connect(fileContextMenu->actions().last(), &QAction::triggered, [=]()
    {
        //remove player
        agView->stopAndDeletePlayers(fileInfo);

        AGProcessAndThread *process = new AGProcessAndThread(this);
        process->command("Archive file", [=]
        {
            QString recycleFolder = fileInfo.absolutePath() + "/" + "MSKRecycleBin/";

            bool success = true;
            bool supportingFilesOnly = false;

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
                            process->addProcessLog("output", tr("%1 moved to recycle folder").arg(srtFileName));
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

                //last the file itseld
                if (success && !supportingFilesOnly)
                {
                    QFile file(fileInfo.absoluteFilePath());

                    if (file.exists())
                    {
                        recursiveFileRenameCopyIfExists(recycleFolder, fileInfo.fileName());
                        success = file.rename(recycleFolder + fileInfo.fileName());
//                        qDebug()<<"moved"<<success<<tr("%1 moved to recycle folder %2").arg(fileInfo.fileName(),recycleFolder);
                        if (success)
                        {
                            process->addProcessLog("output", tr("%1 moved to recycle folder").arg(fileInfo.fileName()));
//                            qDebug()<<"Undo - archive file"<<fileInfo.fileName() <<recycleFolder;
                            emit addUndo(false, "Archive", "MediaFile", this);
                        }
                    }
                }
           }
           else
              process->addProcessLog("error", QString("-1, could not create folder " + recycleFolder));

        });
        process->start();

    });
    fileContextMenu->actions().last()->setToolTip(tr("<p><b>%1</b></p>"
                                                     "<p><i>Move %2 and its supporting files (clips / .srt files) to the Media Sidekick recycle bin folder. If files do already exist in the recycle bin, these files are renamed first, with BU (Backup) added to their name</i></p>"
                                                           ).arg(fileContextMenu->actions().last()->text(), fileInfo.fileName()));

    if (AGlobal().videoExtensions.contains(fileInfo.suffix(), Qt::CaseInsensitive) || AGlobal().audioExtensions.contains(fileInfo.suffix(), Qt::CaseInsensitive) || AGlobal().imageExtensions.contains(fileInfo.suffix(), Qt::CaseInsensitive))
    {
        fileContextMenu->addAction(new QAction("Add clip",fileContextMenu));
        fileContextMenu->actions().last()->setIcon(qApp->style()->standardIcon(QStyle::SP_TrashIcon));
        fileContextMenu->actions().last()->setShortcut(QKeySequence(tr("Ctrl+I")));

        fileContextMenu->actions().last()->setEnabled(m_player != nullptr);

        connect(fileContextMenu->actions().last(), &QAction::triggered, [=]()
        {
            processAction("actionIn");
        });
        fileContextMenu->actions().last()->setToolTip(tr("<p><b>%1</b></p>"
                                                         "<p><i>Add a clip at the current position of video %2</i></p>"
                                                         "<ul>"
                                                         "<li>Clips can only be added if the current video is played</li>"
                                                         "</ul>"
                                                         ).arg(fileContextMenu->actions().last()->text(), fileInfo.fileName()));

//        //check if clips
//        bool hasClips = false;
//        foreach (QGraphicsItem *childItem, clips)
//        {
//            if (childItem->data(mediaTypeIndex).toString() == "Clip" && childItem->data(itemTypeIndex).toString() == "Base")
//                hasClips = true;
//        }

        if (clips.count() > 0)
        {
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
                                    foreach (AGClipRectItem *clipItem, clips)
                                        groupItem->timelineGroupItem->clips.removeOne(clipItem);
                                    clips.clear(); //remove the clips from this item and its groupitem

//                                    qDebug()<<"Undo - archive srt"<<srtFileName <<recycleFolder;
                                    emit addUndo(false, "Archive", "Clips", this);
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
        }
    }

    fileContextMenu->addSeparator();

    AGViewRectItem::onItemRightClicked(pos);

    fileContextMenu->addAction(new QAction("Properties",fileContextMenu));
    fileContextMenu->actions().last()->setIcon(QIcon(QPixmap::fromImage(QImage(":/MediaSidekick.ico"))));
    connect(fileContextMenu->actions().last(), &QAction::triggered, [=]()
    {
        QDialog * dialog = new QDialog(agView);
        dialog->setWindowTitle("Media Sidekick Properties " + fileInfo.fileName());
    #ifdef Q_OS_MAC
        dialog->setWindowFlag(Qt::WindowStaysOnTopHint); //needed for MAC / OSX
    #endif

        QRect savedGeometry = QSettings().value("Geometry").toRect();
        savedGeometry.setX(savedGeometry.x() + savedGeometry.width()/4);
        savedGeometry.setY(savedGeometry.y() + savedGeometry.height()/4);
        savedGeometry.setWidth(savedGeometry.width()/2);
        savedGeometry.setHeight(savedGeometry.height()/2);
        dialog->setGeometry(savedGeometry);

        QVBoxLayout *mainLayout = new QVBoxLayout(dialog);
        QTabWidget *tabWidget = new QTabWidget(dialog);
        mainLayout->addWidget(tabWidget);

        {
            QScrollArea *scrollArea = new QScrollArea(tabWidget);
            scrollArea->setWidgetResizable(true); //otherwise nothing shown...???

            QWidget *scrollAreaWidget = new QWidget(scrollArea);

            scrollArea->setWidget(scrollAreaWidget);

            QVBoxLayout *scrollAreaBoxLayout = new QVBoxLayout(scrollAreaWidget);
            scrollAreaWidget->setLayout(scrollAreaBoxLayout);

            QMapIterator<QString, QMap<QString, MMetaDataStruct>> categoryIterator(exiftoolCategoryProperyMap);
            while (categoryIterator.hasNext())
            {
                categoryIterator.next();

//                    qDebug()<<"categoryIterator.key()"<<fileInfo.fileName()<<categoryIterator.key();
                QString categoryKey = categoryIterator.key().split(" - ")[1];

                QGroupBox *groupBox = new QGroupBox(scrollAreaWidget);
                groupBox->setTitle(categoryKey);
                scrollAreaBoxLayout->addWidget(groupBox);

                QFormLayout *groupBoxFormLayout = new QFormLayout(groupBox);
                groupBox->setLayout(groupBoxFormLayout);

                QMap<QString, MMetaDataStruct> propertyMap = categoryIterator.value();

                QList<MMetaDataStruct> metaDataListSorted;
                foreach (MMetaDataStruct metaDataStruct, propertyMap)
                    metaDataListSorted.append(metaDataStruct);

                std::sort(metaDataListSorted.begin(), metaDataListSorted.end(), [](MMetaDataStruct v1, MMetaDataStruct v2)->bool
                {
                    return v1.propertySortOrder<v2.propertySortOrder;
                });

                foreach (MMetaDataStruct metaDataStruct, metaDataListSorted)
                {
                    if (metaDataStruct.propertyName == "GeoCoordinate")
                    {
                        QString text = "";
                        QStringList valueList = metaDataStruct.value.split(";");

//                            qDebug()<<"GeoCoordinate"<<valueList;

                        if (valueList.count() > 0 && valueList[0] != "")
                            text += "↕" + QString::number(valueList[0].toDouble(), 'f', 3) + "°";
                        if (valueList.count() > 1 && valueList[1] != "")
                            text += " ↔" + QString::number(valueList[1].toDouble(), 'f', 3) + "°";
                        if (valueList.count() > 2 && valueList[2] != "")
                            text += " Δ" + valueList[2] + "m";

                        groupBoxFormLayout->addRow(metaDataStruct.propertyName, new QLabel(text));
                    }
                    else
                        groupBoxFormLayout->addRow(metaDataStruct.propertyName, new QLabel(metaDataStruct.value));
                }

            }
            tabWidget->addTab(scrollArea, "ExifTool");
        }

        QScrollArea *scrollArea = new QScrollArea(tabWidget);
        scrollArea->setWidgetResizable(true); //otherwise nothing shown...???

        QWidget *scrollAreaWidget = new QWidget(scrollArea);

        scrollArea->setWidget(scrollAreaWidget);

        QFormLayout *scrollAreaFormLayout = new QFormLayout(scrollAreaWidget);
        scrollAreaWidget->setLayout(scrollAreaFormLayout);

        scrollAreaFormLayout->addRow("File size", new QLabel(QString::number(fileInfo.size()/1024/1024) + " MB"));
        scrollAreaFormLayout->addRow("path", new QLabel(fileInfo.path()));
        scrollAreaFormLayout->addRow("absolutePath", new QLabel(fileInfo.absolutePath()));
        scrollAreaFormLayout->addRow("filePath", new QLabel(fileInfo.filePath()));
        scrollAreaFormLayout->addRow("absoluteFilePath", new QLabel(fileInfo.absoluteFilePath()));
        scrollAreaFormLayout->addRow("fileName", new QLabel(fileInfo.fileName()));
        scrollAreaFormLayout->addRow("completeBaseName", new QLabel(fileInfo.completeBaseName()));
        scrollAreaFormLayout->addRow("suffix", new QLabel(fileInfo.suffix()));
        scrollAreaFormLayout->addRow("birthTime", new QLabel(fileInfo.birthTime().toString()));
        scrollAreaFormLayout->addRow("lastModified", new QLabel(fileInfo.lastModified().toString()));
        scrollAreaFormLayout->addRow("lastRead", new QLabel(fileInfo.lastRead().toString()));
        scrollAreaFormLayout->addRow("metadataChangeTime", new QLabel(fileInfo.metadataChangeTime().toString()));

        tabWidget->addTab(scrollArea, "File info");

        if (m_player != nullptr)
        {
            QScrollArea *scrollArea = new QScrollArea(tabWidget);
            scrollArea->setWidgetResizable(true); //otherwise nothing shown...???

            QWidget *scrollAreaWidget = new QWidget(scrollArea);

            scrollArea->setWidget(scrollAreaWidget);

            QVBoxLayout *scrollAreaBoxLayout = new QVBoxLayout(scrollAreaWidget);
            scrollAreaWidget->setLayout(scrollAreaBoxLayout);

            if (m_player->availableMetaData().count() > 0)
            {
                QGroupBox *groupBox = new QGroupBox(scrollAreaWidget);
                groupBox->setTitle("Metadata");
                scrollAreaBoxLayout->addWidget(groupBox);

                QFormLayout *groupBoxFormLayout = new QFormLayout(groupBox);
                groupBox->setLayout(groupBoxFormLayout);

                foreach (QString metadata_key, m_player->availableMetaData())
                {
                    QVariant meta = m_player->metaData(metadata_key);
                    if (meta.toSize() != QSize())
                        groupBoxFormLayout->addRow(metadata_key, new QLabel(QString::number( meta.toSize().width()) + " x " + QString::number( meta.toSize().height())));
                    else if (metadata_key.contains("Duration"))
                    {
                        groupBoxFormLayout->addRow(metadata_key, new QLabel(QTime::fromMSecsSinceStartOfDay(meta.toInt()).toString()));
                    }
                    else
                        groupBoxFormLayout->addRow(metadata_key, new QLabel(meta.toString()));
                }
            }

            QGroupBox *groupBox = new QGroupBox(scrollAreaWidget);
            groupBox->setTitle("Player");
            scrollAreaBoxLayout->addWidget(groupBox);

            QFormLayout *groupBoxFormLayout = new QFormLayout(groupBox);
            groupBox->setLayout(groupBoxFormLayout);

            groupBoxFormLayout->addRow("Position", new QLabel(QTime::fromMSecsSinceStartOfDay(m_player->position()).toString()));
            groupBoxFormLayout->addRow("Duration", new QLabel(QTime::fromMSecsSinceStartOfDay(m_player->duration()).toString()));

            tabWidget->addTab(scrollArea, "QMediaPlayer");
        }

        if (ffmpegPropertyMap.count() > 0) // && ffMpegMetaData.first() != "")
        {
            QScrollArea *scrollArea = new QScrollArea(tabWidget);
            scrollArea->setWidgetResizable(true); //otherwise nothing shown...???

            QWidget *scrollAreaWidget = new QWidget(scrollArea);

            scrollArea->setWidget(scrollAreaWidget);

            QFormLayout *scrollAreaFormLayout = new QFormLayout(scrollAreaWidget);
            scrollAreaWidget->setLayout(scrollAreaFormLayout);

            QList<MMetaDataStruct> metaDataListSorted;
            foreach (MMetaDataStruct metaDataStruct, ffmpegPropertyMap)
                metaDataListSorted.append(metaDataStruct);

            std::sort(metaDataListSorted.begin(), metaDataListSorted.end(), [](MMetaDataStruct v1, MMetaDataStruct v2)->bool
            {
                return v1.propertySortOrder<v2.propertySortOrder;
            });

            foreach (MMetaDataStruct metaDataStruct, metaDataListSorted)
            {
                scrollAreaFormLayout->addRow(metaDataStruct.propertyName, new QLabel(metaDataStruct.value));
            }

            tabWidget->addTab(scrollArea, "FFMpeg");
        }

        dialog->show();
    });
    fileContextMenu->actions().last()->setToolTip(tr("<p><b>Properties</b></p>"
                                    "<p><i>Show properties for the currently selected media item</i></p>"
                                          "<ul>"
                                          "<li><b>Properties by FFMpeg</b>: Temporary available to compare with Exiftool. Available for Videos FFMpeg tool shows average framerate and framerate. Last one is the framerate if no drops are present</li>"
                                          "<li><b>Properties by QMediaplayer</b>: Temporary available to compare with Exiftool. Available for Audio and Videos after they started playing</li>"
                                          "<li><b>Properties by Exiftool</b>: See also property tab in classical mode. Properties are also edited there</li>"
                                          "</ul>"));

    QPointF map = agView->mapToGlobal(QPoint(pos.x()+10, pos.y()));
    fileContextMenu->popup(QPoint(map.x(), map.y()));
}

void AGMediaFileRectItem::onRewind(QMediaPlayer *m_player)
{
    if (m_player->state() != QMediaPlayer::PausedState)
        m_player->pause();
    if (m_player->position() > 1000.0 / m_player->metaData("VideoFrameRate").toDouble())
        m_player->setPosition(m_player->position() - 1000.0 / m_player->metaData("VideoFrameRate").toDouble());
//    qDebug()<<"onPreviousKeyframeButtonClicked"<<m_player->position();
}

void AGMediaFileRectItem::onPlayVideoButton(QMediaPlayer *m_player)
{
//    qDebug()<<"AGMediaFileRectItem::onPlayVideoButton toggle play pause"<<m_player->media().request().url().path();

    if (m_player->state() != QMediaPlayer::PlayingState)
        m_player->play();
    else
        m_player->pause();
}

void AGMediaFileRectItem::onMuteVideoButton(QMediaPlayer *m_player)
{
    m_player->setMuted(!m_player->isMuted());

    if (QSettings().value("muteOn").toBool() != m_player->isMuted())
    {
        QSettings().setValue("muteOn", m_player->isMuted());
        QSettings().sync();
    }
}

void AGMediaFileRectItem::onSpeed_Up(QMediaPlayer *m_player)
{
    qreal playbackRate = m_player->playbackRate();

    if (playbackRate == 0)
        playbackRate = 1;

    m_player->setPlaybackRate(playbackRate * 2.0);

    emit showInStatusBar("Speed " + QString::number(m_player->playbackRate()), 3000);
}

void AGMediaFileRectItem::onSpeed_Down(QMediaPlayer *m_player)
{
    qreal playbackRate = m_player->playbackRate();

    if (playbackRate == 0)
        playbackRate = 1;

    m_player->setPlaybackRate(playbackRate / 2.0);

    emit showInStatusBar("Speed " + QString::number(m_player->playbackRate()), 3000);
}

void AGMediaFileRectItem::onVolume_Up(QMediaPlayer *m_player)
{
    m_player->setVolume(m_player->volume() + 10);

    emit showInStatusBar("Volume " + QString::number(m_player->volume()), 3000);
}

void AGMediaFileRectItem::onVolume_Down(QMediaPlayer *m_player)
{
    m_player->setVolume(m_player->volume() - 10);
    emit showInStatusBar("Volume " + QString::number(m_player->volume()), 3000);
}

void AGMediaFileRectItem::onFastForward(QMediaPlayer *m_player)
{
    if (m_player->state() != QMediaPlayer::PausedState)
        m_player->pause();

    if (m_player->position() < duration - 1000.0 / m_player->metaData("VideoFrameRate").toDouble())
        m_player->setPosition(m_player->position() + 1000.0 / m_player->metaData("VideoFrameRate").toDouble());
//    qDebug()<<"onNextKeyframeButtonClicked"<<m_player->position()<<1000.0 / m_player->metaData("VideoFrameRate").toDouble();
}

void AGMediaFileRectItem::onSkipNext(QMediaPlayer *m_player)
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

void AGMediaFileRectItem::onSkipPrevious(QMediaPlayer *m_player)
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

void AGMediaFileRectItem::onStop(QMediaPlayer *m_player)
{
//    qDebug()<<"AGMediaFileRectItem::onStop"<<m_player->media().request().url().path();

    AGView *view = (AGView *)scene()->views().first();
    view->stopAndDeletePlayers(fileInfo);
    view->arrangeItems(nullptr, ""); //reposition playButton
}

void AGMediaFileRectItem::onSetSourceVideoVolume(QMediaPlayer *m_player, int volume)
{
//    sourceVideoVolume = volume;

//    bool exportFileFound = false;
//    foreach (QString exportMethod, AGlobal().exportMethods)
//        if (fileNameLow.contains(exportMethod))
//            exportFileFound = true;

//    if (!(AGlobal().audioExtensions.contains(extension, Qt::CaseInsensitive) || exportFileFound))
//        m_player->setVolume(volume);
}

void AGMediaFileRectItem::initPlayer(bool startPlaying)
{
    AGView *view = (AGView *)scene()->views().first();

//    qDebug()<<"AGMediaFileRectItem::initPlayer"<<fileInfo.fileName()<<startPlaying;

    if (AGlobal().videoExtensions.contains(fileInfo.suffix(), Qt::CaseInsensitive) || AGlobal().audioExtensions.contains(fileInfo.suffix(), Qt::CaseInsensitive))
    {
        if (!view->playInDialog)
        {
            if (playerItem == nullptr)
            {
//                setBrush(Qt::darkRed);

                playerItem = new QGraphicsVideoItem(this);

                if (AGlobal().videoExtensions.contains(fileInfo.suffix(), Qt::CaseInsensitive))
                    playerItem->setSize(QSize(pictureItem->boundingRect().width() * pictureItem->scale(), pictureItem->boundingRect().height() * pictureItem->scale()));
                else
                    playerItem->setSize(QSize(300.0 * 0.8, mediaDefaultHeight * 0.8));

                playerItem->setPos(boundingRect().height() * 0.1, boundingRect().height() * 0.1);

                playerItem->setData(itemTypeIndex, "SubPlayer");
                playerItem->setData(mediaTypeIndex, "MediaFile");

                playerItem->setFlag(QGraphicsItem::ItemIsSelectable, false);

                playButtonProxy->setParentItem(playerItem);
                playButtonProxy->setScale(1.0 / playerItem->scale());
//                playButtonProxy->setPos(QPointF(playerItem->boundingRect().width() * 0.5 - playButtonProxy->boundingRect().width() * 0.5 * playButtonProxy->scale(), playerItem->boundingRect().height() * 0.5 - playButtonProxy->boundingRect().height() * 0.5 * playButtonProxy->scale()));
                view->arrangeItems(nullptr, ""); //reposition playButton
            }

            if (m_player == nullptr)
            {
//                qDebug()<<"setMedia"<<fileInfo.absoluteFilePath();
                m_player = new QMediaPlayer();
                connect(m_player, &QMediaPlayer::mediaStatusChanged, this, &AGMediaFileRectItem::onMediaStatusChanged);
                connect(m_player, QOverload<>::of(&QMediaPlayer::metaDataChanged), this, &AGMediaFileRectItem::onMetaDataChanged);
                connect(m_player, &QMediaPlayer::metaDataAvailableChanged, this, &AGMediaFileRectItem::onMetaDataAvailableChanged);
                connect(m_player, &QMediaPlayer::positionChanged, this, &AGMediaFileRectItem::onPositionChanged);
                connect(m_player, &QMediaPlayer::stateChanged, this, &AGMediaFileRectItem::onPlayerStateChanged);

                m_player->setVideoOutput(playerItem);

//                m_player->setMuted(AGlobal().videoExtensions.contains(fileInfo.suffix(), Qt::CaseInsensitive));

                if (AGlobal().videoExtensions.contains(fileInfo.suffix(), Qt::CaseInsensitive))
                {
                    m_player->setMuted(QSettings().value("muteOn").toBool());
                }
                else //audio always on
                    m_player->setMuted(false);

                if (QSettings().value(groupItem->fileInfo.fileName() + "AudioLevelSlider").toString() != "")
                    m_player->setVolume(QSettings().value(groupItem->fileInfo.fileName() + "AudioLevelSlider").toInt());

                m_player->setProperty("startPlaying", startPlaying?"true":"false");

                m_player->setMedia(QUrl::fromLocalFile(fileInfo.absoluteFilePath()));
            }
            else
            {
                m_player->setProperty("startPlaying", startPlaying?"true":"false");

                if (m_player->state() != QMediaPlayer::PlayingState)
                    m_player->play();
                else
                    m_player->pause();
            }
        }
        else  //playInDialog
        {
            if (view->dialogMediaPlayer != nullptr && !view->dialogMediaPlayer->media().request().url().path().contains(fileInfo.absoluteFilePath()))
                view->stopAndDeletePlayers();

            if (view->playerDialog == nullptr)
            {
                view->playerDialog = new QDialog(view);
                view->playerDialog->setWindowTitle("Media Sidekick Media player");
            #ifdef Q_OS_MAC
                view->playerDialog->setWindowFlag(Qt::WindowStaysOnTopHint); //needed for MAC / OSX
            #endif

                QRect savedGeometry = QSettings().value("Geometry").toRect();
                savedGeometry.setX(savedGeometry.x() + savedGeometry.width()/4);
                savedGeometry.setY(savedGeometry.y() + savedGeometry.height()/4);
                savedGeometry.setWidth(savedGeometry.width()/2);
                savedGeometry.setHeight(savedGeometry.height()/2);
                view->playerDialog->setGeometry(savedGeometry);

                view->dialogVideoWidget = new QVideoWidget(view->playerDialog);
                view->dialogMediaPlayer = new QMediaPlayer();
                connect(view->dialogMediaPlayer, &QMediaPlayer::mediaStatusChanged, this, &AGMediaFileRectItem::onMediaStatusChanged);
                connect(view->dialogMediaPlayer, QOverload<>::of(&QMediaPlayer::metaDataChanged), this, &AGMediaFileRectItem::onMetaDataChanged);
                connect(view->dialogMediaPlayer, &QMediaPlayer::metaDataAvailableChanged, this, &AGMediaFileRectItem::onMetaDataAvailableChanged);
                connect(view->dialogMediaPlayer, &QMediaPlayer::positionChanged, this, &AGMediaFileRectItem::onPositionChanged);
                connect(view->dialogMediaPlayer, &QMediaPlayer::stateChanged, this, &AGMediaFileRectItem::onPlayerStateChanged);
                view->dialogMediaPlayer->setVideoOutput(view->dialogVideoWidget);

                QVBoxLayout *m_pDialogLayout = new QVBoxLayout(view);

                m_pDialogLayout->addWidget(view->dialogVideoWidget);

                view->playerDialog->setLayout(m_pDialogLayout);

                connect(view->playerDialog, &QDialog::finished, view, &AGView::onPlayerDialogFinished);

                view->playerDialog->show();

                m_player = view->dialogMediaPlayer;
            }

            //if dialogMediaPlayer is not already the current file, make it the current file, otherwise toggle
            if (!view->dialogMediaPlayer->media().request().url().path().contains(fileInfo.absoluteFilePath()))
            {

                view->dialogMediaPlayer->setMuted(AGlobal().videoExtensions.contains(fileInfo.suffix(), Qt::CaseInsensitive));

                view->dialogMediaPlayer->setProperty("startPlaying", startPlaying?"true":"false");

//                qDebug()<<"initPlayer setMedia"<<fileInfo.absoluteFilePath();
                view->dialogMediaPlayer->setMedia(QUrl::fromLocalFile(fileInfo.absoluteFilePath()));

                m_player = view->dialogMediaPlayer;
            }
            else
            {
                if (view->dialogMediaPlayer->state() != QMediaPlayer::PlayingState)
                    view->dialogMediaPlayer->play();
                else
                    view->dialogMediaPlayer->pause();
            }
        }
    }
}

void AGMediaFileRectItem::onMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    QMediaPlayer *m_player = qobject_cast<QMediaPlayer *>(sender());

//    qDebug()<<"AGMediaFileRectItem::onMediaStatusChanged"<<status<<m_player->metaData(QMediaMetaData::Title).toString()<<m_player->media().request().url().path()<<m_player->error()<<m_player->errorString();

    if (status == QMediaPlayer::BufferedMedia)
    {
        if (AGlobal().videoExtensions.contains(fileInfo.suffix(), Qt::CaseInsensitive))
            m_player->setNotifyInterval(1000.0 / m_player->metaData("VideoFrameRate").toDouble());
        else
            m_player->setNotifyInterval(40);
    }
    if (status == QMediaPlayer::LoadedMedia)
    {
//        qDebug()<<__func__<<"setNotifyInterval"<<fileInfo.fileName()<<1000.0 / m_player->metaData("VideoFrameRate").toDouble();
        if (AGlobal().videoExtensions.contains(fileInfo.suffix(), Qt::CaseInsensitive))
            m_player->setNotifyInterval(1000.0 / m_player->metaData("VideoFrameRate").toDouble());
        else
            m_player->setNotifyInterval(40);

//        QString folderFileNameLow = m_player->media().request().url().path().toLower();

        if (AGlobal().videoExtensions.contains(fileInfo.suffix(), Qt::CaseInsensitive))
        {

            m_player->play();
//    #ifdef Q_OS_MAC
//            QSize s1 = size();
//            QSize s2 = s1 + QSize(1, 1);
//            resize(s2);// enlarge by one pixel
//            resize(s1);// return to original size
//    #endif

//            qDebug()<<"onMediaStatusChanged toggle play pause"<<m_player->media().request().url().path()<<m_player->state()<<m_player->property("startPlaying");
            if (m_player->property("startPlaying") != "true")
                m_player->pause();


//            if (m_player->state() != QMediaPlayer::PlayingState)
//                m_player->play();
//            else
//                m_player->pause();

//#ifdef Q_OS_MAC
//        QSize s1 = size();
//        QSize s2 = s1 + QSize(1, 1);
//        resize(s2);// enlarge by one pixel
//        resize(s1);// return to original size
//#endif

        }
        else if (AGlobal().audioExtensions.contains(fileInfo.suffix(), Qt::CaseInsensitive))
        {
//            qDebug()<<"toggle play pause"<<m_player->media().request().url().path()<<m_player->state()<<m_player->property("startPlaying");
            if (m_player->state() != QMediaPlayer::PlayingState && m_player->property("startPlaying") == "true")
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

void AGMediaFileRectItem::onPlayerStateChanged(QMediaPlayer::State status)
{
    QPushButton *playButton = (QPushButton *)playButtonProxy->widget();

    if (status == QMediaPlayer::PausedState)
        playButton->setIcon(qApp->style()->standardIcon(QStyle::SP_MediaPlay));
    if (status == QMediaPlayer::PlayingState)
        playButton->setIcon(qApp->style()->standardIcon(QStyle::SP_MediaPause));
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

void AGMediaFileRectItem::onMetaDataChanged()
{
    QMediaPlayer *m_player = qobject_cast<QMediaPlayer *>(sender());

//    QString folderFileName = m_player->media().request().url().toString();
//#ifdef Q_OS_WIN
//    folderFileName = folderFileName.replace("file:///", "");
//#else
//    folderFileName = folderFileName.replace("file://", ""); //on MAX / OSX foldername should be /users... not users...
//#endif
//    folderFileName = folderFileName.replace("file:", ""); //for network folders?

//    int lastIndexOf = folderFileName.lastIndexOf("/");
//    QString folderName = folderFileName.left(lastIndexOf + 1);
//    QString fileName = folderFileName.mid(lastIndexOf + 1);
//    qDebug()<<"AGMediaFileRectItem::onMetaDataChanged"<<folderName<<fileName<<m_player->metaData(QMediaMetaData::Duration).toString()<<metadatalist.count();

    QImage image = QImage();

    if (AGlobal().audioExtensions.contains(fileInfo.suffix(), Qt::CaseInsensitive))
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
//        qDebug() <<"AGMediaFileRectItem::onMetaDataChanged" << metadata_key << var_data.toString();
    }
}

void AGMediaFileRectItem::onMetaDataAvailableChanged(bool available)
{
//    QMediaPlayer *m_player = qobject_cast<QMediaPlayer *>(sender());
//    qDebug()<<"AGMediaFileRectItem::onMetaDataAvailableChanged"<<available<<m_player->media().request().url().path()<<m_player->metaData(QMediaMetaData::Duration).toString();
}

void AGMediaFileRectItem::onPositionChanged(int progress)
{
    QMediaPlayer *m_player = qobject_cast<QMediaPlayer *>(sender());
//    m_player->setProperty("test", "hi");

//    QString folderFileName = m_player->media().request().url().toString();
//#ifdef Q_OS_WIN
//    folderFileName = folderFileName.replace("file:///", "");
//#else
//    folderFileName = folderFileName.replace("file://", ""); //on MAX / OSX foldername should be /users... not users...
//#endif
//    folderFileName = folderFileName.replace("file:", ""); //for network folders?

//    int lastIndexOf = folderFileName.lastIndexOf("/");
//    QString folderName = folderFileName.left(lastIndexOf + 1);
//    QString fileName = folderFileName.mid(lastIndexOf + 1);

//    qDebug()<<"AGMediaFileRectItem::onPositionChanged"<<fileInfo.fileName()<<progress<<m_player->position()<<m_player->duration()<<m_player->media().request().url().path();

    if (m_player->duration() != 0)
    {
        if (progressSliderProxy == nullptr)
        {
            newProgressItem();

            QSlider *progressSlider = (QSlider *)progressSliderProxy->widget();
            connect(progressSlider, &QSlider::valueChanged, [=] (int value)
            {
                if (sender() != m_player) //move by mouse
                    m_player->setPosition(value); //value is position if exiftool duration
            });
        }

        //update progressLine (as arrangeitem not called here)
        if (progressSliderProxy != nullptr)
        {
            QSlider *progressSlider = (QSlider *)progressSliderProxy->widget();
            progressSlider->setMaximum(m_player->duration());
            progressSlider->setSingleStep(progressSlider->maximum() / 10);
            progressSlider->setValue(progress);
        }

        if (playerItem != nullptr && m_player->duration() != 0) //move video to current position
        {
            double minX = boundingRect().height() * 0.1;
            double maxX = boundingRect().width() - playerItem->boundingRect().width() - minX;

            playerItem->setPos(qMin(qMax(boundingRect().width() * progress / m_player->duration() - playerItem->boundingRect().width() / 2.0, minX), maxX), playerItem->pos().y());
        }

        setTextItem(QTime(), QTime());

        //select all clips of mediafile
        //if position in clip position
        //highlight clip

        //if no running processes
        bool running = false;
        foreach (AGProcessAndThread *process, processes)
        {
            if ((process->process != nullptr && process->process->state() != QProcess::NotRunning) || (process->jobThread != nullptr && process->jobThread->isRunning()))
            {
                running = true;
            }
        }

        if (!running) //no processes in progress
        {
            //if this item in selecteditems
            foreach (AGClipRectItem *clipItem, clips)
            {

    //            if (this->scene()->selectedItems().contains(this) || this->scene()->selectedItems().contains(clipItem))
                {
                    if (clipItem->clipIn <= m_player->position() && clipItem->clipOut >= m_player->position())
                    {
                        //set all other clips off
                        clipItem->scene()->clearSelection();
                        clipItem->setSelected(true);
                        clipItem->scene()->setFocus(Qt::OtherFocusReason);
                    }
                }
            }
        }
    }
}

int xmlRecursive(QXmlStreamReader *reader, int depth)
{
    int count = 0;
    while(reader->readNextStartElement())
    {
        if( true || reader->name() == "property")
        {
            QStringList attributesToText;
            foreach (QXmlStreamAttribute attribute, reader->attributes())
                attributesToText << attribute.name() + "=" + attribute.value();

            if (reader->name() == "property")//xmlRecursive(reader, depth + 1) == 0)
                qDebug() << QString(" ").repeated(depth) + reader->name() + " (" + attributesToText.join(", ") + ")" + reader->readElementText();
            else
            {
                qDebug() << QString(" ").repeated(depth) + reader->name() + " (" + attributesToText.join(", ") + ")";
                xmlRecursive(reader, depth + 1);
            }

        }
        else
            reader->skipCurrentElement();
        count++;
    }

    return count;

}

void AGMediaFileRectItem::loadMedia(AGProcessAndThread *process)
{
    //thread

    if (process->processStopped)
    {
        QString output = "loadMedia processStopped for " + fileInfo.absoluteFilePath();
        process->addProcessLog("output", output);
        qDebug()<<output<<fileInfo.fileName()<<process->processStopped;
        return;
    }

    //set new fileInfo (e.g. updated lastModified)
    QFileInfo fileInfo2(fileInfo.absoluteFilePath());
    fileInfo = fileInfo2;

//    qDebug()<<"AGMediaFileRectItem::loadMedia"<<fileInfo.absoluteFilePath()<<thread()<<process->thread();

    process->addProcessLog("output", "LoadMedia: " + fileInfo.absoluteFilePath());

    DerperView::VideoInfo videoInfo;

    QImage myImage = QImage();

    if (AGlobal().videoExtensions.contains(fileInfo.suffix(), Qt::CaseInsensitive))
    {
        DerperView::InputVideoFile input((fileInfo.absoluteFilePath()).toUtf8().constData());

        videoInfo = input.GetVideoInfo();

        if (input.GetLastError() != 0 || (videoInfo.videoDuration >= -5 && videoInfo.videoDuration <= 5)) //5 because on osx webm with only audio has value 2, mkv negative values (but qAbs not working...)...
        {
            QString errorMessage = "";
            int errorNumber = input.GetLastError();
            if (input.GetLastError() == -13)
                errorMessage = tr("Load Media. Error on opening file %1. Permission denied").arg(fileInfo.fileName());
            else if (input.GetLastError() == -1094995529)
                errorMessage = tr("Load Media. Error on opening file %1. Invalid data found when processing input").arg(fileInfo.fileName());

            if ((videoInfo.videoDuration >= -5 && videoInfo.videoDuration <= 5))
            {
                errorMessage = tr("Load Media. Video file %1 does not contain video (%2)").arg(fileInfo.fileName(), QString::number(qAbs(videoInfo.videoDuration)));
                errorNumber = 1;
            }

            process->addProcessLog("error", "Error: " + errorMessage + "(" + QString::number(errorNumber) + ")");

            myImage = QImage(":/images/testbeeld.png");

            if (pictureItem == nullptr) //no testbeeld added yet
            {
                emit mediaLoaded(fileInfo, myImage, duration , QSize(videoInfo.width,videoInfo.height), QList<int>());
            }
        }
        else
        {

            AVFrame *frame = input.GetNextFrame(true);

            int frameCounter = 0;
            while (frame->width == 0 && frame != nullptr) //audioframes
            {
                frame = input.GetNextFrame();
                frameCounter++;
            }

//            for (int i = 0; i<QSettings().value("clipsFramerateComboBox").toInt();) //skip first second
//            {
////                qDebug()<<"next frame";
//                frame = input.GetNextFrame();
//                if (frame->width != 0 || frame == nullptr)
//                    i++;
//            }


    //                AVPixelFormat pixelFormat;

            if (frame->format == 0 || frame->format == 12 || frame->format == 13 || true)
            {
                //AV_PIX_FMT_YUVJ420P = 12 e.g. gopro
                //AV_PIX_FMT_YUV420P = 0 e.g. ffmpeg lossless and encode
                //AV_PIX_FMT_YUVJ422P = 13 (e.g. fatshark avi)

                myImage = QImage(frame->width,frame->height,QImage::Format_RGB888);

                //https://forum.qt.io/topic/22137/convert-yuv420-to-rgb-and-show-on-qt-pixmap/2
                for (int y = 0; y < frame->height; y++)
                {
                    for (int x = 0; x < frame->width; x++)
                    {
                        const int xx = x >> 1;
                        const int yy = y >> 1;
                        const int Y = frame->data[0][y * frame->linesize[0] + x] - 16;
                        const int U = frame->data[1][yy * frame->linesize[1] + xx] - 128;
                        const int V = frame->data[2][yy * frame->linesize[2] + xx] - 128;
                        const int r = qBound(0, (298 * Y + 409 * V + 128) >> 8, 255);
                        const int g = qBound(0, (298 * Y - 100 * U - 208 * V + 128) >> 8, 255);
                        const int b = qBound(0, (298 * Y + 516 * U + 128) >> 8, 255);

                        myImage.setPixel(x, y, qRgb(r, g, b));
                    }
                }
            }
            else
                myImage = QImage((uchar*)frame->data[0], frame->width, frame->height, frame->linesize[0], QImage::Format_RGB888);

//            while (frame != nullptr)
//            {
//                qDebug()<<frameCounter;
//                frame = input.GetNextFrame();
//                frameCounter++;
//            }

            int duration;
            QStringList ffMpegPropertyStringList;
            if (videoInfo.avg_frame_rate.num != 0)
            {
                if (videoInfo.totalFrames != 0)
                    duration = 1000.0 * videoInfo.totalFrames / videoInfo.avg_frame_rate.num * videoInfo.avg_frame_rate.den;
                else //in mkv/mp4 files downloaded by youtube-dl for some reason
                    duration = videoInfo.duration / 1000.0;

                ffMpegPropertyStringList << "Width = " + QString::number(videoInfo.width);
                ffMpegPropertyStringList << "Height = " + QString::number(videoInfo.height);
                ffMpegPropertyStringList << "TotalFrames = " + QString::number(videoInfo.totalFrames);
                ffMpegPropertyStringList << "Duration = " + QString::number(videoInfo.duration);
                ffMpegPropertyStringList << "VideoDuration = " + QString::number(videoInfo.videoDuration);
                ffMpegPropertyStringList << "AudioDuration = " + QString::number(videoInfo.audioDuration);
                if (videoInfo.frameRate.den != 0)
                    ffMpegPropertyStringList << "VideoFrameRate = " + QString::number(qreal(videoInfo.frameRate.num) / qreal(videoInfo.frameRate.den));
                else
                    ffMpegPropertyStringList << "VideoFrameRate = " + QString::number(videoInfo.frameRate.num);
                if (videoInfo.avg_frame_rate.den != 0)
                    ffMpegPropertyStringList << "AverageFrameRate = " + QString::number(qreal(videoInfo.avg_frame_rate.num) / qreal(videoInfo.avg_frame_rate.den));
                else
                    ffMpegPropertyStringList << "AverageFrameRate = " + QString::number(videoInfo.avg_frame_rate.num);
            }
            else
                duration = 0; //for images

            foreach (QString keyValuePair, ffMpegPropertyStringList)
            {
                QStringList keyValueList = keyValuePair.split(" = ");

                MMetaDataStruct metaDataStruct;
                metaDataStruct.propertyName = keyValueList[0].trimmed();
                metaDataStruct.propertySortOrder = QString::number(ffmpegPropertyMap.count()).rightJustified(3, '0');
                metaDataStruct.value = keyValueList[1].trimmed();
                ffmpegPropertyMap[metaDataStruct.propertyName] = metaDataStruct;
            }

//            qDebug()<<"AGMediaFileRectItem::loadMedia videoinfo"<<duration<<videoInfo.totalFrames << videoInfo.avg_frame_rate.num<<videoInfo.avg_frame_rate.den;

            process->addProcessLog("output", "LoadMedia->mediaLoaded: " + fileInfo.absoluteFilePath() + " d: " + QString::number(duration) + " s: " + QString::number(videoInfo.width) + "x" + QString::number(videoInfo.height) + " m: " + ffMpegPropertyStringList.join(";"));

            emit mediaLoaded(fileInfo, myImage, duration , QSize(videoInfo.width,videoInfo.height), QList<int>());

    //                qDebug()<<"AGMediaFileRectItem::loadMedia frame"<<folderName<<fileInfo.fileName()<<videoInfo.audioSampleRate<<videoInfo.totalFrames<<videoInfo.audioBitRate<<videoInfo.audioChannelLayout<<videoInfo.audioTimeBase.num << videoInfo.audioTimeBase.den;
            input.Dump();
        }
    } //if video
    else if (AGlobal().imageExtensions.contains(fileInfo.suffix(), Qt::CaseInsensitive))
    {
        myImage.load(fileInfo.absoluteFilePath());

        process->addProcessLog("output", "LoadMedia->mediaLoaded: " + fileInfo.absolutePath() + "/" + " s: " + QString::number(myImage.width()) + "x" + QString::number(myImage.height()));

        emit mediaLoaded(fileInfo, myImage, 0 , QSize(myImage.width(), myImage.height()), QList<int>()); //as not always image loaded (e.g. png currently)
    }
    else if (AGlobal().audioExtensions.contains(fileInfo.suffix(), Qt::CaseInsensitive))
    {
        DerperView::InputVideoFile input((fileInfo.absoluteFilePath()).toUtf8().constData());

        videoInfo = input.GetVideoInfo();

        if (input.GetLastError() != 0 || videoInfo.audioDuration == 0)
        {
            QString errorMessage = "";
            int errorNumber = input.GetLastError();
            if (input.GetLastError() == -13)
                errorMessage = tr("Load Media. Error on opening file %1. Permission denied").arg(fileInfo.fileName());
            else if (input.GetLastError() == -1094995529)
                errorMessage = tr("Load Media. Error on opening file %1. Invalid data found when processing input").arg(fileInfo.fileName());

            if (videoInfo.audioDuration == 0)
            {
                errorMessage = tr("Load Media. Video file %1 does not contain audio").arg(fileInfo.fileName());;
                errorNumber = 1;
            }


            process->addProcessLog("error", "Error: " + errorMessage + "(" + QString::number(errorNumber) + ")");

        }
        else
        {
            AVFrame *frame = input.GetNextFrame();

            QList<int> samples;

            int counter = 0;
            int progressInMSeconds = 0;
            int durationInMSeconds = 1000 * videoInfo.audioDuration / videoInfo.audioTimeBase.den * videoInfo.audioTimeBase.num;
            while (frame != nullptr)
            {
                if (process->processStopped)
                {
                    QString output = "loadMedia audio processStopped for " + fileInfo.absoluteFilePath();
                    process->addProcessLog("output", output);
                    qDebug()<<output<<fileInfo.fileName()<<process->processStopped;
                    return;
                }

                //https://stackoverflow.com/questions/37151584/de-quantising-audio-with-ffmpeg
                //https://steemit.com/programming/@targodan/decoding-audio-files-with-ffmpeg
                float sample_total = 0;
                float sample_min = 1e99;
                float sample_max = -1e99;

                //                    if (frame->width == 0) // Audio - stream it through
                if (frame->format == AV_SAMPLE_FMT_FLTP )
                {
                    for (int c = 0; c < 1; c++)//frame->channels
                    {
                        unsigned char *samples = frame->extended_data[c];
                        for (int i = 0; i < frame->nb_samples; i++)
                        {
                            float sample = samples[i];
                            sample_total += sample;

                            if (sample_min == 1e99)
                                sample_min = sample;
                            else
                                sample_min = qMin(sample_min, sample);

                            if (sample_max == -1e99)
                                sample_max = sample;
                             else
                                sample_max = qMax(sample_max, sample);
                            // now this sample is accessible, it's in the range [-1.0, 1.0]
                        }
                    }
                }

                progressInMSeconds = 1000 * frame->pkt_dts / videoInfo.audioTimeBase.den * videoInfo.audioTimeBase.num;
//                    qDebug()<<"frame"<<counter<<frame->pts<<frame->pts/368640.0<<frame->pkt_dts<<frame->best_effort_timestamp<<frame->format<<frame->channels<<frame->nb_samples<<sample_total<<sample_min<<sample_max<<sample_total / frame->nb_samples / frame->channels<<frame->extended_data[0][frame->nb_samples / 2]<<progressInMSeconds;

                //frame->sample_rate=44.100

                if (counter%20 == 0)
                {
//                    qDebug()<<counter<<progressInMSeconds / 100<<durationInMSeconds<<frame->extended_data[0][frame->nb_samples / 2]/2.56;
//                    painterPath.lineTo((1.0 * progressInMSeconds / durationInMSeconds) * durationInMSeconds / 500.0, sample_total / frame->nb_samples / frame->channels);
                    samples << frame->extended_data[0][frame->nb_samples / 2]/2.56;
                }

                counter++;
                frame = input.GetNextFrame();
            }

//                qDebug()<<"frames"<<counter;


//                qDebug()<<"AGMediaFileRectItem::loadMedia frame"<<fileInfo.fileName()<<videoInfo.audioSampleRate<<videoInfo.audioBitRate<<videoInfo.audioChannelLayout<<videoInfo.audioTimeBase.num << videoInfo.audioTimeBase.den<<videoInfo.duration<<durationInMSeconds;
            //audioTimeBase is like fps

            process->addProcessLog("output", "LoadMedia->mediaLoaded: " + fileInfo.absoluteFilePath() + " d: " + QString::number(durationInMSeconds) + " s: " + QString::number(samples.count()));

            emit mediaLoaded(fileInfo, QImage(), durationInMSeconds , QSize(), samples); //as not always image loaded (e.g. png currently)

            input.Dump();
        }

    }  //if audio
    else if (AGlobal().projectExtensions.contains(fileInfo.suffix(), Qt::CaseInsensitive))
    {
        QFile file(fileInfo.absoluteFilePath());

        QStringList list;

        if (file.open(QIODevice::ReadOnly))
        {

            QTextStream in(&file);
            while (!in.atEnd())
               {
                  QString line = in.readLine();
                  list << line;
               }

            file.close();
        }

        QString xmlString = "";
        foreach (QString line, list)
        {
            process->addProcessLog("output", line);
            xmlString += line;
        }

        if (fileInfo.suffix() == "mlt")
        {
//            qDebug() << __func__ << fileInfo.fileName();

//            QXmlStreamReader reader(xmlString);
//            xmlRecursive(&reader, 0);

//            if (reader.readNextStartElement())
//            {
//                    if (true || reader.name() == "mlt")
//                    {
//                        QStringList readerToText;
//                        foreach (QXmlStreamAttribute attribute, reader.attributes())
//                            readerToText << attribute.name() + "=" + attribute.value();
//                        qDebug() << reader.name() + " (" + readerToText.join(", ") + ")";

//                        while(reader.readNextStartElement())
//                        {
//                            if(true || reader.name() == "producer")
//                            {
//                                QStringList readerToText;
//                                foreach (QXmlStreamAttribute attribute, reader.attributes())
//                                    readerToText << attribute.name() + "=" + attribute.value();
//                                qDebug() << "  " + reader.name() + " (" + readerToText.join(", ") + ")";

//                                while(reader.readNextStartElement())
//                                {
//                                    if( true || reader.name() == "property")
//                                    {
//                                        QStringList readerToText;
//                                        foreach (QXmlStreamAttribute attribute, reader.attributes())
//                                            readerToText << attribute.name() + "=" + attribute.value();
//                                        qDebug() << "     " + reader.name() + " (" + readerToText.join(", ") + ")";

//                                        while(reader.readNextStartElement())
//                                        {
//                                            if( true || reader.name() == "property")
//                                            {
//                                                QStringList readerToText;
//                                                foreach (QXmlStreamAttribute attribute, reader.attributes())
//                                                    readerToText << attribute.name() + "=" + attribute.value();
//                                                qDebug() << "     " + reader.name() + " (" + readerToText.join(", ") + ")";
//                                            }
//                                            else
//                                                reader.skipCurrentElement();
//                                        }

//                                    }
//                                    else
//                                        reader.skipCurrentElement();
//                                }
//                            }
//                            else
//                                reader.skipCurrentElement();
//                        }
//                    }
//                    else
//                        reader.raiseError(QObject::tr("Incorrect file"));
//            }
//            while(!reader.atEnd() && !reader.hasError()) {
//                if(reader.readNext() == QXmlStreamReader::StartElement && reader.name() == "id") {
//                    qDebug() << reader.readElementText();
//                }
//            }
        }


    }
}
