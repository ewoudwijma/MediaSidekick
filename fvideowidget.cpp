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
    setMinimumSize(minimumSize().width(), 550);
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
    connect(m_scrubber, &SScrubBar::inChanged, this, &FVideoWidget::onScrubberInChanged);
    connect(m_scrubber, &SScrubBar::outChanged, this, &FVideoWidget::onScrubberOutChanged);


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

    connect(m_positionSpinner, SIGNAL(valueChanged(int)), this, SLOT(onSpinnerPositionChanged(int)));

    isTimelinePlaymode = false;
    isLoading = false;

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
    qDebug()<<"FVideoWidget::onFolderIndexClicked"<<index.data().toString()<<selectedFolderName;
    m_player->stop();
    m_scrubber->clearInOuts();
//    onFileIndexClicked(QModelIndex());
}

void FVideoWidget::onFileIndexClicked(QModelIndex index)
{
    selectedFolderName = QSettings().value("LastFileFolder").toString();
    selectedFileName = index.data().toString();

    QString *fpsValue = new QString();
    emit getPropertyValue(selectedFileName, "VideoFrameRate", fpsValue);
    fpsRounded = int( qRound(QString(*fpsValue).toDouble() / 5) * 5);
//    FGlobal().setFramesPerSecond(fpsRounded);

    qDebug()<<"FVideoWidget::onFileIndexClicked"<<index.data().toString()<<selectedFolderName + selectedFileName<<frameRate;
    m_player->setMedia(QUrl(selectedFolderName + selectedFileName));
    m_player->play();
    m_player->setMuted(true);
    m_player->setNotifyInterval(1000 / frameRate);
    m_scrubber->setFramerate(frameRate);

    emit fpsChanged(fpsRounded);

}

void FVideoWidget::onEditIndexClicked(QModelIndex index)
{
    QString folderName = index.model()->index(index.row(),folderIndex).data().toString();
    QString fileName = index.model()->index(index.row(),fileIndex).data().toString();
    if (selectedFileName != fileName)
        selectedFileName = fileName;
    QString folderFileName = folderName + fileName;
    qDebug()<<"FVideoWidget::onEditIndexClicked"<<QUrl(folderFileName)<<m_player->media().canonicalUrl();
    if (QUrl(folderFileName) != m_player->media().canonicalUrl())
    {
        QString *fpsValue = new QString();
        emit getPropertyValue(selectedFileName, "VideoFrameRate", fpsValue);
        fpsRounded = int( qRound(QString(*fpsValue).toDouble() / 5) * 5);
//        FGlobal().setFramesPerSecond(fpsRounded);

        qDebug()<<"FVideoWidget::onEditIndexClicked"<<index.data()<<selectedFolderName + selectedFileName<<frameRate;
        m_player->setMedia(QUrl(folderFileName));
        m_player->play();
        m_player->pause();
        m_player->setMuted(true);
        m_player->setNotifyInterval(1000 / frameRate);
        m_scrubber->setFramerate(frameRate);
    }
    if (index.column() == inIndex)
    {
        QTime inTime = QTime::fromString(index.model()->index(index.row(),inIndex).data().toString(),"HH:mm:ss.zzz");
        m_player->setPosition(inTime.msecsSinceStartOfDay());
        m_player->play();
    }
    else if (index.column() == outIndex)
    {
        QTime outTime = QTime::fromString(index.model()->index(index.row(),outIndex).data().toString(),"HH:mm:ss.zzz");
        m_player->setPosition(outTime.msecsSinceStartOfDay());
        m_player->play();
    }
    else {
        m_player->pause();
    }
}

void FVideoWidget::onEditsChanged(FEditSortFilterProxyModel *editProxyModel)
{
    isLoading = true;
    m_scrubber->clearInOuts();

    qDebug()<<"FVideoWidget::onEditsChanged"<<editProxyModel->rowCount();
    for (int row = 0; row < editProxyModel->rowCount();row++)
    {
        QString fileName = editProxyModel->index(row,fileIndex).data().toString();
//        qDebug()<<"FVideoWidget::onEditsChanged"<<fileName<<selectedFileName;
        if (fileName == selectedFileName)
        {
            QTime inTime = QTime::fromString(editProxyModel->index(row,inIndex).data().toString(),"HH:mm:ss.zzz");
            QTime outTime = QTime::fromString(editProxyModel->index(row,outIndex).data().toString(),"HH:mm:ss.zzz");
            int editCounter = editProxyModel->index(row, orderBeforeLoadIndex).data().toInt();

//            qDebug()<<"FVideoWidget::onEditsChanged"<<inTime<<outTime<<folderFileName<<m_player->media().canonicalUrl();

            m_scrubber->setInOutPoint(editCounter, inTime.msecsSinceStartOfDay(), outTime.msecsSinceStartOfDay());
        }
    }
    isLoading = false;
}

