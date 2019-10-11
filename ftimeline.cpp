#include "fglobal.h"
#include "ftimeline.h"

#include <QTimer>
#include <QStyle>
#include <QTime>
#include <QDebug>
#include <QSettings>

FTimeline::FTimeline(QWidget *parent) : QWidget(parent)
{

    parentLayout = qobject_cast<QVBoxLayout *>(parent->layout());

    setupActions(this);

    timelineModel = new QStandardItemModel(this);
    QStringList labels;
    labels << "OrderBL" << "OrderAL"<<"OrderAM"<<"Changed"<< "Path" << "File"<<"Fps"<<"In"<<"Out"<<"Duration"<<"Rating"<<"Alike"<<"Hint"<<"Tags";
    timelineModel->setHorizontalHeaderLabels(labels);

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

    transitionTime = frameRate;
    stretchTime = 0;

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

void FTimeline::onFolderIndexClicked(QAbstractItemModel *itemModel)
{
//    QString fileFolderName = QSettings().value("LastFolder").toString() + index.data().toString(); //+ "//"
    qDebug()<<"FTimeline::onFolderIndexClicked"<<itemModel->rowCount();

    oneditsChangedToTimeline(itemModel);

//    emit fileIndexClicked(index);
}

void FTimeline::onDurationChanged(int duration)
{
    qDebug()<<"FTimeline::onDurationChanged: " << duration;
//    m_duration = duration / 1000;
//    ui->lcdDuration->display(QTime::fromMSecsSinceStartOfDay(duration).toString("HH:mm:ss.zzz"));

    m_scrubber->setScale(FGlobal().msec_rounded_to_fps(duration));
    m_durationLabel->setText(FGlobal().msec_to_time(duration).prepend(" / "));

    m_isSeekable = true;

    actionPlay->setEnabled(true);
    actionSkipPrevious->setEnabled(m_isSeekable);
    actionSkipNext->setEnabled(m_isSeekable);
    actionRewind->setEnabled(m_isSeekable);
    actionFastForward->setEnabled(m_isSeekable);
}

void FTimeline::stretchDuration(int oldDuration)
{
    double transitionTimeMSecs = transitionTime * 1000.0 / frameRate;

//    double multiplier = 1000 * (stretchTime *1000.0/frameRate ) / (oldDuration);

    double axiDelta = 0;
    double duration = 0;
    for (int row = 0; row < timelineModel->rowCount();row++)
    {
        QString fileName = timelineModel->index(row,fileIndex).data().toString();
        QTime inTime = QTime::fromString(timelineModel->index(row,inIndex).data().toString(),"HH:mm:ss.zzz");
        QTime outTime = QTime::fromString(timelineModel->index(row,outIndex).data().toString(),"HH:mm:ss.zzz");

        double originalEditDuration = inTime.msecsTo(outTime) + 1000.0 / frameRate;
        double addedEditDuration;// = originalEditDuration * (multiplier - 1.0); //can also be negative!

        if (timelineModel->rowCount() == 1)
            addedEditDuration = (originalEditDuration) * (stretchTime *1000.0/frameRate ) / (oldDuration) - originalEditDuration;
        else if (row == 0) //first
        {
            addedEditDuration = (originalEditDuration - transitionTimeMSecs / 2) * (stretchTime *1000.0/frameRate ) / (oldDuration) + transitionTimeMSecs / 2 - originalEditDuration;
        }
        else if (row == timelineModel->rowCount() - 1) //last
        {
            addedEditDuration = (originalEditDuration - transitionTimeMSecs / 2) * (stretchTime *1000.0/frameRate ) / (oldDuration) + transitionTimeMSecs / 2 - originalEditDuration;
        }
        else
        {
            addedEditDuration = (originalEditDuration - transitionTimeMSecs) * (stretchTime *1000.0/frameRate ) / (oldDuration) + transitionTimeMSecs - originalEditDuration;
        }

        addedEditDuration += axiDelta;

        double addedEditDurationRoundedToFPS = FGlobal().msec_rounded_to_fps(addedEditDuration);

        qDebug()<<""<<oldDuration<<stretchTime *1000.0/frameRate<<(stretchTime *1000.0/frameRate ) / (oldDuration)<<axiDelta<<addedEditDurationRoundedToFPS;

        axiDelta = addedEditDuration - addedEditDurationRoundedToFPS;

        if (row == timelineModel->rowCount() - 1) //last
        {
            addedEditDuration += axiDelta;

            addedEditDurationRoundedToFPS = FGlobal().msec_rounded_to_fps(addedEditDuration);
            axiDelta = addedEditDuration - addedEditDurationRoundedToFPS;
        }

        QString *durationString = new QString();
        emit getPropertyValue(fileName, "Duration", durationString); //format <30s: [ss.mm s] >30s: [h.mm:ss]

        QTime durationTime = QTime::fromString(*durationString,"h:mm:ss");
        if (durationTime.msecsSinceStartOfDay() == 0)
            durationTime = QTime::fromMSecsSinceStartOfDay(999000);

        inTime = inTime.addMSecs(-qMin(int(addedEditDurationRoundedToFPS/2), inTime.msecsSinceStartOfDay()));
        outTime = outTime.addMSecs(qMin(int(addedEditDurationRoundedToFPS/2),durationTime.msecsSinceStartOfDay() - outTime.msecsSinceStartOfDay()));

        timelineModel->setData(timelineModel->index(row, inIndex), inTime.toString("HH:mm:ss.zzz"));
        timelineModel->setData(timelineModel->index(row, outIndex), outTime.toString("HH:mm:ss.zzz"));
        timelineModel->setData(timelineModel->index(row, durationIndex), QTime::fromMSecsSinceStartOfDay(inTime.msecsTo(outTime) + 1000.0/frameRate).toString("HH:mm:ss.zzz"));

        duration+=inTime.msecsTo(outTime) + 1000.0 / frameRate;// + addedEditDurationRoundedToFPS;

        qDebug()<<"FTimeline::stretchDuration"<<row<<inTime<<outTime<<originalEditDuration<<addedEditDuration<<addedEditDurationRoundedToFPS<<duration <<axiDelta<<fileName<<durationTime<<*durationString;
    }
}

void FTimeline::oneditsChangedToTimeline(QAbstractItemModel *itemModel)
{
    double originalDuration = 0; //double because of duration of 1 frame
    double transitionTimeMSecs = transitionTime * 1000.0 / frameRate;

    for (int row = 0; row < itemModel->rowCount();row++)
    {
        QTime inTime = QTime::fromString(itemModel->index(row,inIndex).data().toString(),"HH:mm:ss.zzz");
        QTime outTime = QTime::fromString(itemModel->index(row,outIndex).data().toString(),"HH:mm:ss.zzz");

        if (itemModel->rowCount() == 1)
            originalDuration+= inTime.msecsTo(outTime) + 1000.0 / frameRate;
        else if (row == 0) //first
            originalDuration+= inTime.msecsTo(outTime) + 1000.0 / frameRate - transitionTimeMSecs/2 ;
        else if (row == itemModel->rowCount() - 1) //last
            originalDuration+= inTime.msecsTo(outTime) + 1000.0 / frameRate - transitionTimeMSecs/2 ;
        else
            originalDuration+= inTime.msecsTo(outTime) + 1000.0 / frameRate - transitionTimeMSecs ;
    }

    double multiplier = 1;
    if (stretchTime != 0 && originalDuration > 0)
    {
//        multiplier = double(stretchTime *1000/frameRate - (itemModel->rowCount()-1) * transitionTimeMSecs) / double(originalDuration - (itemModel->rowCount()-1) * transitionTimeMSecs);
        multiplier = (stretchTime *1000/frameRate ) / (originalDuration);
    }

//    qDebug()<<"FTimeline::oneditsChangedToTimeline"<<stretchTime *1000/frameRate<<originalDuration<<multiplier;

    m_scrubber->clearInOuts();
    timelineModel->removeRows(0, timelineModel->rowCount());

    double axiDelta = 0;
    double duration = 0;
    for (int row = 0; row < itemModel->rowCount();row++)
    {
        QString fileName = itemModel->index(row,fileIndex).data().toString();
        QTime inTime = QTime::fromString(itemModel->index(row,inIndex).data().toString(),"HH:mm:ss.zzz");
        QTime outTime = QTime::fromString(itemModel->index(row,outIndex).data().toString(),"HH:mm:ss.zzz");

        double originalEditDuration = inTime.msecsTo(outTime) + 1000.0 / frameRate;
        double addedEditDuration;// = originalEditDuration * (multiplier - 1.0); //can also be negative!

        if (itemModel->rowCount() == 1)
            addedEditDuration = (originalEditDuration) * multiplier - originalEditDuration;
        else if (row == 0) //first
        {
            addedEditDuration = (originalEditDuration - transitionTimeMSecs / 2) * multiplier + transitionTimeMSecs / 2 - originalEditDuration;
        }
        else if (row == itemModel->rowCount() - 1) //last
        {
            addedEditDuration = (originalEditDuration - transitionTimeMSecs / 2) * multiplier + transitionTimeMSecs / 2 - originalEditDuration;
        }
        else
        {
            addedEditDuration = (originalEditDuration - transitionTimeMSecs) * multiplier + transitionTimeMSecs - originalEditDuration;
        }

        addedEditDuration += axiDelta;

        double addedEditDurationRoundedToFPS = FGlobal().msec_rounded_to_fps(addedEditDuration);
        axiDelta = addedEditDuration - addedEditDurationRoundedToFPS;

        if (row == itemModel->rowCount() - 1) //last
        {
            addedEditDuration += axiDelta;

            addedEditDurationRoundedToFPS = FGlobal().msec_rounded_to_fps(addedEditDuration);
            axiDelta = addedEditDuration - addedEditDurationRoundedToFPS;
        }

        QString *durationString = new QString();
        emit getPropertyValue(fileName, "Duration", durationString); //format <30s: [ss.mm s] >30s: [h.mm:ss]

        QTime durationTime = QTime::fromString(*durationString,"h:mm:ss");
        if (durationTime.msecsSinceStartOfDay() == 0)
            durationTime = QTime::fromMSecsSinceStartOfDay(999000);

        inTime = inTime.addMSecs(-qMin(int(addedEditDurationRoundedToFPS/2), inTime.msecsSinceStartOfDay()));
        outTime = outTime.addMSecs(qMin(int(addedEditDurationRoundedToFPS/2),durationTime.msecsSinceStartOfDay() - outTime.msecsSinceStartOfDay()));

        QList<QStandardItem *> items;
        for (int column=0;column<itemModel->columnCount();column++)
        {
            if (column == inIndex)
                items.append(new QStandardItem(inTime.toString("HH:mm:ss.zzz")));
            else if (column == outIndex)
                items.append(new QStandardItem(outTime.toString("HH:mm:ss.zzz")));
            else if (column == durationIndex)
                items.append(new QStandardItem(QTime::fromMSecsSinceStartOfDay(inTime.msecsTo(outTime) + 1000.0/frameRate).toString("HH:mm:ss.zzz")));
//            items.append(new QStandardItem(QTime::fromMSecsSinceStartOfDay(originalEditDuration + addedEditDurationRoundedToFPS).toString("HH:mm:ss.zzz")));
            else
                items.append(new QStandardItem(itemModel->index(row,column).data().toString()));
        }

        timelineModel->appendRow(items);

        duration+=inTime.msecsTo(outTime) + 1000.0 / frameRate;// + addedEditDurationRoundedToFPS;

//        qDebug()<<"FTimeline::oneditsChangedToTimeline"<<row<<inTime<<outTime<<originalEditDuration<<addedEditDuration<<addedEditDurationRoundedToFPS<<duration <<axiDelta<<fileName<<durationTime<<*durationString;
    }

    //set order of edits to file order
    QMap<int,int> reorderMap;
    for (int row = 0; row < timelineModel->rowCount();row++)
    {
        reorderMap[timelineModel->index(row, orderBeforeLoadIndex).data().toInt()] = row;
    }

    //combine overlapping
    QTime previousInTime = QTime();
    QTime previousOutTime = QTime();
    QString previousFileName = ""; //tbd: edits are not ordered per filename...

    QMapIterator<int, int> orderIterator(reorderMap);
    while (orderIterator.hasNext()) //all files
    {
        orderIterator.next();
        int row = orderIterator.value();

        QString fileName = timelineModel->index(row,fileIndex).data().toString();
        QTime inTime = QTime::fromString(timelineModel->index(row,inIndex).data().toString(),"HH:mm:ss.zzz");
        QTime outTime = QTime::fromString(timelineModel->index(row,outIndex).data().toString(),"HH:mm:ss.zzz");

        if (previousOutTime != QTime())
        {
//            qDebug()<<"overlapping"<<row<<inTime.msecsTo(previousOutTime)<<inTime.toString("HH:mm:ss.zzz")<<outTime.toString("HH:mm:ss.zzz")<<previousInTime.toString("HH:mm:ss.zzz")<<previousOutTime.toString("HH:mm:ss.zzz");
            if (previousFileName == fileName && FGlobal().msec_rounded_to_fps(inTime.msecsSinceStartOfDay()) <= FGlobal().msec_rounded_to_fps(previousOutTime.msecsSinceStartOfDay()) )
            {
                qDebug()<<"overlapping"<<row<<inTime.msecsTo(previousOutTime)<<inTime.toString("HH:mm:ss.zzz")<<outTime.toString("HH:mm:ss.zzz")<<previousInTime.toString("HH:mm:ss.zzz")<<previousOutTime.toString("HH:mm:ss.zzz");
                inTime = previousInTime;
                timelineModel->setData(timelineModel->index(row,inIndex), inTime.toString("HH:mm:ss.zzz"));
                timelineModel->setData(timelineModel->index(row,durationIndex), QTime::fromMSecsSinceStartOfDay(inTime.msecsTo(outTime) + 1000 / frameRate).toString("HH:mm:ss.zzz"));
                timelineModel->setData(timelineModel->index(row-1,hintIndex), "overlapping");
            }

        }

        previousFileName = fileName;
        previousInTime = inTime;
        previousOutTime = outTime;
    }

    //remove edits
    duration = 0;
//    if (false)
    for (int row = timelineModel->rowCount() -1; row >=0;row--)
    {
        QString hint = timelineModel->index(row,hintIndex).data().toString();
        if (hint == "overlapping")
            timelineModel->takeRow(row);
        else
        {
            QTime inTime = QTime::fromString(timelineModel->index(row,inIndex).data().toString(),"HH:mm:ss.zzz");
            QTime outTime = QTime::fromString(timelineModel->index(row,outIndex).data().toString(),"HH:mm:ss.zzz");
            duration+=inTime.msecsTo(outTime) + 1000 / frameRate;
        }
    }
//    qDebug()<<"duration after remove"<<duration;

    if (duration > 0 && duration != stretchTime *1000/frameRate)
    {
//        stretchDuration(duration);
    }

    duration = 0;
    double previousDuration = 0;

    reorderMap.clear();
    for (int row = 0; row < timelineModel->rowCount();row++)
    {
        QString hint = timelineModel->index(row,hintIndex).data().toString();
        if (hint != "overlapping")
            reorderMap[timelineModel->index(row, orderAfterMovingIndex).data().toInt()] = row;
    }

    int counter = 0;
    QMapIterator<int, int> orderIterator2(reorderMap);
    while (orderIterator2.hasNext()) //all files
    {
        orderIterator2.next();
        int row = orderIterator2.value();
        QTime inTime = QTime::fromString(timelineModel->index(row,inIndex).data().toString(),"HH:mm:ss.zzz");
        QTime outTime = QTime::fromString(timelineModel->index(row,outIndex).data().toString(),"HH:mm:ss.zzz");
        int editCounter = timelineModel->index(row, orderBeforeLoadIndex).data().toInt();

        if (reorderMap.count() == 1)
        {
            duration+= ((inTime.msecsTo(outTime) + 1000 / frameRate)) ;
            m_scrubber->setInOutPoint(editCounter, previousDuration, duration);
        }
        else if (counter == 0) //first
        {
            duration+= ((inTime.msecsTo(outTime) + 1000 / frameRate) - transitionTimeMSecs/2) ;
            m_scrubber->setInOutPoint(editCounter, previousDuration, duration + transitionTimeMSecs / 2);
        }
        else if (counter == reorderMap.count() - 1) //last
        {
            duration+= ((inTime.msecsTo(outTime) + 1000 / frameRate ) - transitionTimeMSecs/2)  ;
            m_scrubber->setInOutPoint(editCounter, previousDuration - transitionTimeMSecs /2, duration);

        }
        else
        {
            duration+= ((inTime.msecsTo(outTime) + 1000 / frameRate) - transitionTimeMSecs);
            m_scrubber->setInOutPoint(editCounter, previousDuration - transitionTimeMSecs /2, duration + transitionTimeMSecs /2);
        }
//        qDebug()<<"FTimeline::oneditsChangedToTimeline"<<previousDuration<<duration;

        previousDuration = duration;
        counter++;
    }

//    qDebug()<<"FTimeline::oneditsChangedToTimeline"<<itemModel->rowCount()<<stretchTime*1000/frameRate<<originalDuration<<multiplier<<duration<<transitionTimeMSecs;

    if (stretchTime == 0)
    {
        m_scrubber->setScale(FGlobal().msec_rounded_to_fps(originalDuration));
        m_durationLabel->setText(FGlobal().msec_to_time(originalDuration).prepend(" / "));
    }
    else
    {
        m_scrubber->setScale(FGlobal().msec_rounded_to_fps(duration));
        m_durationLabel->setText(FGlobal().msec_to_time(originalDuration).prepend(" / ") + FGlobal().msec_to_time(duration).prepend(" -> "));
    }
} //oneditsChangedToTimeline

void FTimeline::onFileIndexClicked(QModelIndex index)
{
    qDebug()<<"FTimeline::onFileIndexClicked"<<index.data().toString();
}

void FTimeline::onVideoPositionChanged(int progress, int row, int relativeProgress)
{
//    qDebug()<<"FTimeline::onVideoPositionChanged"<<progress;
    int *relativeProgressl = new int();
    m_scrubber->rowToPosition(row, relativeProgressl);

    if (*relativeProgressl != -1)
    {
        m_scrubber->onSeek(FGlobal().msec_rounded_to_fps(relativeProgress + *relativeProgressl));
        m_positionSpinner->blockSignals(true); //do not fire valuechanged signal
        m_positionSpinner->setValue(FGlobal().msec_to_frames(relativeProgress + *relativeProgressl));
        m_positionSpinner->blockSignals(false);
    }

}

void FTimeline::onTimelineWidgetsChanged(int p_transitionTime, Qt::CheckState p_transitionChecked, int p_stretchTime, Qt::CheckState p_stretchChecked, FEditTableView *editTableView)
{
    qDebug()<<"FTimeline::onTimelineWidgetsChanged"<<p_transitionTime<<p_transitionChecked<<p_stretchTime<<p_stretchChecked;
    if (p_transitionChecked == Qt::Checked)
        transitionTime = p_transitionTime;
    else
        transitionTime = 0;

    if (p_stretchChecked == Qt::Checked)
        stretchTime = p_stretchTime;
    else
        stretchTime = 0;
//    transitionChecked = p_transitionChecked;
    oneditsChangedToTimeline(editTableView->editProxyModel);

    qDebug()<<"FTimeline::onTimelineWidgetsChanged done"<<p_transitionTime<<p_transitionChecked<<p_stretchTime<<p_stretchChecked;
    emit editsChangedToVideo(editTableView->model());
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
