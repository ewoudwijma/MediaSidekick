#include "aclipsitemdelegate.h"
#include "aclipstableview.h"
#include "fstarrating.h"
#include "stimespinbox.h"

#include <QApplication>
#include <QHeaderView>
#include <QMenu>
#include <QMessageBox>
#include <QSettings>
#include <QTextStream>

#include <QDebug>

#include "fglobal.h"

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

//    setEditTriggers(QAbstractItemView::AnyKeyPressed | QAbstractItemView::SelectedClicked);
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
//    qDebug()<<"AClipsTableView::onFileIndexClicked"<<index.data().toString();

    selectedFolderName = QSettings().value("LastFileFolder").toString();
    selectedFileName = index.data().toString();

    selectClips();

    emit fileIndexClicked(index, selectedIndices);
}

void AClipsTableView::selectClips()
{
//    clearSelection();
//    setSelectionMode(QAbstractItemView::MultiSelection);

    doNotUpdate = true; //to avoid datachanged trigger
    int firstRow = -1;
    for (int row=0; row < model()->rowCount(); row++)
    {
        QModelIndex clipsFileIndex = model()->index(row, fileIndex);
//        QModelIndex clipsInIndex = model()->index(row, inIndex);

        QColor backgroundColor;
        if (clipsFileIndex.data().toString() == selectedFileName)
        {
            if (firstRow == -1)
                firstRow = row;
//            qDebug()<<"selectClips"<<selectedFileName<<clipsFileIndex.data();
//            setCurrentIndex(clipsInIndex); //does also the scrollTo
            backgroundColor = qApp->palette().color(QPalette::AlternateBase);
//            emit clipAdded(clipInIndex);
        }
        else {
            backgroundColor = qApp->palette().color(QPalette::Base);
        }

        for (int col=0;col<model()->columnCount();col++)
        {
            QModelIndex index = model()->index(row, col);
            model()->setData(index, backgroundColor, Qt::BackgroundRole);
        }
//        scrollTo(model()->index(firstRow, inIndex));
    }
    doNotUpdate = false;
}

