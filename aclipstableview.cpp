#include "aclipsitemdelegate.h"
#include "aclipstableview.h"
#include "astarrating.h"
#include "stimespinbox.h"

#include <QApplication>
#include <QHeaderView>
#include <QMenu>
#include <QMessageBox>
#include <QSettings>
#include <QTextStream>

#include <QDebug>

#include "aglobal.h"

AClipsTableView::AClipsTableView(QWidget *parent) : QTableView(parent)
{
    clipsItemModel = new AClipsItemModel(this);
    QStringList labels;
    labels << "OrderBL" << "OrderAL"<<"OrderAM"<<"Changed"<< "Path" << "File"<<"Fps"<<"FDur"<<"In"<<"Out"<<"Duration"<<"Rating"<<"Alike"<<"Hint"<<"Tags";
    clipsItemModel->setHorizontalHeaderLabels(labels);

    clipsProxyModel = new AClipsSortFilterProxyModel(this);

    clipsProxyModel->setSourceModel(clipsItemModel);

    setModel(clipsProxyModel);

    horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    verticalHeader()->setSectionsMovable(true);
    horizontalHeader()->setSectionsClickable(true);
    horizontalHeader()->setStretchLastSection( true ); //tags

    sortByColumn(orderAtLoadIndex, Qt::AscendingOrder);

//    setColumnWidth(fileIndex,columnWidth(fileIndex) * 2);
//    setColumnWidth(durationIndex,int(columnWidth(durationIndex) / 1.5));
    setColumnWidth(orderBeforeLoadIndex, 1);
    setColumnWidth(orderAtLoadIndex, 1);
    setColumnWidth(orderAfterMovingIndex, 1);
    setColumnWidth(changedIndex, 1);
    setColumnWidth(fpsIndex,int(columnWidth(fpsIndex) / 2));
//    setColumnWidth(fileDurationIndex,int(columnWidth(fileDurationIndex) / 2));
    setColumnWidth(ratingIndex,int(columnWidth(ratingIndex) / 1.5));
    setColumnWidth(alikeIndex,columnWidth(alikeIndex) / 2);

    setColumnHidden(folderIndex, true);
    setColumnHidden(fileIndex, true);
    setColumnHidden(hintIndex, true);

    //from designer defaults
    setDragDropMode(DragDropMode::DropOnly);
    setHorizontalScrollMode(ScrollPerPixel);
    setVerticalScrollMode(ScrollPerPixel);

    setSelectionMode(QAbstractItemView::ContiguousSelection);

    verticalHeader()->setSectionResizeMode (QHeaderView::Fixed);

    setEditTriggers(QAbstractItemView::AnyKeyPressed | QAbstractItemView::SelectedClicked);
//    setEditTriggers(QAbstractItemView::DoubleClicked
//                                | QAbstractItemView::SelectedClicked);
//    setSelectionBehavior(QAbstractItemView::SelectRows);

    AClipsItemDelegate *clipsItemDelegate = new AClipsItemDelegate(this);
    setItemDelegate(clipsItemDelegate);

    connect( this, &QTableView::clicked, this, &AClipsTableView::onIndexClicked);
    connect( this, &QTableView::activated, this, &AClipsTableView::onIndexActivated);
    connect( clipsItemModel, &AClipsItemModel::dataChanged, this, &AClipsTableView::onDataChanged);
    connect( verticalHeader(), &QHeaderView::sectionMoved, this, &AClipsTableView::onSectionMoved);
    connect( verticalHeader(), &QHeaderView::sectionEntered, this, &AClipsTableView::onSectionEntered);

//    connect( clipsItemDelegate, &AClipsItemDelegate::spinnerChanged, this, &AClipsTableView::onSpinnerChanged);

    clipContextMenu = new QMenu(this);
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(onClipRightClickMenu(const QPoint &)));

    clipContextMenu->addAction(new QAction("Delete",clipContextMenu));
    connect(clipContextMenu->actions().last(), SIGNAL(triggered()), this, SLOT(onClipDelete()));

    srtFileItemModel = new QStandardItemModel(this);

    doNotUpdate = false;

    nrOfDeletedItems = 0;
}

void AClipsTableView::onIndexClicked(QModelIndex index)
{
    qDebug()<<"AClipsTableView::onIndexClicked"<<index.data()<<clipsProxyModel->index(index.row(),fileIndex).data().toString()<<selectedFileName;
//    if (clipsProxyModel->index(index.row(),fileIndex).data().toString()!=fileUrl.fileName())
//    {
//        qDebug()<<"tableClicked different!!!"<<index.data()<<clipsProxyModel->index(index.row(),fileIndex).data()<<fileUrl.fileName();
//    }

    emit indexClicked(index);

    if (clipsProxyModel->index(index.row(),fileIndex).data().toString() != selectedFileName)
    {
        emit clipsChangedToVideo(model()); //not to timeline because this can only happen if all clips are shown and they are in the timeline already then
        selectedFileName = clipsProxyModel->index(index.row(),fileIndex).data().toString();
//        qDebug()<<"AClipsTableView::onIndexClicked change selectedFileName"<<selectedFileName;
        selectClips();
    }
}

void AClipsTableView::onIndexActivated(QModelIndex index)
{
    qDebug()<<"AClipsTableView::onIndexActivated"<<index.data();
}

void AClipsTableView::onSectionEntered(int logicalIndex)
{
    qDebug()<<"AClipsTableView::onSectionEntered"<<logicalIndex;
}

void AClipsTableView::onSectionMoved(int , int , int ) //logicalIndex, oldVisualIndex, newVisualIndex
{

//    qDebug()<<"AClipsTableView::onSectionMoved"<<logicalIndex<< oldVisualIndex<< newVisualIndex;
//    doNotUpdate = true;
    for (int row=0; row<clipsProxyModel->rowCount();row++)
    {
        int newOrder = (verticalHeader()->visualIndex(row) + 1) * 10;
        if (clipsProxyModel->index(row, orderAfterMovingIndex).data().toInt() != newOrder)
        {
            clipsProxyModel->setData(clipsProxyModel->index(row, orderAfterMovingIndex), newOrder);
            clipsProxyModel->setData(clipsProxyModel->index(row, changedIndex), "yes");
        }
    }
//    doNotUpdate = false;

    emit clipsChangedToTimeline(clipsProxyModel); //not to video because this will not change the video
}

void AClipsTableView::onFolderIndexClicked(QModelIndex )//index
{
    selectedFolderName = QSettings().value("LastFolder").toString();
//    qDebug()<<"AClipsTableView::onFolderIndexClicked"<<selectedFolderName;
    loadModel(selectedFolderName);

    emit folderIndexClickedItemModel(clipsItemModel);
    emit folderIndexClickedProxyModel(model());
}

void AClipsTableView::onFileIndexClicked(QModelIndex index, QModelIndexList selectedIndices)
{
    selectedFolderName = QSettings().value("LastFileFolder").toString();
    selectedFileName = index.data().toString();

//    qDebug()<<"AClipsTableView::onFileIndexClicked"<<selectedFolderName<<selectedFileName;

    selectClips();

    emit fileIndexClicked(index, selectedIndices);
}

