#include "fedititemdelegate.h"
#include "fedittableview.h"
#include "fstarrating.h"
#include "stimespinbox.h"

#include <QApplication>
#include <QHeaderView>
#include <QMenu>
#include <QSettings>
#include <QTextStream>
#include <QTime>

#include <QDebug>

#include "fglobal.h"

FEditTableView::FEditTableView(QWidget *parent) : QTableView(parent)
{
    editItemModel = new FEditItemModel(this);
    QStringList labels;
    labels << "OrderBL" << "OrderAL"<<"OrderAM"<< "Path" << "File"<<"Fps"<<"In"<<"Out"<<"Duration"<<"Rating"<<"Repeat"<<"Hint"<<"Tags";
    editItemModel->setHorizontalHeaderLabels(labels);

    editProxyModel = new FEditSortFilterProxyModel(this);

    editProxyModel->setSourceModel(editItemModel);

    setModel(editProxyModel);

//    setModel(editItemModel);
    horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    verticalHeader()->setSectionsMovable(true);
    horizontalHeader()->setSectionsClickable(true);
    horizontalHeader()->setStretchLastSection( true );

    sortByColumn(orderAtLoadIndex, Qt::AscendingOrder);

    //    setColumnWidth(fileIndex,columnWidth(fileIndex) * 2);
//    setColumnWidth(durationIndex,int(columnWidth(durationIndex) / 1.5));
    setColumnWidth(orderBeforeLoadIndex, 1);
    setColumnWidth(orderAtLoadIndex, 1);
    setColumnWidth(orderAfterMovingIndex, 1);
    setColumnWidth(fpsIndex,int(columnWidth(ratingIndex) / 2));
    setColumnWidth(ratingIndex,int(columnWidth(ratingIndex) / 1.5));
    setColumnWidth(repeatIndex,columnWidth(repeatIndex) / 2);

//    setColumnHidden(orderBeforeLoadIndex, true);
//    setColumnHidden(orderAtLoadIndex, true);
//    setColumnHidden(orderAfterMovingIndex, true);
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
    connect( verticalHeader(), &QHeaderView::sectionMoved, this, &FEditTableView::onSectionMoved);

    editContextMenu = new QMenu(this);
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(onEditRightClickMenu(const QPoint &)));

    editContextMenu->addAction(new QAction("Delete",editContextMenu));
    connect(editContextMenu->actions().last(), SIGNAL(triggered()), this, SLOT(onEditDelete()));

//    editItemModel->moveRows()

//    QString lastFolder = QSettings().value("LastFolder").toString();
//    if (lastFolder != ""  && lastFolder.length()>3) //not the root folder
//    {
//        onFolderIndexClicked(QModelIndex());
//        loadModel(QSettings().value("LastFolder").toString());
//    }

    processManager = new FProcessManager(this);

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
        emit editsChanged(editProxyModel);
        selectedFileName = editProxyModel->index(index.row(),fileIndex).data().toString();
        qDebug()<<"FEditTableView::onIndexClicked change selectedFileName"<<selectedFileName;
        selectEdits();
    }

}

void FEditTableView::onSectionMoved(int logicalIndex, int oldVisualIndex, int newVisualIndex)
{

    qDebug()<<"FEditTableView::onSectionMoved"<<logicalIndex<< oldVisualIndex<< newVisualIndex;
    for (int row=0; row<editProxyModel->rowCount();row++)
    {
//        qDebug()<<"  FEditTableView::onSectionMoved"<<row<<verticalHeader()->visualIndex(row)<<verticalHeader()->logicalIndex(row);
//        editProxyModel->setData(editProxyModel->index(verticalHeader()->logicalIndex(row), orderAtLoadIndex), QString::number((row)*10).rightJustified(5, '0'));
//        editProxyModel->setData(editProxyModel->index(row, orderAtLoadIndex), QString::number((verticalHeader()->logicalIndex(row))*10).rightJustified(5, '0'));
        editProxyModel->setData(editProxyModel->index(row, orderAfterMovingIndex), verticalHeader()->visualIndex(row));
    }
//    setSortingEnabled(true);
    emit editsChangedFromVideo(editProxyModel);
//    verticalHeader()->saveState();
//    sortByColumn(orderAtLoadIndex, Qt::AscendingOrder);
}

