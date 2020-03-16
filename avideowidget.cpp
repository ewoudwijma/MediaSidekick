#include "aglobal.h"
#include "avideowidget.h"

#include <QAbstractVideoSurface>
#include <QDebug>
#include <QFileInfo>
#include <QFileSystemModel>
#include <QMediaMetaData>
#include <QSettings>
#include <QTime>
#include <QTimer>

#include <QStyle>

#include <QMediaMetaData>

//http://digital-thinking.de/cv-opengl-and-qt-video-as-a-texture/
//https://stackoverflow.com/questions/26229633/use-of-qabstractvideosurface
//https://stackoverflow.com/questions/32952429/how-to-implement-qabstractvideosurface-and-monitor-video-per-frame-in-qtcreator

AVideoWidget::AVideoWidget(QWidget *parent) : QVideoWidget(parent)
  , m_position(0)
  , m_previousIn(-1)
  , m_previousOut(-1)
  , m_duration(0),
    m_isSeekable(false)
{
    QRect savedGeometry = QSettings().value("Geometry").toRect();

//    qDebug()<<"AVideoWidget::AVideoWidget"<<topLevelWidget()<<topLevelWidget()->size().height()<<savedGeometry.height();
    setMinimumSize(minimumSize().width(), qMax(100,int(savedGeometry.height() / 5.0)));

    m_player = new QMediaPlayer(this);
//    auto pal = palette();

//    // Sets the brush pixmap to pixmap. The style is set to Qt::TexturePattern.
//    pal.setBrush(QPalette::Window, QBrush(QPixmap (":/acvclogo.png")));
//    setAutoFillBackground(true);
//    setPalette(pal);

    m_player->setVideoOutput(this);

    connect(m_player, &QMediaPlayer::durationChanged, this, &AVideoWidget::onDurationChanged);
    connect(m_player, &QMediaPlayer::positionChanged, this, &AVideoWidget::onPlayerPositionChanged);
    connect(m_player, &QMediaPlayer::stateChanged, this, &AVideoWidget::onPlayerStateChanged);
    connect(m_player, &QMediaPlayer::mediaStatusChanged, this, &AVideoWidget::onMediaStatusChanged);
    connect(m_player, &QMediaPlayer::mutedChanged, this, &AVideoWidget::onMutedChanged);
    connect(m_player, &QMediaPlayer::playbackRateChanged, this, &AVideoWidget::onPlaybackRateChanged);
    connect(m_player, QOverload<>::of(&QMediaPlayer::metaDataChanged), this, &AVideoWidget::onMetaDataChanged);
    connect(m_player, &QMediaPlayer::metaDataAvailableChanged, this, &AVideoWidget::onMetaDataAvailableChanged);

    m_scrubber = new SScrubBar(this);
//    m_scrubber->setFocusPolicy(Qt::NoFocus);
    m_scrubber->setObjectName("m_scrubber");
    m_scrubber->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
//    m_scrubber->setMinimumSize(200,200);
    m_scrubber->setEnabled(true);
    m_scrubber->readOnly = false;
    m_scrubber->setScale(10000);
    m_scrubber->onSeek(0);

    connect(m_scrubber, &SScrubBar::seeked, this, &AVideoWidget::onScrubberSeeked);
    connect(m_scrubber, &SScrubBar::scrubberInChanged, this, &AVideoWidget::onScrubberInChanged);
    connect(m_scrubber, &SScrubBar::scrubberOutChanged, this, &AVideoWidget::onScrubberOutChanged);

    m_scrubber->setToolTip(tr("<p><b>Scrub bar</b></p>"
                              "<p><i>Show and change current position and clips</i></p>"
                              "<ul>"
                              "<li>Drag begin of clip: Change in point</li>"
                              "<li>Drag end of clip: Change out point</li>"
                              "<li>Drag middle of clip: Change in and out point, duration unchanged</li>"
                              "<li>Drag outside of clip: Change position</li>"
                              "</ul>"));

    isTimelinePlaymode = false;
    isLoading = false;

    lastHighlightedRow = -1;

    selectedFileName = "";

    QVBoxLayout *parentLayout = qobject_cast<QVBoxLayout *>(parent->layout());
    QTimer::singleShot(0, this, [this, parentLayout]()->void
    {
                           parentLayout->insertWidget(1, m_scrubber);
    });
}