void AClipsTableView::selectClips()
{
//    clearSelection();
//    setSelectionMode(QAbstractItemView::MultiSelection);

    doNotUpdate = true; //to avoid datachanged trigger
//    int firstRow = -1;
    for (int row = 0; row < model()->rowCount(); row++)
    {
        QModelIndex clipsFileIndex = model()->index(row, fileIndex);
//        QModelIndex clipsInIndex = model()->index(row, inIndex);

        QColor backgroundColor;
        if (clipsFileIndex.data().toString() == selectedFileName)
        {
//            if (firstRow == -1)
//                firstRow = row;
//            setCurrentIndex(clipsInIndex); //does also the scrollTo
            backgroundColor = qApp->palette().color(QPalette::AlternateBase);
//            emit clipAdded(clipInIndex);
        }
        else
        {
            backgroundColor = qApp->palette().color(QPalette::Base);
        }
//        qDebug()<<"selectClips"<<selectedFileName<<clipsFileIndex.data()<<backgroundColor;

        for (int col = 0; col < model()->columnCount(); col++)
        {
            QModelIndex index = model()->index(row, col);
            model()->setData(index, backgroundColor, Qt::BackgroundRole);
        }
//        scrollTo(model()->index(firstRow, inIndex));
    }
    doNotUpdate = false;
}

void AClipsTableView::addClip(int rating, bool alike, QAbstractItemModel *tagFilter1Model, QAbstractItemModel *tagFilter2Model)
{
    if (selectedFileName == "")
    {
        QMessageBox::information(this, "Add clip", tr("No file selected"));
        return;
    }

    QTime newInTime = QTime::fromMSecsSinceStartOfDay(position);
    QTime newOutTime = QTime::fromMSecsSinceStartOfDay(position + 3000 - AGlobal().frames_to_msec(1));

    qDebug()<<"AClipsTableView::addClip"<<newInTime<<newOutTime<<selectedFileName;

    int rowToAppendBefore = -1;
    int fileRow = -1;
    int maxOrderAtLoad = 0;
    int maxOrderAfterMoving = 0;
    for (int row = 0; row < clipsItemModel->rowCount(); row++)
    {
        QString fileName = clipsItemModel->index(row, fileIndex).data().toString();
        QTime inTime = QTime::fromString(clipsItemModel->index(row,inIndex).data().toString(), "HH:mm:ss.zzz");
        QTime outTime = QTime::fromString(clipsItemModel->index(row,outIndex).data().toString(), "HH:mm:ss.zzz");
        QTime fileDuration = QTime::fromString(clipsItemModel->index(row, fileDurationIndex).data().toString(), "HH:mm:ss.zzz");

        //check overlapping, remove overlap from this clip if this is the case
        if (fileName == selectedFileName)
        {
            if (inTime.msecsTo(newOutTime) >= 0 && outTime.msecsTo(newInTime) <= 0) //intime before newouttime and outtime after newintime
            {
                qDebug()<<"addclip"<<row<<"overlapping"<<inTime<<outTime<<newInTime<<newOutTime;
                if (outTime.msecsTo(newOutTime) <= 0) //outtime after newouttime
                {
    //                qDebug()<<"addclip"<<row<<"adjust outtime";
                    newOutTime = inTime.addMSecs( - AGlobal().frames_to_msec(1) );
                }
                if (inTime.msecsTo(newInTime) >= 0) //intime after newintime
                {
    //                qDebug()<<"addclip"<<row<<"adjust intime";
                    newInTime = outTime.addMSecs( AGlobal().frames_to_msec(1) );
                }
            }
            if (newOutTime.msecsSinceStartOfDay() > fileDuration.msecsSinceStartOfDay())
            {
                newOutTime = fileDuration;
                qDebug()<<"AClipsTableView::addClip adjust"<<newInTime<<newOutTime<<fileDuration;
            }

        }

        QString fileNameAudioVideoPrefix;
        if (fileName.toLower().contains(".mp3"))
            fileNameAudioVideoPrefix = "ZZZ";
        else
            fileNameAudioVideoPrefix = "AAA";

        QString selectedFileNameAudioVideoPrefix;
        if (selectedFileName.toLower().contains(".mp3"))
            selectedFileNameAudioVideoPrefix = "ZZZ";
        else
            selectedFileNameAudioVideoPrefix = "AAA";

        if (fileNameAudioVideoPrefix + fileName > selectedFileNameAudioVideoPrefix + selectedFileName && fileRow == -1)
            fileRow = row;

        if (selectedFileName == fileName && newInTime.msecsTo(inTime) > 0 && rowToAppendBefore == -1)
            rowToAppendBefore = row;

        maxOrderAtLoad = qMax(maxOrderAtLoad, clipsItemModel->index(row, orderAtLoadIndex).data().toInt());
        maxOrderAfterMoving = qMax(maxOrderAtLoad, clipsItemModel->index(row, orderAfterMovingIndex).data().toInt());
    }

    qDebug()<<"AClipsTableView::addClip after"<<newInTime<<newOutTime<<fileRow<<rowToAppendBefore<<maxOrderAtLoad<<maxOrderAfterMoving;

    if (newInTime.msecsTo(newOutTime) <= 0) //if no place for new clip then cancel operation
    {
        QMessageBox::information(this, "Add clip", tr("No place for new clip") + newInTime.toString() + " " + newOutTime.toString());
        return;
    }

    if (rowToAppendBefore == -1)
        rowToAppendBefore = fileRow;

    if (rowToAppendBefore != -1) //move all rows after rowToAppendBefore
    {
        for (int row = rowToAppendBefore; row < clipsItemModel->rowCount(); row++)
        {
                clipsItemModel->setData(clipsItemModel->index(row, orderBeforeLoadIndex), clipsItemModel->index(row, orderBeforeLoadIndex).data().toInt()+1);
                clipsItemModel->setData(clipsItemModel->index(row, orderAtLoadIndex), clipsItemModel->index(row, orderAtLoadIndex).data().toInt()+10);
                clipsItemModel->setData(clipsItemModel->index(row, orderAfterMovingIndex), clipsItemModel->index(row, orderAfterMovingIndex).data().toInt()+10);
                clipsItemModel->setData(clipsItemModel->index(row, changedIndex), "yes");
        }
    }

    qDebug()<<"AClipsTableView::addClip"<<position<<newInTime<<newOutTime<<selectedFolderName<<selectedFileName<<rowToAppendBefore<<clipsItemModel->index(rowToAppendBefore, orderBeforeLoadIndex).data().toInt();

    QList<QStandardItem *> items;

    QStandardItem *starItem = new QStandardItem;
    QVariant starVar = QVariant::fromValue(AStarRating(rating));
    starItem->setData(starVar, Qt::EditRole);

    if (rowToAppendBefore == -1) //add at the end
    {
        items.append(new QStandardItem(QString::number(clipsItemModel->rowCount()+1)));
        items.append(new QStandardItem(QString::number(maxOrderAtLoad + 10)));
        items.append(new QStandardItem(QString::number(maxOrderAfterMoving + 10)));
//        qDebug()<<"AClipsTableView::addClipAfter"<<position<<newInTime<<newOutTime<<selectedFolderName<<selectedFileName<<rowToAppendBefore<<clipsItemModel->index(rowToAppendBefore, orderBeforeLoadIndex).data().toString();
    }
    else //add in between
    {
        QModelIndex orderBeforeLoadIndexx = clipsItemModel->index(rowToAppendBefore, orderBeforeLoadIndex);
        QModelIndex orderAtLoadIndexx = clipsItemModel->index(rowToAppendBefore, orderAtLoadIndex);
        QModelIndex orderAFterMovingIndexx = clipsItemModel->index(rowToAppendBefore, orderAfterMovingIndex);
//        qDebug()<<"AClipsTableView::addClipBetween"<<position<<newInTime<<newOutTime<<orderAtLoadIndexx.data().toString()<<selectedFolderName<<selectedFileName<<rowToAppendBefore<<clipsItemModel->index(rowToAppendBefore, orderBeforeLoadIndex).data().toInt();

        items.append(new QStandardItem(QString::number(orderBeforeLoadIndexx.data().toInt()-1)));
        items.append(new QStandardItem(QString::number(orderAtLoadIndexx.data().toInt()-1)));
        items.append(new QStandardItem(QString::number(orderAFterMovingIndexx.data().toInt()-1)));
    }

    QString *frameratePointer = new QString();
    emit getPropertyValue(selectedFileName, "VideoFrameRate", frameratePointer);

    QString *durationPointer = new QString();
    emit getPropertyValue(selectedFileName, "Duration", durationPointer);
    *durationPointer = QString(*durationPointer).replace(" (approx)", "");
    QTime fileDurationTime = QTime::fromString(*durationPointer,"h:mm:ss");
    if (fileDurationTime == QTime())
    {
        QString durationString = *durationPointer;
        durationString = durationString.left(durationString.length() - 2); //remove " s"
        fileDurationTime = QTime::fromMSecsSinceStartOfDay(int(durationString.toDouble() * 1000.0));
    }

    if (fileDurationTime.msecsSinceStartOfDay() == 0)
        fileDurationTime = QTime::fromMSecsSinceStartOfDay(24 * 60 * 60 * 1000 - 1);

    QStringList tagList;
    for (int i=0; i < tagFilter1Model->rowCount();i++)
    {
        QString tag = tagFilter1Model->index(i,0).data().toString();
        if (!tagList.contains(tag))
            tagList.append(tag);
    }
    for (int i=0; i < tagFilter2Model->rowCount();i++)
    {
        QString tag = tagFilter2Model->index(i,0).data().toString();
        if (!tagList.contains(tag))
            tagList.append(tag);
    }

    qDebug()<<"addClip"<<alike<<rating;

    items.append(new QStandardItem("yes"));
    items.append(new QStandardItem(selectedFolderName));
    items.append(new QStandardItem(selectedFileName));
    items.append(new QStandardItem(*frameratePointer));
    items.append(new QStandardItem(fileDurationTime.toString("HH:mm:ss.zzz")));
    items.append(new QStandardItem(newInTime.toString("HH:mm:ss.zzz")));
    items.append(new QStandardItem(newOutTime.toString("HH:mm:ss.zzz")));
    items.append(new QStandardItem(QTime::fromMSecsSinceStartOfDay(AGlobal().frames_to_msec(AGlobal().msec_to_frames(newOutTime.msecsSinceStartOfDay()) - AGlobal().msec_to_frames(newInTime.msecsSinceStartOfDay()) + 1)).toString("HH:mm:ss.zzz"))); //durationIndex
    items.append(starItem);
    if (alike)
        items.append(new QStandardItem("true"));
    else
        items.append(new QStandardItem(""));
    items.append(new QStandardItem("")); //hint
    items.append(new QStandardItem(tagList.join(";")));

    if (rowToAppendBefore == -1)
    {
        clipsItemModel->appendRow(items);
//        qDebug()<<"setCurrentIndex1"<<clipsItemModel->rowCount()-1<<clipsItemModel->index(clipsItemModel->rowCount()-1, inIndex).data();
        setCurrentIndex(model()->index(clipsItemModel->rowCount()-1, inIndex));
    }
    else
    {
        clipsItemModel->insertRow(rowToAppendBefore,items);
//        qDebug()<<"setCurrentIndex2"<<rowToAppendBefore<<clipsItemModel->index(rowToAppendBefore, inIndex).data()<<model()->index(rowToAppendBefore, inIndex).data();
        setCurrentIndex(model()->index(rowToAppendBefore, inIndex));
    }

    emit clipsChangedToVideo(model());
    emit clipsChangedToTimeline(clipsProxyModel);
} //addClip

