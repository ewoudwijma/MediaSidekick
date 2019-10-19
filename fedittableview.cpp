#include "fedititemdelegate.h"
#include "fedittableview.h"
#include "fstarrating.h"
#include "stimespinbox.h"

#include <QApplication>
#include <QHeaderView>
#include <QMenu>
#include <QMessageBox>
#include <QSettings>
#include <QTextStream>
#include <QTime>

#include <QDebug>

#include "fglobal.h"

FEditTableView::FEditTableView(QWidget *parent) : QTableView(parent)
{
    editItemModel = new FEditItemModel(this);
    QStringList labels;
    labels << "OrderBL" << "OrderAL"<<"OrderAM"<<"Changed"<< "Path" << "File"<<"Fps"<<"In"<<"Out"<<"Duration"<<"Rating"<<"Alike"<<"Hint"<<"Tags";
    editItemModel->setHorizontalHeaderLabels(labels);

    editProxyModel = new FEditSortFilterProxyModel(this);

    editProxyModel->setSourceModel(editItemModel);

    setModel(editProxyModel);

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
    setColumnWidth(fpsIndex,int(columnWidth(ratingIndex) / 2));
    setColumnWidth(ratingIndex,int(columnWidth(ratingIndex) / 1.5));
    setColumnWidth(alikeIndex,columnWidth(alikeIndex) / 2);

    setColumnHidden(folderIndex, true);
    setColumnHidden(fileIndex, true);

    //from designer defaults
    setDragDropMode(DragDropMode::DropOnly);
    setHorizontalScrollMode(ScrollPerPixel);
    setVerticalScrollMode(ScrollPerPixel);

    setSelectionMode(QAbstractItemView::ContiguousSelection);

//    setEditTriggers(QAbstractItemView::AnyKeyPressed | QAbstractItemView::SelectedClicked);
//    setEditTriggers(QAbstractItemView::DoubleClicked
//                                | QAbstractItemView::SelectedClicked);
//    setSelectionBehavior(QAbstractItemView::SelectRows);

    FEditItemDelegate *editItemDelegate = new FEditItemDelegate(this);
    setItemDelegate(editItemDelegate);

    connect( this, &QTableView::clicked, this, &FEditTableView::onIndexClicked);
    connect( editItemModel, &FEditItemModel::dataChanged, this, &FEditTableView::onDataChanged);
    connect( verticalHeader(), &QHeaderView::sectionMoved, this, &FEditTableView::onSectionMoved);

//    connect( editItemDelegate, &FEditItemDelegate::spinnerChanged, this, &FEditTableView::onSpinnerChanged);

    editContextMenu = new QMenu(this);
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(onEditRightClickMenu(const QPoint &)));

    editContextMenu->addAction(new QAction("Delete",editContextMenu));
    connect(editContextMenu->actions().last(), SIGNAL(triggered()), this, SLOT(onEditDelete()));

    processManager = new FProcessManager(this);

    srtFileItemModel = new QStandardItemModel(this);

    doNotUpdate = false;
}

void FEditTableView::onIndexClicked(QModelIndex index)
{
    qDebug()<<"FEditTableView::onIndexClicked"<<index.data()<<editProxyModel->index(index.row(),fileIndex).data().toString()<<selectedFileName;
//    if (editProxyModel->index(index.row(),fileIndex).data().toString()!=fileUrl.fileName())
//    {
//        qDebug()<<"tableClicked different!!!"<<index.data()<<editProxyModel->index(index.row(),fileIndex).data()<<fileUrl.fileName();
//    }

    emit indexClicked(index);

    if (editProxyModel->index(index.row(),fileIndex).data().toString() != selectedFileName)
    {
        emit editsChangedToVideo(model()); //not to timeline because this can only happen if all edits are shown and they are in the timeline already then
        selectedFileName = editProxyModel->index(index.row(),fileIndex).data().toString();
        qDebug()<<"FEditTableView::onIndexClicked change selectedFileName"<<selectedFileName;
        selectEdits();
    }
}

void FEditTableView::onSectionMoved(int logicalIndex, int oldVisualIndex, int newVisualIndex)
{

//    qDebug()<<"FEditTableView::onSectionMoved"<<logicalIndex<< oldVisualIndex<< newVisualIndex;
//    doNotUpdate = true;
    for (int row=0; row<editProxyModel->rowCount();row++)
    {
        int newOrder = (verticalHeader()->visualIndex(row) + 1) * 10;
        if (editProxyModel->index(row, orderAfterMovingIndex).data().toInt() != newOrder)
        {
            editProxyModel->setData(editProxyModel->index(row, orderAfterMovingIndex), newOrder);
            editProxyModel->setData(editProxyModel->index(row, changedIndex), "yes");
        }
    }
//    doNotUpdate = false;

    emit editsChangedToTimeline(editProxyModel); //not to video because this will not change the video
}