void FEditTableView::onFolderIndexClicked(QModelIndex index)
{
    selectedFolderName = QSettings().value("LastFolder").toString();
    qDebug()<<"FEditTableView::onFolderIndexClicked"<<index.data().toString()<<selectedFolderName;
    loadModel(selectedFolderName);

//        QStandardItemModel *modelx = qobject_cast<QStandardItemModel *>(model());

    emit folderIndexClickedItemModel(editItemModel);
    emit folderIndexClickedProxyModel(editProxyModel);

}

void FEditTableView::onFileIndexClicked(QModelIndex index)
{
    qDebug()<<"FEditTableView::onFileIndexClicked"<<index.data().toString();

    selectedFileName = index.data().toString();

    selectEdits();

    emit fileIndexClicked(index);
    emit editsChanged(editProxyModel);
}

void FEditTableView::selectEdits()
{
//    clearSelection();
//    setSelectionMode(QAbstractItemView::MultiSelection);

    int firstRow = -1;
    for (int row=0; row < editProxyModel->rowCount(); row++)
    {
        QModelIndex editFileIndex = editProxyModel->index(row, fileIndex);
        QModelIndex editInIndex = editProxyModel->index(row, inIndex);

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

        for (int col=2;col<editProxyModel->columnCount();col++)
        {
            QModelIndex index = editProxyModel->index(row, col);
//            QStandardItem *item = editProxyModel->item(index);
//            item->setData(backgroundColor, Qt::BackgroundRole);
//            qDebug()<<"before setdata"<<row<<col<<index.data().toString();
            editProxyModel->setData(index, backgroundColor, Qt::BackgroundRole);
//            qDebug()<<"after setdata"<<index.data().toString();
        }

//        scrollTo(editProxyModel->index(firstRow, inIndex));
    }

}

