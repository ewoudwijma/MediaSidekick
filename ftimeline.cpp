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
    labels << "OrderBL" << "OrderAL"<<"OrderAM"<<"Changed"<< "Path" << "File"<<"Fps"<<"FDur"<<"In"<<"Out"<<"Duration"<<"Rating"<<"Alike"<<"Hint"<<"Tags";
    timelineModel->setHorizontalHeaderLabels(labels);

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

    transitiontime = 0;
    stretchTime = 0;
    transitiontimeDuration = 0;
    transitiontimeLastGood = -1;
    stretchTimeLastGood = -1;

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

//    onEditsChangedToTimeline(itemModel);

//    emit fileIndexClicked(index);
}

void FTimeline::onDurationChanged(int duration)
{
    qDebug()<<"FTimeline::onDurationChanged: " << duration;
//    m_duration = duration / 1000;
//    ui->lcdDuration->display(QTime::fromMSecsSinceStartOfDay(duration).toString("HH:mm:ss.zzz"));

//    m_duration = FGlobal().msec_to_frames(duration);
    m_scrubber->setScale(FGlobal().msec_rounded_to_fps(duration));
    m_durationLabel->setText(FGlobal().msec_to_time(duration).prepend(" / "));

    m_isSeekable = true;

    actionPlay->setEnabled(true);
    actionSkipPrevious->setEnabled(m_isSeekable);
    actionSkipNext->setEnabled(m_isSeekable);
    actionRewind->setEnabled(m_isSeekable);
    actionFastForward->setEnabled(m_isSeekable);
}

