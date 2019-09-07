#include "fglobal.h"
#include "fvideowidget.h"

#include <QAbstractVideoSurface>
#include <QDebug>
#include <QSettings>
#include <QTime>
#include <QTimer>
#include <QToolBar>

#include <QStyle>

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

    m_player->setVideoOutput(this);

    m_player->setNotifyInterval(40);

    connect(m_player, &QMediaPlayer::durationChanged, this, &FVideoWidget::onDurationChanged);
    connect(m_player, &QMediaPlayer::positionChanged, this, &FVideoWidget::onPositionChanged);
    connect(m_player, &QMediaPlayer::stateChanged, this, &FVideoWidget::onPlayerStateChanged);


    m_scrubber = new SScrubBar(this);
//    m_scrubber->setFocusPolicy(Qt::NoFocus);
    m_scrubber->setObjectName("m_scrubber");
    m_scrubber->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
//    m_scrubber->setMinimumSize(200,200);
//        parentLayout->insertWidget(-1, m_scrubber);
    m_scrubber->setEnabled(true);
    m_scrubber->setFramerate(25);
    m_scrubber->setScale(20000);
    m_scrubber->readOnly = false;
    m_scrubber->onSeek(0);

    connect(actionPlay, SIGNAL(triggered()), this, SLOT(togglePlayPaused()));
//    connect(actionPause, SIGNAL(triggered()), this, SLOT(pause()));
//    connect(actionFastForward, SIGNAL(triggered()), this, SLOT(fastForward()));
//    connect(actionRewind, SIGNAL(triggered()), this, SLOT(rewind()));
    connect(m_scrubber, &SScrubBar::seeked, this, &FVideoWidget::onScrubberSeeked);
    connect(m_scrubber, &SScrubBar::inChanged, this, &FVideoWidget::onScrubberInChanged);
    connect(m_scrubber, &SScrubBar::outChanged, this, &FVideoWidget::onScrubberOutChanged);


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


//    connect(m_positionSpinner, SIGNAL(valueChanged(int)), this, SLOT(onSpinnerChanged(int)));


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
//    retranslateUi(widget);
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
    selectedFileName = index.data().toString();

    qDebug()<<"FVideoWidget::onFileIndexClicked"<<index.data()<<selectedFolderName + selectedFileName;
    m_player->setMedia(QUrl(selectedFolderName + selectedFileName));
    m_player->play();
    m_player->setMuted(true);
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
        m_player->setMedia(QUrl(folderFileName));
        m_player->play();
        m_player->pause();
        m_player->setMuted(true);
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
    m_scrubber->clearInOuts();

    qDebug()<<"FVideoWidget::onEditsChanged"<<editProxyModel->rowCount()<<m_player->media().canonicalUrl();
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

//            m_scrubber->setInPoint(-1, inTime.msecsSinceStartOfDay());
//            m_scrubber->setOutPoint(-1, outTime.msecsSinceStartOfDay());
            m_scrubber->setInOutPoint(editCounter, inTime.msecsSinceStartOfDay(), outTime.msecsSinceStartOfDay());
        }
    }
}

void FVideoWidget::onDurationChanged(int duration)
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

void FVideoWidget::onPositionChanged(int progress)
{
//    qDebug()<<"FVideoWidget::onPositionChanged"<<progress;
   m_scrubber->onSeek(FGlobal().msec_rounded_to_fps(25, progress));
   m_positionSpinner->setValue(FGlobal().msec_to_frames(25, progress));


//   if (m_player->state() == QMediaPlayer::PlayingState)
        emit positionChanged(FGlobal().msec_rounded_to_fps(25, progress));
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

void FVideoWidget::onScrubberSeeked(int mseconds)
{
//    qDebug()<<"FVideoWidget::onScrubberSeeked"<<mseconds;
    if (m_player->state() != QMediaPlayer::PausedState)
        m_player->pause();
    m_player->setPosition(mseconds);
}

void FVideoWidget::onScrubberInChanged(int row, int in)
{
//    qDebug()<<"FVideoWidget::onScrubberInChanged"<<old<<in;
    emit inChanged(row, in);
}

void FVideoWidget::onScrubberOutChanged(int row, int out)
{
//    qDebug()<<"FVideoWidget::onScrubberOutChanged"<<old<<out;
    emit outChanged(row, out);
}


void FVideoWidget::onSpinnerChanged(int mseconds)
{
//    qDebug()<<"seek"<<mseconds;
    m_player->setPosition(mseconds);
}