void FEditTableView::addEdit(int minus, int plus)
{
    QTime newInTime = QTime::fromMSecsSinceStartOfDay(position - minus);
    QTime newOutTime = QTime::fromMSecsSinceStartOfDay(position + plus - 1000 / frameRate);

    int timeRow = -1;
    int fileRow = -1;
    for (int row=0; row<editItemModel->rowCount();row++)
    {
        QString fileName = editItemModel->index(row, fileIndex).data().toString();
        QTime inTime = QTime::fromString(editItemModel->index(row,inIndex).data().toString(), "HH:mm:ss.zzz");
        if (fileName > selectedFileName && fileRow == -1)
            fileRow = row;
        if (selectedFileName == fileName && newInTime.msecsTo(inTime) > 0 && timeRow == -1)
            timeRow = row;
    }

    if (timeRow == -1)
        timeRow = fileRow;

    if (timeRow != -1)
    {
        for (int row=timeRow; row<editItemModel->rowCount();row++)
        {
                editItemModel->setData(editItemModel->index(row, orderBeforeLoadIndex), editItemModel->index(row, orderBeforeLoadIndex).data().toInt()+1);
                editItemModel->setData(editItemModel->index(row, orderAtLoadIndex), editItemModel->index(row, orderAtLoadIndex).data().toInt()+1);
                editItemModel->setData(editItemModel->index(row, orderAfterMovingIndex), editItemModel->index(row, orderAfterMovingIndex).data().toInt()+1);
        }
    }

    qDebug()<<"FEditTableView::addEdit"<<position<<newInTime<<newOutTime<<selectedFolderName<<selectedFileName<<timeRow<<editItemModel->index(timeRow, orderBeforeLoadIndex).data().toInt();

    QList<QStandardItem *> items;

    QStandardItem *starItem = new QStandardItem;
    QVariant starVar = QVariant::fromValue(FStarRating(0));
    starItem->setData(starVar, Qt::EditRole);

    if (timeRow == -1)
    {
        items.append(new QStandardItem(QString::number(editItemModel->rowCount()+1)));
        items.append(new QStandardItem(QString::number(editItemModel->rowCount())));
        items.append(new QStandardItem(QString::number(editItemModel->rowCount())));
        qDebug()<<"FEditTableView::addEditAfter"<<position<<newInTime<<newOutTime<<selectedFolderName<<selectedFileName<<timeRow<<editItemModel->index(timeRow, orderBeforeLoadIndex).data().toString();
    }
    else {
        QModelIndex orderBeforeLoadIndexx = editItemModel->index(timeRow, orderBeforeLoadIndex);
        QModelIndex orderAtLoadIndexx = editItemModel->index(timeRow, orderAtLoadIndex);
        QModelIndex orderAFterMovingIndexx = editItemModel->index(timeRow, orderAfterMovingIndex);
        qDebug()<<"FEditTableView::addEditBetween"<<position<<newInTime<<newOutTime<<orderAtLoadIndexx.data().toString()<<selectedFolderName<<selectedFileName<<timeRow<<editItemModel->index(timeRow, orderBeforeLoadIndex).data().toInt();

        items.append(new QStandardItem(QString::number(orderBeforeLoadIndexx.data().toInt()-1)));
        items.append(new QStandardItem(QString::number(orderAtLoadIndexx.data().toInt()-1)));
        items.append(new QStandardItem(QString::number(orderAFterMovingIndexx.data().toInt()-1)));
    }

    QString *fpsValue = new QString();
    emit getPropertyValue(selectedFileName, "VideoFrameRate", fpsValue);


    items.append(new QStandardItem(selectedFolderName));
    items.append(new QStandardItem(selectedFileName));
    items.append(new QStandardItem(*fpsValue));
    items.append(new QStandardItem(newInTime.toString("HH:mm:ss.zzz")));
    items.append(new QStandardItem(newOutTime.toString("HH:mm:ss.zzz")));
    items.append(new QStandardItem(QTime::fromMSecsSinceStartOfDay(newInTime.msecsTo(newOutTime) + 1000 / frameRate).toString("HH:mm:ss.zzz"))); //durationIndex
    items.append(starItem);
    items.append(new QStandardItem(""));
    items.append(new QStandardItem(""));
    items.append(new QStandardItem(""));

    if (timeRow == -1)
    {
        editItemModel->appendRow(items);
        setCurrentIndex(editItemModel->index(editItemModel->rowCount()-1, inIndex));
    }
    else
    {
        editItemModel->insertRow(timeRow,items);
        setCurrentIndex(editItemModel->index(timeRow, inIndex));
    }


    emit editsChanged(editProxyModel);

}

void FEditTableView::onEditRightClickMenu(const QPoint &point)
{

    QModelIndex index = indexAt(point);
//    qDebug()<<"onEditRightClickMenu"<<point;
        if (index.isValid() ) {
            editContextMenu->exec(viewport()->mapToGlobal(point));
        }
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
        emit editsChanged(editProxyModel);
}