void FTimeline::onEditsChangedToTimeline(QAbstractItemModel *itemModel)
{
    if (stretchTime == 0)
        return;

    timelineModel->removeRows(0, timelineModel->rowCount());

    //order edits on chosen order
    originalDuration = 0;
    QMap<int,int> reorderMap;
    for (int row = 0; row < itemModel->rowCount();row++)
    {
        QTime inTime = QTime::fromString(itemModel->index(row,inIndex).data().toString(),"HH:mm:ss.zzz");
        QTime outTime = QTime::fromString(itemModel->index(row,outIndex).data().toString(),"HH:mm:ss.zzz");

        int frameDuration = FGlobal().msec_to_frames(outTime.msecsSinceStartOfDay()) - FGlobal().msec_to_frames(inTime.msecsSinceStartOfDay()) + 1;

        originalDuration += frameDuration;

        reorderMap[itemModel->index(row, orderAfterMovingIndex).data().toInt()] = row;
    }

    //copy itemModel to timelineModel
    QMapIterator<int, int> orderIterator(reorderMap);
    while (orderIterator.hasNext()) //all files
    {
        orderIterator.next();
        int row = orderIterator.value();

        QString fileName = itemModel->index(row,fileIndex).data().toString();

        QList<QStandardItem *> items;
        for (int column=0;column<itemModel->columnCount();column++)
        {
            if (column == ratingIndex)
            {
                FStarRating starRating = qvariant_cast<FStarRating>(itemModel->index(row, ratingIndex).data());
                QStandardItem *starItem = new QStandardItem;
                QVariant starVar = QVariant::fromValue(starRating);
                starItem->setData(starVar, Qt::EditRole);
                items.append(starItem);
            }
            else
                items.append(new QStandardItem(itemModel->index(row,column).data().toString()));
        }

//        //add duration as extra
//        QString *durationPointer = new QString();
//        emit getPropertyValue(fileName, "Duration", durationPointer); //format <30s: [ss.mm s] >30s: [h.mm:ss]
//        QTime fileDurationTime = QTime::fromString(*durationPointer,"h:mm:ss");
//        if (fileDurationTime == QTime())
//        {
//            QString durationString = *durationPointer;
//            durationString = durationString.left(durationString.length()-2); //remove " s"
//            fileDurationTime = QTime::fromMSecsSinceStartOfDay(durationString.toDouble()*1000);
//        }

//        if (fileDurationTime.msecsSinceStartOfDay() == 0)
//            fileDurationTime = QTime::fromMSecsSinceStartOfDay(24*60*60*1000 - 1);

////        qDebug()<<"fileDurationTime"<<*durationPointer<<fileDurationTime;

        //add extra columns (temporary?)
        items.append(new QStandardItem("text")); //fileDurationTime.toString("HH:mm:ss.zzz")
        items.append(new QStandardItem("in"));
        items.append(new QStandardItem("out"));

        timelineModel->appendRow(items);
    }

    bool stretchingPossible = true;
    int whileCounter = 0;
    int timelineDuration = -1;
    int previousTimelineDuration = -1;

//    qDebug()<<"FTimeline::onEditsChangedToTimeline"<<timelineDuration<<stretchTime<<transitiontimeDuration<<stretchingPossible<<timelineModel->rowCount();

    //adjust timeline edits until stretchedDuration is the same as stretchTime (or if it is not possible)
    while (stretchingPossible) // && timelineDuration != stretchTime
    {
//        calculateTransitiontimeDuration(timelineModel);

        timelineDuration = 0;
        int stretchableDuration = 0;
        int notOverlappingCounter = 0;
        int notOverlappingNotMaxCounter = 0;
        for (int row = 0; row < timelineModel->rowCount();row++)
        {
            if (timelineModel->index(row, tagIndex + 1).data().toString() != "overlapping") // && timelineModel->index(row, tagIndex + 1).data().toString() != "max"
            {
                QTime inTime = QTime::fromString(timelineModel->index(row,inIndex).data().toString(),"HH:mm:ss.zzz");
                QTime outTime = QTime::fromString(timelineModel->index(row,outIndex).data().toString(),"HH:mm:ss.zzz");

                int frameDuration = FGlobal().msec_to_frames(outTime.msecsSinceStartOfDay()) - FGlobal().msec_to_frames(inTime.msecsSinceStartOfDay()) + 1;

                timelineDuration += frameDuration;
                notOverlappingCounter++;
                if (timelineModel->index(row, tagIndex + 1).data().toString() != "max")
                {
                    notOverlappingNotMaxCounter++;
                    stretchableDuration += frameDuration;
                }
            }
        }

        int notOverlappingCount = notOverlappingCounter;
        int notOverlappingNotMaxCount = notOverlappingNotMaxCounter;
        timelineDuration -= transitiontime * (notOverlappingCount-1);

        if (whileCounter == 0)
            transitiontimeDuration = timelineDuration;

//        double multiplier = 1;
//        if (stretchTime != timelineDuration && timelineDuration > 0)
//            multiplier = double(stretchTime) / double(timelineDuration);

        int delta = stretchTime - timelineDuration;

        qDebug()<<"FTimeline::onEditsChangedToTimeline multi"<<timelineDuration<<stretchTime<<delta<<timelineModel->rowCount()<<notOverlappingCount<<notOverlappingNotMaxCount;

        double axiDelta = 0;

        bool allowed = true;

        bool debug = false;

        notOverlappingNotMaxCounter = 0;
        for (int row = 0; row < timelineModel->rowCount();row++)
        {
            if (timelineModel->index(row, tagIndex + 1).data().toString() != "overlapping" && timelineModel->index(row, tagIndex + 1).data().toString() != "max")
            {
                if (debug)
                    qDebug()<<"stretching"<<row<<timelineModel->index(row, tagIndex + 1).data().toString();
                QTime inTime = QTime::fromString(timelineModel->index(row,inIndex).data().toString(),"HH:mm:ss.zzz");
                QTime outTime = QTime::fromString(timelineModel->index(row,outIndex).data().toString(),"HH:mm:ss.zzz");
                QTime fileDurationTime = QTime::fromString(timelineModel->index(row, fileDurationIndex).data().toString(),"HH:mm:ss.zzz");

                int editduration = FGlobal().msec_to_frames(outTime.msecsSinceStartOfDay()) - FGlobal().msec_to_frames(inTime.msecsSinceStartOfDay()) + 1;

                //stretch edits
                if (timelineDuration != stretchTime)
                {
                    double addedEditDuration;// = originalEditDuration * (multiplier - 1.0); //can also be negative!

//                    if (notOverlappingNotMaxCount == 1)
//                        addedEditDuration = (editduration) * multiplier - editduration;
//                    else if (notOverlappingNotMaxCounter == 0) //first
//                        addedEditDuration = (editduration - double(transitiontime) / 2.0) * multiplier + double(transitiontime) / 2.0 - editduration;
//                    else if (notOverlappingNotMaxCounter == notOverlappingNotMaxCount - 1) //last
//                        addedEditDuration = (editduration - double(transitiontime) / 2.0) * multiplier + double(transitiontime) / 2.0 - editduration;
//                    else
//                        addedEditDuration = (editduration - double(transitiontime)) * multiplier + double(transitiontime) - editduration;

                    addedEditDuration = double(delta) * double(editduration) / double(stretchableDuration);//double to avoid rounding

                    if (debug)
                        qDebug()<<"addedEditDuration1"<<row<<addedEditDuration<<axiDelta<<notOverlappingNotMaxCount<<editduration<<stretchableDuration<<delta;

                    addedEditDuration += axiDelta;
                    axiDelta = addedEditDuration - qRound(addedEditDuration);

                    if (debug)
                        qDebug()<<"addedEditDuration2"<<row<<addedEditDuration<<axiDelta<<notOverlappingNotMaxCount;

                    if (notOverlappingNotMaxCounter == notOverlappingNotMaxCount - 1) //last
                    {
                        addedEditDuration += axiDelta;
                        axiDelta = addedEditDuration - qRound(addedEditDuration);
                    }

                    if (debug)
                        qDebug()<<"addedEditDuration3"<<row<<addedEditDuration<<axiDelta<<notOverlappingNotMaxCount;

                    int framesLeftNeeded = qRound(addedEditDuration) / 2;
                    int framesRightNeeded = qRound(addedEditDuration) - framesLeftNeeded;
                    int framesLeftAvailable = FGlobal().msec_to_frames(inTime.msecsSinceStartOfDay());
                    int framesRightAvailable = FGlobal().msec_to_frames(fileDurationTime.msecsSinceStartOfDay()) - FGlobal().msec_to_frames(outTime.msecsSinceStartOfDay());
                    int framesLeftAssigned = framesLeftNeeded;
                    int framesRightAssigned = framesRightNeeded;

                    if (debug)
                        qDebug()<<"frameadding 0"<<framesLeftNeeded<<framesLeftAvailable<<framesLeftAssigned<<framesRightNeeded<<framesRightAvailable<<framesRightAssigned;

                    if (framesLeftAvailable < framesLeftNeeded) //if not enough available on the left, add to the right
                    {
                        if (debug)
                            qDebug()<<"frameadding 1 left"<<framesLeftNeeded<<framesLeftAvailable<<framesLeftAssigned<<framesRightNeeded<<framesRightAvailable<<framesRightAssigned;
                        framesLeftAssigned = framesLeftAvailable;
                        framesRightAssigned += framesLeftNeeded - framesLeftAvailable;
                    }
                    if (framesRightAvailable < framesRightNeeded) //if not enough available on the right, add to the left
                    {
                        if (debug)
                            qDebug()<<"frameadding 1 right"<<framesLeftNeeded<<framesLeftAvailable<<framesLeftAssigned<<framesRightNeeded<<framesRightAvailable<<framesRightAssigned;
                        framesRightAssigned = framesRightAvailable;
                        framesLeftAssigned += framesRightNeeded - framesRightAvailable;
                    }

                    if (framesLeftAssigned > framesLeftAvailable) //if (still) not enough available, assign only what is available
                    {
                        if (debug)
                            qDebug()<<"frameadding 2 left"<<framesLeftNeeded<<framesLeftAvailable<<framesLeftAssigned<<framesRightNeeded<<framesRightAvailable<<framesRightAssigned;
                        framesLeftAssigned = framesLeftAvailable;
                    }
                    if (framesRightAssigned > framesRightAvailable) //if (still) not enough available, assign only what is available
                    {
                        if (debug)
                            qDebug()<<"frameadding 2 right"<<framesLeftNeeded<<framesLeftAvailable<<framesLeftAssigned<<framesRightNeeded<<framesRightAvailable<<framesRightAssigned;
                        framesRightAssigned = framesRightAvailable;
                    }

                    if (framesLeftNeeded != framesLeftAssigned || framesRightNeeded != framesRightAssigned) //if not assigned what is needed
                        if (debug)
                            qDebug()<<"frameadding 3"<<row <<framesLeftNeeded<<framesLeftAvailable<<framesLeftAssigned<<framesRightNeeded<<framesRightAvailable<<framesRightAssigned<<FGlobal().msec_to_frames(inTime.msecsSinceStartOfDay());

                    if (framesLeftAssigned != 0)
                    {
                        if (debug)
                            qDebug()<<"frameadding 3 left"<<row <<-qMin(FGlobal().frames_to_msec(framesLeftAssigned), inTime.msecsSinceStartOfDay())<<FGlobal().msec_to_frames(inTime.msecsSinceStartOfDay());
                        inTime = inTime.addMSecs(-qMin(FGlobal().frames_to_msec(framesLeftAssigned), inTime.msecsSinceStartOfDay())); //not less then 0
                    }
                    if (framesRightAssigned != 0)
                    {
                        if (debug)
                            qDebug()<<"frameadding 3 right"<<row <<FGlobal().frames_to_msec(framesRightAssigned)<<FGlobal().msec_to_frames(outTime.msecsSinceStartOfDay());
                        outTime = outTime.addMSecs(FGlobal().frames_to_msec(framesRightAssigned));
//                        qDebug()<<"frameadding 3 right"<<row <<FGlobal().frames_to_msec(framesRightAssigned)<<FGlobal().msec_to_frames(outTime.msecsSinceStartOfDay());
                    }

                    if (framesLeftNeeded != framesLeftAssigned || framesRightNeeded != framesRightAssigned) //if not assigned what is needed
                        if (debug)
                            qDebug()<<"frameadding 3"<<row <<framesLeftNeeded<<framesLeftAvailable<<framesLeftAssigned<<framesRightNeeded<<framesRightAvailable<<framesRightAssigned<<FGlobal().msec_to_frames(inTime.msecsSinceStartOfDay());

                    editduration = FGlobal().msec_to_frames(outTime.msecsSinceStartOfDay()) - FGlobal().msec_to_frames(inTime.msecsSinceStartOfDay()) + 1;

                    if (framesLeftNeeded != framesLeftAssigned || framesRightNeeded != framesRightAssigned)
                        if (debug)
                            qDebug()<<"frameadding 3"<<timelineModel->index(row, inIndex).data().toString();

                    if (framesLeftAssigned != 0 || framesRightAssigned != 0)
                    {
                        if (debug)
                            qDebug()<<"SET inout frameadding"<<row<<inTime.msecsSinceStartOfDay()<<outTime.msecsSinceStartOfDay()<<FGlobal().msec_to_frames(inTime.msecsSinceStartOfDay())<<FGlobal().msec_to_frames(outTime.msecsSinceStartOfDay())<<editduration;
                        timelineModel->setData(timelineModel->index(row, inIndex), inTime.toString("HH:mm:ss.zzz"));
                        timelineModel->setData(timelineModel->index(row, outIndex), outTime.toString("HH:mm:ss.zzz"));
                        timelineModel->setData(timelineModel->index(row, durationIndex), QTime::fromMSecsSinceStartOfDay(FGlobal().frames_to_msec(editduration)).toString("HH:mm:ss.zzz"));
                    }

                    if (debug)
                        qDebug()<<"check max"<<row<<editduration<<FGlobal().msec_to_frames(fileDurationTime.msecsSinceStartOfDay())<<framesLeftAssigned<<framesRightAssigned;
                    if (editduration == FGlobal().msec_to_frames(fileDurationTime.msecsSinceStartOfDay()) + 1)
                    {
//                        qDebug()<<"check max"<<row<<editduration<<FGlobal().msec_to_frames(fileDurationTime.msecsSinceStartOfDay());
                        timelineModel->setData(timelineModel->index(row,  tagIndex + 1), "max");
                    }

                    if (framesLeftNeeded != framesLeftAssigned || framesRightNeeded != framesRightAssigned)
                        if (debug)
                            qDebug()<<"frameadding 3"<<timelineModel->index(row, inIndex).data().toString();
                }
                notOverlappingNotMaxCounter++;
            } //if
        } //for

        if (debug)
            qDebug()<<"Axidelta"<<axiDelta;

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
        int previousRow = -1;

        QMapIterator<int, int> orderIterator(reorderMap);
        while (orderIterator.hasNext()) //all files
        {
            orderIterator.next();
            int row = orderIterator.value();

            if (timelineModel->index(row, tagIndex + 1).data().toString() != "overlapping")
            {
                QString fileName = timelineModel->index(row,fileIndex).data().toString();

                if (previousFileName != fileName)
                {
                    previousInTime = QTime();
                    previousOutTime = QTime();
                }

                QTime inTime = QTime::fromString(timelineModel->index(row,inIndex).data().toString(),"HH:mm:ss.zzz");
                QTime outTime = QTime::fromString(timelineModel->index(row,outIndex).data().toString(),"HH:mm:ss.zzz");

                if (previousOutTime != QTime())
                {
        //            qDebug()<<"overlapping"<<row<<inTime.msecsTo(previousOutTime)<<inTime.toString("HH:mm:ss.zzz")<<outTime.toString("HH:mm:ss.zzz")<<previousInTime.toString("HH:mm:ss.zzz")<<previousOutTime.toString("HH:mm:ss.zzz");
                    if (previousFileName == fileName && FGlobal().msec_to_frames(inTime.msecsSinceStartOfDay()) <= FGlobal().msec_to_frames(previousOutTime.msecsSinceStartOfDay()) )
                    {
                        qDebug()<<"overlapping"<<previousRow<<row<<inTime.msecsTo(previousOutTime)<<previousInTime.toString("HH:mm:ss.zzz")<<previousOutTime.toString("HH:mm:ss.zzz")<<inTime.toString("HH:mm:ss.zzz")<<outTime.toString("HH:mm:ss.zzz");
//                        previousInTime = previousInTime.addMSecs(FGlobal().frames_to_msec(transitiontime));
//                        qDebug()<<"SET in frameadding"<<row<<previousInTime<<FGlobal().msec_to_frames(previousInTime.msecsTo(outTime))+1;

                        timelineModel->setData(timelineModel->index(row, inIndex), previousInTime.toString("HH:mm:ss.zzz"));

                        FStarRating starRating0 = qvariant_cast<FStarRating>(itemModel->index(previousRow, ratingIndex).data());
                        FStarRating starRating1 = qvariant_cast<FStarRating>(itemModel->index(row, ratingIndex).data());
//                        qDebug()<<"starRating1starRating1"<<starRating0.starCount()<<starRating1.starCount()<<(starRating0.starCount() + starRating1.starCount()) / 2;
                        QVariant starVar = QVariant::fromValue(FStarRating(qMax(starRating0.starCount(), starRating1.starCount())));

                        timelineModel->setData(timelineModel->index(row, ratingIndex), starVar);
                        timelineModel->setData(timelineModel->index(row, alikeIndex), timelineModel->index(previousRow, alikeIndex).data().toBool() || timelineModel->index(row,  alikeIndex).data().toBool());
                        timelineModel->setData(timelineModel->index(row, hintIndex), timelineModel->index(previousRow, hintIndex).data().toString() + " " + timelineModel->index(row, hintIndex).data().toString());
                        timelineModel->setData(timelineModel->index(row, tagIndex), timelineModel->index(previousRow, tagIndex).data().toString() + ";" + timelineModel->index(row, tagIndex).data().toString());
                        timelineModel->setData(timelineModel->index(row, durationIndex), QTime::fromMSecsSinceStartOfDay(FGlobal().frames_to_msec(FGlobal().msec_to_frames(outTime.msecsSinceStartOfDay()) - FGlobal().msec_to_frames(inTime.msecsSinceStartOfDay()) + 1)).toString("HH:mm:ss.zzz"));
                        timelineModel->setData(timelineModel->index(previousRow, tagIndex + 1), "overlapping");

                        inTime = previousInTime;
                    }
//                    else
//                        qDebug()<<"not overlapping"<<row<<inTime.msecsTo(previousOutTime)<<previousInTime.toString("HH:mm:ss.zzz")<<previousOutTime.toString("HH:mm:ss.zzz")<<inTime.toString("HH:mm:ss.zzz")<<outTime.toString("HH:mm:ss.zzz");
                }

                previousFileName = fileName;
                previousInTime = inTime;
                previousOutTime = outTime;
                previousRow = row;
            }
        }

        timelineDuration = 0;
        int previousPreviousRow =  -1;
        previousRow = -1;
        int previousOut = 0;

        int countNrOfEditsNotOverlapping = 0;
        for (int row = 0; row < timelineModel->rowCount();row++)
        {
            if (timelineModel->index(row, tagIndex + 1).data().toString() != "overlapping")
            {
                QTime inTime = QTime::fromString(timelineModel->index(row,inIndex).data().toString(),"HH:mm:ss.zzz");
                QTime outTime = QTime::fromString(timelineModel->index(row,outIndex).data().toString(),"HH:mm:ss.zzz");
//                QTime fileDurationTime = QTime::fromString(timelineModel->index(row,fileDurationIndex).data().toString(),"HH:mm:ss.zzz");

                int editduration = FGlobal().msec_to_frames(outTime.msecsSinceStartOfDay()) - FGlobal().msec_to_frames(inTime.msecsSinceStartOfDay()) + 1;

                int inpoint, outpoint;

//                if (timelineModel->rowCount() == 1)
//                {
//                    inpoint = 0;
//                    outpoint = editduration;
//                }
                if (countNrOfEditsNotOverlapping == 0) //first
                {
                    inpoint = 0;
                    outpoint = editduration;
                }
//                else if (row == timelineModel->rowCount() - 1) //last tbd: exclude overlappings
//                {
//                    inpoint = previousOut - transitiontime;
//                    outpoint = inpoint + editduration;
//                }
                else
                {
                    inpoint = previousOut - transitiontime;
                    outpoint = inpoint + editduration;
                }

                timelineDuration += editduration;

//                if (timelineModel->index(row, tagIndex + 1).data().toString() != "max")
//                    timelineModel->setData(timelineModel->index(row,  tagIndex + 1), QString::number(timelineDuration));
                timelineModel->setData(timelineModel->index(row, tagIndex + 2), QString::number(inpoint));
                timelineModel->setData(timelineModel->index(row, tagIndex + 3), QString::number(outpoint));

                //check if even and odd edits are not overlapping (below 0 or above length will --most likely-- not happen)
    //            if (inpoint < 0)
    //            {
    //                allowed = false;
    //                qDebug()<<"FTimeline::onEditsChangedToTimeline transitiontime. in less then 0"<<row<<inpoint;
    //            }
    //            if (outpoint > timelineDuration) //cannot be checked here already tbd: move below in different for loop
    //            {
    //                allowed = false;
    //                qDebug()<<"FTimeline::onEditsChangedToTimeline transitiontime.  out > duration"<<row<<outpoint<<FGlobal().msec_to_frames(fileDurationTime.msecsSinceStartOfDay());
    //            }
                if (previousPreviousRow != -1 && timelineModel->index(previousPreviousRow, tagIndex + 3).data().toInt() >= inpoint)
                {
                    allowed = false;
//                    qDebug()<<"FTimeline::onEditsChangedToTimeline transitiontime. out/in overlap"<<row<<timelineModel->index(previousPreviousRow, tagIndex + 3).data().toInt()<<inpoint;
                }

                previousOut = outpoint;
                previousPreviousRow = previousRow;
                previousRow = row;
                countNrOfEditsNotOverlapping ++;
            }
        }
        timelineDuration -= transitiontime * (countNrOfEditsNotOverlapping - 1); //subtrackt all the transitions

        if (!allowed)
        {
            if (transitiontimeLastGood != -1 && stretchTimeLastGood != -1)
            {
//                qDebug()<<"timeline error"<<transitiontimeLastGood<<stretchTimeLastGood;
                emit adjustTransitionAndStretchTime(transitiontimeLastGood, stretchTimeLastGood);
                return;
            }
        }
        else
        {
//            qDebug()<<"timeline good"<<transitiontimeLastGood<<stretchTimeLastGood;
            transitiontimeLastGood = transitiontime;
            stretchTimeLastGood = stretchTime;
        }

        //recalculate duration...
//        double duration = 0;
//        for (int row = timelineModel->rowCount() -1; row >=0;row--)
//        {
//            if (timelineModel->index(row, tagIndex + 1).data().toString() == "overlapping")
//            {
////                timelineModel->takeRow(row);
//            }
//            else
//            {
//                QTime inTime = QTime::fromString(timelineModel->index(row,inIndex).data().toString(),"HH:mm:ss.zzz");
//                QTime outTime = QTime::fromString(timelineModel->index(row,outIndex).data().toString(),"HH:mm:ss.zzz");
//                duration += FGlobal().msec_to_frames(outTime.msecsSinceStartOfDay()) - FGlobal().msec_to_frames(inTime.msecsSinceStartOfDay()) + 1;
//            }
//        }
        qDebug()<<"duration after remove"<<whileCounter<<timelineDuration<<stretchTime;
//        timelineDuration = duration - transitiontime * (timelineModel->rowCount() - 1); //subtrackt all the transitions

        whileCounter++;
        stretchingPossible = timelineDuration != stretchTime && previousTimelineDuration != timelineDuration && whileCounter <= 10 ;
        previousTimelineDuration = timelineDuration;
    }

    //display on timeline
    m_scrubber->clearInOuts();
    for (int row = 0; row < timelineModel->rowCount();row++)
    {
        if (timelineModel->index(row, tagIndex + 1).data().toString() != "overlapping")
        {
            int porderBeforeLoadIndex = timelineModel->index(row, orderBeforeLoadIndex).data().toInt();
            m_scrubber->setInOutPoint(porderBeforeLoadIndex,FGlobal().frames_to_msec( timelineModel->index(row,tagIndex + 2).data().toInt()), FGlobal().frames_to_msec(timelineModel->index(row,tagIndex + 3).data().toInt()));
        }
    }

    //set timeline length and labels right
    if (stretchTime == transitiontimeDuration) //no stretch
    {
        m_scrubber->setScale(FGlobal().frames_to_msec(transitiontimeDuration));
        m_durationLabel->setText(FGlobal().frames_to_time(originalDuration).prepend(" / ") + FGlobal().frames_to_time(transitiontimeDuration).prepend(" -> "));
    }
    else
    {
        m_scrubber->setScale(FGlobal().frames_to_msec(timelineDuration));//timelineDuration
        m_durationLabel->setText(FGlobal().frames_to_time(originalDuration).prepend(" / ") + FGlobal().frames_to_time(transitiontimeDuration).prepend(" -> ") + FGlobal().frames_to_time(timelineDuration).prepend(" -> "));
    }

    emit editsChangedToTimeline(timelineModel);
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

void FTimeline::onTimelineWidgetsChanged(int p_transitiontime, QString transitionType, int p_stretchTime, FEditTableView *editTableView)
{
    if (transitionType != "No transition")
        transitiontime = p_transitiontime;
    else
        transitiontime = 0;

    qDebug()<<"FTimeline::onTimelineWidgetsChanged"<<p_transitiontime<<transitionType<<p_stretchTime;

    stretchTime = p_stretchTime;

    onEditsChangedToTimeline(editTableView->editProxyModel);

//    qDebug()<<"FTimeline::onTimelineWidgetsChanged done"<<p_transitiontime<<transitionType<<p_stretchTime<<p_stretchChecked;
    emit editsChangedToVideo(editTableView->model());
}

void FTimeline::onScrubberSeeked(int mseconds)
{
    int *prevRow = new int();
    int *nextRow = new int();
    int *relativeProgress = new int();
    m_scrubber->progressToRow(mseconds, prevRow, nextRow, relativeProgress);

    qDebug()<<"FTimeline::onScrubberSeeked"<<mseconds<< *prevRow<< *relativeProgress;
//    if (m_player->state() != QMediaPlayer::PausedState)
//        m_player->pause();
//    m_player->setPosition(mseconds);
    emit timelinePositionChanged(mseconds, *prevRow, *relativeProgress);
}