void AClipsTableView::addClip()
{
    if (selectedFileName == "")
    {
        QMessageBox::information(this, "Add clip", tr("No file selected"));
        return;
    }

    QTime newInTime = QTime::fromMSecsSinceStartOfDay(position);
    QTime newOutTime = QTime::fromMSecsSinceStartOfDay(position + 3000 - FGlobal().frames_to_msec(1));

    int rowToAppendBefore = -1;
    int fileRow = -1;
    int maxOrderAtLoad = 0;
    int maxOrderAfterMoving = 0;
    for (int row=0; row<clipsItemModel->rowCount();row++)
    {
        QString fileName = clipsItemModel->index(row, fileIndex).data().toString();
        QTime inTime = QTime::fromString(clipsItemModel->index(row,inIndex).data().toString(), "HH:mm:ss.zzz");
        QTime outTime = QTime::fromString(clipsItemModel->index(row,outIndex).data().toString(), "HH:mm:ss.zzz");
        QTime fileDuration = QTime::fromString(clipsItemModel->index(row, fileDurationIndex).data().toString(), "HH:mm:ss.zzz");

        //check overlapping, remove overlap from this clip if this is the case
        if (fileName == selectedFileName && inTime.msecsTo(newOutTime) >= 0 && outTime.msecsTo(newInTime) <= 0) //intime before newouttime and outtime after newintime
        {
            qDebug()<<"addclip"<<row<<"overlapping"<<inTime<<outTime<<newInTime<<newOutTime;
            if (outTime.msecsTo(newOutTime) <= 0) //outtime after newouttime
            {
//                qDebug()<<"addclip"<<row<<"adjust outtime";
                newOutTime = inTime.addMSecs( - FGlobal().frames_to_msec(1) );
            }
            if (inTime.msecsTo(newInTime) >= 0) //intime after newintime
            {
//                qDebug()<<"addclip"<<row<<"adjust intime";
                newInTime = outTime.addMSecs( FGlobal().frames_to_msec(1) );
            }
        }

        if (newOutTime.msecsSinceStartOfDay() > fileDuration.msecsSinceStartOfDay())
            newOutTime = fileDuration;

        if (fileName > selectedFileName && fileRow == -1)
            fileRow = row;

        if (selectedFileName == fileName && newInTime.msecsTo(inTime) > 0 && rowToAppendBefore == -1)
            rowToAppendBefore = row;

        maxOrderAtLoad = qMax(maxOrderAtLoad, clipsItemModel->index(row, orderAtLoadIndex).data().toInt());
        maxOrderAfterMoving = qMax(maxOrderAtLoad, clipsItemModel->index(row, orderAfterMovingIndex).data().toInt());
    }

    if (newInTime.msecsTo(newOutTime) <= 0) //if no place for new clip then cancel operation
        return;

    if (rowToAppendBefore == -1)
        rowToAppendBefore = fileRow;

    if (rowToAppendBefore != -1) //move all rows after rowToAppendBefore
    {
        for (int row=rowToAppendBefore; row<clipsItemModel->rowCount();row++)
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
    QVariant starVar = QVariant::fromValue(FStarRating(0));
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

    QString *fpsPointer = new QString();
    emit getPropertyValue(selectedFileName, "VideoFrameRate", fpsPointer);

    QString *durationPointer = new QString();
    emit getPropertyValue(selectedFileName, "Duration", durationPointer);
    QTime fileDurationTime = QTime::fromString(*durationPointer,"h:mm:ss");
    if (fileDurationTime == QTime())
    {
        QString durationString = *durationPointer;
        durationString = durationString.left(durationString.length()-2); //remove " s"
        fileDurationTime = QTime::fromMSecsSinceStartOfDay(int(durationString.toDouble()*1000.0));
    }

    if (fileDurationTime.msecsSinceStartOfDay() == 0)
        fileDurationTime = QTime::fromMSecsSinceStartOfDay(24*60*60*1000 - 1);

    items.append(new QStandardItem("yes"));
    items.append(new QStandardItem(selectedFolderName));
    items.append(new QStandardItem(selectedFileName));
    items.append(new QStandardItem(*fpsPointer));
    items.append(new QStandardItem(fileDurationTime.toString("HH:mm:ss.zzz")));
    items.append(new QStandardItem(newInTime.toString("HH:mm:ss.zzz")));
    items.append(new QStandardItem(newOutTime.toString("HH:mm:ss.zzz")));
    items.append(new QStandardItem(QTime::fromMSecsSinceStartOfDay(FGlobal().frames_to_msec(FGlobal().msec_to_frames(newOutTime.msecsSinceStartOfDay()) - FGlobal().msec_to_frames(newInTime.msecsSinceStartOfDay()) + 1)).toString("HH:mm:ss.zzz"))); //durationIndex
    items.append(starItem);
    items.append(new QStandardItem(""));
    items.append(new QStandardItem(""));
    items.append(new QStandardItem(""));

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

void AClipsTableView::onTrim(QString pfileName)
{
//    qDebug()<<"AClipsTableView::onTrim"<< pfileName;
    if (checkSaveIfClipsChanged())
    {
        onSectionMoved(-1,-1,-1); //to reorder the items

        for (int row =0; row < srtFileItemModel->rowCount();row++)
        {
            saveModel(srtFileItemModel->index(row, 0).data().toString(), srtFileItemModel->index(row, 1).data().toString());
        }
    }

//    qDebug()<<"AClipsTableView::onTrim saved"<< pfileName;

    for (int row = 0; row < clipsItemModel->rowCount(); row++) // for each clip of file
    {
        QString fileName = clipsItemModel->index(row,fileIndex).data().toString();

        if (fileName == pfileName)
        {
            qDebug()<<"AClipsTableView::onTrim saved"<< fileName;
            QTime inTime = QTime::fromString(clipsItemModel->index(row,inIndex).data().toString(),"HH:mm:ss.zzz");
            QTime outTime = QTime::fromString(clipsItemModel->index(row,outIndex).data().toString(),"HH:mm:ss.zzz");

            int deltaIn = qMin(1000, inTime.msecsSinceStartOfDay());
            inTime = inTime.addMSecs(-deltaIn);
            int deltaOut = 1000;//fmin(1000, ui->videoWidget->m_player->duration() - outTime.msecsSinceStartOfDay());
            outTime = outTime.addMSecs(deltaOut);
            int duration = FGlobal().frames_to_msec(FGlobal().msec_to_frames(outTime.msecsSinceStartOfDay()) - FGlobal().msec_to_frames(inTime.msecsSinceStartOfDay()) + 1);

    //                qDebug()<<i<<srtItemModel->index(i,inIndex).data().toString()<<srtItemModel->index(i,outIndex).data().toString();
    //                qDebug()<<"times"<<inTime<< deltaIn<< outTime<< deltaOut<< duration<<ui->videoWidget->m_player->duration();

            QString targetFileName = fileName;
            int lastIndex = targetFileName.lastIndexOf(".");
            if (lastIndex > -1) //if extension exists
                targetFileName = targetFileName.left(lastIndex) + "+" + QString::number(inTime.msecsSinceStartOfDay()) + "ms" + targetFileName.mid(lastIndex);
            else
                targetFileName = targetFileName + "+" + QString::number(inTime.msecsSinceStartOfDay()) + "ms";

            qDebug()<<"AClipsTableView::onTrim before emit"<< selectedFolderName<<fileName<<targetFileName<<inTime<<outTime;
            emit trim(selectedFolderName, fileName, targetFileName, inTime, outTime, 10);

//            QString code = "ffmpeg -y -i \"" + QString(selectedFolderName + fileName).replace("/", "//") + "\" -ss " + inTime.toString("HH:mm:ss.zzz") + " -t " + QTime::fromMSecsSinceStartOfDay(duration).toString("hh:mm:ss.zzz") + " -map_metadata 0 -vcodec copy \"" + QString(selectedFolderName + targetFileName).replace("/", "//") + "\"";

//            QMap<QString, QString> parameters;

//            QString *processId = new QString();
//            emit addLogEntry(selectedFolderName, targetFileName, "Trim", processId);
//            emit addLogToEntry(*processId, code);
//            parameters["processId"] = *processId;

//            processManager->startProcess(code, parameters, [](QWidget *parent, QMap<QString, QString> parameters, QString result)
//            {
//                AClipsTableView *clipsTableView = qobject_cast<AClipsTableView *>(parent);
//                emit clipsTableView->addLogToEntry(parameters["processId"], result);
//            }, [] (QWidget *parent, QString command, QMap<QString, QString> parameters, QStringList result)
//            {
//                AClipsTableView *clipsTableView = qobject_cast<AClipsTableView *>(parent);
//                qDebug()<<"trim"<<parent <<command;
//    //            emit clipsTableView->addLogToEntry("trim ", result.join("\n"));
//                emit clipsTableView->addLogToEntry(parameters["processId"], "Completed");
//            });

            emit propertyUpdate(selectedFolderName, fileName, targetFileName);

//            emit addLogEntry(selectedFolderName, targetFileName, "Property update", processId);

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
//            emit addLogToEntry(*processId, command + "\n");
//            parameters["processId"] = *processId;
//            processManager->startProcess(command, parameters, [](QWidget *parent, QMap<QString, QString> parameters, QString result)
//            {
//                AClipsTableView *clipsTableView = qobject_cast<AClipsTableView *>(parent);
//                emit clipsTableView->addLogToEntry(parameters["processId"], result);
//            }, [] (QWidget *parent, QString, QMap<QString, QString> parameters, QStringList result)
//            {
//                AClipsTableView *clipsTableView = qobject_cast<AClipsTableView *>(parent);
//                emit clipsTableView->addLogToEntry(parameters["processId"], "Completed");
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
                FStarRating starRating = qvariant_cast<FStarRating>(clipsItemModel->index(row, ratingIndex).data());

                int order = clipsItemModel->index(row, orderAfterMovingIndex).data().toInt();

                QString srtContentString = "";
                srtContentString += "<o>" + QString::number(order + 1) + "</o>"; //+1 for file trim, +2 for export
                srtContentString += "<r>" + QString::number(starRating.starCount()) + "</r>";
                srtContentString += "<a>" + clipsItemModel->index(row, alikeIndex).data().toString() + "</a>";
                srtContentString += "<h>" + clipsItemModel->index(row, hintIndex).data().toString() + "</h>";
                srtContentString += "<t>" + clipsItemModel->index(row, tagIndex).data().toString() + "</t>";

                stream << 1 << endl;
                stream << QTime::fromMSecsSinceStartOfDay(deltaIn).toString("HH:mm:ss.zzz") << " --> " << QTime::fromMSecsSinceStartOfDay(duration - deltaOut -  FGlobal().frames_to_msec(1)).toString("HH:mm:ss.zzz") << endl;
                stream << srtContentString << endl;
                stream << endl;

                file.close();
            }
//            clipsItemModel->setData(clipsItemModel->index(row, hintIndex), "copied");
//            clipsItemModel->setData(clipsItemModel->index(row, changedIndex), "yes");
        }
    } //for each clip of file

    //save copied, auto save because trim is done anyway
//    onSectionMoved(-1,-1,-1); //to reorder the items
//    for (int row =0; row < srtFileItemModel->rowCount();row++)
//    {
//        saveModel(srtFileItemModel->index(row, 0).data().toString(), srtFileItemModel->index(row, 1).data().toString());
//    }

    emit reloadAll(true);

//    QMap<QString, QString> parameters;
//    processManager->startProcess(parameters, [] (QWidget *parent, QString, QMap<QString, QString> parameters, QStringList result)
//    {
//        AClipsTableView *clipsTableView = qobject_cast<AClipsTableView *>(parent);
//        clipsTableView->onFolderIndexClicked(QModelIndex()); //reload stuff
//        emit clipsTableView->reloadProperties(""); //triggers property load
//    });

} //onTrim

void AClipsTableView::onPropertiesLoaded()
{
    int fpsSuggested = -1;
    bool fpsFound = false;

    QMap<int, QString> fpsMap;
    for (int row=0; row<clipsItemModel->rowCount();row++)
    {
        QString *fpsPointer = new QString();
        emit getPropertyValue(clipsItemModel->index(row, fileIndex).data().toString(), "VideoFrameRate", fpsPointer);
//        qDebug()<<"AClipsTableView::onPropertiesLoaded"<<*fpsPointer<<(*fpsPointer).toDouble();
        clipsItemModel->setData(clipsItemModel->index(row, fpsIndex), *fpsPointer);

        if (qRound((*fpsPointer).toDouble()) == QSettings().value("frameRate").toInt())
            fpsFound = true;
        else
        {
            fpsSuggested = qRound((*fpsPointer).toDouble());
            fpsMap[fpsSuggested] = QString::number(fpsSuggested);
        }

        QString *durationPointer = new QString();
        emit getPropertyValue(clipsItemModel->index(row, fileIndex).data().toString(), "Duration", durationPointer);
        QTime fileDurationTime = QTime::fromString(*durationPointer,"h:mm:ss");
        if (fileDurationTime == QTime())
        {
            QString durationString = *durationPointer;
            durationString = durationString.left(durationString.length()-2); //remove " s"
            fileDurationTime = QTime::fromMSecsSinceStartOfDay(durationString.toDouble()*1000);
        }

        if (fileDurationTime.msecsSinceStartOfDay() == 0)
            fileDurationTime = QTime::fromMSecsSinceStartOfDay(24*60*60*1000 - 1);

        clipsItemModel->setData(clipsItemModel->index(row, fileDurationIndex), fileDurationTime.toString("HH:mm:ss.zzz"));
    }

//    qDebug()<<"AClipsTableView::onPropertiesLoaded"<<fpsSuggested<<fpsFound;

    if (!fpsFound && fpsSuggested > 0)
    {
        QMessageBox messageBox;
//        messageBox.setParent(this); //then messagebox disappears

        QMapIterator<int, QString> fpsIterator(fpsMap);
        QList<int> fpsList;
        while (fpsIterator.hasNext()) //all files in reverse order
        {
            fpsIterator.next();
            fpsList<<fpsIterator.key();
            messageBox.addButton(fpsIterator.value(), QMessageBox::ActionRole);
        }

        messageBox.addButton(QMessageBox::No);

        messageBox.setWindowTitle( "Loading clips");
        messageBox.setText("There are no clips with the frame rate used (" + QSettings().value("frameRate").toString() + "). Do you want to update to one of below founded frame rates ?");
        int result = messageBox.exec();

//                   QMessageBox::StandardButton reply;

//         reply = QMessageBox::question(this, "Loading clips", "There are no clips with the frame rate used (" + QSettings().value("frameRate").toString() + "). Do you want to update to " + QString::number(fpsSuggested) + " ?",
//                                       QMessageBox::Yes|QMessageBox::No);
//         reply = messageBox.question(this, "Loading clips", "There are no clips with the frame rate used (" + QSettings().value("frameRate").toString() + "). Do you want to update to " + QString::number(fpsSuggested) + " ?",
//                             QMessageBox::Yes|QMessageBox::No);

        if (result != QMessageBox::No)
        {
            emit frameRateChanged(fpsList[result]);
//            QSettings().setValue("frameRate",fpsSuggested);
        }
    }
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

//    //auto save because clip is gone
//    onSectionMoved(-1,-1,-1); //to reorder the items
//    for (int row =0; row < srtFileItemModel->rowCount();row++)
//    {
//        saveModel(srtFileItemModel->index(row, 0).data().toString(), srtFileItemModel->index(row, 1).data().toString());
//    }

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
                for (int col = 0; col < model()->columnCount();col++)
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

void AClipsTableView::onScrubberInChanged(int row, int in)
{
    QTime newInTime = QTime::fromMSecsSinceStartOfDay(FGlobal().msec_rounded_to_fps(in));

    if (!doNotUpdate)
    {
        for (int i=0;i<clipsItemModel->rowCount();i++)
        {
            int clipsCounter = clipsItemModel->index(i,orderBeforeLoadIndex).data().toInt();
            if (clipsCounter == row)
            {
                QTime outTime = QTime::fromString(clipsItemModel->index(i,outIndex).data().toString(),"HH:mm:ss.zzz");
    //            qDebug()<<"AClipsTableView::onScrubberInChanged"<<i<< in<<newInTime.msecsSinceStartOfDay();
                doNotUpdate = true; //avoid onupdatein trigger which fires also scrubberchanged
                clipsItemModel->item(i, inIndex)->setData(newInTime.toString("HH:mm:ss.zzz"),Qt::EditRole);
                clipsItemModel->item(i, durationIndex)->setData(QTime::fromMSecsSinceStartOfDay(FGlobal().frames_to_msec(FGlobal().msec_to_frames(outTime.msecsSinceStartOfDay()) - FGlobal().msec_to_frames(newInTime.msecsSinceStartOfDay()) + 1)).toString("HH:mm:ss.zzz"),Qt::EditRole);
                clipsItemModel->item(i, changedIndex)->setData("yes",Qt::EditRole);
                doNotUpdate = false;
                emit clipsChangedToTimeline(clipsProxyModel); //not to video because video initiated it
            }
        }
    }
}

void AClipsTableView::onScrubberOutChanged(int row, int out)
{
    QTime newOutTime = QTime::fromMSecsSinceStartOfDay(FGlobal().msec_rounded_to_fps(out));

    if (!doNotUpdate)
    {
        for (int i=0;i<clipsItemModel->rowCount();i++)
        {
            int clipCounter = clipsItemModel->index(i,orderBeforeLoadIndex).data().toInt();
            if (clipCounter == row)
            {
                QTime inTime = QTime::fromString(clipsItemModel->index(i,inIndex).data().toString(),"HH:mm:ss.zzz");
    //            qDebug()<<"AClipsTableView::onScrubberOutChanged"<<i<< out<<newOutTime.msecsSinceStartOfDay()<<QSettings().value("frameRate");
                doNotUpdate = true; //avoid onupdatein trigger which fires also scrubberchanged
                clipsItemModel->item(i, changedIndex)->setData("yes",Qt::EditRole);
                clipsItemModel->item(i, outIndex)->setData(newOutTime.toString("HH:mm:ss.zzz"),Qt::EditRole);
                clipsItemModel->item(i, durationIndex)->setData(QTime::fromMSecsSinceStartOfDay(FGlobal().frames_to_msec(FGlobal().msec_to_frames(newOutTime.msecsSinceStartOfDay()) - FGlobal().msec_to_frames(inTime.msecsSinceStartOfDay()) + 1)).toString("HH:mm:ss.zzz"),Qt::EditRole);
                doNotUpdate = false;
                emit clipsChangedToTimeline(clipsProxyModel); //not to video because video initiated it
            }
        }
    }
}

void AClipsTableView::onDataChanged(const QModelIndex &topLeft, const QModelIndex &) //bottomRight
{
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
                clipsItemModel->item(topLeft.row(), durationIndex)->setData(QTime::fromMSecsSinceStartOfDay(FGlobal().frames_to_msec(FGlobal().msec_to_frames(outTime.msecsSinceStartOfDay()) - FGlobal().msec_to_frames(time.msecsSinceStartOfDay()) + 1)).toString("HH:mm:ss.zzz"),Qt::DisplayRole);
                emit updateIn(FGlobal().msec_to_frames(time.msecsSinceStartOfDay()));
            }
            else if (topLeft.column() == outIndex)
            {
                doNotUpdate = true;
                clipsItemModel->item(topLeft.row(), durationIndex)->setData(QTime::fromMSecsSinceStartOfDay(FGlobal().frames_to_msec(FGlobal().msec_to_frames(time.msecsSinceStartOfDay()) - FGlobal().msec_to_frames(inTime.msecsSinceStartOfDay()) + 1)).toString("HH:mm:ss.zzz"),Qt::DisplayRole);
                emit updateOut(FGlobal().msec_to_frames(time.msecsSinceStartOfDay()));
            }
            else if (topLeft.column() == durationIndex)
            {
                int duration = FGlobal().frames_to_msec(FGlobal().msec_to_frames(outTime.msecsSinceStartOfDay()) - FGlobal().msec_to_frames(inTime.msecsSinceStartOfDay()) + 1);
                int delta = time.msecsSinceStartOfDay() - duration;
                int newIn = inTime.addMSecs(-delta/2).msecsSinceStartOfDay();
                int newOut = outTime.addMSecs(delta/2).msecsSinceStartOfDay();

    //            qDebug()<<"AClipsTableView::onDataChanged"<<topLeft.column()<<time.msecsSinceStartOfDay()<<inTime.msecsSinceStartOfDay()<<newIn<<outTime.msecsSinceStartOfDay()<<newOut;

                doNotUpdate = true;
                if (newIn != inTime.msecsSinceStartOfDay())
                {
                    clipsItemModel->item(topLeft.row(), inIndex)->setData(QTime::fromMSecsSinceStartOfDay(newIn).toString("HH:mm:ss.zzz"),Qt::DisplayRole);
                    emit updateIn(FGlobal().msec_to_frames(newIn));
                }
                if (newOut != outTime.msecsSinceStartOfDay())
                {
                    clipsItemModel->item(topLeft.row(), outIndex)->setData(QTime::fromMSecsSinceStartOfDay(newOut).toString("HH:mm:ss.zzz"),Qt::DisplayRole);
                    emit updateOut(FGlobal().msec_to_frames(newOut));
                }
            }
            if (doNotUpdate)
            {
                emit clipsChangedToTimeline(clipsProxyModel); //not to video because video done using emit update in/out
                doNotUpdate = false;
            }
        }
    }
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
            starItem->setData(QVariant::fromValue(FStarRating(value.toInt())), Qt::EditRole);

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

                QVariant starVar = QVariant::fromValue(FStarRating(0));
                if (tags.indexOf("r9") >= 0)
                    starVar = QVariant::fromValue(FStarRating(5));
                else if (tags.indexOf("r8") >= 0)
                    starVar = QVariant::fromValue(FStarRating(4));
                else if (tags.indexOf("r7") >= 0)
                    starVar = QVariant::fromValue(FStarRating(3));
                else if (tags.indexOf("r6") >= 0)
                    starVar = QVariant::fromValue(FStarRating(2));
                else if (tags.indexOf("r5") >= 0)
                    starVar = QVariant::fromValue(FStarRating(1));
                starItem->setData(starVar, Qt::EditRole);
                tags.replace(" r5", "").replace("r5","");
                tags.replace(" r6", "").replace("r6","");
                tags.replace(" r7", "").replace("r7","");
                tags.replace(" r8", "").replace("r8","");
                tags.replace(" r9", "").replace("r9","");
                tags.replace(" ", ";");
            }

            int duration = FGlobal().frames_to_msec(FGlobal().msec_to_frames(outTime.msecsSinceStartOfDay()) - FGlobal().msec_to_frames(inTime.msecsSinceStartOfDay()) + 1);
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
            items.append(new QStandardItem(alike));
            items.append(new QStandardItem(hint));
            items.append(new QStandardItem(tags));
            srtItemModel->appendRow(items);
