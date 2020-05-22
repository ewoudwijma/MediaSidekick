#include "aglobal.h"
#include "atimeline.h"

#include <QTimer>
#include <QStyle>
#include <QTime>
#include <QDebug>
#include <QSettings>

ATimeline::ATimeline(QWidget *parent) : QWidget(parent)
{
    parentLayout = qobject_cast<QVBoxLayout *>(parent->layout());

    setupActions(this);

    m_scrubber = new SScrubBar(this);
//    m_scrubber->setFocusPolicy(Qt::NoFocus);
    m_scrubber->setObjectName("m_scrubber");
    m_scrubber->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
//    m_scrubber->setMinimumSize(200,200);
//        parentLayout->insertWidget(-1, m_scrubber);
    m_scrubber->setEnabled(true);
    m_scrubber->setFramerate(QSettings().value("frameRate").toInt());
    m_scrubber->setScale(10000);
    m_scrubber->onSeek(0);
//    m_scrubber->readOnly = false;

    connect(m_scrubber, &SScrubBar::seeked, this, &ATimeline::onScrubberSeeked);

    toolbar = new QToolBar(tr("Transport Controls"), this);
    int s = style()->pixelMetric(QStyle::PM_SmallIconSize);
    toolbar->setIconSize(QSize(s, s));
    toolbar->setContentsMargins(0, 0, 0, 0);
    QWidget *spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    m_positionSpinner = new STimeSpinBox(this);
    m_positionSpinner->setToolTip(tr("Current position"));
    m_positionSpinner->setFixedWidth(120);

//    m_positionSpinner->setEnabled(false);
    m_positionSpinner->setKeyboardTracking(false);
    m_durationLabel = new QLabel(this);
    m_durationLabel->setToolTip(tr("Total Duration"));
    m_durationLabel->setText(" / 00:00:00:00");
//    m_durationLabel->setFixedWidth(m_positionSpinner->width());
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
//    toolbar->addAction(actionSkipPrevious);
//    toolbar->addAction(actionRewind);
//    toolbar->addAction(actionPlay);
//    toolbar->addAction(actionFastForward);
//    toolbar->addAction(actionSkipNext);

    spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    toolbar->addWidget(spacer);

    transitiontime = 0;
    transitiontimeLastGood = -1;

    QTimer::singleShot(0, this, [this]()->void
    {
         parentLayout->insertWidget(-1, m_scrubber);
         parentLayout->insertWidget(-1, toolbar);
    });

}

void ATimeline::setupActions(QWidget* widget)
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
//    retranslateUi(widget);
    QMetaObject::connectSlotsByName(widget);
}

void ATimeline::onDurationChanged(int duration)
{
    qDebug()<<"ATimeline::onDurationChanged: " << duration;
//    m_duration = duration / 1000;
//    ui->lcdDuration->display(QTime::fromMSecsSinceStartOfDay(duration).toString("HH:mm:ss.zzz"));

//    m_duration = AGlobal().msec_to_frames(duration);
    m_scrubber->setScale(AGlobal().msec_rounded_to_fps(duration));
    m_durationLabel->setText(AGlobal().msec_to_time(duration).prepend(" / "));

    m_isSeekable = true;

    actionPlay->setEnabled(true);
    actionSkipPrevious->setEnabled(m_isSeekable);
    actionSkipNext->setEnabled(m_isSeekable);
    actionRewind->setEnabled(m_isSeekable);
    actionFastForward->setEnabled(m_isSeekable);
}