void AVideoWidget::onFolderIndexClicked(QModelIndex )//index
{
    selectedFolderName = QSettings().value("selectedFolderName").toString();
//    qDebug()<<"AVideoWidget::onFolderIndexClicked"<<index.data().toString()<<selectedFolderName;
    m_player->stop();
    m_scrubber->clearInOuts();
    selectedFileName = "";
}

void AVideoWidget::onFileIndexClicked(QModelIndex index, QStringList filePathList)
{
    int lastIndexOf = filePathList[0].lastIndexOf("/");
    selectedFolderName = filePathList[0].left(lastIndexOf + 1);
    selectedFileName = filePathList[0].mid(lastIndexOf + 1);

//    qDebug()<<"AVideoWidget::onFileIndexClicked"<<index.column()<<index.data().toString()<<selectedFolderName + selectedFileName<<QSettings().value("frameRate").toInt();
    oldState = m_player->state();

    m_player->setMedia(QUrl::fromLocalFile(selectedFolderName + selectedFileName));

    lastHighlightedRow = -1;
}

void AVideoWidget::onClipIndexClicked(QModelIndex index)
{
    QString folderName = index.model()->index(index.row(),folderIndex).data().toString();
    QString fileName = index.model()->index(index.row(),fileIndex).data().toString();
    QString folderFileName = folderName + fileName;
    QTime inTime = QTime::fromString(index.model()->index(index.row(),inIndex).data().toString(),"HH:mm:ss.zzz");
    QTime outTime = QTime::fromString(index.model()->index(index.row(),outIndex).data().toString(),"HH:mm:ss.zzz");

    if (selectedFileName != fileName)
        selectedFileName = fileName;

//    qDebug()<<"AVideoWidget::onClipIndexClicked"<<QUrl::fromLocalFile(folderFileName)<<m_player->media().canonicalUrl();

    if (QUrl::fromLocalFile(folderFileName) != m_player->currentMedia().canonicalUrl()) //another media file
    {
//        qDebug()<<"AVideoWidget::onClipIndexClicked"<<index.data().toString()<<selectedFolderName + selectedFileName;
        oldState = m_player->state();
        m_player->setMedia(QUrl::fromLocalFile(folderFileName));
    }

//    qDebug()<<"AVideoWidget::onClipIndexClicked"<<index.column()<<m_player->position();

    if (index.column() == inIndex)
        m_player->setPosition(inTime.msecsSinceStartOfDay());
    else if (index.column() == outIndex)
        m_player->setPosition(outTime.msecsSinceStartOfDay());
    else if (index.column() == durationIndex)
        m_player->setPosition(inTime.msecsSinceStartOfDay() + inTime.msecsTo(outTime) / 2);
//    else
//        m_player->pause();
}

void AVideoWidget::onClipsChangedToVideo(QAbstractItemModel *itemModel)
{
    isLoading = true;
    m_scrubber->clearInOuts();

    if (selectedFileName.toLower().contains(".mp3"))
        m_scrubber->setAudioOrVideo("A");
    else
        m_scrubber->setAudioOrVideo("V");

    AClipsSortFilterProxyModel *clipsitemModel = qobject_cast<AClipsSortFilterProxyModel *>(itemModel);
//    qDebug()<<"AVideoWidget::onClipsChangedToVideo"<<itemModel<<clipsitemModel;

    if (clipsitemModel == nullptr) //not clip
        m_scrubber->readOnly = true;
    else
        m_scrubber->readOnly = false;

    for (int row = 0; row < itemModel->rowCount();row++)
    {
        QString fileName = itemModel->index(row,fileIndex).data().toString();
        if (fileName == selectedFileName)
        {
            QTime inTime = QTime::fromString(itemModel->index(row,inIndex).data().toString(),"HH:mm:ss.zzz");
            QTime outTime = QTime::fromString(itemModel->index(row,outIndex).data().toString(),"HH:mm:ss.zzz");
            int clipCounter = itemModel->index(row, orderBeforeLoadIndex).data().toInt();

            QString AV;
            if (fileName.toLower().contains(".mp3"))
                AV = "A";
            else
                AV = "V";

            m_scrubber->setInOutPoint(AV, clipCounter, inTime.msecsSinceStartOfDay(), outTime.msecsSinceStartOfDay());
        }
    }
    isLoading = false;
}

