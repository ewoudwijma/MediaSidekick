#include "fglobal.h"
#include "fvideowidget.h"

#include <QAbstractVideoSurface>
#include <QDebug>
#include <QFileInfo>
#include <QFileSystemModel>
#include <QSettings>
#include <QTime>
#include <QTimer>
#include <QToolBar>

#include <QStyle>

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
//    qDebug()<<"AVideoWidget::AVideoWidget"<<parent;
    setMinimumSize(minimumSize().width(), 300);
//    setMinimumHeight(250);

    parentLayout = qobject_cast<QVBoxLayout *>(parent->layout());
//    qDebug()<<"cast"<<parentLayout<<parent<<layout();

    setupActions(this);

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

    m_scrubber = new SScrubBar(this);
//    m_scrubber->setFocusPolicy(Qt::NoFocus);
    m_scrubber->setObjectName("m_scrubber");
    m_scrubber->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
//    m_scrubber->setMinimumSize(200,200);
//        parentLayout->insertWidget(-1, m_scrubber);
    m_scrubber->setEnabled(true);
    m_scrubber->setScale(10000);
    m_scrubber->readOnly = false;
    m_scrubber->onSeek(0);

    connect(actionPlay, SIGNAL(triggered()), this, SLOT(togglePlayPaused()));
//    connect(actionPause, SIGNAL(triggered()), this, SLOT(pause()));
    connect(actionFastForward, SIGNAL(triggered()), this, SLOT(fastForward()));
    connect(actionRewind, SIGNAL(triggered()), this, SLOT(rewind()));
    connect(actionSkipNext, SIGNAL(triggered()), this, SLOT(skipNext()));
    connect(actionSkipPrevious, SIGNAL(triggered()), this, SLOT(skipPrevious()));
    connect(actionUpdateIn, SIGNAL(triggered()), this, SLOT(onUpdateIn()));
    connect(actionUpdateOut, SIGNAL(triggered()), this, SLOT(onUpdateOut()));
    connect(actionStop, &QAction::triggered, this, &AVideoWidget::onStop);
    connect(actionMute, &QAction::triggered, this, &AVideoWidget::onMute);
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
    actionPlay->setToolTip(tr("<p><b>Play or pause</b></p>"
                              "<p><i>Play or pause the video</i></p>"
                              "<ul>"
                              "<li>Shortcut: Space</li>"
                              "</ul>"));
    actionStop->setToolTip(tr("<p><b>Stop the video</b></p>"
                                "<p><i>Stop the video</i></p>"
                                ));

    actionSkipNext->setToolTip(tr("<p><b>Go to next or previous clip</b></p>"
                              "<p><i>Go to next or previous clip</i></p>"
                              "<ul>"
                              "<li>Shortcut: ctrl-up and ctrl-down</li>"
                              "</ul>"));
    actionSkipPrevious->setToolTip(actionSkipNext->toolTip());

    actionRewind->setToolTip(tr("<p><b>Go to next or previous frame</b></p>"
                              "<p><i>Go to next or previous frame</i></p>"
                              "<ul>"
                              "<li>Shortcut: ctrl-left and ctrl-right</li>"
                              "</ul>"));
    actionFastForward->setToolTip(actionRewind->toolTip());

    actionUpdateIn->setToolTip(tr("<p><b>Update in- and out- point</b></p>"
                                  "<p><i>Change the in- or outpoint of the current clip to the current position on the timeline</i></p>"
                                  "<ul>"
                                  "<li>Shortcut: ctrl-i and ctrl-o</li>"
                                  "</ul>"));
    actionUpdateOut->setToolTip(actionUpdateIn->toolTip());

    actionMute->setToolTip(tr("<p><b>Mute or unmute</b></p>"
                              "<p><i>Mute or unmute sound</i></p>"
                              "<ul>"
                              "<li>Shortcut: ctrl-i and ctrl-o</li>"
                              "</ul>"));


    toolbar = new QToolBar(tr("Transport Controls"), this);
    int s = style()->pixelMetric(QStyle::PM_SmallIconSize);
    toolbar->setIconSize(QSize(s, s));
    toolbar->setContentsMargins(0, 0, 0, 0);