void FVideoWidget::onDurationChanged(int duration)
{
//    m_duration = duration / 1000;
//    ui->lcdDuration->display(QTime::fromMSecsSinceStartOfDay(duration).toString("HH:mm:ss.zzz"));

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

void FVideoWidget::onPlayerPositionChanged(int progress)
{
//    qDebug()<<"FVideoWidget::onPlayerPositionChanged"<<progress<<*row<<*relativeProgress<<FGlobal().msec_to_frames(progress);
   m_scrubber->onSeek(FGlobal().msec_rounded_to_fps(progress));
//   m_positionSpinner->blockSignals(true);
   m_positionSpinner->setValue(FGlobal().msec_to_frames(progress));
//   m_positionSpinner->blockSignals(false);

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

void FVideoWidget::onSpinnerPositionChanged(int progress)
{
//    qDebug()<<"FVideoWidget::onSpinnerPositionChanged"<<progress;
    isTimelinePlaymode = false;
//    m_player->setPosition(progress);
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
        m_player->stop();
        m_player->setMedia(QMediaContent());
    }
}

void FVideoWidget::onPlayerStateChanged(QMediaPlayer::State state)
{
//    qDebug()<<"stateChanged"<<state;
    if (state == QMediaPlayer::PlayingState)
        actionPlay->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
    if (state == QMediaPlayer::PausedState)
        actionPlay->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    if (state == QMediaPlayer::StoppedState)
        actionPlay->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
}

void FVideoWidget::togglePlayPaused()
{
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
    if (m_player->position() < m_player->duration() - 1000 / frameRate)
        m_player->setPosition(m_player->position() + 1000 / frameRate);
    qDebug()<<"onNextKeyframeButtonClicked"<<m_player->position();

}

void FVideoWidget::rewind()
{
    if (m_player->state() != QMediaPlayer::PausedState)
        m_player->pause();
    if (m_player->position() > 1000 / frameRate)
        m_player->setPosition(m_player->position() - 1000 / frameRate);
    qDebug()<<"onPreviousKeyframeButtonClicked"<<m_player->position();
}

void FVideoWidget::skipNext()
{
    int *row = new int();
    int *relativeProgress = new int();
    m_scrubber->progressToRow(m_player->position(), row, relativeProgress);

    int *relativeProgressl = new int();
    m_scrubber->rowToPosition(*row+1, relativeProgressl);

    qDebug()<<"FVideoWidget::skipNext"<<m_player->position()<<row<<*relativeProgress<<*relativeProgressl;

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
        playbackRate = speed.left(speed.lastIndexOf(" fps")).toDouble() / frameRate;
//    qDebug()<<"FVideoWidget::onSpeedChanged"<<speed<<playbackRate<<speed.lastIndexOf("x")<<speed.left(speed.lastIndexOf("x"));
    m_player->setPlaybackRate(playbackRate);
}

void FVideoWidget::onPlaybackRateChanged(qreal rate)
{
    speedComboBox->setCurrentText(QString::number(rate) + "x");
}

void FVideoWidget::onMutedChanged(bool muted)
{
    qDebug()<<"mutedChanged"<<muted;
    actionMute->setIcon(style()->standardIcon(muted
            ? QStyle::SP_MediaVolumeMuted
            : QStyle::SP_MediaVolume));
}

void FVideoWidget::onScrubberSeeked(int mseconds)
{
    isTimelinePlaymode = false;

//    qDebug()<<"FVideoWidget::onScrubberSeeked"<<mseconds;
    if (m_player->state() != QMediaPlayer::PausedState)
        m_player->pause();
    m_player->setPosition(mseconds);
}

void FVideoWidget::onScrubberInChanged(int row, int in)
{
    if (!isLoading)
    {
        isTimelinePlaymode = false;
//        qDebug()<<"FVideoWidget::onScrubberInChanged"<<row<<in;
        emit inChanged(row, in);
    }
}

void FVideoWidget::onScrubberOutChanged(int row, int out)
{
    if (!isLoading)
    {
        isTimelinePlaymode = false;
//        qDebug()<<"FVideoWidget::onScrubberOutChanged"<<row<<out;
        emit outChanged(row, out);
    }
}


void FVideoWidget::onUpdateIn()
{
//    int *row = new int();
//    int *relativeProgress = new int();
//    m_scrubber->progressToRow(m_player->position(), row, relativeProgress);
    qDebug()<<"FVideoWidget::onUpdateIn"<<m_player->position()<<lastHighlightedRow;
    if (lastHighlightedRow != -1)
    {
        EditInOutStruct inOut = m_scrubber->getInOutPoint(lastHighlightedRow);

        m_scrubber->setInOutPoint(lastHighlightedRow, m_player->position(),  inOut.out);
//        onScrubberInChanged(lastHighlightedRow, m_player->position());
    }
}

void FVideoWidget::onUpdateOut()
{
//    int *row = new int();
//    int *relativeProgress = new int();
//    m_scrubber->progressToRow(m_player->position(), row, relativeProgress);
    qDebug()<<"FVideoWidget::onUpdateOut"<<m_player->position()<<lastHighlightedRow;
    if (lastHighlightedRow != -1)
    {
        EditInOutStruct inOut = m_scrubber->getInOutPoint(lastHighlightedRow);

        m_scrubber->setInOutPoint(lastHighlightedRow, inOut.in, m_player->position());
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