void ATimeline::onClipsChangedToTimeline(QAbstractItemModel *itemModel)
{
//    qDebug()<<"ATimeline::onClipsChangedToTimeline"<<itemModel->rowCount();
    int videoOriginalDuration = 0;
    int audioOriginalDuration = 0;
    int videoCountNrOfClips = 0;
    int audioCountNrOfClips = 0;

    QMap<int,int> reorderMap;
    for (int row = 0; row < itemModel->rowCount();row++)
    {
        QTime inTime = QTime::fromString(itemModel->index(row,inIndex).data().toString(),"HH:mm:ss.zzz");
        QTime outTime = QTime::fromString(itemModel->index(row,outIndex).data().toString(),"HH:mm:ss.zzz");
        QString fileName = itemModel->index(row,fileIndex).data().toString();

        int frameDuration = AGlobal().msec_to_frames(outTime.msecsSinceStartOfDay()) - AGlobal().msec_to_frames(inTime.msecsSinceStartOfDay()) + 1;

        QString fileNameLow = fileName.toLower();
        int lastIndexOf = fileNameLow.lastIndexOf(".");
        QString extension = fileNameLow.mid(lastIndexOf + 1);

        if (AGlobal().videoExtensions.contains(extension, Qt::CaseInsensitive))
        {
            videoOriginalDuration += frameDuration;
            videoCountNrOfClips++;
        }
        else
        {
            audioOriginalDuration += frameDuration;
            audioCountNrOfClips++;
        }

        reorderMap[itemModel->index(row, orderAfterMovingIndex).data().toInt()] = row;
    }

    int maxOriginalDuration = qMax(videoOriginalDuration, audioOriginalDuration);
//    int maxCombinedDuration = qMax(videoOriginalDuration - transitiontime * (videoCountNrOfClips-1), audioOriginalDuration - transitiontime * (audioCountNrOfClips-1));

    if (audioCountNrOfClips == 0)
        maxAudioDuration = 0;
    else
        maxAudioDuration = audioOriginalDuration - transitiontime * (audioCountNrOfClips-1);

    if (videoCountNrOfClips == 0)
        maxVideoDuration = 0;
    else
        maxVideoDuration = videoOriginalDuration - transitiontime * (videoCountNrOfClips-1);

    maxCombinedDuration = qMax(maxVideoDuration, maxAudioDuration);

    bool allowed = true;

    m_scrubber->clearInOuts();

    QStringList mediaTypeList;
    mediaTypeList << "A";
//    if (includeAudio)
        mediaTypeList << "V";
    foreach (QString mediaType, mediaTypeList)
    {
        int previousPreviousRow =  -1;
        int previousRow = -1;
        int previousPreviousOut = 0;
        int previousOut = 0;

        QMapIterator<int, int> orderIterator(reorderMap);
        while (orderIterator.hasNext()) //all files
        {
            orderIterator.next();
            int row = orderIterator.value();

            QString fileName = itemModel->index(row,fileIndex).data().toString();

            QString fileNameLow = fileName.toLower();
            int lastIndexOf = fileNameLow.lastIndexOf(".");
            QString extension = fileNameLow.mid(lastIndexOf + 1);

            if ((AGlobal().audioExtensions.contains(extension, Qt::CaseInsensitive) && mediaType == "A") || (AGlobal().videoExtensions.contains(extension, Qt::CaseInsensitive) && mediaType == "V"))
            {
                QTime inTime = QTime::fromString(itemModel->index(row,inIndex).data().toString(),"HH:mm:ss.zzz");
                QTime outTime = QTime::fromString(itemModel->index(row,outIndex).data().toString(),"HH:mm:ss.zzz");
    //                QTime fileDurationTime = QTime::fromString(itemModel->index(row,fileDurationIndex).data().toString(),"HH:mm:ss.zzz");

                int clipduration = AGlobal().msec_to_frames(outTime.msecsSinceStartOfDay()) - AGlobal().msec_to_frames(inTime.msecsSinceStartOfDay()) + 1;

                int inpoint, outpoint;

                if (previousRow == -1) //first
                {
                    inpoint = 0;
                    outpoint = clipduration;
                }
                else
                {
                    inpoint = previousOut - transitiontime;
                    outpoint = inpoint + clipduration;
                }

                int porderBeforeLoadIndex = itemModel->index(row, orderBeforeLoadIndex).data().toInt();
                QString AV;

                QString fileNameLow = fileName.toLower();
                int lastIndexOf = fileNameLow.lastIndexOf(".");
                QString extension = fileNameLow.mid(lastIndexOf + 1);

                if (AGlobal().audioExtensions.contains(extension, Qt::CaseInsensitive))
                    AV = "A";
                else
                    AV = "V";

                m_scrubber->setInOutPoint(AV, porderBeforeLoadIndex, AGlobal().frames_to_msec( inpoint), AGlobal().frames_to_msec(outpoint));

                if (previousPreviousOut != 0 && previousPreviousOut >= inpoint)
                {
                    allowed = false;
    //                    qDebug()<<"ATimeline::onClipsChangedToTimeline transitiontime. out/in overlap"<<row<<itemModel->index(previousPreviousRow, tagIndex + 3).data().toInt()<<inpoint;
                }

                previousPreviousOut = previousOut;
                previousOut = outpoint;
                previousPreviousRow = previousRow;
                previousRow = row;
            }
        }
    }

    if (!allowed)
    {
        if (transitiontimeLastGood != -1 )
        {
            emit adjustTransitionTime(transitiontimeLastGood);
            return;
        }
    }
    else
    {
        transitiontimeLastGood = transitiontime;
    }

    m_scrubber->setScale(AGlobal().frames_to_msec(maxCombinedDuration));

    if (maxOriginalDuration == maxCombinedDuration)
        m_durationLabel->setText(AGlobal().frames_to_time(maxOriginalDuration).prepend(" / "));
    else
        m_durationLabel->setText(AGlobal().frames_to_time(maxOriginalDuration).prepend(" / ") + AGlobal().frames_to_time(maxCombinedDuration).prepend(" -> "));

    emit clipsChangedToTimeline(itemModel);
}