void AClipsTableView::onClipRightClickMenu(const QPoint &point)
{

    QModelIndex index = indexAt(point);
//    qDebug()<<"onClipRightClickMenu"<<point;
    if (index.isValid() )
        clipContextMenu->exec(viewport()->mapToGlobal(point));
}

void AClipsTableView::onClipsDelete(QString fileName)
{
    bool isClipChanged = false;
    for (int row=clipsItemModel->rowCount()-1; row>=0; row--) //reverse as takeRow is changing rows
    {
        QString folderNameX = clipsItemModel->index(row, folderIndex).data().toString();
        QString fileNameX = clipsItemModel->index(row, fileIndex).data().toString();

//        qDebug()<<"AClipsTableView::onClipsDelete"<<folderNameX<<selectedFolderName<<fileNameX<<fileName;
        if (folderNameX == selectedFolderName && fileNameX == fileName)
        {
            clipsItemModel->takeRow(row);
            isClipChanged = true;
        }
    }

    if (isClipChanged) //reset orderBeforeLoadIndex
    {
        for (int row = 0; row<clipsItemModel->rowCount();row++)
        {
            int order = clipsItemModel->index(row, orderBeforeLoadIndex).data().toInt();
            if (order != row+1)
            {
                clipsItemModel->setData(clipsItemModel->index(row, orderBeforeLoadIndex), row+1);
                clipsItemModel->setData(clipsItemModel->index(row, changedIndex), "yes");
            }
        }

        emit clipsChangedToVideo(model());
        emit clipsChangedToTimeline(clipsProxyModel);
    }
}

void AClipsTableView::onReloadClips()
{
//    qDebug()<<"AClipsTableView::onReloadClips";
    loadModel(selectedFolderName);
}