//    toolbar->setToolButtonStyle(Qt::ToolButtonStyle);


    QWidget *spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    m_positionSpinner = new STimeSpinBox(this);
    m_positionSpinner->setToolTip(tr("Current position"));
//    m_positionSpinner->setEnabled(false);
    m_positionSpinner->setKeyboardTracking(false);
    m_positionSpinner->setFixedWidth(120);


    m_durationLabel = new QLabel(this);
    m_durationLabel->setToolTip(tr("Total Duration"));
    m_durationLabel->setText(" / 00:00:00:00");
    m_durationLabel->setFixedWidth(m_positionSpinner->width());
//    m_inPointLabel = new QLabel(this);
//    m_inPointLabel->setText("--:--:--:--");
//    m_inPointLabel->setToolTip(tr("In Point"));
//    m_inPointLabel->setFixedWidth(m_inPointLabel->width());
//    m_selectedLabel = new QLabel(this);
//    m_selectedLabel->setText("--:--:--:--");
//    m_selectedLabel->setToolTip(tr("Selected Duration"));
//    m_selectedLabel->setFixedWidth(m_selectedLabel->width());
    toolbar->addWidget(m_positionSpinner);
    toolbar->addWidget(m_durationLabel);
//    toolbar->addWidget(spacer);
    toolbar->addAction(actionSkipPrevious);
    toolbar->addAction(actionRewind);
    toolbar->addAction(actionPlay);
    toolbar->addAction(actionFastForward);
    toolbar->addAction(actionSkipNext);
    toolbar->addAction(actionStop);
    toolbar->addAction(actionMute);
    toolbar->addAction(actionUpdateIn);
    toolbar->addAction(actionUpdateOut);

//    toolbar->setStyleSheet(
//                "QToolButton#actionPlay { background:red }"
//                "QToolButton#actionMute { background:blue }");

//    if (QSettings().value("theme").toString() == "Black")
        toolbar->setStyleSheet("QToolButton:!hover {background-color:lightgrey }");//QToolBar {background: rgb(30, 30, 30)}

    speedComboBox = new QComboBox(this);
    speedComboBox->addItem("-2x");
    speedComboBox->addItem("-1x");
    speedComboBox->addItem("1 fps");
    speedComboBox->addItem("2 fps");
    speedComboBox->addItem("4 fps");
    speedComboBox->addItem("8 fps");
    speedComboBox->addItem("16 fps");
    speedComboBox->addItem("1x");
    speedComboBox->addItem("2x");
    speedComboBox->addItem("4x");
    speedComboBox->addItem("8x");
    speedComboBox->addItem("16x");

    speedComboBox->setCurrentText("1x");
    toolbar->addWidget(speedComboBox);

    connect(speedComboBox, &QComboBox::currentTextChanged, this, &AVideoWidget::onSpeedChanged);

    speedComboBox->setToolTip(tr("<p><b>Speed</b></p>"
                                 "<p><i>Change the play speed of the video</i></p>"
                                 "<ul>"
                                 "<li>Supported speeds are depending on the codec</li>"
                                 "</ul>"));

    spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
//    toolbar->addWidget(spacer);

    connect(m_positionSpinner, SIGNAL(valueChanged(int)), this, SLOT(onSpinnerPositionChanged(int))); //using other syntax not working...

    isTimelinePlaymode = false;
    isLoading = false;

    lastHighlightedRow = -1;

    QTimer::singleShot(0, this, [this]()->void
    {
         parentLayout->insertWidget(-1, m_scrubber);
         parentLayout->insertWidget(-1, toolbar);
    });
}