//                QLineEdit *edit = new QLineEdit(this);
            clipCounter++;
            originalDuration += FGlobal().msec_to_frames(duration);
        }
    }

    return srtItemModel;
} //read

void AClipsTableView::scanDir(QDir dir)
{
    if (!continueLoading)
        return;

    dir.setFilter(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::NoSymLinks);
    dir.setSorting(QDir::Name | QDir::LocaleAware); //localeaware to get +99999ms sorted the right way
    QStringList dirList = dir.entryList();
    for (int i=0; i<dirList.size(); ++i)
    {
        QString newPath = QString("%1/%2").arg(dir.absolutePath()).arg(dirList.at(i));
        scanDir(QDir(newPath));
    }

    QStringList filters;
    filters << "*.mp4"<<"*.jpg"<<"*.avi"<<"*.wmv"<<"*.mts";
    filters << "*.MP4"<<"*.JPG"<<"*.AVI"<<"*.WMV"<<"*.MTS";

    dir.setNameFilters(filters);
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
//    qDebug() << "AClipsTableView::loadModel" << folderName;
    if (checkSaveIfClipsChanged())
    {
        onSectionMoved(-1,-1,-1); //to reorder the items

        for (int row =0; row < srtFileItemModel->rowCount();row++)
        {
            saveModel(srtFileItemModel->index(row, 0).data().toString(), srtFileItemModel->index(row, 1).data().toString());
        }
    }

    clipCounter = 1;
    fileCounter = 0;
    originalDuration = 0;
    continueLoading = true;
    clipsItemModel->removeRows(0, clipsItemModel->rowCount());
    srtFileItemModel->removeRows(0,srtFileItemModel->rowCount());
    scanDir(QDir(folderName));
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
    if (changeCount>0)
    {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "There are " + QString::number(changeCount) + " clips with changes", "Do you want to save these changes ?",
                                      QMessageBox::Yes|QMessageBox::No);

        if (reply == QMessageBox::Yes)
            return true;
    }
    return false;
}