void AClipsTableView::onTrimF(QString pfileName)
{
    qDebug()<<"AClipsTableView::onTrim"<< pfileName;
    if (checkSaveIfClipsChanged())
    {
        saveModels();
    }

//    qDebug()<<"AClipsTableView::onTrim saved"<< pfileName;

    for (int row = 0; row < clipsItemModel->rowCount(); row++) // for each clip of file
    {
        QString fileName = clipsItemModel->index(row,fileIndex).data().toString();

        if (fileName == pfileName)
        {
            qDebug()<<"AClipsTableView::onTrim file"<<row<< fileName;
            QTime inTime = QTime::fromString(clipsItemModel->index(row,inIndex).data().toString(),"HH:mm:ss.zzz");
            QTime outTime = QTime::fromString(clipsItemModel->index(row,outIndex).data().toString(),"HH:mm:ss.zzz");

            int deltaIn = qMin(1000, inTime.msecsSinceStartOfDay());
            inTime = inTime.addMSecs(-deltaIn);
            int deltaOut = 1000;//fmin(1000, ui->videoWidget->m_player->duration() - outTime.msecsSinceStartOfDay());
            outTime = outTime.addMSecs(deltaOut);
            int clipDuration = AGlobal().frames_to_msec(AGlobal().msec_to_frames(outTime.msecsSinceStartOfDay()) - AGlobal().msec_to_frames(inTime.msecsSinceStartOfDay()) + 1);

    //                qDebug()<<i<<srtItemModel->index(i,inIndex).data().toString()<<srtItemModel->index(i,outIndex).data().toString();
    //                qDebug()<<"times"<<inTime<< deltaIn<< outTime<< deltaOut<< duration<<ui->videoWidget->m_player->duration();

            QString targetFileName = fileName;
            int lastIndex = targetFileName.lastIndexOf(".");
            if (lastIndex > -1) //if extension exists
                targetFileName = targetFileName.left(lastIndex) + "+" + QString::number(inTime.msecsSinceStartOfDay()) + "ms" + targetFileName.mid(lastIndex);
            else
                targetFileName = targetFileName + "+" + QString::number(inTime.msecsSinceStartOfDay()) + "ms";

            qDebug()<<"AClipsTableView::onTrimC before emit"<< selectedFolderName<<fileName<<targetFileName<<inTime<<outTime;
            emit trimC(selectedFolderName, fileName, targetFileName, inTime, outTime, 10);

//            QString code = "ffmpeg -y -i \"" + QString(selectedFolderName + fileName).replace("/", "//") + "\" -ss " + inTime.toString("HH:mm:ss.zzz") + " -t " + QTime::fromMSecsSinceStartOfDay(duration).toString("hh:mm:ss.zzz") + " -map_metadata 0 -vcodec copy \"" + QString(selectedFolderName + targetFileName).replace("/", "//") + "\"";

//            QMap<QString, QString> parameters;

//            QString *processId = new QString();
//            emit addJobsEntry(selectedFolderName, targetFileName, "Trim", processId);
//            emit addToJob(*processId, code);
//            parameters["processId"] = *processId;

//            processManager->startProcess(code, parameters, [](QWidget *parent, QMap<QString, QString> parameters, QString result)
//            {
//                AClipsTableView *clipsTableView = qobject_cast<AClipsTableView *>(parent);
//                emit clipsTableView->addToJob(parameters["processId"], result);
//            }, [] (QWidget *parent, QString command, QMap<QString, QString> parameters, QStringList result)
//            {
//                AClipsTableView *clipsTableView = qobject_cast<AClipsTableView *>(parent);
//                qDebug()<<"trim"<<parent <<command;
//    //            emit clipsTableView->addToJob("trim ", result.join("\n"));
//                emit clipsTableView->addToJob(parameters["processId"], "Completed");
//            });

            qDebug()<<"AClipsTableView::onTrim before propertyUpdate"<< selectedFolderName<<fileName<<targetFileName;
            emit propertyUpdate(selectedFolderName, fileName, targetFileName);

//            emit addJobsEntry(selectedFolderName, targetFileName, "Property update", processId);

//            QString attributeString = "";
//            QStringList texts;
//            texts << "CreateDate" << "GPSLongitude" << "GPSLatitude" << "GPSAltitude" << "GPSAltitudeRef" << "Make" << "Model" << "Director" << "Producer"  << "Publisher";
//            for (int iText = 0; iText < texts.count(); iText++)
//            {
//                QString *value = new QString();
//                emit getPropertyValue(fileName, texts[iText], value);

//                attributeString += " -" + texts[iText] + "=\"" + *value + "\"";
//            }

//            QString command = "exiftool" + attributeString + " -overwrite_original \"" + QString(selectedFolderName + targetFileName).replace("/", "//") + "\"";
//            emit addToJob(*processId, command + "\n");
//            parameters["processId"] = *processId;
//            processManager->startProcess(command, parameters, [](QWidget *parent, QMap<QString, QString> parameters, QString result)
//            {
//                AClipsTableView *clipsTableView = qobject_cast<AClipsTableView *>(parent);
//                emit clipsTableView->addToJob(parameters["processId"], result);
//            }, [] (QWidget *parent, QString, QMap<QString, QString> parameters, QStringList result)
//            {
//                AClipsTableView *clipsTableView = qobject_cast<AClipsTableView *>(parent);
//                emit clipsTableView->addToJob(parameters["processId"], "Completed");
//            });

            //create srt file
            QString fileName;
            lastIndex = targetFileName.lastIndexOf(".");
            if (lastIndex > -1)
                fileName = targetFileName.left(lastIndex) + ".srt";

            QFile file(selectedFolderName + fileName);
            if ( file.open(QIODevice::WriteOnly) )
            {
                QTextStream stream( &file );
                AStarRating starRating = qvariant_cast<AStarRating>(clipsItemModel->index(row, ratingIndex).data());

                int order = clipsItemModel->index(row, orderAfterMovingIndex).data().toInt();

                QString srtContentString = "";
                srtContentString += "<o>" + QString::number(order + 1) + "</o>"; //+1 for file trim, +2 for export
                srtContentString += "<r>" + QString::number(starRating.starCount()) + "</r>";
                srtContentString += "<a>" + clipsItemModel->index(row, alikeIndex).data().toString() + "</a>";
                srtContentString += "<h>" + clipsItemModel->index(row, hintIndex).data().toString() + "</h>";
                srtContentString += "<t>" + clipsItemModel->index(row, tagIndex).data().toString() + "</t>";

                stream << 1 << endl;
                stream << QTime::fromMSecsSinceStartOfDay(deltaIn).toString("HH:mm:ss.zzz") << " --> " << QTime::fromMSecsSinceStartOfDay(clipDuration - deltaOut -  AGlobal().frames_to_msec(1)).toString("HH:mm:ss.zzz") << endl;
                stream << srtContentString << endl;
                stream << endl;

                file.close();
            }
        }
    } //for each clip of file

    qDebug()<<"AClipsTableView::onTrimF"<<"ReloadAll";
    emit reloadAll(true);

} //onTrim

