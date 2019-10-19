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

FVideoWidget::FVideoWidget(QWidget *parent) : QVideoWidget(parent)
  , m_position(0)
  , m_previousIn(-1)
  , m_previousOut(-1)
  , m_duration(0),
    m_isSeekable(false)
{
//    qDebug()<<"FVideoWidget::FVideoWidget"<<parent;
    setMinimumSize(minimumSize().width(), 250);
//    setMinimumHeight(250);

    parentLayout = qobject_cast<QVBoxLayout *>(parent->layout());
//    qDebug()<<"cast"<<parentLayout<<parent<<layout();

    setupActions(this);

    m_player = new QMediaPlayer(this);
//    auto pal = palette();

//    // Sets the brush pixmap to pixmap. The style is set to Qt::TexturePattern.
//    pal.setBrush(QPalette::Window, QBrush(QPixmap (":/fiprelogo.png")));
//    setAutoFillBackground(true);
//    setPalette(pal);

    m_player->setVideoOutput(this);

    connect(m_player, &QMediaPlayer::durationChanged, this, &FVideoWidget::onDurationChanged);
    connect(m_player, &QMediaPlayer::positionChanged, this, &FVideoWidget::onPlayerPositionChanged);
    connect(m_player, &QMediaPlayer::stateChanged, this, &FVideoWidget::onPlayerStateChanged);
    connect(m_player, &QMediaPlayer::mediaStatusChanged, this, &FVideoWidget::onMediaStatusChanged);
    connect(m_player, &QMediaPlayer::mutedChanged, this, &FVideoWidget::onMutedChanged);
    connect(m_player, &QMediaPlayer::playbackRateChanged, this, &FVideoWidget::onPlaybackRateChanged);

    m_scrubber = new SScrubBar(this);
//    m_scrubber->setFocusPolicy(Qt::NoFocus);
    m_scrubber->setObjectName("m_scrubber");
    m_scrubber->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
//    m_scrubber->setMinimumSize(200,200);
//        parentLayout->insertWidget(-1, m_scrubber);
    m_scrubber->setEnabled(true);
    m_scrubber->setScale(20000);
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
    connect(actionStop, &QAction::triggered, this, &FVideoWidget::onStop);
    connect(actionMute, &QAction::triggered, this, &FVideoWidget::onMute);
    connect(m_scrubber, &SScrubBar::seeked, this, &FVideoWidget::onScrubberSeeked);
    connect(m_scrubber, &SScrubBar::scrubberInChanged, this, &FVideoWidget::onScrubberInChanged);
    connect(m_scrubber, &SScrubBar::scrubberOutChanged, this, &FVideoWidget::onScrubberOutChanged);


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

    connect(speedComboBox, &QComboBox::currentTextChanged, this, &FVideoWidget::onSpeedChanged);

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

void FVideoWidget::setupActions(QWidget* widget)
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

void FVideoWidget::onFolderIndexClicked(QModelIndex index)
{
    selectedFolderName = QSettings().value("LastFolder").toString();
//    qDebug()<<"FVideoWidget::onFolderIndexClicked"<<index.data().toString()<<selectedFolderName;
    m_player->stop();
    m_player->setMuted(true);
    m_scrubber->clearInOuts();
}

void FVideoWidget::onFileIndexClicked(QModelIndex index)
{
    selectedFolderName = QSettings().value("LastFileFolder").toString();
    selectedFileName = index.data().toString();

    QString *fpsValue = new QString();
    emit getPropertyValue(selectedFileName, "VideoFrameRate", fpsValue);
    fpsRounded = int( qRound(QString(*fpsValue).toDouble() / 5) * 5);
//    FGlobal().setFramesPerSecond(fpsRounded);

    qDebug()<<"FVideoWidget::onFileIndexClicked"<<index.data().toString()<<selectedFolderName + selectedFileName<<QSettings().value("frameRate").toInt();
    m_player->setMedia(QUrl(selectedFolderName + selectedFileName));
    m_player->play();
    m_player->pause();
    m_player->setNotifyInterval(1000 / QSettings().value("frameRate").toInt());
    m_scrubber->setFramerate(QSettings().value("frameRate").toInt());

    emit fpsChanged(fpsRounded);
}

void FVideoWidget::onEditIndexClicked(QModelIndex index)
{
    QString folderName = index.model()->index(index.row(),folderIndex).data().toString();
    QString fileName = index.model()->index(index.row(),fileIndex).data().toString();
    QString folderFileName = folderName + fileName;
    QTime inTime = QTime::fromString(index.model()->index(index.row(),inIndex).data().toString(),"HH:mm:ss.zzz");
    QTime outTime = QTime::fromString(index.model()->index(index.row(),outIndex).data().toString(),"HH:mm:ss.zzz");

    if (selectedFileName != fileName)
        selectedFileName = fileName;

    qDebug()<<"FVideoWidget::onEditIndexClicked"<<QUrl(folderFileName)<<m_player->media().canonicalUrl();

    if (QUrl(folderFileName) != m_player->media().canonicalUrl())
    {
        QString *fpsValue = new QString();
        emit getPropertyValue(selectedFileName, "VideoFrameRate", fpsValue);
        fpsRounded = int( qRound(QString(*fpsValue).toDouble() / 5) * 5);
//        FGlobal().setFramesPerSecond(fpsRounded);

//        qDebug()<<"FVideoWidget::onEditIndexClicked"<<index.data().toString()<<selectedFolderName + selectedFileName;
        QMediaPlayer::State oldState = m_player->state();
//        bool oldMuted = m_player->isMuted();
        m_player->setMedia(QUrl(folderFileName));

        m_player->play();
        if (oldState == QMediaPlayer::PausedState)
            m_player->pause();

//        m_player->setMuted(oldMuted);
        m_player->setNotifyInterval(1000 / QSettings().value("frameRate").toInt());
        m_scrubber->setFramerate(QSettings().value("frameRate").toInt());
    }

//    qDebug()<<"FVideoWidget::onEditIndexClicked"<<index.column()<<m_player->position();

    if (index.column() == inIndex)
        m_player->setPosition(inTime.msecsSinceStartOfDay());
    else if (index.column() == outIndex)
        m_player->setPosition(outTime.msecsSinceStartOfDay());
    else if (index.column() == durationIndex)
        m_player->setPosition(inTime.msecsSinceStartOfDay() + inTime.msecsTo(outTime) / 2);
//    else
//        m_player->pause();
}

void FVideoWidget::onEditsChangedToVideo(QAbstractItemModel *itemModel)
{
    isLoading = true;
    m_scrubber->clearInOuts();

    FEditSortFilterProxyModel *edititemModel = qobject_cast<FEditSortFilterProxyModel *>(itemModel);
//    qDebug()<<"FVideoWidget::onEditsChangedToVideo"<<itemModel<<edititemModel;

    if (edititemModel == nullptr) //not edit
        m_scrubber->readOnly = true;
    else
        m_scrubber->readOnly = false;

//    qDebug()<<"FVideoWidget::onEditsChangedToVideo"<<itemModel->rowCount();
    for (int row = 0; row < itemModel->rowCount();row++)
    {
        QString fileName = itemModel->index(row,fileIndex).data().toString();
//        qDebug()<<"FVideoWidget::onEditsChangedToVideo"<<fileName<<selectedFileName;
        if (fileName == selectedFileName)
        {
            QTime inTime = QTime::fromString(itemModel->index(row,inIndex).data().toString(),"HH:mm:ss.zzz");
            QTime outTime = QTime::fromString(itemModel->index(row,outIndex).data().toString(),"HH:mm:ss.zzz");
            int editCounter = itemModel->index(row, orderBeforeLoadIndex).data().toInt();

//            qDebug()<<"FVideoWidget::onEditsChangedToVideo"<<inTime<<outTime<<folderFileName<<m_player->media().canonicalUrl();

            m_scrubber->setInOutPoint(editCounter, inTime.msecsSinceStartOfDay(), outTime.msecsSinceStartOfDay());
        }
    }
    isLoading = false;
}

void FVideoWidget::onDurationChanged(int duration)
{
//    m_duration = duration / 1000;
//    ui->lcdDuration->display(QTime::fromMSecsSinceStartOfDay(duration).toString("HH:mm:ss.zzz"));

    if (duration > 0)
    {
        m_scrubber->setScale(FGlobal().msec_rounded_to_fps(duration));
        m_durationLabel->setText(FGlobal().msec_to_time(duration).prepend(" / "));
        qDebug()<<"onDurationChanged: " << duration<<FGlobal().msec_rounded_to_fps(duration)<<FGlobal().msec_to_time(duration);

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

void FVideoWidget::onPlayerPositionChanged(int progress)
{
//    qDebug()<<"FVideoWidget::onPlayerPositionChanged"<<progress<<*row<<*relativeProgress<<FGlobal().msec_to_frames(progress);
   m_scrubber->onSeek(FGlobal().msec_rounded_to_fps(progress));
   m_positionSpinner->blockSignals(true); //do not fire valuechanged signal
   m_positionSpinner->setValue(FGlobal().msec_to_frames(progress));
   m_positionSpinner->blockSignals(false);

//   if (m_player->state() == QMediaPlayer::PlayingState)
   if (!isTimelinePlaymode)
   {
       int *row = new int();
       int *relativeProgress = new int();
       m_scrubber->progressToRow(progress, row, relativeProgress);
       if (*row != -1)
            lastHighlightedRow = *row;
       emit videoPositionChanged(FGlobal().msec_rounded_to_fps(progress), *row, *relativeProgress);
   }
   m_position = progress;
}

void FVideoWidget::onTimelinePositionChanged(int progress, int row, int relativeProgress)
{
    qDebug()<<"FVideoWidget::onTimelinePositionChanged"<<progress<<row<<relativeProgress<<FGlobal().msec_to_frames(progress);
    isTimelinePlaymode = true;
//    int *relativeProgressl = new int();
//    m_scrubber->rowToPosition(row, relativeProgressl);

//    if (*relativeProgressl != -1)
//    {
//        m_scrubber->onSeek(FGlobal().msec_rounded_to_fps(relativeProgress + *relativeProgressl));
//        m_positionSpinner->setValue(FGlobal().msec_to_frames(relativeProgress + *relativeProgressl));
//    }

}

void FVideoWidget::onSpinnerPositionChanged(int frames)
{
    qDebug()<<"FVideoWidget::onSpinnerPositionChanged"<<frames;
    isTimelinePlaymode = false;

//    m_player->pause();
    m_player->setPosition(FGlobal().frames_to_msec(frames));
//    int *relativeProgressl = new int();
//    m_scrubber->rowToPosition(row, relativeProgressl);

//    if (*relativeProgressl != -1)
//    {
//        m_scrubber->onSeek(FGlobal().msec_rounded_to_fps(relativeProgress + *relativeProgressl));
//        m_positionSpinner->setValue(FGlobal().msec_to_frames(relativeProgress + *relativeProgressl));
//    }
}

void FVideoWidget::onFileDelete(QString fileName)
{
    if (fileName == selectedFileName)
    {
        qDebug()<<"FVideoWidget::onFileDelete"<<fileName;
        m_player->stop();
        m_player->setMedia(QMediaContent());
    }
}

void FVideoWidget::onFileRename()
{
    qDebug()<<"FVideoWidget::onFileRename";
//    if (fileName == selectedFileName)
    {
        m_player->stop();
        m_player->setMedia(QMediaContent());
    }
}

void FVideoWidget::onPlayerStateChanged(QMediaPlayer::State state)
{
//    qDebug()<<"FVideoWidget::onPlayerStateChanged"<<state;
    if (state == QMediaPlayer::PlayingState)
        actionPlay->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
    if (state == QMediaPlayer::PausedState)
        actionPlay->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    if (state == QMediaPlayer::StoppedState)
        actionPlay->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
}

void FVideoWidget::onMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    qDebug()<<"FVideoWidget::onMediaStatusChanged"<<status;

    if (status == QMediaPlayer::BufferedMedia)
    {
//        m_player->setPosition(0);
    }
}

void FVideoWidget::togglePlayPaused()
{
    qDebug()<<"FVideoWidget::togglePlayPaused"<<m_player->state();

    if (m_player->state() != QMediaPlayer::PlayingState)
        m_player->play();
    else if (m_isSeekable)
        m_player->pause();
    else
        m_player->stop();
}

void FVideoWidget::fastForward()
{
    if (m_player->state() != QMediaPlayer::PausedState)
        m_player->pause();
    if (m_player->position() < m_player->duration() - 1000 / QSettings().value("frameRate").toInt())
        m_player->setPosition(m_player->position() + 1000 / QSettings().value("frameRate").toInt());
    qDebug()<<"onNextKeyframeButtonClicked"<<m_player->position();

}

void FVideoWidget::rewind()
{
    if (m_player->state() != QMediaPlayer::PausedState)
        m_player->pause();
    if (m_player->position() > 1000 / QSettings().value("frameRate").toInt())
        m_player->setPosition(m_player->position() - 1000 / QSettings().value("frameRate").toInt());
    qDebug()<<"onPreviousKeyframeButtonClicked"<<m_player->position();
}

void FVideoWidget::skipNext()
{
    int *row = new int();
    int *relativeProgress = new int();
    m_scrubber->progressToRow(m_player->position(), row, relativeProgress);

    int *relativeProgressl = new int();
    m_scrubber->rowToPosition(*row+1, relativeProgressl);

    qDebug()<<"FVideoWidget::skipNext"<<m_player->position()<<*row<<*relativeProgress<<*relativeProgressl;

    if (*relativeProgressl != -1)
    {
//        m_scrubber->onSeek(FGlobal().msec_rounded_to_fps(*relativeProgressl));
//        m_positionSpinner->setValue(FGlobal().msec_to_frames(*relativeProgressl));
        m_player->setPosition(*relativeProgressl);
    }

//    emit videoPositionChanged(FGlobal().msec_rounded_to_fps(progress), *row+1, *relativeProgressl);
}

void FVideoWidget::skipPrevious()
{
    int *row = new int();
    int *relativeProgress = new int();
    m_scrubber->progressToRow(m_player->position(), row, relativeProgress);

    int *relativeProgressl = new int();
    m_scrubber->rowToPosition(*row-1, relativeProgressl);

    qDebug()<<"FVideoWidget::skipPrevious"<<m_player->position()<<row<<*relativeProgress<<*relativeProgressl;

    if (*relativeProgressl != -1)
    {
//        m_scrubber->onSeek(FGlobal().msec_rounded_to_fps(*relativeProgressl));
//        m_positionSpinner->setValue(FGlobal().msec_to_frames(*relativeProgressl));
        m_player->setPosition(*relativeProgressl);
    }
}

void FVideoWidget::onStop()
{
    qDebug()<<"FVideoWidget::onStop"<<m_player->state();

    m_player->stop();
}

void FVideoWidget::onMute()
{
    m_player->setMuted(!m_player->isMuted());
}

void FVideoWidget::onSpeedChanged(QString speed)
{
    double playbackRate = speed.left(speed.lastIndexOf("x")).toDouble();
    if (speed.indexOf(" fps")  > 0)
        playbackRate = speed.left(speed.lastIndexOf(" fps")).toDouble() / QSettings().value("frameRate").toInt();
//    qDebug()<<"FVideoWidget::onSpeedChanged"<<speed<<playbackRate<<speed.lastIndexOf("x")<<speed.left(speed.lastIndexOf("x"));
    m_player->setPlaybackRate(playbackRate);
}

void FVideoWidget::onPlaybackRateChanged(qreal rate)
{
    speedComboBox->setCurrentText(QString::number(rate) + "x");
}

void FVideoWidget::onMutedChanged(bool muted)
{
//    qDebug()<<"FVideoWidget::onMutedChanged"<<muted;
    actionMute->setIcon(style()->standardIcon(muted
            ? QStyle::SP_MediaVolumeMuted
            : QStyle::SP_MediaVolume));
}

void FVideoWidget::onScrubberSeeked(int mseconds)
{
    isTimelinePlaymode = false;

//    qDebug()<<"FVideoWidget::onScrubberSeeked"<<mseconds;
//    if (m_player->state() != QMediaPlayer::PausedState)
//        m_player->pause();
    m_player->setPosition(mseconds);
}

void FVideoWidget::onScrubberInChanged(int row, int in)
{
    if (!isLoading)
    {
        isTimelinePlaymode = false;
//        qDebug()<<"FVideoWidget::onScrubberInChanged"<<row<<in;
        emit scrubberInChanged(row, in);
    }
}

void FVideoWidget::onScrubberOutChanged(int row, int out)
{
    if (!isLoading)
    {
        isTimelinePlaymode = false;
//        qDebug()<<"FVideoWidget::onScrubberOutChanged"<<row<<out;
        emit scrubberOutChanged(row, out);
    }
}


void FVideoWidget::onUpdateIn(int frames)
{
//    int *row = new int();
//    int *relativeProgress = new int();
//    m_scrubber->progressToRow(m_player->position(), row, relativeProgress);

    if (frames != -1)
        m_player->setPosition(FGlobal().frames_to_msec(frames));
    else
        frames = FGlobal().msec_to_frames( m_player->position());

    qDebug()<<"FVideoWidget::onUpdateIn"<<frames<<m_player->position()<<lastHighlightedRow;
    if (lastHighlightedRow != -1)
    {
        EditInOutStruct inOut = m_scrubber->getInOutPoint(lastHighlightedRow);

        m_scrubber->setInOutPoint(lastHighlightedRow, FGlobal().frames_to_msec(frames),  inOut.out);
//        onScrubberInChanged(lastHighlightedRow, m_player->position());
    }
}

void FVideoWidget::onUpdateOut(int frames)
{
//    int *row = new int();
//    int *relativeProgress = new int();
//    m_scrubber->progressToRow(m_player->position(), row, relativeProgress);

    if (frames != -1)
        m_player->setPosition(FGlobal().frames_to_msec(frames));
    else
        frames = FGlobal().msec_to_frames( m_player->position());

    qDebug()<<"FVideoWidget::onUpdateOut"<<frames<<m_player->position()<<lastHighlightedRow;
    if (lastHighlightedRow != -1)
    {
        EditInOutStruct inOut = m_scrubber->getInOutPoint(lastHighlightedRow);

        m_scrubber->setInOutPoint(lastHighlightedRow, inOut.in, FGlobal().frames_to_msec(frames));
//        onScrubberOutChanged(lastHighlightedRow, m_player->position());
    }
}

//void FVideoWidget::onSpinnerChanged(int mseconds)
//{
//    if (m_player->state() != QMediaPlayer::PausedState)
//        m_player->pause();
//    isTimelinePlaymode = false;
////    qDebug()<<"seek"<<mseconds;
//    m_player->setPosition(mseconds);
//}