void AVideoWidget::onDurationChanged(int duration)
{
//    qDebug()<<"AVideoWidget::onDurationChanged" << duration<<AGlobal().msec_rounded_to_fps(duration)<<AGlobal().msec_to_time(duration);

    if (duration > 0)
    {
        playerDuration = duration;
        m_scrubber->setScale(AGlobal().msec_rounded_to_fps(duration));

        m_isSeekable = true;
    }

    emit durationChanged(duration);
}

void AVideoWidget::onPlayerPositionChanged(int progress)
{
   m_scrubber->onSeek(AGlobal().msec_rounded_to_fps(progress));

   if (!isTimelinePlaymode)
   {
       int *prevRow = new int();
       int *nextRow = new int();
       int *relativeProgress = new int();
       m_scrubber->progressToRow("V", progress, prevRow, nextRow, relativeProgress);
//       qDebug()<<"AVideoWidget::onPlayerPositionChanged"<<progress<<*prevRow<<*nextRow<<*relativeProgress<<AGlobal().msec_to_frames(progress);
       if (*prevRow == *nextRow)
            lastHighlightedRow = *prevRow;
       emit videoPositionChanged(AGlobal().msec_rounded_to_fps(progress), *prevRow, *relativeProgress);
   }
   m_position = progress;
}

void AVideoWidget::onTimelinePositionChanged(int progress, int row, int relativeProgress)
{
    qDebug()<<"AVideoWidget::onTimelinePositionChanged"<<progress<<row<<relativeProgress<<AGlobal().msec_to_frames(progress);
    isTimelinePlaymode = true;
}

void AVideoWidget::onSpinnerPositionChanged(int frames)
{
//    qDebug()<<"AVideoWidget::onSpinnerPositionChanged"<<frames;
    isTimelinePlaymode = false;

//    m_player->pause();
    m_player->setPosition(AGlobal().frames_to_msec(frames));
}

void AVideoWidget::onReleaseMedia(QString folderName, QString fileName)
{
    if (folderName == selectedFolderName && fileName == selectedFileName)
    {
//        qDebug()<<"AVideoWidget::onReleaseMedia"<<folderFileName;
        m_player->stop();
        m_player->setMedia(QMediaContent());
    }
}

void AVideoWidget::onPlayerStateChanged(QMediaPlayer::State state)
{
    emit playerStateChanged(state);
}

void AVideoWidget::onMediaStatusChanged(QMediaPlayer::MediaStatus status)//
{
    qDebug()<<"AVideoWidget::onMediaStatusChanged"<<status<<m_player->metaData(QMediaMetaData::Title).toString()<<m_player->media().canonicalUrl()<<m_player->error()<<m_player->errorString();

//    if (status == QMediaPlayer::BufferedMedia)
//    {
//        m_player->setPosition(0);
//    }

    if (status == QMediaPlayer::LoadedMedia)
    {
//        resize(10,10);
        m_player->play();
#ifdef Q_OS_MAC
        QSize s1 = size();
        QSize s2 = s1 + QSize(1, 1);
        resize(s2);// enlarge by one pixel
        resize(s1);// return to original size
#endif
        if (oldState != QMediaPlayer::PlayingState)
            m_player->pause();
        m_player->setNotifyInterval(AGlobal().frames_to_msec(1));
        m_scrubber->setFramerate(QSettings().value("frameRate").toInt());
    //    m_player->setMuted(!selectedFileName.contains(".mp3"));

        if (selectedFileName.toLower().contains(".mp3") || selectedFileName.toLower().contains("lossless") || selectedFileName.toLower().contains("encode") || selectedFileName.toLower().contains("shotcut") || selectedFileName.toLower().contains("premiere"))
            m_player->setVolume(100);
        else
            m_player->setVolume(sourceVideoVolume);
    }
}