void AClipsTableView::onPropertiesLoaded()
{
    bool clipsFramerateExistsInClips = false;

    //set duration and fps to each clip
    QMap<int, QString> fpsMap;
    for (int row=0; row<clipsItemModel->rowCount();row++)
    {
        QString *frameratePointer = new QString();
        emit getPropertyValue(clipsItemModel->index(row, fileIndex).data().toString(), "VideoFrameRate", frameratePointer);
//        qDebug()<<"AClipsTableView::onPropertiesLoaded"<<*frameratePointer<<(*frameratePointer).toDouble();
        clipsItemModel->setData(clipsItemModel->index(row, fpsIndex), *frameratePointer);

        if (qRound((*frameratePointer).toDouble()) == QSettings().value("frameRate").toInt())
            clipsFramerateExistsInClips = true;
        else
        {
            int fpsSuggested = -1;
            fpsSuggested = qRound((*frameratePointer).toDouble());
            fpsMap[fpsSuggested] = QString::number(fpsSuggested);
        }

        QString *durationPointer = new QString();
        emit getPropertyValue(clipsItemModel->index(row, fileIndex).data().toString(), "Duration", durationPointer);
        *durationPointer = QString(*durationPointer).replace(" (approx)", "");
        QTime fileDurationTime = QTime::fromString(*durationPointer,"h:mm:ss");
        if (fileDurationTime == QTime())
        {
            QString durationString = *durationPointer;
            durationString = durationString.left(durationString.length() - 2); //remove " s"
            fileDurationTime = QTime::fromMSecsSinceStartOfDay(int(durationString.toDouble() * 1000.0));
        }

        if (fileDurationTime.msecsSinceStartOfDay() == 0)
            fileDurationTime = QTime::fromMSecsSinceStartOfDay(24 * 60 * 60 * 1000 - 1);

        clipsItemModel->setData(clipsItemModel->index(row, fileDurationIndex), fileDurationTime.toString("HH:mm:ss.zzz"));
    }

//    qDebug()<<"AClipsTableView::onPropertiesLoaded"<<fpsSuggested<<clipsFramerateExistsInClips;

//    if (!clipsFramerateExistsInClips && fpsMap.count() > 0 && false)
//    {
//        QMessageBox messageBox;
////        messageBox.setParent(this); //then messagebox disappears

//        QMapIterator<int, QString> fpsIterator(fpsMap);
//        QList<int> fpsList;
//        while (fpsIterator.hasNext()) //all files in reverse order
//        {
//            fpsIterator.next();
//            fpsList<<fpsIterator.key();
//            messageBox.addButton(fpsIterator.value(), QMessageBox::ActionRole);
//        }

//        messageBox.addButton(QMessageBox::No);

//        messageBox.setWindowTitle( "Loading clips");
//        messageBox.setText("There are no clips with the frame rate used (" + QSettings().value("frameRate").toString() + "). Do you want to update to one of below founded frame rates ?");
//        int result = messageBox.exec();

////                   QMessageBox::StandardButton reply;

////         reply = QMessageBox::question(this, "Loading clips", "There are no clips with the frame rate used (" + QSettings().value("frameRate").toString() + "). Do you want to update to " + QString::number(fpsSuggested) + " ?",
////                                       QMessageBox::Yes|QMessageBox::No);
////         reply = messageBox.question(this, "Loading clips", "There are no clips with the frame rate used (" + QSettings().value("frameRate").toString() + "). Do you want to update to " + QString::number(fpsSuggested) + " ?",
////                             QMessageBox::Yes|QMessageBox::No);

//        if (result != QMessageBox::No)
//        {
//            emit frameRateChanged(fpsList[result]);
////            QSettings().setValue("frameRate",fpsSuggested);
//        }
//    }
    emit propertiesLoaded();
}

void AClipsTableView::onClipDelete()
{
    QModelIndexList indexList = selectionModel()->selectedIndexes();
//    qDebug()<<"AClipsTableView::onClipDelete"<<selectionModel()<<indexList.count()<<indexList.first().data().toString();

    QMap<int, int> rowMap; //sorted by fileName and inTime
    for (int i=0; i < indexList.count() ;i++)
    {
        QModelIndex currentModelIndex = indexList[i];
        if (currentModelIndex.row() >= 0 && currentModelIndex.column() >= 0)
        {
            rowMap[currentModelIndex.row()] = clipsProxyModel->index(currentModelIndex.row(), orderAtLoadIndex).data().toInt();
//            qDebug()<<"  AClipsTableView::onClipDelete"<<currentModelIndex.row()<<currentModelIndex.data().toString();
        }
    }

    for (int row = 0; row < clipsItemModel->rowCount(); row++)
    {
        QMapIterator<int, int> rowIterator(rowMap);
        while (rowIterator.hasNext()) //all files in reverse order
        {
            rowIterator.next();

//            qDebug()<<"  AClipsTableView::onClipDelete"<<row<<rowIterator.key()<<rowIterator.value()<<clipsItemModel->index(row, orderAtLoadIndex).data().toInt();
            if (clipsItemModel->index(row, orderAtLoadIndex).data().toInt() == rowIterator.value())
            {
                clipsItemModel->takeRow(row);
                nrOfDeletedItems++;
            }
        }
    }

    //set orderBeforeLoadIndex correct
    for (int row = 0; row<clipsItemModel->rowCount();row++)
    {
        int order = clipsItemModel->index(row, orderBeforeLoadIndex).data().toInt();
        if (order != row+1)
        {
            clipsItemModel->setData(clipsItemModel->index(row, orderBeforeLoadIndex), row+1);
            clipsItemModel->setData(clipsItemModel->index(row, changedIndex), "yes");
        }
        else
            clipsItemModel->setData(clipsItemModel->index(row, changedIndex), "yes"); //set to yes to make sure delete is saved!
    }

    emit clipsChangedToVideo(model());
    emit clipsChangedToTimeline(clipsProxyModel);
} //onClipDelete

void AClipsTableView::onVideoPositionChanged(int progress, int row, int ) //relativeProgress
{
    position = progress;

    if (progress != 0)
    {

        int foundRow = -1;
        for (int i = 0; i < model()->rowCount(); i++)
        {
            if (model()->index(i,orderBeforeLoadIndex).data().toInt() == row)
            {
                foundRow = i;
            }
        }
//        qDebug()<<"AClipsTableView::onVideoPositionChanged: " << progress<<row<<foundRow;

        if (foundRow != -1)
        {
            if (highLightedRow != foundRow) // if not already selected
            {
                clearSelection();
                setSelectionMode(QAbstractItemView::MultiSelection);
                for (int col = 0; col < model()->columnCount(); col++)
                {
//                    qDebug()<<"setCurrentIndex3"<<foundRow<<model()->index(foundRow, col).data();
                    setCurrentIndex(model()->index(foundRow,col));
                }
                setSelectionMode(QAbstractItemView::ContiguousSelection);
                highLightedRow = foundRow;
            }
        }
        else
        {
            clearSelection();
            highLightedRow = -1;
        }
    }
}

void AClipsTableView::onScrubberInChanged(QString AV, int row, int in)
{
    QTime newInTime = QTime::fromMSecsSinceStartOfDay(AGlobal().msec_rounded_to_fps(in));

    if (!doNotUpdate)
    {
        for (int i=0;i<clipsItemModel->rowCount();i++)
        {
            int clipsCounter = clipsItemModel->index(i,orderBeforeLoadIndex).data().toInt();
            if (clipsCounter == row)
            {
                QTime outTime = QTime::fromString(clipsItemModel->index(i,outIndex).data().toString(),"HH:mm:ss.zzz");
    //            qDebug()<<"AClipsTableView::onScrubberInChanged"<<i<< in<<newInTime.msecsSinceStartOfDay();
                doNotUpdate = true; //avoid onsetin trigger which fires also scrubberchanged
                clipsItemModel->item(i, inIndex)->setData(newInTime.toString("HH:mm:ss.zzz"),Qt::EditRole);
                clipsItemModel->item(i, durationIndex)->setData(QTime::fromMSecsSinceStartOfDay(AGlobal().frames_to_msec(AGlobal().msec_to_frames(outTime.msecsSinceStartOfDay()) - AGlobal().msec_to_frames(newInTime.msecsSinceStartOfDay()) + 1)).toString("HH:mm:ss.zzz"),Qt::EditRole);
                clipsItemModel->item(i, changedIndex)->setData("yes",Qt::EditRole);
                doNotUpdate = false;
                emit clipsChangedToTimeline(clipsProxyModel); //not to video because video initiated it
            }
        }
    }
}