void AVideoWidget::setupActions(QWidget* widget)
{
    actionPlay = new QAction(widget);
    actionPlay->setObjectName(QString::fromUtf8("actionPlay"));
    QPixmap pix = style()->standardIcon(QStyle::SP_MediaPlay).pixmap(QSize(32,32));
    actionPlay->setIcon(pix);
    actionPlay->setDisabled(true);
    actionPause = new QAction(widget);
    actionPause->setObjectName(QString::fromUtf8("actionPause"));
    actionPause->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
    actionPause->setDisabled(true);
    actionSkipNext = new QAction(widget);
    actionSkipNext->setObjectName(QString::fromUtf8("actionSkipNext"));
    actionSkipNext->setIcon(style()->standardIcon(QStyle::SP_MediaSkipForward));
    actionSkipNext->setDisabled(true);
    actionSkipPrevious = new QAction(widget);
    actionSkipPrevious->setObjectName(QString::fromUtf8("actionSkipPrevious"));
    actionSkipPrevious->setIcon(style()->standardIcon(QStyle::SP_MediaSkipBackward));
    actionSkipPrevious->setDisabled(true);
    actionRewind = new QAction(widget);
    actionRewind->setObjectName(QString::fromUtf8("actionRewind"));
    actionRewind->setIcon(style()->standardIcon(QStyle::SP_MediaSeekBackward));
    actionRewind->setDisabled(true);
    actionFastForward = new QAction(widget);
    actionFastForward->setObjectName(QString::fromUtf8("actionFastForward"));
    actionFastForward->setIcon(style()->standardIcon(QStyle::SP_MediaSeekForward));
    actionFastForward->setDisabled(true);
    actionVolume = new QAction(widget);
    actionVolume->setObjectName(QString::fromUtf8("actionVolume"));
    actionVolume->setIcon(style()->standardIcon(QStyle::SP_MediaVolume));

    actionStop = new QAction(widget);
    actionStop->setObjectName(QString::fromUtf8("actionStop"));
    actionStop->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
    actionStop->setDisabled(true);

    actionMute = new QAction(widget);
    actionMute->setObjectName(QString::fromUtf8("actionMute"));
    actionMute->setIcon(style()->standardIcon(QStyle::SP_MediaVolume));
    actionMute->setDisabled(true);

    actionUpdateIn = new QAction(widget);
    actionUpdateIn->setObjectName(QString::fromUtf8("actionUpdateIn"));
    actionUpdateIn->setIcon(style()->standardIcon(QStyle::SP_ArrowLeft));
    actionUpdateIn->setDisabled(true);
//    retranslateUi(widget);
    actionUpdateOut = new QAction(widget);
    actionUpdateOut->setObjectName(QString::fromUtf8("actionUpdateOut"));
    actionUpdateOut->setIcon(style()->standardIcon(QStyle::SP_ArrowRight));
    actionUpdateOut->setDisabled(true);
    QMetaObject::connectSlotsByName(widget);
}

void AVideoWidget::onFolderIndexClicked(QModelIndex index)
{
    selectedFolderName = QSettings().value("LastFolder").toString();
//    qDebug()<<"AVideoWidget::onFolderIndexClicked"<<index.data().toString()<<selectedFolderName;
    m_player->stop();
    m_player->setMuted(true);
    m_scrubber->clearInOuts();
}