void FEditTableView::onTrim(QString fileName)
{
    QString code = nullptr;
    for (int i = 0; i < editItemModel->rowCount(); i++) // for each edit of file
    {
        QTime inTime = QTime::fromString(editItemModel->index(i,inIndex).data().toString(),"HH:mm:ss.zzz");
        QTime outTime = QTime::fromString(editItemModel->index(i,outIndex).data().toString(),"HH:mm:ss.zzz");

        int deltaIn = qMin(1000, inTime.msecsSinceStartOfDay());
        inTime = inTime.addMSecs(-deltaIn);
        int deltaOut = 1000;//fmin(1000, ui->videoWidget->m_player->duration() - outTime.msecsSinceStartOfDay());
        outTime = outTime.addMSecs(deltaOut);
        int duration = inTime.msecsTo(outTime) + 1000 / frameRate;

//                qDebug()<<i<<srtItemModel->index(i,inIndex).data().toString()<<srtItemModel->index(i,outIndex).data().toString();
//                qDebug()<<"times"<<inTime<< deltaIn<< outTime<< deltaOut<< duration<<ui->videoWidget->m_player->duration();

        QString targetFileName = selectedFolderName + fileName;
        int lastIndex = targetFileName.lastIndexOf(".");
        if (lastIndex > -1)
            targetFileName = targetFileName.left(lastIndex) + "+" + QString::number(inTime.msecsSinceStartOfDay()) + "ms" + targetFileName.mid(lastIndex);
        else
            targetFileName = targetFileName + "+" + QString::number(inTime.msecsSinceStartOfDay()) + "ms";
        code = "ffmpeg -y -i \"" + QString(selectedFolderName + fileName).replace("/", "//") + "\" -ss " + inTime.toString("HH:mm:ss.zzz") + " -t " + QTime::fromMSecsSinceStartOfDay(duration).toString("hh:mm:ss.zzz") + " -map_metadata 0 -vcodec copy -acodec copy \"" + QString(targetFileName).replace("/", "//") + "\"";

        emit addLogEntry("trim");
        emit addLogToEntry("trim", "Start trim of " + targetFileName + "...");
        processManager->startProcess(code, [](QWidget *parent, QString result)
        {
            FEditTableView *editTableView = qobject_cast<FEditTableView *>(parent);
            emit editTableView->addLogToEntry("trim", result);
        }, [] (QWidget *parent, QString, QStringList result)
        {
            FEditTableView *editTableView = qobject_cast<FEditTableView *>(parent);
//                    qDebug()<<"trim"<<parent <<command<< result;
            emit editTableView->addLogToEntry("trim", result.join("\n"));
        });

        if (false)
        {
            emit addLogToEntry("trim", "Start updating metadata of " + targetFileName + "...");
            QString attributeString = "";
            QStringList texts;
            texts << "GPSLongitude" << "GPSLatitude" << "GPSAltitude" << "GPSAltitudeRef" << "Make" << "Model";
            for (int iText = 0; iText < texts.count(); iText++)
            {
                QVariant value;// = metadata->getItemVariantValue(srtItemModel->index(i,fileIndex).data().toString(),texts[iText]);
                attributeString += " -" + texts[iText] + "=\"" + value.toString() + "\"";

            }

            processManager->startProcess("exiftool" + attributeString + " -overwrite_original \"" + QString(targetFileName).replace("/", "//") + "\"", nullptr, [] (QWidget *parent, QString, QStringList result)
            {
                FEditTableView *editTableView = qobject_cast<FEditTableView *>(parent);
                emit editTableView->addLogToEntry("trim", result.join("\n"));
    //                    qDebug()<<"exiftool update date"<<parent <<command<< result;
            });

        }

        //create srt file
        QString fileName;
        lastIndex = targetFileName.lastIndexOf(".");
        if (lastIndex > -1)
            fileName = targetFileName.left(lastIndex) + ".srt";

        QFile file(fileName);
        if ( file.open(QIODevice::WriteOnly) )
        {
            QTextStream stream( &file );

            stream << 1 << endl;
            stream << QTime::fromMSecsSinceStartOfDay(deltaIn).toString("HH:mm:ss.zzz") << " --> " << QTime::fromMSecsSinceStartOfDay(duration - deltaOut -  1000 / frameRate).toString("HH:mm:ss.zzz") << endl;
            stream << editItemModel->index(i,tagIndex).data().toString() << endl;
            stream << endl;

            file.close();
        }
    } //for
}

void FEditTableView::onPropertiesLoaded()
{
    for (int row=0; row<editItemModel->rowCount();row++)
    {
        QString *fpsValue = new QString();
        emit getPropertyValue(editItemModel->index(row, fileIndex).data().toString(), "VideoFrameRate", fpsValue);
        editItemModel->setData(editItemModel->index(row, fpsIndex), *fpsValue);
    }
}