void AClipsTableView::onScrubberOutChanged(QString AV, int row, int out)
{
    QTime newOutTime = QTime::fromMSecsSinceStartOfDay(AGlobal().msec_rounded_to_fps(out));

    if (!doNotUpdate)
    {
        for (int i=0;i<clipsItemModel->rowCount();i++)
        {
            int clipCounter = clipsItemModel->index(i,orderBeforeLoadIndex).data().toInt();
            if (clipCounter == row)
            {
                QTime inTime = QTime::fromString(clipsItemModel->index(i,inIndex).data().toString(),"HH:mm:ss.zzz");
    //            qDebug()<<"AClipsTableView::onScrubberOutChanged"<<i<< out<<newOutTime.msecsSinceStartOfDay()<<QSettings().value("frameRate");
                doNotUpdate = true; //avoid onsetin trigger which fires also scrubberchanged
                clipsItemModel->item(i, changedIndex)->setData("yes",Qt::EditRole);
                clipsItemModel->item(i, outIndex)->setData(newOutTime.toString("HH:mm:ss.zzz"),Qt::EditRole);
                clipsItemModel->item(i, durationIndex)->setData(QTime::fromMSecsSinceStartOfDay(AGlobal().frames_to_msec(AGlobal().msec_to_frames(newOutTime.msecsSinceStartOfDay()) - AGlobal().msec_to_frames(inTime.msecsSinceStartOfDay()) + 1)).toString("HH:mm:ss.zzz"),Qt::EditRole);
                doNotUpdate = false;
                emit clipsChangedToTimeline(clipsProxyModel); //not to video because video initiated it
            }
        }
    }
}

void AClipsTableView::onDataChanged(const QModelIndex &topLeft, const QModelIndex &) //bottomRight
{
//    qDebug()<<"AClipsTableView::onDataChanged"<<topLeft.column()<<topLeft.row()<<bottomRight.column()<<bottomRight.row()<<doNotUpdate;
    if (!doNotUpdate)
    {
        if (topLeft.column() == inIndex || topLeft.column() == outIndex || topLeft.column() == durationIndex)
        {
            QTime time = QTime::fromString(topLeft.data().toString(),"HH:mm:ss.zzz");
            QTime inTime = QTime::fromString(clipsItemModel->index(topLeft.row(),inIndex).data().toString(), "HH:mm:ss.zzz");
            QTime outTime = QTime::fromString(clipsItemModel->index(topLeft.row(),outIndex).data().toString(), "HH:mm:ss.zzz");
//            qDebug()<<"AClipsTableView::onDataChanged"<<topLeft.column()<<time.msecsSinceStartOfDay()<<inTime.msecsSinceStartOfDay()<<outTime.msecsSinceStartOfDay();

            if (topLeft.column() == inIndex)
            {
                doNotUpdate = true;
                clipsItemModel->item(topLeft.row(), durationIndex)->setData(QTime::fromMSecsSinceStartOfDay(AGlobal().frames_to_msec(AGlobal().msec_to_frames(outTime.msecsSinceStartOfDay()) - AGlobal().msec_to_frames(time.msecsSinceStartOfDay()) + 1)).toString("HH:mm:ss.zzz"),Qt::DisplayRole);
                emit setIn(AGlobal().msec_to_frames(time.msecsSinceStartOfDay()));
            }
            else if (topLeft.column() == outIndex)
            {
                doNotUpdate = true;
                clipsItemModel->item(topLeft.row(), durationIndex)->setData(QTime::fromMSecsSinceStartOfDay(AGlobal().frames_to_msec(AGlobal().msec_to_frames(time.msecsSinceStartOfDay()) - AGlobal().msec_to_frames(inTime.msecsSinceStartOfDay()) + 1)).toString("HH:mm:ss.zzz"),Qt::DisplayRole);
                emit setOut(AGlobal().msec_to_frames(time.msecsSinceStartOfDay()));
            }
            else if (topLeft.column() == durationIndex)
            {
                int duration = AGlobal().frames_to_msec(AGlobal().msec_to_frames(outTime.msecsSinceStartOfDay()) - AGlobal().msec_to_frames(inTime.msecsSinceStartOfDay()) + 1);
                int delta = time.msecsSinceStartOfDay() - duration;
                int newIn = inTime.addMSecs(-delta/2).msecsSinceStartOfDay();
                int newOut = outTime.addMSecs(delta/2).msecsSinceStartOfDay();

    //            qDebug()<<"AClipsTableView::onDataChanged"<<topLeft.column()<<time.msecsSinceStartOfDay()<<inTime.msecsSinceStartOfDay()<<newIn<<outTime.msecsSinceStartOfDay()<<newOut;

                doNotUpdate = true;
                if (newIn != inTime.msecsSinceStartOfDay())
                {
                    clipsItemModel->item(topLeft.row(), inIndex)->setData(QTime::fromMSecsSinceStartOfDay(newIn).toString("HH:mm:ss.zzz"),Qt::DisplayRole);
                    emit setIn(AGlobal().msec_to_frames(newIn));
                }
                if (newOut != outTime.msecsSinceStartOfDay())
                {
                    clipsItemModel->item(topLeft.row(), outIndex)->setData(QTime::fromMSecsSinceStartOfDay(newOut).toString("HH:mm:ss.zzz"),Qt::DisplayRole);
                    emit setOut(AGlobal().msec_to_frames(newOut));
                }
            }
            if (doNotUpdate)
            {
                emit clipsChangedToTimeline(clipsProxyModel); //not to video because video done using emit update in/out
                doNotUpdate = false;
            }
        }
    }
//    qDebug()<<"AClipsTableView::onDataChanged done"<<topLeft.column();
}