void AVideoWidget::onMetaDataChanged()
{

    QStringList metadatalist = m_player->availableMetaData();
//    qDebug()<<"AVideoWidget::onMetaDataChanged VideoFrameRate:"<<m_player->metaData(QMediaMetaData::VideoFrameRate).toString()<<metadatalist.count();

       // Get the size of the list
       int list_size = metadatalist.size();

       //qDebug() << player->isMetaDataAvailable() << list_size;

       // Define variables to store metadata key and value
       QString metadata_key;
       QVariant var_data;

       for (int indx = 0; indx < list_size; indx++)
       {
         // Get the key from the list
         metadata_key = metadatalist.at(indx);

         // Get the value for the key
         var_data = m_player->metaData(metadata_key);

//        qDebug() <<"AVideoWidget::onMetaDataChanged" << metadata_key << var_data.toString();
       }
}

void AVideoWidget::onMetaDataAvailableChanged(bool available)
{
//    qDebug()<<"AVideoWidget::onMetaDataAvailableChanged"<<available<<m_player->metaData(QMediaMetaData::VideoFrameRate).toString();
}

void AVideoWidget::togglePlayPaused()
{
//    qDebug()<<"AVideoWidget::togglePlayPaused"<<m_player->state();

    if (m_player->state() != QMediaPlayer::PlayingState)
    {
        m_player->play();
#ifdef Q_OS_MAC
        //https://stackoverflow.com/questions/38374158/qvideowidget-video-is-cut-off
        QSize s1 = size();
        QSize s2 = s1 + QSize(1, 1);
        resize(s2);// enlarge by one pixel
        resize(s1);// return to original size
#endif
    }
    else if (m_isSeekable)
        m_player->pause();
    else
        m_player->stop();

    onMetaDataChanged();
}

void AVideoWidget::fastForward()
{
    if (m_player->state() != QMediaPlayer::PausedState)
        m_player->pause();
    if (m_player->position() < m_player->duration() - AGlobal().frames_to_msec(1))
        m_player->setPosition(m_player->position() + AGlobal().frames_to_msec(1));
//    qDebug()<<"onNextKeyframeButtonClicked"<<m_player->position();
}

void AVideoWidget::rewind()
{
    if (m_player->state() != QMediaPlayer::PausedState)
        m_player->pause();
    if (m_player->position() > AGlobal().frames_to_msec(1))
        m_player->setPosition(m_player->position() - AGlobal().frames_to_msec(1));
//    qDebug()<<"onPreviousKeyframeButtonClicked"<<m_player->position();
}

void AVideoWidget::skipNext()
{
    int *prevRow = new int();
    int *nextRow = new int();
    int *relativeProgress = new int();
    m_scrubber->progressToRow("V", int(m_player->position()), prevRow, nextRow, relativeProgress);

    if (*nextRow == -1)
        *nextRow = lastHighlightedRow;

    int *relativeProgressl = new int();

    if (*prevRow == *nextRow)
        m_scrubber->rowToPosition("V", *nextRow+1, relativeProgressl);
    else
        m_scrubber->rowToPosition("V", *nextRow, relativeProgressl);

//    qDebug()<<"AVideoWidget::skipNext"<<m_player->position()<<*nextRow<<*relativeProgress<<*relativeProgressl;

    if (*relativeProgressl != -1)
    {
        m_player->setPosition(*relativeProgressl);
    }
}

void AVideoWidget::skipPrevious()
{
    int *prevRow = new int();
    int *nextRow = new int();
    int *relativeProgress = new int();
    m_scrubber->progressToRow("V", m_player->position(), prevRow, nextRow, relativeProgress);

    if (*prevRow == -1)
        *prevRow = lastHighlightedRow;

    int *relativeProgressl = new int();

    if (*prevRow == *nextRow)
        m_scrubber->rowToPosition("V", *prevRow-1, relativeProgressl);
    else
        m_scrubber->rowToPosition("V", *prevRow, relativeProgressl);

//    qDebug()<<"AVideoWidget::skipPrevious"<<m_player->position()<<*prevRow<<*relativeProgress<<*relativeProgressl;

    if (*relativeProgressl != -1)
    {
        m_player->setPosition(*relativeProgressl);
    }
}

void AVideoWidget::onStop()
{
    qDebug()<<"AVideoWidget::onStop"<<m_player->state();

    m_player->stop();
}