void ATimeline::onVideoPositionChanged(int , int row, int relativeProgress)//progress
{
//    qDebug()<<"ATimeline::onVideoPositionChanged"<<progress<<row<<relativeProgress;
    int *relativeProgressl = new int();
    m_scrubber->rowToPosition("V", row, relativeProgressl);
//    qDebug()<<"  ATimeline::onVideoPositionChanged"<<progress<<row<<*relativeProgressl;

    if (relativeProgress != -1)
    {
        m_scrubber->onSeek(AGlobal().msec_rounded_to_fps(relativeProgress + *relativeProgressl));
        m_positionSpinner->blockSignals(true); //do not fire valuechanged signal
        m_positionSpinner->setValue(AGlobal().msec_to_frames(relativeProgress + *relativeProgressl));
        m_positionSpinner->blockSignals(false);
    }
}

void ATimeline::onTimelineWidgetsChanged(int p_transitiontime, QString transitionType, AClipsTableView *clipsTableView)
{
//    if (transitionType != "No transition")
        transitiontime = p_transitiontime;
//    else
//        transitiontime = 0;

//    qDebug()<<"ATimeline::onTimelineWidgetsChanged"<<p_transitiontime<<transitionType<<clipsTableView->model()->rowCount();

    onClipsChangedToTimeline(clipsTableView->clipsProxyModel);

//    qDebug()<<"ATimeline::onTimelineWidgetsChanged done"<<p_transitiontime<<transitionType;
    emit clipsChangedToVideo(clipsTableView->model());
}

void ATimeline::onScrubberSeeked(int mseconds)
{
    int *prevRow = new int();
    int *nextRow = new int();
    int *relativeProgress = new int();
    m_scrubber->progressToRow("V", mseconds, prevRow, nextRow, relativeProgress);

    qDebug()<<"ATimeline::onScrubberSeeked"<<mseconds<< *prevRow<< *relativeProgress;
//    if (m_player->state() != QMediaPlayer::PausedState)
//        m_player->pause();
//    m_player->setPosition(mseconds);
    emit timelinePositionChanged(mseconds, *prevRow, *relativeProgress);
}