void AClipsTableView::saveModel(QString folderName, QString fileName)
{
    int changeCount = 0;
    QMap<QString, int> inMap; //sorted by fileName and inTime
    for (int row = 0; row<clipsItemModel->rowCount();row++)
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

    if (inMap.count() > 0)
    {
        qDebug()<<"AClipsTableView::saveModel"<<clipsItemModel->rowCount()<<fileName<<changeCount<<inMap.count();

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

                FStarRating starRating = qvariant_cast<FStarRating>(clipsItemModel->index(row, ratingIndex).data());

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

    QString starCount;
    if (ratingFilterComboBox->currentText() == ".....")
        starCount = "0";
    else if (ratingFilterComboBox->currentText() == "*....")
        starCount = "1";
    else if (ratingFilterComboBox->currentText() == "**...")
        starCount = "2";
    else if (ratingFilterComboBox->currentText() == "***..")
        starCount = "3";
    else if (ratingFilterComboBox->currentText() == "****.")
        starCount = "4";
    else if (ratingFilterComboBox->currentText() == "*****")
        starCount = "5";

    QString regExp = starCount + "|" + alikeVariant.toString() + "|" + string1 + "|" + string2 + "|" + fileName;
//    qDebug()<<"AClipsTableView::onClipsFilterChanged"<<starEditorFilterWidget->starRating().starCount()<<tagFilter1ListView->model()->rowCount()<<tagFilter2ListView->model()->rowCount()<<regExp;

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
    clipsProxyModel->setData(clipsProxyModel->index(currentIndex().row(), ratingIndex), QVariant::fromValue(FStarRating(starCount)));
    clipsProxyModel->setData(clipsProxyModel->index(currentIndex().row(), changedIndex), "yes");
}

void AClipsTableView::toggleAlike()
{
    clipsProxyModel->setData(clipsProxyModel->index(currentIndex().row(), alikeIndex), !clipsProxyModel->index(currentIndex().row(), alikeIndex).data().toBool());
    clipsProxyModel->setData(clipsProxyModel->index(currentIndex().row(), changedIndex), "yes");
}