void FEditTableView::onEditDelete()
{
    QModelIndexList indexList = selectionModel()->selectedIndexes();
    qDebug()<<"onEditDelete"<<selectionModel()<<indexList.count();
    for (int i=indexList.count() - 1; i>=0 ;i--)
    {
//        if (indexList[i].column() == 0) //first column
        {
            QModelIndex currentModelIndex = indexList[i];
            qDebug()<<"  currentModelIndex"<<currentModelIndex.row()<<currentModelIndex.column()<<currentModelIndex.data();
            if (currentModelIndex.row() >= 0 && currentModelIndex.column() >= 0)
            {

                editItemModel->takeRow(currentModelIndex.row());

                emit editsChanged(editProxyModel);
            }
        }
    }
}

void FEditTableView::onVideoPositionChanged(int progress, int row, int relativeProgress)
{
//    if (!ui->videoProgressSlider->isSliderDown())
//        ui->videoProgressSlider->setValue(progress / 1000);

    position = progress;

    if (progress != 0)
    {
//        QTime time = QTime::fromMSecsSinceStartOfDay(progress);
//        QString text = time.toString("hh:mm:ss.zzz");

//        qDebug()<<"FEditTableView::onVideoPositionChanged: " << progress<<time<<text<<currentIndex().data();
        int foundRow = -1;
        for (int i = 0; i < editProxyModel->rowCount(); i++)
        {
//            QTime inTime = QTime::fromString(editProxyModel->index(row,inIndex).data().toString(),"HH:mm:ss.zzz");
//            QTime outTime = QTime::fromString(editProxyModel->index(row,outIndex).data().toString(),"HH:mm:ss.zzz");

            if (editProxyModel->index(i,orderBeforeLoadIndex).data().toInt() == row)
            {
                foundRow = i;
    //            QModelIndex currentModelIndex = ui->editTableView->currentIndex();
    //            if (currentModelIndex.row() != i)
            }
        }
        if (foundRow != -1)
        {
            if (highLightedRow != foundRow) // if not already selected
            {
                clearSelection();
                setSelectionMode(QAbstractItemView::MultiSelection);
                for (int col = 2; col < editProxyModel->columnCount();col++)
                {
                    setCurrentIndex(editProxyModel->index(foundRow,col));
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


void FEditTableView::onInChanged(int row, int in)
{
    QTime newInTime = QTime::fromMSecsSinceStartOfDay(in);

    for (int i=0;i<editItemModel->rowCount();i++)
    {
        int editCounter = editItemModel->index(i,orderBeforeLoadIndex).data().toInt();
        if (editCounter == row)
        {
            QTime outTime = QTime::fromString(editItemModel->index(i,outIndex).data().toString(),"HH:mm:ss.zzz");
//            qDebug()<<"FEditTableView::onInChanged"<<i<< in<<newInTime;
            editItemModel->item(i, inIndex)->setData(newInTime.toString("HH:mm:ss.zzz"),Qt::DisplayRole);
            editItemModel->item(i, durationIndex)->setData(QTime::fromMSecsSinceStartOfDay(newInTime.msecsTo(outTime) + 1000 / frameRate).toString("HH:mm:ss.zzz"),Qt::DisplayRole);
        //            setCurrentIndex(editItemModel->index(i,inIndex));
            emit editsChangedFromVideo(editProxyModel);

        }
    }


}

void FEditTableView::onOutChanged(int row, int out)
{
    QTime newOutTime = QTime::fromMSecsSinceStartOfDay(out);
    for (int i=0;i<editItemModel->rowCount();i++)
    {
        int editCounter = editItemModel->index(i,orderBeforeLoadIndex).data().toInt();
        if (editCounter == row)
        {
            QTime inTime = QTime::fromString(editItemModel->index(i,inIndex).data().toString(),"HH:mm:ss.zzz");
//            qDebug()<<"FEditTableView::onOutChanged"<<i<< out<<newOutTime;
            editItemModel->item(i, outIndex)->setData(newOutTime.toString("HH:mm:ss.zzz"),Qt::DisplayRole);
            editItemModel->item(i, durationIndex)->setData(QTime::fromMSecsSinceStartOfDay(inTime.msecsTo(newOutTime) + 1000 / frameRate).toString("HH:mm:ss.zzz"),Qt::DisplayRole);
        //            setCurrentIndex(editItemModel->index(i,inIndex));
            emit editsChangedFromVideo(editProxyModel);
        }
    }

}

QStandardItemModel* FEditTableView::read(QString folderName, QString fileName)
{
    qDebug()<<"FEditTableView::read"<<folderName << fileName;
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

            start = srtContentString.indexOf("<s>");
            end = srtContentString.indexOf("</s>");
            if (start >= 0 && end >= 0)
                value = srtContentString.mid(start+3, end - start - 3);
            else
                value = "";
            QStandardItem *starItem = new QStandardItem;
            starItem->setData(QVariant::fromValue(FStarRating(value.toInt())), Qt::EditRole);

            start = srtContentString.indexOf("<r>");
            end = srtContentString.indexOf("</r>");
            if (start >= 0 && end >= 0)
                value = srtContentString.mid(start+3, end - start - 3);
            else
                value = "";
            QString repeat = value;

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
                order = "500";
                int indexOfO = tags.indexOf(" o");
                if (indexOfO >= 0)
                    order = tags.mid(indexOfO+2, 3);

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

            QList<QStandardItem *> items;
            items.append(new QStandardItem(QString::number(editCounter)));
            items.append(new QStandardItem(order));
            items.append(new QStandardItem(order));
            items.append(new QStandardItem( folderName));
            items.append(new QStandardItem( fileName));
            items.append(new QStandardItem(""));
            items.append(new QStandardItem(inTime.toString("HH:mm:ss.zzz")));
            items.append(new QStandardItem(outTime.toString("HH:mm:ss.zzz")));
            items.append(new QStandardItem(QTime::fromMSecsSinceStartOfDay(inTime.msecsTo(outTime)+1000 / frameRate).toString("HH:mm:ss.zzz"))); //durationIndex
            items.append(starItem);
            items.append(new QStandardItem(repeat));
            items.append(new QStandardItem(hint));
            items.append(new QStandardItem(tags));
            srtItemModel->appendRow(items);
//                QLineEdit *edit = new QLineEdit(this);
            editCounter++;
        }
    }

    return srtItemModel;
} //read

void FEditTableView::scanDir(QDir dir)
{
    dir.setNameFilters(QStringList("*.mp4"));
    dir.setFilter(QDir::Files | QDir::NoDotAndDotDot | QDir::NoSymLinks);

    qDebug() << "FEditTableView::scanDir" << dir.path();

    QStringList fileList = dir.entryList();
    for (int i=0; i<fileList.count(); i++)
    {
        QStandardItemModel *srtItemModel = read(dir.path() + "/", fileList[i]);
        while (srtItemModel->rowCount()>0)
            editItemModel->appendRow(srtItemModel->takeRow(0));
    }

    dir.setFilter(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::NoSymLinks);
    QStringList dirList = dir.entryList();
    for (int i=0; i<dirList.size(); ++i)
    {
        QString newPath = QString("%1/%2").arg(dir.absolutePath()).arg(dirList.at(i));
        scanDir(QDir(newPath));
    }
}

void FEditTableView::loadModel(QString folderName)
{
    editCounter = 1;
    editItemModel->removeRows(0, editItemModel->rowCount());
    scanDir(QDir(folderName));
}

void FEditTableView::saveModel()
{
    qDebug()<<"FEditTableView::saveModel"<<editItemModel->rowCount();

    QMap<QString, QMap<QString, int>> editMap;
    for (int row = 0; row<editItemModel->rowCount();row++)
    {
        QString folderName = editItemModel->index(row, folderIndex).data().toString();
        QString fileName = editItemModel->index(row, fileIndex).data().toString();
        QString inTime = editItemModel->index(row, inIndex).data().toString();
        editMap[fileName][inTime] = row;
    }

    QString previousFileName = "";
    QFile file;

    QMapIterator<QString, QMap<QString, int>> fileIterator(editMap);
    while (fileIterator.hasNext()) //all files
    {
        fileIterator.next();
        qDebug()<<"File"<<fileIterator.key();
        int editPerFileCounter = 0;

        QMapIterator<QString, int> inIterator(fileIterator.value());
        while (inIterator.hasNext()) //all files
        {
            inIterator.next();

            int row = inIterator.value();

            QString folderName = editItemModel->index(row, folderIndex).data().toString();
            QString fileName = editItemModel->index(row, fileIndex).data().toString();

            if (fileName != previousFileName)
            {
                editPerFileCounter = 1;
                qDebug()<<"FEditTableView::saveModel"<<fileName;
                int lastIndex = fileName.lastIndexOf(".");

                if (file.isOpen())
                    file.close();

                file.setFileName(folderName + fileName.left(lastIndex) + ".srt");
    //            file = file (folderName + fileName.left(lastIndex) + ".srt");
                file.open(QIODevice::WriteOnly);
    //            stream.setDevice(QIODevice(file));
            }

            QTextStream stream(&file);

            FStarRating starRating = qvariant_cast<FStarRating>(editItemModel->index(row, ratingIndex).data());

            qDebug()<<"  FEditTableView::saveModel"<<editItemModel->index(row, inIndex).data().toString()<<starRating.starCount()<<editItemModel->index(row, orderAtLoadIndex).data().toString()<<editItemModel->index(row, orderAfterMovingIndex).data().toString();
    //        qDebug()<<"editItemModel->index(i,inIndex).data().toString()"<<editItemModel->index(i,inIndex).data().toString();

            QString srtContentString = "";
            srtContentString += "<o>" + editItemModel->index(row, orderAfterMovingIndex).data().toString() + "</o>";
            srtContentString += "<s>" + QString::number(starRating.starCount()) + "</s>";
            srtContentString += "<r>" + editItemModel->index(row,repeatIndex).data().toString() + "</r>";
            srtContentString += "<h>" + editItemModel->index(row, FGlobal().hintIndex).data().toString() + "</h>";
            srtContentString += "<t>" + editItemModel->index(row,tagIndex).data().toString() + "</t>";

            stream << editPerFileCounter << endl;
            stream << editItemModel->index(row,inIndex).data().toString() << " --> " << editItemModel->index(row,outIndex).data().toString() << endl;
            stream << srtContentString << endl;
            stream << endl;
            previousFileName = fileName;
            editPerFileCounter++;
        }
    }
    file.close();

}

void FEditTableView::onEditFilterChanged(FStarEditor *starEditorFilterWidget, QListView *tagFilter1ListView, QListView *tagFilter2ListView )
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

    QString regExp = QString::number(starEditorFilterWidget->starRating().starCount()) + ";" + string1 + ";" + string2;
    qDebug()<<"FEditTableView::onEditFilterChanged"<<starEditorFilterWidget->starRating().starCount()<<tagFilter1ListView->model()->rowCount()<<tagFilter2ListView->model()->rowCount()<<regExp;

    editProxyModel->setFilterRegExp(QRegExp(regExp, Qt::CaseInsensitive,
                                                QRegExp::FixedString));
    editProxyModel->setFilterKeyColumn(-1);

//    qDebug()<<"FEditTableView::onEditFilterChanged"<<starEditorFilterWidget->starRating().starCount();

    emit editsChanged(editProxyModel);

}

void FEditTableView::giveStars(int starCount)
{
    qDebug()<<"FEditTableView::onGiveStars"<<starCount<<currentIndex();
    editProxyModel->setData(editProxyModel->index(currentIndex().row(), ratingIndex), QVariant::fromValue(FStarRating(starCount)));
}

void FEditTableView::toggleRepeat()
{
    editProxyModel->setData(editProxyModel->index(currentIndex().row(), repeatIndex), !editProxyModel->index(currentIndex().row(), repeatIndex).data().toBool());
}