QStandardItemModel* AClipsTableView::read(QString folderName, QString fileName)
{
//    qDebug()<<"AClipsTableView::read"<<folderName << fileName;
    QString srtFileName;
//    fileName = QString(mediaFilePath.toString()).replace(".mp4",".srt").replace(".jpg",".srt").replace(".avi",".srt").replace(".wmv",".srt");
//    fileName.replace(".MP4",".srt").replace(".JPG",".srt").replace(".AVI",".srt").replace(".WMV",".srt");
    int lastIndex = fileName.lastIndexOf(".");
    if (lastIndex > -1)
        srtFileName = folderName + fileName.left(lastIndex) + ".srt";

    QStandardItemModel *srtItemModel = new QStandardItemModel(this);
    QStringList labels;
    labels << "Path"<< "File"<<"In"<<"Out"<<"Duration"<<"Order"<<"Rating"<<"Tags";
    srtItemModel->setHorizontalHeaderLabels(labels);

    QStringList list;
    QFile file(srtFileName);
    if(file.open(QIODevice::ReadOnly))
    {
        QTextStream in(&file);
        while (!in.atEnd())
           {
              QString line = in.readLine();
              list << line;
           }

        file.close();
    }

//    bool tagsContainSemiColon = false;
    for (int i = 0; i< list.count(); i++)
    {
        QString line = list[i];
        int pos = line.indexOf("-->");
        if (pos >= 0)
        {
//            qDebug()<<line;
            line.replace(",",".");

            QTime inTime = QTime::fromString(line.left(12),"HH:mm:ss.zzz");
            QTime outTime = QTime::fromString(line.mid(17,12),"HH:mm:ss.zzz");

//            STimeSpinBox *inSpin = new STimeSpinBox();
//            inSpin->setValue(inTime.msecsSinceStartOfDay());
//            QStandardItem *inSpinItem = new QStandardItem;
//            inSpinItem->setData(QVariant::fromValue(inSpin));

            QString srtContentString = list[i+1];
            int start;
            int end;
            QString value;

            start = srtContentString.indexOf("<o>");
            end = srtContentString.indexOf("</o>");
            if (start >= 0 && end >= 0)
                value = srtContentString.mid(start+3, end - start - 3);
            else
                value = "";
            QString order = value;

            start = srtContentString.indexOf("<r>");
            end = srtContentString.indexOf("</r>");
            if (start >= 0 && end >= 0)
                value = srtContentString.mid(start+3, end - start - 3);
            else
                value = "";
            QStandardItem *starItem = new QStandardItem;
            starItem->setData(QVariant::fromValue(AStarRating(value.toInt())), Qt::EditRole);

            start = srtContentString.indexOf("<a>");
            end = srtContentString.indexOf("</a>");
            if (start >= 0 && end >= 0)
                value = srtContentString.mid(start+3, end - start - 3);
            else
                value = "";
            QString alike = value;

            start = srtContentString.indexOf("<h>");
            end = srtContentString.indexOf("</h>");
            if (start >= 0 && end >= 0)
                value = srtContentString.mid(start+3, end - start - 3);
            else
                value = "";
            QString hint = value;

            start = srtContentString.indexOf("<t>");
            end = srtContentString.indexOf("</t>");
            if (start >= 0 && end >= 0)
            {
                value = srtContentString.mid(start+3, end - start - 3);
//                tagsContainSemiColon = tagsContainSemiColon || value.contains(";");
//                if (!tagsContainSemiColon)
//                    value = value.replace(" ",";");
            }
            else
                value = "";
            QString tags = value;

            if (start == -1 || end == -1) //backwards compatibility
            {
                tags = srtContentString;

                order = QString::number(clipCounter*10);

                QVariant starVar = QVariant::fromValue(AStarRating(0));
                if (tags.indexOf("r9") >= 0)
                    starVar = QVariant::fromValue(AStarRating(5));
                else if (tags.indexOf("r8") >= 0)
                    starVar = QVariant::fromValue(AStarRating(4));
                else if (tags.indexOf("r7") >= 0)
                    starVar = QVariant::fromValue(AStarRating(3));
                else if (tags.indexOf("r6") >= 0)
                    starVar = QVariant::fromValue(AStarRating(2));
                else if (tags.indexOf("r5") >= 0)
                    starVar = QVariant::fromValue(AStarRating(1));
                starItem->setData(starVar, Qt::EditRole);
                tags.replace(" r5", "").replace("r5","");
                tags.replace(" r6", "").replace("r6","");
                tags.replace(" r7", "").replace("r7","");
                tags.replace(" r8", "").replace("r8","");
                tags.replace(" r9", "").replace("r9","");
                tags.replace(" ", ";");
            }

            QStandardItem *alikeItem = new QStandardItem(alike);
            alikeItem->setTextAlignment(Qt::AlignCenter); //tbd: not working ...

            int duration = AGlobal().frames_to_msec(AGlobal().msec_to_frames(outTime.msecsSinceStartOfDay()) - AGlobal().msec_to_frames(inTime.msecsSinceStartOfDay()) + 1);
            QList<QStandardItem *> items;
            items.append(new QStandardItem(QString::number(clipCounter)));
            items.append(new QStandardItem(order));
            items.append(new QStandardItem(order));
            items.append(new QStandardItem("no"));
            items.append(new QStandardItem(folderName));
            items.append(new QStandardItem(fileName));
            items.append(new QStandardItem("")); //fps
            items.append(new QStandardItem("")); //fdur
            items.append(new QStandardItem(inTime.toString("HH:mm:ss.zzz")));
            items.append(new QStandardItem(outTime.toString("HH:mm:ss.zzz")));
            items.append(new QStandardItem(QTime::fromMSecsSinceStartOfDay(duration).toString("HH:mm:ss.zzz"))); //durationIndex
            items.append(starItem);
            items.append(alikeItem);
            items.append(new QStandardItem(hint));
            QStandardItem *tagItem = new QStandardItem(tags);
//            tagItem->setFlags(Qt::ItemIsDropEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsSelectable);
//            tagItem->setFlags(tagItem->flags() &~Qt::ItemIsEditable);
            items.append(tagItem);
            srtItemModel->appendRow(items);
//                QLineEdit *edit = new QLineEdit(this);
            clipCounter++;
            originalDuration += AGlobal().msec_to_frames(duration);
        }
    }

    return srtItemModel;
} //read

void AClipsTableView::scanDir(QDir dir, QStringList extensionList)
{
    if (!continueLoading)
        return;

    dir.setFilter(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::NoSymLinks);
    dir.setSorting(QDir::Name | QDir::LocaleAware); //localeaware to get +99999ms sorted the right way
    QStringList dirList = dir.entryList();
    for (int i=0; i<dirList.size(); ++i)
    {
        QString newPath = QString("%1/%2").arg(dir.absolutePath()).arg(dirList.at(i));
        scanDir(QDir(newPath), extensionList);
    }

//    filters << "*.mp4"<<"*.jpg"<<"*.avi"<<"*.wmv"<<"*.mts";

    dir.setNameFilters(extensionList);
    dir.setFilter(QDir::Files | QDir::NoDotAndDotDot | QDir::NoSymLinks);

//    qDebug() << "AClipsTableView::scanDir" << dir.path();

    QStringList fileList = dir.entryList();
    for (int i=0; i<fileList.count(); i++)
    {
        QStandardItemModel *srtItemModel = read(dir.path() + "/", fileList[i]);

        if (srtItemModel->rowCount()>0)
            fileCounter++;

        if (fileCounter > 10 && fileCounter%100==0)
        {
            QMessageBox::StandardButton reply;
             reply = QMessageBox::question(this, "Loading files", "Are you sure you want to continue (" + QString::number(fileCounter) + " files with clips counted in "  + selectedFolderName + ")?",
                                           QMessageBox::Yes|QMessageBox::No);

            if (reply == QMessageBox::No)
            {
                continueLoading = false;
                return;
            }
        }

        QList<QStandardItem *> items;
        items.append(new QStandardItem(dir.path() + "/"));
        items.append(new QStandardItem(fileList[i]));
        srtFileItemModel->appendRow(items);

        bool continueLoadingClips = true;
        if (srtItemModel->rowCount()>100)
        {
            QMessageBox::StandardButton reply;
             reply = QMessageBox::question(this, "Loading clips", "Are you sure you want to add " + QString::number(srtItemModel->rowCount()) + " clips from " + dir.path() + fileList[i] + "?",
                                           QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel);

            if (reply == QMessageBox::No)
                continueLoadingClips = false;
            if (reply == QMessageBox::Cancel)
            {
                continueLoading = false;
                return;
            }
        }

        if (continueLoadingClips)
            while (srtItemModel->rowCount()>0)
                clipsItemModel->appendRow(srtItemModel->takeRow(0));
    }
}