void AVideoWidget::onFileIndexClicked(QModelIndex index)
{
    selectedFolderName = QSettings().value("LastFileFolder").toString();
    selectedFileName = index.model()->index(index.row(), 0, index.parent()).data().toString();

//    qDebug()<<"AVideoWidget::onFileIndexClicked"<<index.column()<<index.data().toString()<<selectedFolderName + selectedFileName<<QSettings().value("frameRate").toInt();
    m_player->setMedia(QUrl(selectedFolderName + selectedFileName));
    m_player->play();
    m_player->pause();
    m_player->setNotifyInterval(AGlobal().frames_to_msec(1));
    m_scrubber->setFramerate(QSettings().value("frameRate").toInt());
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

    qDebug()<<"AVideoWidget::onClipIndexClicked"<<QUrl(folderFileName)<<m_player->media().canonicalUrl();

    if (QUrl(folderFileName) != m_player->media().canonicalUrl())
    {
//        QString *fpsPointer = new QString();
//        emit getPropertyValue(selectedFileName, "VideoFrameRate", fpsPointer);
//        fpsRounded = int( qRound(index.model()->index(index.row(),fpsIndex).data().toDouble() / 5) * 5);

//        qDebug()<<"AVideoWidget::onClipIndexClicked"<<index.data().toString()<<selectedFolderName + selectedFileName;
        QMediaPlayer::State oldState = m_player->state();
//        bool oldMuted = m_player->isMuted();
        m_player->setMedia(QUrl(folderFileName));

        m_player->play();
        if (oldState == QMediaPlayer::PausedState)
            m_player->pause();

//        m_player->setMuted(oldMuted);
        m_player->setNotifyInterval(AGlobal().frames_to_msec(1));
        m_scrubber->setFramerate(QSettings().value("frameRate").toInt());
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

    AClipsSortFilterProxyModel *clipsitemModel = qobject_cast<AClipsSortFilterProxyModel *>(itemModel);
//    qDebug()<<"AVideoWidget::onClipsChangedToVideo"<<itemModel<<clipsitemModel;

    if (clipsitemModel == nullptr) //not clip
        m_scrubber->readOnly = true;
    else
        m_scrubber->readOnly = false;

//    qDebug()<<"AVideoWidget::onClipsChangedToVideo"<<itemModel->rowCount();
    for (int row = 0; row < itemModel->rowCount();row++)
    {
        QString fileName = itemModel->index(row,fileIndex).data().toString();
//        qDebug()<<"AVideoWidget::onClipsChangedToVideo"<<fileName<<selectedFileName;
        if (fileName == selectedFileName)
        {
            QTime inTime = QTime::fromString(itemModel->index(row,inIndex).data().toString(),"HH:mm:ss.zzz");
            QTime outTime = QTime::fromString(itemModel->index(row,outIndex).data().toString(),"HH:mm:ss.zzz");
            int clipCounter = itemModel->index(row, orderBeforeLoadIndex).data().toInt();

//            qDebug()<<"AVideoWidget::onClipsChangedToVideo"<<inTime<<outTime<<folderFileName<<m_player->media().canonicalUrl();

            m_scrubber->setInOutPoint(clipCounter, inTime.msecsSinceStartOfDay(), outTime.msecsSinceStartOfDay());
        }
    }
    isLoading = false;
}

void AVideoWidget::onDurationChanged(int duration)
{
    if (duration > 0)
    {
        m_scrubber->setScale(AGlobal().msec_rounded_to_fps(duration));
        m_durationLabel->setText(AGlobal().msec_to_time(duration).prepend(" / "));
//        qDebug()<<"AVideoWidget::onDurationChanged" << duration<<AGlobal().msec_rounded_to_fps(duration)<<AGlobal().msec_to_time(duration);

        m_isSeekable = true;

        actionPlay->setEnabled(true);
        actionSkipPrevious->setEnabled(m_isSeekable);
        actionSkipNext->setEnabled(m_isSeekable);
        actionRewind->setEnabled(m_isSeekable);
        actionFastForward->setEnabled(m_isSeekable);
        actionUpdateIn->setEnabled(m_isSeekable);
        actionUpdateOut->setEnabled(m_isSeekable);
        actionMute->setEnabled(true);
        actionStop->setEnabled(true);
    }
}

void AVideoWidget::onPlayerPositionChanged(int progress)
{
   m_scrubber->onSeek(AGlobal().msec_rounded_to_fps(progress));
   m_positionSpinner->blockSignals(true); //do not fire valuechanged signal
   m_positionSpinner->setValue(AGlobal().msec_to_frames(progress));
   m_positionSpinner->blockSignals(false);

   if (!isTimelinePlaymode)
   {
       int *prevRow = new int();
       int *nextRow = new int();
       int *relativeProgress = new int();
       m_scrubber->progressToRow(progress, prevRow, nextRow, relativeProgress);
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
//    int *relativeProgressl = new int();
//    m_scrubber->rowToPosition(row, relativeProgressl);

//    if (*relativeProgressl != -1)
//    {
//        m_scrubber->onSeek(AGlobal().msec_rounded_to_fps(relativeProgress + *relativeProgressl));
//        m_positionSpinner->setValue(AGlobal().msec_to_frames(relativeProgress + *relativeProgressl));
//    }

}

void AVideoWidget::onSpinnerPositionChanged(int frames)
{
    qDebug()<<"AVideoWidget::onSpinnerPositionChanged"<<frames;
    isTimelinePlaymode = false;

//    m_player->pause();
    m_player->setPosition(AGlobal().frames_to_msec(frames));
//    int *relativeProgressl = new int();
//    m_scrubber->rowToPosition(row, relativeProgressl);

//    if (*relativeProgressl != -1)
//    {
//        m_scrubber->onSeek(AGlobal().msec_rounded_to_fps(relativeProgress + *relativeProgressl));
//        m_positionSpinner->setValue(AGlobal().msec_to_frames(relativeProgress + *relativeProgressl));
//    }
}

void AVideoWidget::onReleaseMedia(QString fileName)
{
    if (fileName == selectedFileName)
    {
        qDebug()<<"AVideoWidget::onReleaseMedia"<<fileName;
        m_player->stop();
        m_player->setMedia(QMediaContent());
    }
}

void AVideoWidget::onFileRename()
{
    qDebug()<<"AVideoWidget::onFileRename";
//    if (fileName == selectedFileName)
    {
        m_player->stop();
        m_player->setMedia(QMediaContent());
    }
}

void AVideoWidget::onPlayerStateChanged(QMediaPlayer::State state)
{
//    qDebug()<<"AVideoWidget::onPlayerStateChanged"<<state;
    if (state == QMediaPlayer::PlayingState)
        actionPlay->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
    if (state == QMediaPlayer::PausedState)
        actionPlay->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    if (state == QMediaPlayer::StoppedState)
        actionPlay->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
}

void AVideoWidget::onMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
//    qDebug()<<"AVideoWidget::onMediaStatusChanged"<<status;

//    if (status == QMediaPlayer::BufferedMedia)
//    {
//        m_player->setPosition(0);
//    }
}

void AVideoWidget::togglePlayPaused()
{
//    qDebug()<<"AVideoWidget::togglePlayPaused"<<m_player->state();

    if (m_player->state() != QMediaPlayer::PlayingState)
        m_player->play();
    else if (m_isSeekable)
        m_player->pause();
    else
        m_player->stop();
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
    m_scrubber->progressToRow(int(m_player->position()), prevRow, nextRow, relativeProgress);

    if (*nextRow == -1)
        *nextRow = lastHighlightedRow;

    int *relativeProgressl = new int();

    if (*prevRow == *nextRow)
        m_scrubber->rowToPosition(*nextRow+1, relativeProgressl);
    else
        m_scrubber->rowToPosition(*nextRow, relativeProgressl);

//    qDebug()<<"AVideoWidget::skipNext"<<m_player->position()<<*nextRow<<*relativeProgress<<*relativeProgressl;

    if (*relativeProgressl != -1)
    {
//        m_scrubber->onSeek(AGlobal().msec_rounded_to_fps(*relativeProgressl));
//        m_positionSpinner->setValue(AGlobal().msec_to_frames(*relativeProgressl));
        m_player->setPosition(*relativeProgressl);
    }

//    emit videoPositionChanged(AGlobal().msec_rounded_to_fps(progress), *row+1, *relativeProgressl);
}

void AVideoWidget::skipPrevious()
{
    int *prevRow = new int();
    int *nextRow = new int();
    int *relativeProgress = new int();
    m_scrubber->progressToRow(m_player->position(), prevRow, nextRow, relativeProgress);

    if (*prevRow == -1)
        *prevRow = lastHighlightedRow;

    int *relativeProgressl = new int();

    if (*prevRow == *nextRow)
        m_scrubber->rowToPosition(*prevRow-1, relativeProgressl);
    else
        m_scrubber->rowToPosition(*prevRow, relativeProgressl);

//    qDebug()<<"AVideoWidget::skipPrevious"<<m_player->position()<<*prevRow<<*relativeProgress<<*relativeProgressl;

    if (*relativeProgressl != -1)
    {
//        m_scrubber->onSeek(AGlobal().msec_rounded_to_fps(*relativeProgressl));
//        m_positionSpinner->setValue(AGlobal().msec_to_frames(*relativeProgressl));
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

void AVideoWidget::onSpeedChanged(QString speed)
{
    double playbackRate = speed.left(speed.lastIndexOf("x")).toDouble();
    if (speed.indexOf(" fps")  > 0)
        playbackRate = speed.left(speed.lastIndexOf(" fps")).toDouble() / QSettings().value("frameRate").toInt();
//    qDebug()<<"AVideoWidget::onSpeedChanged"<<speed<<playbackRate<<speed.lastIndexOf("x")<<speed.left(speed.lastIndexOf("x"));
    m_player->setPlaybackRate(playbackRate);
}

void AVideoWidget::onPlaybackRateChanged(qreal rate)
{
    speedComboBox->setCurrentText(QString::number(rate) + "x");
}

void AVideoWidget::onMutedChanged(bool muted)
{
//    qDebug()<<"AVideoWidget::onMutedChanged"<<muted;
    actionMute->setIcon(style()->standardIcon(muted
            ? QStyle::SP_MediaVolumeMuted
            : QStyle::SP_MediaVolume));
}

void AVideoWidget::onScrubberSeeked(int mseconds)
{
    isTimelinePlaymode = false;

//    qDebug()<<"AVideoWidget::onScrubberSeeked"<<mseconds;
//    if (m_player->state() != QMediaPlayer::PausedState)
//        m_player->pause();
    m_player->setPosition(mseconds);
}

void AVideoWidget::onScrubberInChanged(int row, int in)
{
    if (!isLoading)
    {
        isTimelinePlaymode = false;
//        qDebug()<<"AVideoWidget::onScrubberInChanged"<<row<<in;
        emit scrubberInChanged(row, in);
    }
}

void AVideoWidget::onScrubberOutChanged(int row, int out)
{
    if (!isLoading)
    {
        isTimelinePlaymode = false;
//        qDebug()<<"AVideoWidget::onScrubberOutChanged"<<row<<out;
        emit scrubberOutChanged(row, out);
    }
}


void AVideoWidget::onUpdateIn(int frames)
{
//    int *row = new int();
//    int *relativeProgress = new int();
//    m_scrubber->progressToRow(m_player->position(), row, relativeProgress);

    if (frames != -1)
        m_player->setPosition(AGlobal().frames_to_msec(frames));
    else
        frames = AGlobal().msec_to_frames( int(m_player->position()));

    qDebug()<<"AVideoWidget::onUpdateIn"<<frames<<m_player->position()<<lastHighlightedRow;
    if (lastHighlightedRow != -1)
    {
        ClipInOutStruct inOut = m_scrubber->getInOutPoint(lastHighlightedRow);

        m_scrubber->setInOutPoint(lastHighlightedRow, AGlobal().frames_to_msec(frames),  inOut.out);
//        onScrubberInChanged(lastHighlightedRow, m_player->position());
    }
}

void AVideoWidget::onUpdateOut(int frames)
{
//    int *row = new int();
//    int *relativeProgress = new int();
//    m_scrubber->progressToRow(m_player->position(), row, relativeProgress);

    if (frames != -1)
        m_player->setPosition(AGlobal().frames_to_msec(frames));
    else
        frames = AGlobal().msec_to_frames( int(m_player->position()));

    qDebug()<<"AVideoWidget::onUpdateOut"<<frames<<m_player->position()<<lastHighlightedRow;
    if (lastHighlightedRow != -1)
    {
        ClipInOutStruct inOut = m_scrubber->getInOutPoint(lastHighlightedRow);

        m_scrubber->setInOutPoint(lastHighlightedRow, inOut.in, AGlobal().frames_to_msec(frames));
//        onScrubberOutChanged(lastHighlightedRow, m_player->position());
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