void FEditTableView::onFolderIndexClicked(QModelIndex )//index
{
    selectedFolderName = QSettings().value("LastFolder").toString();
//    qDebug()<<"FEditTableView::onFolderIndexClicked"<<selectedFolderName;
    loadModel(selectedFolderName);

    emit folderIndexClickedItemModel(editItemModel);
    emit folderIndexClickedProxyModel(model());
}

void FEditTableView::onFileIndexClicked(QModelIndex index, QModelIndexList selectedIndices)
{
    qDebug()<<"FEditTableView::onFileIndexClicked"<<index.data().toString();

    selectedFolderName = QSettings().value("LastFileFolder").toString();
    selectedFileName = index.data().toString();

    selectEdits();

    emit fileIndexClicked(index, selectedIndices);
}

void FEditTableView::selectEdits()
{
//    clearSelection();
//    setSelectionMode(QAbstractItemView::MultiSelection);

    doNotUpdate = true; //to avoid datachanged trigger
    int firstRow = -1;
    for (int row=0; row < model()->rowCount(); row++)
    {
        QModelIndex editFileIndex = model()->index(row, fileIndex);
        QModelIndex editInIndex = model()->index(row, inIndex);

        QColor backgroundColor;
        if (editFileIndex.data().toString() == selectedFileName)
        {
            if (firstRow == -1)
                firstRow = row;
//            qDebug()<<"selectEdits"<<selectedFileName<<editFileIndex.data();
//            setCurrentIndex(editInIndex); //does also the scrollTo
            backgroundColor = qApp->palette().color(QPalette::AlternateBase);
//            emit editAdded(editInIndex);
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

void FEditTableView::addEdit(int minus, int plus)
{
    if (selectedFileName == "")
    {
        QMessageBox::information(this, "Add edit", tr("No file selected"));
        return;
    }

    QTime newInTime = QTime::fromMSecsSinceStartOfDay(position - minus);
    QTime newOutTime = QTime::fromMSecsSinceStartOfDay(position + plus - 1000 / QSettings().value("frameRate").toInt());

    int rowToAppendBefore = -1;
    int fileRow = -1;
    int maxOrderAtLoad = 0;
    int maxOrderAfterMoving = 0;
    for (int row=0; row<editItemModel->rowCount();row++)
    {
        QString fileName = editItemModel->index(row, fileIndex).data().toString();
        QTime inTime = QTime::fromString(editItemModel->index(row,inIndex).data().toString(), "HH:mm:ss.zzz");
        if (fileName > selectedFileName && fileRow == -1)
            fileRow = row;
        if (selectedFileName == fileName && newInTime.msecsTo(inTime) > 0 && rowToAppendBefore == -1)
            rowToAppendBefore = row;
        maxOrderAtLoad = qMax(maxOrderAtLoad, editItemModel->index(row, orderAtLoadIndex).data().toInt());
        maxOrderAfterMoving = qMax(maxOrderAtLoad, editItemModel->index(row, orderAfterMovingIndex).data().toInt());
    }

    if (rowToAppendBefore == -1)
        rowToAppendBefore = fileRow;

    if (rowToAppendBefore != -1) //move all rows after rowToAppendBefore
    {
        for (int row=rowToAppendBefore; row<editItemModel->rowCount();row++)
        {
                editItemModel->setData(editItemModel->index(row, orderBeforeLoadIndex), editItemModel->index(row, orderBeforeLoadIndex).data().toInt()+1);
                editItemModel->setData(editItemModel->index(row, orderAtLoadIndex), editItemModel->index(row, orderAtLoadIndex).data().toInt()+10);
                editItemModel->setData(editItemModel->index(row, orderAfterMovingIndex), editItemModel->index(row, orderAfterMovingIndex).data().toInt()+10);
                editItemModel->setData(editItemModel->index(row, changedIndex), "yes");
        }
    }

    qDebug()<<"FEditTableView::addEdit"<<position<<newInTime<<newOutTime<<selectedFolderName<<selectedFileName<<rowToAppendBefore<<editItemModel->index(rowToAppendBefore, orderBeforeLoadIndex).data().toInt();

    QList<QStandardItem *> items;

    QStandardItem *starItem = new QStandardItem;
    QVariant starVar = QVariant::fromValue(FStarRating(0));
    starItem->setData(starVar, Qt::EditRole);

    if (rowToAppendBefore == -1) //add at the end
    {
        items.append(new QStandardItem(QString::number(editItemModel->rowCount()+1)));
        items.append(new QStandardItem(QString::number(maxOrderAtLoad + 10)));
        items.append(new QStandardItem(QString::number(maxOrderAfterMoving + 10)));
        qDebug()<<"FEditTableView::addEditAfter"<<position<<newInTime<<newOutTime<<selectedFolderName<<selectedFileName<<rowToAppendBefore<<editItemModel->index(rowToAppendBefore, orderBeforeLoadIndex).data().toString();
    }
    else //add in between
    {
        QModelIndex orderBeforeLoadIndexx = editItemModel->index(rowToAppendBefore, orderBeforeLoadIndex);
        QModelIndex orderAtLoadIndexx = editItemModel->index(rowToAppendBefore, orderAtLoadIndex);
        QModelIndex orderAFterMovingIndexx = editItemModel->index(rowToAppendBefore, orderAfterMovingIndex);
        qDebug()<<"FEditTableView::addEditBetween"<<position<<newInTime<<newOutTime<<orderAtLoadIndexx.data().toString()<<selectedFolderName<<selectedFileName<<rowToAppendBefore<<editItemModel->index(rowToAppendBefore, orderBeforeLoadIndex).data().toInt();

        items.append(new QStandardItem(QString::number(orderBeforeLoadIndexx.data().toInt()-1)));
        items.append(new QStandardItem(QString::number(orderAtLoadIndexx.data().toInt()-1)));
        items.append(new QStandardItem(QString::number(orderAFterMovingIndexx.data().toInt()-1)));
    }

    QString *fpsValue = new QString();
    emit getPropertyValue(selectedFileName, "VideoFrameRate", fpsValue);

    items.append(new QStandardItem("yes"));
    items.append(new QStandardItem(selectedFolderName));
    items.append(new QStandardItem(selectedFileName));
    items.append(new QStandardItem(*fpsValue));
    items.append(new QStandardItem(newInTime.toString("HH:mm:ss.zzz")));
    items.append(new QStandardItem(newOutTime.toString("HH:mm:ss.zzz")));
    items.append(new QStandardItem(QTime::fromMSecsSinceStartOfDay(newInTime.msecsTo(newOutTime) + 1000 / QSettings().value("frameRate").toInt()).toString("HH:mm:ss.zzz"))); //durationIndex
    items.append(starItem);
    items.append(new QStandardItem(""));
    items.append(new QStandardItem(""));
    items.append(new QStandardItem(""));

    if (rowToAppendBefore == -1)
    {
        editItemModel->appendRow(items);
        setCurrentIndex(editItemModel->index(editItemModel->rowCount()-1, inIndex));
    }
    else
    {
        editItemModel->insertRow(rowToAppendBefore,items);
        setCurrentIndex(editItemModel->index(rowToAppendBefore, inIndex));
    }

    emit editsChangedToVideo(model());
    emit editsChangedToTimeline(editProxyModel);
} //addEdit

void FEditTableView::onEditRightClickMenu(const QPoint &point)
{

    QModelIndex index = indexAt(point);
//    qDebug()<<"onEditRightClickMenu"<<point;
    if (index.isValid() )
        editContextMenu->exec(viewport()->mapToGlobal(point));
}

void FEditTableView::onFileDelete(QString fileName)
{
    bool isEditChanged = false;
    for (int row=editItemModel->rowCount()-1; row>=0; row--) //reverse as takeRow is changing rows
    {
        QString folderNameX = editItemModel->index(row, folderIndex).data().toString();
        QString fileNameX = editItemModel->index(row, fileIndex).data().toString();

        qDebug()<<"FEditTableView::onFileDelete"<<folderNameX<<selectedFolderName<<fileNameX<<fileName;
        if (folderNameX == selectedFolderName && fileNameX == fileName)
        {
            editItemModel->takeRow(row);
            isEditChanged = true;
        }
    }

    if (isEditChanged)
    {
        for (int row = 0; row<editItemModel->rowCount();row++)
        {
            int order = editItemModel->index(row, orderBeforeLoadIndex).data().toInt();
            if (order != row+1)
            {
                editItemModel->setData(editItemModel->index(row, orderBeforeLoadIndex), row+1);
                editItemModel->setData(editItemModel->index(row, changedIndex), "yes");
            }
        }

        emit editsChangedToVideo(model());
        emit editsChangedToTimeline(editProxyModel);
    }
}

void FEditTableView::onFileRename()
{
    qDebug()<<"FEditTableView::onFileRename";
    loadModel(selectedFolderName);
}

void FEditTableView::onTrim(QString pfileName)
{
    qDebug()<<"FEditTableView::onTrim"<< pfileName;
    if (checkSaveIfEditsChanged())
    {
        onSectionMoved(-1,-1,-1); //to reorder the items

        for (int row =0; row < srtFileItemModel->rowCount();row++)
        {
            saveModel(srtFileItemModel->index(row, 0).data().toString(), srtFileItemModel->index(row, 1).data().toString());
        }
    }

    QString code = nullptr;
    for (int row = 0; row < editItemModel->rowCount(); row++) // for each edit of file
    {
        QString fileName = editItemModel->index(row,fileIndex).data().toString();

        if (fileName == pfileName)
        {
            QTime inTime = QTime::fromString(editItemModel->index(row,inIndex).data().toString(),"HH:mm:ss.zzz");
            QTime outTime = QTime::fromString(editItemModel->index(row,outIndex).data().toString(),"HH:mm:ss.zzz");

            int deltaIn = qMin(1000, inTime.msecsSinceStartOfDay());
            inTime = inTime.addMSecs(-deltaIn);
            int deltaOut = 1000;//fmin(1000, ui->videoWidget->m_player->duration() - outTime.msecsSinceStartOfDay());
            outTime = outTime.addMSecs(deltaOut);
            int duration = inTime.msecsTo(outTime) + 1000 / QSettings().value("frameRate").toInt();

    //                qDebug()<<i<<srtItemModel->index(i,inIndex).data().toString()<<srtItemModel->index(i,outIndex).data().toString();
    //                qDebug()<<"times"<<inTime<< deltaIn<< outTime<< deltaOut<< duration<<ui->videoWidget->m_player->duration();

            QString targetFileName = fileName;
            int lastIndex = targetFileName.lastIndexOf(".");
            if (lastIndex > -1)
                targetFileName = targetFileName.left(lastIndex) + "+" + QString::number(inTime.msecsSinceStartOfDay()) + "ms" + targetFileName.mid(lastIndex);
            else
                targetFileName = targetFileName + "+" + QString::number(inTime.msecsSinceStartOfDay()) + "ms";
            code = "ffmpeg -y -i \"" + QString(selectedFolderName + fileName).replace("/", "//") + "\" -ss " + inTime.toString("HH:mm:ss.zzz") + " -t " + QTime::fromMSecsSinceStartOfDay(duration).toString("hh:mm:ss.zzz") + " -map_metadata 0 -vcodec copy -acodec copy \"" + QString(selectedFolderName + targetFileName).replace("/", "//") + "\"";

            emit addLogEntry("trim " + targetFileName);
            emit addLogToEntry("trim " + targetFileName, "Start trim of " + targetFileName + "...");
            QMap<QString, QString> parameters;
            parameters["targetFileName"] = targetFileName;
            processManager->startProcess(code, parameters, [](QWidget *parent, QMap<QString, QString> parameters, QString result)
            {
                FEditTableView *editTableView = qobject_cast<FEditTableView *>(parent);
                emit editTableView->addLogToEntry("trim " + parameters["targetFileName"], result);
            }, [] (QWidget *parent, QString command, QMap<QString, QString> parameters, QStringList result)
            {
                FEditTableView *editTableView = qobject_cast<FEditTableView *>(parent);
                qDebug()<<"trim"<<parent <<command;
    //            emit editTableView->addLogToEntry("trim ", result.join("\n"));
            });

            emit addLogEntry("property update " + targetFileName);

            emit addLogToEntry("property update " + targetFileName, "Start updating metadata of " + targetFileName + "...");
            QString attributeString = "";
            QStringList texts;
            texts << "GPSLongitude" << "GPSLatitude" << "GPSAltitude" << "GPSAltitudeRef" << "Make" << "Model";
            for (int iText = 0; iText < texts.count(); iText++)
            {
                QString *value = new QString();
                emit getPropertyValue(fileName, texts[iText], value);

                attributeString += " -" + texts[iText] + "=\"" + *value + "\"";
            }

            QString command = "exiftool" + attributeString + " -overwrite_original \"" + QString(selectedFolderName + targetFileName).replace("/", "//") + "\"";
            processManager->startProcess(command, parameters, nullptr, [] (QWidget *parent, QString, QMap<QString, QString> parameters, QStringList result)
            {
                FEditTableView *editTableView = qobject_cast<FEditTableView *>(parent);
                emit editTableView->addLogToEntry("property update " + parameters["targetFileName"], result.join("\n"));
    //                    qDebug()<<"exiftool update date"<<parent <<command<< result;
//                editTableView->onFolderIndexClicked(QModelIndex()); //reload stuff
//                emit editTableView->trim(parameters["targetFileName"]);
            });

            //create srt file
            QString fileName;
            lastIndex = targetFileName.lastIndexOf(".");
            if (lastIndex > -1)
                fileName = targetFileName.left(lastIndex) + ".srt";

            QFile file(selectedFolderName + fileName);
            if ( file.open(QIODevice::WriteOnly) )
            {
                QTextStream stream( &file );
                FStarRating starRating = qvariant_cast<FStarRating>(editItemModel->index(row, ratingIndex).data());

                int order = editItemModel->index(row, orderAfterMovingIndex).data().toInt();
                order++;

                QString srtContentString = "";
                srtContentString += "<o>" + QString::number(order) + "</o>";
                srtContentString += "<r>" + QString::number(starRating.starCount()) + "</r>";
                srtContentString += "<a>" + editItemModel->index(row, alikeIndex).data().toString() + "</a>";
                srtContentString += "<h>" + editItemModel->index(row, FGlobal().hintIndex).data().toString() + "</h>";
                srtContentString += "<t>" + editItemModel->index(row, tagIndex).data().toString() + "</t>";

                stream << 1 << endl;
                stream << QTime::fromMSecsSinceStartOfDay(deltaIn).toString("HH:mm:ss.zzz") << " --> " << QTime::fromMSecsSinceStartOfDay(duration - deltaOut -  1000 / QSettings().value("frameRate").toInt()).toString("HH:mm:ss.zzz") << endl;
                stream << srtContentString << endl;
                stream << endl;

                file.close();
            }
//            editItemModel->setData(editItemModel->index(row, hintIndex), "copied");
//            editItemModel->setData(editItemModel->index(row, changedIndex), "yes");
        }
    } //for each edit of file

    //save copied, auto save because trim is done anyway
//    onSectionMoved(-1,-1,-1); //to reorder the items
//    for (int row =0; row < srtFileItemModel->rowCount();row++)
//    {
//        saveModel(srtFileItemModel->index(row, 0).data().toString(), srtFileItemModel->index(row, 1).data().toString());
//    }

    QMap<QString, QString> parameters;
    processManager->startProcess("exiftool", parameters, nullptr, [] (QWidget *parent, QString, QMap<QString, QString> parameters, QStringList result) //dummy process to have this executed last
    {
        FEditTableView *editTableView = qobject_cast<FEditTableView *>(parent);
        editTableView->onFolderIndexClicked(QModelIndex()); //reload stuff
        emit editTableView->trim(""); //triggers property load
    });

} //onTrim

void FEditTableView::onPropertiesLoaded()
{
    int fpsSuggested = -1;
    bool fpsFound = false;
    for (int row=0; row<editItemModel->rowCount();row++)
    {
        QString *fpsValue = new QString();
        emit getPropertyValue(editItemModel->index(row, fileIndex).data().toString(), "VideoFrameRate", fpsValue);
        editItemModel->setData(editItemModel->index(row, fpsIndex), *fpsValue);

        if ((*fpsValue).toInt() == QSettings().value("frameRate").toInt())
            fpsFound = true;
        else
            fpsSuggested = (*fpsValue).toInt();
    }

    if (!fpsFound && fpsSuggested > 0)
    {
        QMessageBox::StandardButton reply;
         reply = QMessageBox::question(this, "Loading edits", "There are no edits with the frame rate used (" + QSettings().value("frameRate").toString() + "). Do you want to update to " + QString::number(fpsSuggested) + " ?",
                                       QMessageBox::Yes|QMessageBox::No);

        if (reply == QMessageBox::Yes)
            emit frameRateChanged(fpsSuggested);
//            QSettings().setValue("frameRate",fpsSuggested);
    }
}

void FEditTableView::onEditDelete()
{
    QModelIndexList indexList = selectionModel()->selectedIndexes();
//    qDebug()<<"FEditTableView::onEditDelete"<<selectionModel()<<indexList.count()<<indexList.first().data().toString();

    QMap<int, int> rowMap; //sorted by fileName and inTime
    for (int i=0; i < indexList.count() ;i++)
    {
        QModelIndex currentModelIndex = indexList[i];
        if (currentModelIndex.row() >= 0 && currentModelIndex.column() >= 0)
        {
            rowMap[currentModelIndex.row()] = editProxyModel->index(currentModelIndex.row(), orderAtLoadIndex).data().toInt();
//            qDebug()<<"  FEditTableView::onEditDelete"<<currentModelIndex.row()<<currentModelIndex.data().toString();
        }
    }

    for (int row = 0; row < editItemModel->rowCount(); row++)
    {
        QMapIterator<int, int> rowIterator(rowMap);
    //    rowIterator.toBack();
        while (rowIterator.hasNext()) //all files in reverse order
        {
            rowIterator.next();

//            qDebug()<<"  FEditTableView::onEditDelete"<<row<<rowIterator.key()<<rowIterator.value()<<editItemModel->index(row, orderAtLoadIndex).data().toInt();
            if (editItemModel->index(row, orderAtLoadIndex).data().toInt() == rowIterator.value())
            {
//                qDebug()<<"  FEditTableView::onEditDelete TAKEROW"<<row<<verticalHeader()->logicalIndex(row);
                editItemModel->takeRow(row);
            }
        }
    }

    //set orderBeforeLoadIndex correct
    for (int row = 0; row<editItemModel->rowCount();row++)
    {
        int order = editItemModel->index(row, orderBeforeLoadIndex).data().toInt();
        if (order != row+1)
        {
            editItemModel->setData(editItemModel->index(row, orderBeforeLoadIndex), row+1);
            editItemModel->setData(editItemModel->index(row, changedIndex), "yes");
        }
    }

//    //auto save because edit is gone
//    onSectionMoved(-1,-1,-1); //to reorder the items
//    for (int row =0; row < srtFileItemModel->rowCount();row++)
//    {
//        saveModel(srtFileItemModel->index(row, 0).data().toString(), srtFileItemModel->index(row, 1).data().toString());
//    }

    emit editsChangedToVideo(model());
    emit editsChangedToTimeline(editProxyModel);
}

void FEditTableView::onVideoPositionChanged(int progress, int row, int relativeProgress)
{
//    if (!ui->videoProgressSlider->isSliderDown())
//        ui->videoProgressSlider->setValue(progress / 1000);

    position = progress;

    if (progress != 0)
    {

//        qDebug()<<"FEditTableView::onVideoPositionChanged: " << progress<<time<<text<<currentIndex().data();
        int foundRow = -1;
        for (int i = 0; i < model()->rowCount(); i++)
        {
            if (model()->index(i,orderBeforeLoadIndex).data().toInt() == row)
            {
                foundRow = i;
            }
        }
        if (foundRow != -1)
        {
            if (highLightedRow != foundRow) // if not already selected
            {
                clearSelection();
                setSelectionMode(QAbstractItemView::MultiSelection);
                for (int col = 0; col < model()->columnCount();col++)
                {
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

void FEditTableView::onScrubberInChanged(int row, int in)
{
    QTime newInTime = QTime::fromMSecsSinceStartOfDay(FGlobal().msec_rounded_to_fps(in));

    if (!doNotUpdate)
    for (int i=0;i<editItemModel->rowCount();i++)
    {
        int editCounter = editItemModel->index(i,orderBeforeLoadIndex).data().toInt();
        if (editCounter == row)
        {
            QTime outTime = QTime::fromString(editItemModel->index(i,outIndex).data().toString(),"HH:mm:ss.zzz");
            qDebug()<<"FEditTableView::onScrubberInChanged"<<i<< in<<newInTime.msecsSinceStartOfDay();
//            editItemModel->blockSignals(false);
            doNotUpdate = true; //avoid onupdatein trigger which fires also scrubberchanged
            editItemModel->item(i, inIndex)->setData(newInTime.toString("HH:mm:ss.zzz"),Qt::EditRole);
            editItemModel->item(i, durationIndex)->setData(QTime::fromMSecsSinceStartOfDay(newInTime.msecsTo(outTime) + 1000 / QSettings().value("frameRate").toInt()).toString("HH:mm:ss.zzz"),Qt::EditRole);
            editItemModel->item(i, changedIndex)->setData("yes",Qt::EditRole);
            doNotUpdate = false;
//            editItemModel->blockSignals(false);
        //            setCurrentIndex(editItemModel->index(i,inIndex));
            emit editsChangedToTimeline(editProxyModel); //not to video because video initiated it
        }
    }
}

void FEditTableView::onScrubberOutChanged(int row, int out)
{
    QTime newOutTime = QTime::fromMSecsSinceStartOfDay(FGlobal().msec_rounded_to_fps(out));

    if (!doNotUpdate)
    for (int i=0;i<editItemModel->rowCount();i++)
    {
        int editCounter = editItemModel->index(i,orderBeforeLoadIndex).data().toInt();
        if (editCounter == row)
        {
            QTime inTime = QTime::fromString(editItemModel->index(i,inIndex).data().toString(),"HH:mm:ss.zzz");
            qDebug()<<"FEditTableView::onScrubberOutChanged"<<i<< out<<newOutTime.msecsSinceStartOfDay();
//            editItemModel->blockSignals(true);
            doNotUpdate = true; //avoid onupdatein trigger which fires also scrubberchanged
            editItemModel->item(i, changedIndex)->setData("yes",Qt::EditRole);
            editItemModel->item(i, outIndex)->setData(newOutTime.toString("HH:mm:ss.zzz"),Qt::EditRole);
            editItemModel->item(i, durationIndex)->setData(QTime::fromMSecsSinceStartOfDay(inTime.msecsTo(newOutTime) + 1000 / QSettings().value("frameRate").toInt()).toString("HH:mm:ss.zzz"),Qt::EditRole);
            doNotUpdate = false;
//            editItemModel->blockSignals(false);
        //            setCurrentIndex(editItemModel->index(i,inIndex));
            emit editsChangedToTimeline(editProxyModel); //not to video because video initiated it
        }
    }
}

//void FEditTableView::onSpinnerChanged(STimeSpinBox *timeSpinBox)
//{
//    qDebug()<<"FEditTableView::onSpinnerChanged"<<timeSpinBox->value();

//}

void FEditTableView::onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
//    return;
    if (!doNotUpdate)
    if (topLeft.column() == inIndex || topLeft.column() == outIndex || topLeft.column() == durationIndex)
    {
        QTime time = QTime::fromString(topLeft.data().toString(),"HH:mm:ss.zzz");
        QTime inTime = QTime::fromString(editItemModel->index(topLeft.row(),inIndex).data().toString(), "HH:mm:ss.zzz");
        QTime outTime = QTime::fromString(editItemModel->index(topLeft.row(),outIndex).data().toString(), "HH:mm:ss.zzz");
        qDebug()<<"FEditTableView::onDataChanged"<<topLeft.column()<<time.msecsSinceStartOfDay()<<inTime.msecsSinceStartOfDay()<<outTime.msecsSinceStartOfDay();

        if (topLeft.column() == inIndex)
        {
            doNotUpdate = true;
            editItemModel->item(topLeft.row(), durationIndex)->setData(QTime::fromMSecsSinceStartOfDay(time.msecsTo(outTime) + 1000 / QSettings().value("frameRate").toInt()).toString("HH:mm:ss.zzz"),Qt::DisplayRole);
            emit updateIn(FGlobal().msec_to_frames(time.msecsSinceStartOfDay()));
        }
        else if (topLeft.column() == outIndex)
        {
            doNotUpdate = true;
            editItemModel->item(topLeft.row(), durationIndex)->setData(QTime::fromMSecsSinceStartOfDay(inTime.msecsTo(time) + 1000 / QSettings().value("frameRate").toInt()).toString("HH:mm:ss.zzz"),Qt::DisplayRole);
            emit updateOut(FGlobal().msec_to_frames(time.msecsSinceStartOfDay()));
        }
        else if (topLeft.column() == durationIndex)
        {
            int duration = inTime.msecsTo(outTime) + 1000 / QSettings().value("frameRate").toInt();
            int delta = time.msecsSinceStartOfDay() - duration;
            int newIn = inTime.addMSecs(-delta/2).msecsSinceStartOfDay();
            int newOut = outTime.addMSecs(delta/2).msecsSinceStartOfDay();

//            qDebug()<<"FEditTableView::onDataChanged"<<topLeft.column()<<time.msecsSinceStartOfDay()<<inTime.msecsSinceStartOfDay()<<newIn<<outTime.msecsSinceStartOfDay()<<newOut;

            doNotUpdate = true;
            if (newIn != inTime.msecsSinceStartOfDay())
            {
                editItemModel->item(topLeft.row(), inIndex)->setData(QTime::fromMSecsSinceStartOfDay(newIn).toString("HH:mm:ss.zzz"),Qt::DisplayRole);
                emit updateIn(FGlobal().msec_to_frames(newIn));
            }
            if (newOut != outTime.msecsSinceStartOfDay())
            {
                editItemModel->item(topLeft.row(), outIndex)->setData(QTime::fromMSecsSinceStartOfDay(newOut).toString("HH:mm:ss.zzz"),Qt::DisplayRole);
                emit updateOut(FGlobal().msec_to_frames(newOut));
            }
        }
        if (doNotUpdate)
        {
            emit editsChangedToTimeline(editProxyModel); //not to video because video done using emit update in/out
            doNotUpdate = false;
        }
    }
}

QStandardItemModel* FEditTableView::read(QString folderName, QString fileName)
{
//    qDebug()<<"FEditTableView::read"<<folderName << fileName;
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

                order = QString::number(editCounter*10);

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

            int duration = inTime.msecsTo(outTime)+1000 / QSettings().value("frameRate").toInt();
            QList<QStandardItem *> items;
            items.append(new QStandardItem(QString::number(editCounter)));
            items.append(new QStandardItem(order));
            items.append(new QStandardItem(order));
            items.append(new QStandardItem("no"));
            items.append(new QStandardItem(folderName));
            items.append(new QStandardItem(fileName));
            items.append(new QStandardItem(""));
            items.append(new QStandardItem(inTime.toString("HH:mm:ss.zzz")));
            items.append(new QStandardItem(outTime.toString("HH:mm:ss.zzz")));
            items.append(new QStandardItem(QTime::fromMSecsSinceStartOfDay(duration).toString("HH:mm:ss.zzz"))); //durationIndex
            items.append(starItem);
            items.append(new QStandardItem(alike));
            items.append(new QStandardItem(hint));
            items.append(new QStandardItem(tags));
            srtItemModel->appendRow(items);
//                QLineEdit *edit = new QLineEdit(this);
            editCounter++;
            originalDuration += FGlobal().msec_to_frames(duration);
        }
    }

    return srtItemModel;
} //read

void FEditTableView::scanDir(QDir dir)
{
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

//    qDebug() << "FEditTableView::scanDir" << dir.path();

    QStringList fileList = dir.entryList();
    for (int i=0; i<fileList.count(); i++)
    {
        QStandardItemModel *srtItemModel = read(dir.path() + "/", fileList[i]);

        QList<QStandardItem *> items;
        items.append(new QStandardItem(dir.path() + "/"));
        items.append(new QStandardItem(fileList[i]));
        srtFileItemModel->appendRow(items);

        if (srtItemModel->rowCount()>100)
        {
            QMessageBox::StandardButton reply;
             reply = QMessageBox::question(this, "Loading edits of " + fileList[i], "Are you sure you want to add " + QString::number(srtItemModel->rowCount()) + " edits?",
                                           QMessageBox::Yes|QMessageBox::No);

            if (reply == QMessageBox::No)
                return;
        }

        while (srtItemModel->rowCount()>0)
            editItemModel->appendRow(srtItemModel->takeRow(0));
    }
}

void FEditTableView::loadModel(QString folderName)
{
    if (checkSaveIfEditsChanged())
    {
        onSectionMoved(-1,-1,-1); //to reorder the items

        for (int row =0; row < srtFileItemModel->rowCount();row++)
        {
            saveModel(srtFileItemModel->index(row, 0).data().toString(), srtFileItemModel->index(row, 1).data().toString());
        }
    }

    editCounter = 1;
    originalDuration = 0;
    editItemModel->removeRows(0, editItemModel->rowCount());
    srtFileItemModel->removeRows(0,srtFileItemModel->rowCount());
    scanDir(QDir(folderName));
}

bool FEditTableView::checkSaveIfEditsChanged()
{
    int changeCount = 0;
    for (int row=0; row<editItemModel->rowCount();row++)
    {
        if (editItemModel->index(row, changedIndex).data().toString() == "yes")
            changeCount++;
    }
    if (changeCount>0)
    {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "There are " + QString::number(changeCount) + " edits with changes", "Do you want to save these changes ?",
                                      QMessageBox::Yes|QMessageBox::No);

        if (reply == QMessageBox::Yes)
            return true;
    }
    return false;
}

void FEditTableView::saveModel(QString folderName, QString fileName)
{
    int changeCount = 0;
    QMap<QString, int> inMap; //sorted by fileName and inTime
    for (int row = 0; row<editItemModel->rowCount();row++)
    {
        if (editItemModel->index(row, folderIndex).data().toString() == folderName && editItemModel->index(row, fileIndex).data().toString() == fileName)
        {
            QString inTime = editItemModel->index(row, inIndex).data().toString();
            inMap[inTime] = row;
            if (editItemModel->index(row, changedIndex).data().toString() == "yes")
                changeCount++;
        }
    }

    int lastIndex = fileName.lastIndexOf(".");
    QString srtFileName = fileName.left(lastIndex) + ".srt";
    QFile file;
    file.setFileName(folderName + srtFileName);

    if (inMap.count() > 0)
    {
        qDebug()<<"FEditTableView::saveModel"<<editItemModel->rowCount()<<fileName<<changeCount<<inMap.count();

        if (changeCount > 0) //changes in this file
        {
            int editPerFileCounter = 0;
            editPerFileCounter = 1;

//            qDebug()<<"FEditTableView::saveModel"<<srtFileName;

            file.open(QIODevice::WriteOnly);

            QMapIterator<QString, int> inIterator(inMap);
            while (inIterator.hasNext()) //all files
            {
                inIterator.next();

                int row = inIterator.value();

                QTextStream stream(&file);

                FStarRating starRating = qvariant_cast<FStarRating>(editItemModel->index(row, ratingIndex).data());

                QString srtContentString = "";
                srtContentString += "<o>" + editItemModel->index(row, orderAfterMovingIndex).data().toString() + "</o>";
                srtContentString += "<r>" + QString::number(starRating.starCount()) + "</r>";
                srtContentString += "<a>" + editItemModel->index(row, alikeIndex).data().toString() + "</a>";
                srtContentString += "<h>" + editItemModel->index(row, FGlobal().hintIndex).data().toString() + "</h>";
                srtContentString += "<t>" + editItemModel->index(row, tagIndex).data().toString() + "</t>";

                stream << editPerFileCounter << endl;
                stream << editItemModel->index(row,inIndex).data().toString() << " --> " << editItemModel->index(row,outIndex).data().toString() << endl;
                stream << srtContentString << endl;
                stream << endl;
                editPerFileCounter++;

                editItemModel->setData(editItemModel->index(row, changedIndex), "no");
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

void FEditTableView::onEditFilterChanged(FStarEditor *starEditorFilterWidget, QCheckBox *alikeCheckBox, QListView *tagFilter1ListView, QListView *tagFilter2ListView, QCheckBox *fileOnlyCheckBox )
{
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

    QString regExp = QString::number(starEditorFilterWidget->starRating().starCount()) + "|" + alikeVariant.toString() + "|" + string1 + "|" + string2 + "|" + fileName;
    qDebug()<<"FEditTableView::onEditFilterChanged"<<starEditorFilterWidget->starRating().starCount()<<tagFilter1ListView->model()->rowCount()<<tagFilter2ListView->model()->rowCount()<<regExp;

    editProxyModel->setFilterRegExp(QRegExp(regExp, Qt::CaseInsensitive,
                                                QRegExp::FixedString));
    editProxyModel->setFilterKeyColumn(-1);

//    qDebug()<<"FEditTableView::onEditFilterChanged"<<starEditorFilterWidget->starRating().starCount();

    emit editsChangedToVideo(model());
    emit editsChangedToTimeline(editProxyModel);
}

void FEditTableView::giveStars(int starCount)
{
    qDebug()<<"FEditTableView::onGiveStars"<<starCount<<currentIndex();
    editProxyModel->setData(editProxyModel->index(currentIndex().row(), ratingIndex), QVariant::fromValue(FStarRating(starCount)));
    editProxyModel->setData(editProxyModel->index(currentIndex().row(), changedIndex), "yes");
}

void FEditTableView::toggleAlike()
{
    editProxyModel->setData(editProxyModel->index(currentIndex().row(), alikeIndex), !editProxyModel->index(currentIndex().row(), alikeIndex).data().toBool());
    editProxyModel->setData(editProxyModel->index(currentIndex().row(), changedIndex), "yes");
}