void AClipsTableView::loadModel(QString folderName)
{
    qDebug() << "AClipsTableView::loadModel" << folderName;
    if (checkSaveIfClipsChanged())
    {
        saveModels();
    }

    qDebug() << "AClipsTableView::loadModel check done" << folderName;

    clipCounter = 1;
    fileCounter = 0;
    originalDuration = 0;
    continueLoading = true;
    clipsItemModel->removeRows(0, clipsItemModel->rowCount());
    srtFileItemModel->removeRows(0,srtFileItemModel->rowCount());

    scanDir(QDir(folderName), QStringList() << "*.MP4"<<"*.JPG"<<"*.AVI"<<"*.WMV"<<"*.MTS");
    scanDir(QDir(folderName), QStringList() << "*.mp3");

//    qDebug() << "AClipsTableView::loadModel done" << folderName;
}

bool AClipsTableView::checkSaveIfClipsChanged()
{
    int changeCount = 0;
    for (int row=0; row<clipsItemModel->rowCount();row++)
    {
        if (clipsItemModel->index(row, changedIndex).data().toString() == "yes")
            changeCount++;
    }

    if (changeCount > 0 || nrOfDeletedItems > 0)
    {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Check changes", tr("There are %1 clips with changes and %2 clips deleted. Do you want to save these changes?").arg(QString::number(changeCount), QString::number(nrOfDeletedItems)),
                                      QMessageBox::Yes|QMessageBox::No);

        qDebug() << "AClipsTableView::checkSaveIfClipsChanged answer" ;

        if (reply == QMessageBox::Yes)
            return true;
    }
    return false;
}

void AClipsTableView::saveModels()
{
    qDebug() << "AClipsTableView::saveModels" << srtFileItemModel->rowCount();

    onSectionMoved(-1,-1,-1); //to reorder the items

    for (int row = 0; row < srtFileItemModel->rowCount(); row++) //go through all srt files
     {
         QString folderName = srtFileItemModel->index(row, 0).data().toString();
         QString fileName = srtFileItemModel->index(row, 1).data().toString();
//            qDebug()<<"MainWindow::on_actionSave_triggered"<<ui->clipsTableView->srtFileItemModel->rowCount()<<row<<folderName<<fileName;
         saveModel(folderName, fileName);
     }

    nrOfDeletedItems = 0;
}

void AClipsTableView::saveModel(QString folderName, QString fileName)
{
    int changeCount = 0;
    QMap<QString, int> inMap; //sorted by fileName and inTime
    for (int row = 0; row < clipsItemModel->rowCount();row++)
    {
        if (clipsItemModel->index(row, folderIndex).data().toString() == folderName && clipsItemModel->index(row, fileIndex).data().toString() == fileName)
        {
            QString inTime = clipsItemModel->index(row, inIndex).data().toString();
            inMap[inTime] = row;
            if (clipsItemModel->index(row, changedIndex).data().toString() == "yes")
                changeCount++;
        }
    }

    int lastIndex = fileName.lastIndexOf(".");
    QString srtFileName = fileName.left(lastIndex) + ".srt";
    QFile file;
    file.setFileName(folderName + srtFileName);

    if (inMap.count() > 0) //clips exists with changes
    {
//        qDebug()<<"AClipsTableView::saveModel"<<clipsItemModel->rowCount()<<fileName<<changeCount<<inMap.count();

        if (changeCount > 0) //changes in this file
        {
            int clipPerFileCounter = 1;

//            qDebug()<<"AClipsTableView::saveModel"<<srtFileName;

            file.open(QIODevice::WriteOnly);

            QMapIterator<QString, int> inIterator(inMap);
            while (inIterator.hasNext()) //all files
            {
                inIterator.next();

                int row = inIterator.value();

                QTextStream stream(&file);

                AStarRating starRating = qvariant_cast<AStarRating>(clipsItemModel->index(row, ratingIndex).data());

                QString srtContentString = "";
                srtContentString += "<o>" + clipsItemModel->index(row, orderAfterMovingIndex).data().toString() + "</o>";
                srtContentString += "<r>" + QString::number(starRating.starCount()) + "</r>";
                srtContentString += "<a>" + clipsItemModel->index(row, alikeIndex).data().toString() + "</a>";
                srtContentString += "<h>" + clipsItemModel->index(row, hintIndex).data().toString() + "</h>";
                srtContentString += "<t>" + clipsItemModel->index(row, tagIndex).data().toString() + "</t>";

                stream << clipPerFileCounter << endl;
                stream << clipsItemModel->index(row,inIndex).data().toString() << " --> " << clipsItemModel->index(row,outIndex).data().toString() << endl;
                stream << srtContentString << endl;
                stream << endl;
                clipPerFileCounter++;

                clipsItemModel->setData(clipsItemModel->index(row, changedIndex), "no");
            }
            file.close();
        }

    }
    else
    {
        if (file.exists())
           file.remove();
    }
}

void AClipsTableView::onClipsFilterChanged(QComboBox *ratingFilterComboBox, QCheckBox *alikeCheckBox, QListView *tagFilter1ListView, QListView *tagFilter2ListView, QCheckBox *fileOnlyCheckBox )
{
//    qDebug()<<"AClipsTableView::onClipsFilterChanged"<<ratingFilterComboBox->currentText()<<tagFilter1ListView->model()->rowCount()<<tagFilter2ListView->model()->rowCount();
    QString string1 = "";
    QString sep = "";
    for (int i=0; i < tagFilter1ListView->model()->rowCount();i++)
    {
        string1 += sep + tagFilter1ListView->model()->index(i,0).data().toString();
        sep = ";";
    }

    QString string2 = "";
    sep = "";
    for (int i=0; i < tagFilter2ListView->model()->rowCount();i++)
    {
        string2 += sep + tagFilter2ListView->model()->index(i,0).data().toString();
        sep = ";";
    }

    QVariant alikeVariant = alikeCheckBox->isChecked();

    QString fileName = "";
    if (fileOnlyCheckBox->isChecked())
        fileName = selectedFileName;

    QString starCount = QString::number(ratingFilterComboBox->currentIndex());

    //    qDebug()<<"AClipsTableView::onClipsFilterChanged"<<starEditorFilterWidget->starRating().starCount()<<tagFilter1ListView->model()->rowCount()<<tagFilter2ListView->model()->rowCount()<<regExp;

    QString regExp = starCount + "|" + alikeVariant.toString() + "|" + string1 + "|" + string2 + "|" + fileName;
    clipsProxyModel->setFilterRegExp(QRegExp(regExp, Qt::CaseInsensitive,
                                                QRegExp::FixedString));
    clipsProxyModel->setFilterKeyColumn(-1);

//    qDebug()<<"AClipsTableView::onClipsFilterChanged"<<starEditorFilterWidget->starRating().starCount();

    emit clipsChangedToVideo(model());
    emit clipsChangedToTimeline(clipsProxyModel);
}

void AClipsTableView::giveStars(int starCount)
{
//    qDebug()<<"AClipsTableView::onGiveStars"<<starCount<<currentIndex().data().toString();
    clipsProxyModel->setData(clipsProxyModel->index(currentIndex().row(), ratingIndex), QVariant::fromValue(AStarRating(starCount)));
    clipsProxyModel->setData(clipsProxyModel->index(currentIndex().row(), changedIndex), "yes");
}

void AClipsTableView::toggleAlike()
{
    clipsProxyModel->setData(clipsProxyModel->index(currentIndex().row(), alikeIndex), !clipsProxyModel->index(currentIndex().row(), alikeIndex).data().toBool());
    clipsProxyModel->setData(clipsProxyModel->index(currentIndex().row(), changedIndex), "yes");
}
