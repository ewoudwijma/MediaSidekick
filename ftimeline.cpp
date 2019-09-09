#include "fglobal.h"
#include "ftimeline.h"

#include <QTimer>
#include <QStyle>
#include <QTime>
#include <QDebug>
#include <QSettings>

static const int orderBeforeLoadIndex = 0;
static const int orderAtLoadIndex = 1;
static const int orderAfterMovingIndex = 2;
static const int folderIndex = 3;
static const int fileIndex = 4;
static const int inIndex = 5;
static const int outIndex = 6;
static const int durationIndex = 7;
static const int ratingIndex = 8;
static const int repeatIndex = 9;
static const int hintIndex = 10;
static const int tagIndex = 11;

FTimeline::FTimeline(QWidget *parent) : QWidget(parent)
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
    m_scrubber->setFramerate(25);
    m_scrubber->setScale(10000);
    m_scrubber->onSeek(0);
//    m_scrubber->readOnly = false;

    connect(m_scrubber, &SScrubBar::seeked, this, &FTimeline::onScrubberSeeked);


    toolbar = new QToolBar(tr("Transport Controls"), this);
    int s = style()->pixelMetric(QStyle::PM_SmallIconSize);
    toolbar->setIconSize(QSize(s, s));
    toolbar->setContentsMargins(0, 0, 0, 0);
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
    toolbar->addWidget(spacer);
    toolbar->addAction(actionSkipPrevious);
    toolbar->addAction(actionRewind);
    toolbar->addAction(actionPlay);
    toolbar->addAction(actionFastForward);
    toolbar->addAction(actionSkipNext);

    spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    toolbar->addWidget(spacer);


    QTimer::singleShot(0, this, [this]()->void
    {
         parentLayout->insertWidget(-1, m_scrubber);
         parentLayout->insertWidget(-1, toolbar);
    });

}

void FTimeline::setupActions(QWidget* widget)
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

void FTimeline::onFolderIndexClicked(FEditSortFilterProxyModel *editProxyModel)
{
//    QString fileFolderName = QSettings().value("LastFolder").toString() + index.data().toString(); //+ "//"
    qDebug()<<"FTimeline::onFolderIndexClicked"<<editProxyModel->rowCount();

    onEditsChanged(editProxyModel);

//    emit fileIndexClicked(index);
}

void FTimeline::onDurationChanged(int duration)
{
    qDebug()<<"onDurationChanged: " << duration;
//    m_duration = duration / 1000;
//    ui->lcdDuration->display(QTime::fromMSecsSinceStartOfDay(duration).toString("HH:mm:ss.zzz"));

    m_scrubber->setScale(FGlobal().msec_rounded_to_fps(25, duration));
    m_durationLabel->setText(FGlobal().msec_to_time(25, duration).prepend(" / "));

    m_isSeekable = true;

    actionPlay->setEnabled(true);
    actionSkipPrevious->setEnabled(m_isSeekable);
    actionSkipNext->setEnabled(m_isSeekable);
    actionRewind->setEnabled(m_isSeekable);
    actionFastForward->setEnabled(m_isSeekable);


}

void FTimeline::onEditsChanged(FEditSortFilterProxyModel *editProxyModel)
{
    int duration = 0;
    int previousDuration = 0;

    m_scrubber->clearInOuts();

    QMap<int,int> reorderMap;
    for (int row = 0; row < editProxyModel->rowCount();row++)
    {
        reorderMap[editProxyModel->index(row, orderAfterMovingIndex).data().toInt()] = row;
    }

//    qDebug()<<"FTimeline::onEditsChanged"<<editProxyModel->rowCount();

    QMapIterator<int, int> orderIterator(reorderMap);
    while (orderIterator.hasNext()) //all files
    {
        orderIterator.next();
        int row = orderIterator.value();
        QTime inTime = QTime::fromString(editProxyModel->index(row,inIndex).data().toString(),"HH:mm:ss.zzz");
        QTime outTime = QTime::fromString(editProxyModel->index(row,outIndex).data().toString(),"HH:mm:ss.zzz");

        int orderAfterMovingIndexx = editProxyModel->index(row, orderAfterMovingIndex).data().toInt();
        int orderAtLoadIndexx = editProxyModel->index(row, orderAtLoadIndex).data().toInt();
        int editCounter = editProxyModel->index(row, orderBeforeLoadIndex).data().toInt();

        duration+= inTime.msecsTo(outTime);

//        qDebug()<<"FTimeline::onEditsChanged"<<row<<orderAtLoadIndexx<<orderAfterMovingIndexx<<inTime<<outTime<<duration;

//        m_scrubber->setInPoint(-1, previousDuration);
//        m_scrubber->setOutPoint(-1, duration - 40);
        m_scrubber->setInOutPoint(editCounter, previousDuration, duration - 40);
        previousDuration = duration;
    }

    m_scrubber->setScale(FGlobal().msec_rounded_to_fps(25, duration));
    m_durationLabel->setText(FGlobal().msec_to_time(25, duration).prepend(" / "));
}

void FTimeline::onFileIndexClicked(QModelIndex index)
{
    qDebug()<<"FTimeline::onFileIndexClicked"<<index.data();
}

void FTimeline::onVideoPositionChanged(int progress, int row, int relativeProgress)
{
    int *relativeProgressl = new int();
    m_scrubber->rowToPosition(row, relativeProgressl);

    if (*relativeProgressl != -1)
    {
        m_scrubber->onSeek(FGlobal().msec_rounded_to_fps(25, relativeProgress + *relativeProgressl));
        m_positionSpinner->setValue(FGlobal().msec_to_frames(25, relativeProgress + *relativeProgressl));
    }

}

void FTimeline::onScrubberSeeked(int mseconds)
{
    int *row = new int();
    int *relativeProgress = new int();
    m_scrubber->progressToRow(mseconds, row, relativeProgress);

    qDebug()<<"FTimeline::onScrubberSeeked"<<mseconds<< *row<< *relativeProgress;
//    if (m_player->state() != QMediaPlayer::PausedState)
//        m_player->pause();
//    m_player->setPosition(mseconds);
    emit timelinePositionChanged(mseconds, *row, *relativeProgress);
}