void AVideoWidget::onMute()
{
    m_player->setMuted(!m_player->isMuted());
}

void AVideoWidget::setSourceVideoVolume(int volume)
{
    sourceVideoVolume = volume;

    if (!(selectedFileName.toLower().contains(".mp3") || selectedFileName.toLower().contains("lossless") || selectedFileName.toLower().contains("encode") || selectedFileName.toLower().contains("shotcut") || selectedFileName.toLower().contains("premiere")))
        m_player->setVolume(volume);
}

void AVideoWidget::setPlaybackRate(qreal rate)
{
    m_player->setPlaybackRate(rate);
}

void AVideoWidget::onPlaybackRateChanged(qreal rate)
{
    emit playbackRateChanged(rate);
}

void AVideoWidget::onMutedChanged(bool muted)
{
    emit mutedChanged(muted);
}

void AVideoWidget::onScrubberSeeked(int mseconds)
{
    isTimelinePlaymode = false;

//    qDebug()<<"AVideoWidget::onScrubberSeeked"<<mseconds;
//    if (m_player->state() != QMediaPlayer::PausedState)
//        m_player->pause();
    m_player->setPosition(mseconds);
}

void AVideoWidget::onScrubberInChanged(QString AV, int row, int in)
{
    if (!isLoading)
    {
        isTimelinePlaymode = false;
//        qDebug()<<"AVideoWidget::onScrubberInChanged"<<row<<in;
        emit scrubberInChanged(AV, row, in);
    }
}

void AVideoWidget::onScrubberOutChanged(QString AV, int row, int out)
{
    if (!isLoading)
    {
        isTimelinePlaymode = false;
//        qDebug()<<"AVideoWidget::onScrubberOutChanged"<<row<<out;
        emit scrubberOutChanged(AV, row, out);
    }
}


void AVideoWidget::onSetIn(int frames)
{
//    int *row = new int();
//    int *relativeProgress = new int();
//    m_scrubber->progressToRow(m_player->position(), row, relativeProgress);

    if (frames != -1)
        m_player->setPosition(AGlobal().frames_to_msec(frames));
    else
        frames = AGlobal().msec_to_frames( int(m_player->position()));

    QString AV;

    if (selectedFileName.toLower().contains(".mp3"))
            AV = "A";
    else
            AV = "V";

    bool pointSet = false;
    if (lastHighlightedRow != -1)
    {
        ClipInOutStruct inOut = m_scrubber->getInOutPoint(AV, lastHighlightedRow);

        pointSet = m_scrubber->setInOutPoint(inOut.AV, lastHighlightedRow, AGlobal().frames_to_msec(frames),  inOut.out);
    }
//    else
//        pointSet = m_scrubber->setInOutPoint(AV, lastHighlightedRow, AGlobal().frames_to_msec(frames), AGlobal().frames_to_msec(frames) + 3000);

    qDebug()<<"AVideoWidget::onSetIn"<<selectedFileName<<frames<<m_player->position()<<lastHighlightedRow<<pointSet;

    if (!pointSet)
        emit createNewEdit(frames);
}

void AVideoWidget::onSetOut(int frames)
{
//    int *row = new int();
//    int *relativeProgress = new int();
//    m_scrubber->progressToRow(m_player->position(), row, relativeProgress);

    if (frames != -1)
        m_player->setPosition(AGlobal().frames_to_msec(frames));
    else
        frames = AGlobal().msec_to_frames( int(m_player->position()));

    qDebug()<<"AVideoWidget::onSetOut"<<frames<<m_player->position()<<lastHighlightedRow;
    if (lastHighlightedRow != -1)
    {
        ClipInOutStruct inOut = m_scrubber->getInOutPoint("V", lastHighlightedRow);

        m_scrubber->setInOutPoint(inOut.AV, lastHighlightedRow, inOut.in, AGlobal().frames_to_msec(frames));
    }
}

//void AVideoWidget::onSpinnerChanged(int mseconds)
//{
//    if (m_player->state() != QMediaPlayer::PausedState)
//        m_player->pause();
//    isTimelinePlaymode = false;
////    qDebug()<<"seek"<<mseconds;
//    m_player->setPosition(mseconds);
//}
