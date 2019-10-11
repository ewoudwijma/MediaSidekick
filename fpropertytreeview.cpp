#include "fpropertyitemdelegate.h"
#include "fpropertytreeview.h"

#include <QHeaderView>
#include <QDebug>
#include <QSettings>
#include <QUrl>
#include <QScrollBar>
#include "fglobal.h"
#include <QDateTime>

static const int deltaIndex = 2;
static const int firstFileColumnIndex = 3;

FPropertyTreeView::FPropertyTreeView(QWidget *parent) : QTreeView(parent)
{
    propertyItemModel = new QStandardItemModel(this);
    QStringList labels;
    labels << "Property" << "Type" << "Diff";
    propertyItemModel->setHorizontalHeaderLabels(labels);

    propertyProxyModel = new FPropertySortFilterProxyModel(this);

    propertyProxyModel->setSourceModel(propertyItemModel);

    setModel(propertyProxyModel);

    header()->setSectionResizeMode(QHeaderView::Interactive);
//    header()->setSectionsMovable(true);
//    header()->setSectionsClickable(true);
//    horizontalHeader()->stretchSectionCount();
//    setSortingEnabled(true);
    setItemsExpandable(false);
    setColumnWidth(0,columnWidth(0) * 2);

    FPropertyItemDelegate *propertyItemDelegate = new FPropertyItemDelegate(this);
    setItemDelegate(propertyItemDelegate);
    setColumnHidden(1, true); //type column
    setColumnHidden(2, true); //diffcolumn

    setSelectionMode(QAbstractItemView::MultiSelection);

    frozenTableView = new QTreeView(this);

    init();

    //connect the headers and scrollbars of both tableviews together
    connect(header(),&QHeaderView::sectionResized, this,
            &FPropertyTreeView::updateSectionWidth);
//      connect(verticalHeader(),&QHeaderView::sectionResized, this,
//              &FreezeTableWidget::updateSectionHeight);

    connect(frozenTableView->verticalScrollBar(), &QAbstractSlider::valueChanged,
            verticalScrollBar(), &QAbstractSlider::setValue);
    connect(verticalScrollBar(), &QAbstractSlider::valueChanged,
            frozenTableView->verticalScrollBar(), &QAbstractSlider::setValue);

    connect(propertyItemModel, &QStandardItemModel::itemChanged, this, &FPropertyTreeView::onPropertyChanged);
    isLoading = false;

    processManager = new FProcessManager(this);

//    QString lastFolder = QSettings().value("LastFolder").toString();
//    if (lastFolder != ""  && lastFolder.length()>3) //not the root folder
//    {
//        loadModel(QSettings().value("LastFolder").toString());
//    }
}

FPropertyTreeView::~FPropertyTreeView()
{
      delete frozenTableView;
}

void FPropertyTreeView::onFolderIndexClicked(QModelIndex index)
{
    QString lastFolder = QSettings().value("LastFolder").toString();
//    qDebug()<<"FPropertyTreeView::onFolderIndexClicked"<<index.data().toString()<<lastFolder;
    loadModel(lastFolder);
}

void FPropertyTreeView::setCellStyle(QStringList fileNames)
{
//    qDebug()<<"FPropertyTreeView::setCellStyle"<<fileNames.count()<<propertyProxyModel->rowCount();
    isLoading = true;
    QFont boldFont;
    boldFont.setBold(true);
    for (int parentRow=0; parentRow<propertyProxyModel->rowCount(); parentRow++)
    {
        QModelIndex parentIndex = propertyProxyModel->index(parentRow, 0);
//        propertyProxyModel->setData(parentIndex, boldFont, Qt::FontRole);

//        qDebug()<<"FPropertyTreeView::setCellStyle"<<propertyProxyModel->rowCount(parentIndex)<<propertyProxyModel->columnCount(parentIndex);
        for (int childRow=0; childRow<propertyProxyModel->rowCount(parentIndex); childRow++)
        {
            for (int childColumn=0; childColumn<propertyProxyModel->columnCount(parentIndex);childColumn++)
            {
                QModelIndex childIndex = propertyProxyModel->index(childRow, childColumn, parentIndex);
                QVariant hItem = propertyItemModel->headerData(childColumn, Qt::Horizontal);

                if (propertyProxyModel->index(childRow, deltaIndex, parentIndex).data().toBool() && childColumn == 0) //&& childIndex.data(Qt::FontRole) != boldFont)
                    propertyProxyModel->setData(childIndex, boldFont, Qt::FontRole);
                if (fileNames.contains(hItem.toString()))
                    propertyProxyModel->setData(childIndex,  QVariant(palette().highlight()) , Qt::BackgroundRole);
                else if (childIndex.data(Qt::BackgroundRole) == QVariant(palette().highlight()))
                    propertyProxyModel->setData(childIndex,  QVariant(palette().base()) , Qt::BackgroundRole);
            }
        }
    }
    isLoading = false;

    //        propertyTreeView->scrollTo(propertyTreeView->propertyItemModel->index(3,propertyTreeView->propertyItemModel->columnCount()-1), QAbstractItemView::EnsureVisible);
    //    scrollTo(modelIndex);

}

void FPropertyTreeView::onFileIndexClicked(QModelIndex index, QModelIndexList selectedIndices)
{
    QStringList selectedFileNames;
    for (int i=0;i<selectedIndices.count();i++)
    {
        QModelIndex index = selectedIndices[i];
        if (index.column() == 0)
        {
            selectedFileNames << index.data().toString();
        }
    }
    qDebug()<<"FPropertyTreeView::onFileIndexClicked"<<index.data().toString()<<selectedIndices.count()<<selectedFileNames.count();
//    QModelIndexList *selectedIndices = index.model().selectionModel->sele;
//    QModelIndex modelIndex = propertyProxyModel->index(propertyProxyModel->rowCount(),0);
    setCellStyle(selectedFileNames);

}

void FPropertyTreeView::onEditIndexClicked(QModelIndex index)
{
//    qDebug()<<"FFilesTreeView::onEditIndexClicked"<<index;
    QString fileName = index.model()->index(index.row(),fileIndex).data().toString();
    QStringList selectedFileNames;
    selectedFileNames <<fileName;
    setCellStyle(selectedFileNames);
}

void FPropertyTreeView::loadModel(QString folderName)
{
    propertyItemModel->removeRows(0, propertyItemModel->rowCount());
    while (propertyItemModel->columnCount()>firstFileColumnIndex) //remove old columns
        propertyItemModel->removeColumn(propertyItemModel->columnCount()-1);

    QString command = "exiftool -s -c \"%02.6f\" \"" + folderName.replace("/", "//") + "*\""; ////
//    qDebug()<<"FPropertyTreeView::loadModel"<<folderName<<command<<processManager;

    QMap<QString, QString> parameters;
    parameters["folderName"] = folderName;
    emit addLogEntry("PropertyLoad " + folderName);
    isLoading = true;
    processManager->startProcess(command, parameters
                                   , [] (QWidget *parent, QMap<QString, QString> parameters, QString result)
    {
        FPropertyTreeView *propertyTreeView = qobject_cast<FPropertyTreeView *>(parent);
        emit propertyTreeView->addLogToEntry("PropertyLoad " + parameters["folderName"], result);
    }
                                   , [] (QWidget *parent, QString , QMap<QString, QString> parameters, QStringList result)
     {
        FPropertyTreeView *propertyTreeView = qobject_cast<FPropertyTreeView *>(parent);

        //create topLevelItems
        QStringList topLevelItemNames;
        topLevelItemNames << "General" << "Stream" << "Video" << "Media" << "File" << "Date" << "Audio" << "Other";
        QMap<QString, QStandardItem *> topLevelItems;
        for (int i=0; i<topLevelItemNames.count();i++)
        {
            QStandardItem *topLevelItem =  new QStandardItem( topLevelItemNames[i] );
            topLevelItem->setEditable(false);
            propertyTreeView->propertyItemModel->setItem(i,0,topLevelItem);
            topLevelItems[topLevelItemNames[i]] = topLevelItem;
        }

//        qDebug()<<"loadModel2"<<parent<<result.last();
        QString folderFileName;
        QUrl folderFile;

        QMap<QString, QMap<QString, QString>> valueMap;
        QMap<QString, QString> fileMediaMap;
        QMap<QString, QStandardItem *> labelMap;

        for (int resultIndex=0;resultIndex<result.count();resultIndex++)
        {
//            emit propertyTreeView->addLogToEntry("PropertyLoad " + parameters["folderName"], result[resultIndex]);
            int indexOf = result[resultIndex].indexOf("======== "); //next file
            if (indexOf > -1)
            {
                folderFileName = result[resultIndex].mid(indexOf+9);
                folderFile = QUrl(folderFileName);
//                qDebug()<<folderFile.fileName();
            }
            else {
                int pos = result[resultIndex].indexOf(":");
                QString labelString = result[resultIndex].left(pos).trimmed();
                QString valueString = result[resultIndex].mid(pos+1).trimmed();

                if (labelString == "MIMEType" && (valueString.contains("video") || valueString.contains("audio")))
                    fileMediaMap[folderFile.fileName()] = folderFile.fileName();

                valueMap[labelString][folderFile.fileName()] = valueString;

                if (labelString == "FileName" || labelString == "GeneratedName" || labelString == "CreateDate" || labelString == "GPSLatitude" || labelString == "GPSLongitude" || labelString == "GPSAltitude" || labelString == "Make" || labelString == "Model" || labelString == "Location")
                    labelMap[labelString] = topLevelItems["General"];
                else if (labelString == "ImageWidth" || labelString == "ImageHeight" || labelString == "Duration" || labelString == "TrackDuration")
                    labelMap[labelString] = topLevelItems["Stream"];
                else if (labelString == "CompressorID" || labelString == "ImageWidth" || labelString == "ImageHeight" || labelString == "VideoFrameRate" || labelString == "AvgBitrate" || labelString == "BitDepth" )
                    labelMap[labelString] = topLevelItems["Video"];
                else if (labelString.contains("Duration") || labelString.contains("Image") || labelString.contains("Video") || labelString.contains("Compressor") || labelString.contains("AvgBitrate") )
                    labelMap[labelString] = topLevelItems["Media"];
                else if (labelString.contains("File") || labelString.contains("Directory"))
                    labelMap[labelString] = topLevelItems["File"];
                else if (labelString.contains("Date"))
                    labelMap[labelString] = topLevelItems["Date"];
                else if (labelString.contains("Audio"))
                    labelMap[labelString] = topLevelItems["Audio"];
                else
                    labelMap[labelString] = topLevelItems["Other"];
            }
        }
        labelMap["FileName"] = topLevelItems["General"];
        labelMap["GeneratedName"] = topLevelItems["General"];
        labelMap["GPSLatitude"] = topLevelItems["General"];
        labelMap["GPSLongitude"] = topLevelItems["General"];
        labelMap["Make"] = topLevelItems["General"];
        labelMap["Model"] = topLevelItems["General"];

        QMapIterator<QString, QString> iFile(fileMediaMap);
        QStringList labels;
        labels << "Property" << "Type" << "Diff";

        while (iFile.hasNext()) //all files
        {
            iFile.next();
            labels<<iFile.key();
//            emit propertyTreeView->addLogToEntry("PropertyLoad " + parameters["folderName"], iFile.key() + "\n");
        }
        propertyTreeView->propertyItemModel->setHorizontalHeaderLabels(labels);

//        for (int i=firstFileColumnIndex; i< propertyTreeView->propertyItemModel->columnCount(); i++)
//        {
//            propertyTreeView->setColumnWidth(i, propertyTreeView->columnWidth(i) * 2);
//        }

        QMapIterator<QString, QStandardItem *> iLabel(labelMap);
        while (iLabel.hasNext()) //all labels
        {
            iLabel.next();
//            qDebug()<< iLabel.key() << ": " << iLabel.value();

            QList<QStandardItem *> sublevelItems;
            QStandardItem *item;
            item = new QStandardItem( iLabel.key() );
            item->setEditable(false);
        //    sublevelItems.clear();
            sublevelItems.append(item);
            if (iLabel.key() == "CreateDate")
                sublevelItems.append(new QStandardItem( "QDateTime" ));
            else
                sublevelItems.append(new QStandardItem( "QString" ));
            sublevelItems.append(new QStandardItem( "false" )); //diff

            bool valueFound = false; //in case label is added by a non media file
            QString previousValue = "";
            QMapIterator<QString, QString> iFile(fileMediaMap);
            while (iFile.hasNext()) //all files
            {
                iFile.next();
                QString value = valueMap[iLabel.key()][iFile.key()];
                QStandardItem *item;
                item = new QStandardItem( value );
                item->setEditable(iLabel.value() == topLevelItems["General"]); //only general labels are editable.
                sublevelItems.append(item);
                valueFound = valueFound || value != "";
                if (previousValue != "" && value != "" && value != previousValue) //diff
                    sublevelItems[2] = new QStandardItem( "true" );
                previousValue = value;
//                qDebug()<< iFile.key() << ": " << iFile.value();
            }
            if (true)
                iLabel.value()->appendRow(sublevelItems);
//            propertyTreeView->setCurrentIndex(item->index());
//            lastIndex = item->index();
        } // all labels

//        QMapIterator<QString, QString> iFile2(fileMediaMap);
//        while (iFile2.hasNext()) //all files
//        {
//            iFile2.next();

//        }

        for (int col = 1; col < propertyTreeView->model()->columnCount(); ++col)
        {
              propertyTreeView->frozenTableView->setColumnHidden(col, true);
              if (col > 2)
              {
                  QStandardItem *item = topLevelItems["General"];
                  QModelIndex parentIndex = propertyTreeView->propertyItemModel->indexFromItem(item);
                  QModelIndex childIndex = parentIndex.model()->index(0,col,parentIndex); //0 is first child
                  propertyTreeView->updateGeneratedName(childIndex);
              }
        }

        propertyTreeView->propertyProxyModel->setFilterRegExp(QRegExp(";1", Qt::CaseInsensitive,
                                                    QRegExp::FixedString));
        propertyTreeView->propertyProxyModel->setFilterKeyColumn(-1);

        propertyTreeView->expandAll();
        propertyTreeView->frozenTableView->expandAll();

        propertyTreeView->setCellStyle(QStringList()); //to set diff values
        propertyTreeView->isLoading = false;
        emit propertyTreeView->propertiesLoaded();
    });
}

//! [init part1]
void FPropertyTreeView::init()
{
      frozenTableView->setModel(model());
      frozenTableView->setFocusPolicy(Qt::NoFocus);
//      frozenTableView->header()->hide();
      frozenTableView->header()->setSectionResizeMode(QHeaderView::Fixed);
      frozenTableView->setItemsExpandable(false);

      viewport()->stackUnder(frozenTableView);
//! [init part1]

//! [init part2]
//      frozenTableView->setStyleSheet("QTreeView { border: none;"
//                                     "background-color: #8EDE21;"
//                                     "selection-background-color: #999}"); //for demo purposes
      frozenTableView->setSelectionModel(selectionModel());
      for (int col = 1; col < model()->columnCount(); ++col)
            frozenTableView->setColumnHidden(col, true);

      frozenTableView->setColumnWidth(0, columnWidth(0) );
//      frozenTableView->setColumnWidth(1, columnWidth(1) );

      frozenTableView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
      frozenTableView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
      frozenTableView->show();

      updateFrozenTableGeometry();

      setHorizontalScrollMode(ScrollPerPixel);
      setVerticalScrollMode(ScrollPerPixel);
      frozenTableView->setVerticalScrollMode(ScrollPerPixel);

}
//! [init part2]


//! [sections]
void FPropertyTreeView::updateSectionWidth(int logicalIndex, int /* oldSize */, int newSize)
{
      if (logicalIndex == 0){
            frozenTableView->setColumnWidth(logicalIndex, newSize);
            updateFrozenTableGeometry();
      }
}

void FPropertyTreeView::updateSectionHeight(int logicalIndex, int , int newSize)
{
//      frozenTableView->setRowHeight(logicalIndex, newSize);
}
//! [sections]


//! [resize]
void FPropertyTreeView::resizeEvent(QResizeEvent * event)
{
      QTreeView::resizeEvent(event);
      updateFrozenTableGeometry();
 }
//! [resize]


//! [navigate]
QModelIndex FPropertyTreeView::moveCursor(CursorAction cursorAction,
                                          Qt::KeyboardModifiers modifiers)
{
      QModelIndex current = QTreeView::moveCursor(cursorAction, modifiers);

      if (cursorAction == MoveLeft && current.column() > 0
              && visualRect(current).topLeft().x() < frozenTableView->columnWidth(0) ){
            const int newValue = horizontalScrollBar()->value() + visualRect(current).topLeft().x()
                                 - frozenTableView->columnWidth(0);
            horizontalScrollBar()->setValue(newValue);
      }
      return current;
}
//! [navigate]

void FPropertyTreeView::scrollTo (const QModelIndex & index, ScrollHint hint){
    if (index.column() > 0)
        QTreeView::scrollTo(index, hint);
}

//! [geometry]
void FPropertyTreeView::updateFrozenTableGeometry()
{
      frozenTableView->setGeometry(frameWidth(),
                                   frameWidth(), columnWidth(0),
                                   viewport()->height()+header()->height());
}
//! [geometry]

void FPropertyTreeView::onPropertyFilterChanged(QLineEdit *propertyFilterLineEdit, QCheckBox *propertyDiffCheckBox)
{
//    qDebug()<<"FPropertyTreeView::onPropertyFilterChanged"<<propertyFilterLineEdit->text()<<propertyDiffCheckBox->checkState();
    QString diffString;
    if (propertyDiffCheckBox->checkState() == Qt::Unchecked)
        diffString = "0";
    else if (propertyDiffCheckBox->checkState() == Qt::Checked)
        diffString = "2";
    else
        diffString = "1";

//    qDebug()<<"on_propertySearchLineEdit_textChanged"<<arg1<<ui->diffOnlyCheckBox->checkState();
    propertyProxyModel->setFilterRegExp(QRegExp(propertyFilterLineEdit->text()+";"+diffString, Qt::CaseInsensitive,
                                                QRegExp::FixedString));
    propertyProxyModel->setFilterKeyColumn(-1);

    expandAll();
    frozenTableView->expandAll();
}

void FPropertyTreeView::onGetPropertyValue(QString fileName, QString key, QString *value)
{
    *value = "testValue" + fileName + key;
    //get column / file value
    int fileColumnNr = -1;
    for(int col = 0; col < propertyItemModel->columnCount(); col++)
    {
      if (propertyItemModel->headerData(col, Qt::Horizontal).toString() == fileName)
      {
          fileColumnNr = col;
      }
    }
//    qDebug()<<"FPropertyTreeView::onGetPropertyValue"<<fileName<<fileColumnNr<<key<<propertyItemModel->rowCount();

    if (fileColumnNr != -1)
    {
        //get row/ item value
        for(int rowIndex = 0; rowIndex < propertyItemModel->rowCount(); rowIndex++)
        {
            QModelIndex topLevelIndex = propertyItemModel->index(rowIndex,0);

            for (int childRowIndex=0; childRowIndex<propertyItemModel->rowCount(topLevelIndex); childRowIndex++)
            {
                QModelIndex sublevelIndex = propertyItemModel->index(childRowIndex,0, topLevelIndex);

                if (sublevelIndex.data().toString() == key)
                {
                    *value = propertyItemModel->index(childRowIndex, fileColumnNr, topLevelIndex).data().toString();
//                    qDebug()<<  "FPropertyTreeView::onGetPropertyValue"<<topLevelIndex<<childRowIndex<<sublevelIndex.data().toString()<<*value;
                    return;
                }
            }

        }
        *value = "";
    }
    else
        *value = "";
}

void FPropertyTreeView::updateGeneratedName(QModelIndex index)
{
    QString fileName = index.model()->headerData(index.column(), Qt::Horizontal).toString();
    {
        QString generatedName = "";
        QString sep = "";
        for (int row= 0; row<=7; row++)
        {
            QString genValue = index.model()->index(row,index.column(),index.parent()).data().toString();
//            qDebug()<<row<<"genValue1"<<genValue;
            if (row == 0) //createdate
            {
//                genValue = genValue.replace(":", "-");
                QDateTime createDateTime = QDateTime::fromString(genValue, "yyyy:MM:dd HH:mm:ss");
//                qDebug()<<"genValue"<<genValue<<createDateTime<<createDateTime.toString("yyyyMMdd HHmmss");
                genValue = createDateTime.toString("yyyy-MM-dd HH.mm.ss");
            }
            else if (row == 2 || row == 3) //latlon
                genValue = genValue.left(4);
//            qDebug()<<row<<"genValue2"<<genValue;

            if (row!=1 && row!= 4 && row!= 5) //not filename or model or generatedname itself
            {
                if (genValue != "")
                {
                    generatedName += sep + genValue;
                    sep = " ";
                }
            }
        }

        int pos = fileName.indexOf("+"); //first +
        int extPos = fileName.lastIndexOf(".");
        QString msString = "";
        if (pos > 0)
        {
            msString = fileName.mid(pos, extPos - pos);
//            qDebug()<<"msString"<<msString;
            generatedName += " " + msString;
        }

        QModelIndex generatedNameIndex = index.model()->index(4,index.column(),index.parent());
        propertyItemModel->setData(generatedNameIndex, generatedName, Qt::EditRole);
    }
}

void FPropertyTreeView::onPropertyChanged(QStandardItem *item)
{
    QModelIndex index = item->index();
    QString key = item->model()->index(item->row(),0,index.parent()).data().toString();

    if (key == "CreateDate" || key == "GPSLongitude" || key == "GPSLatitude" || key == "GPSAltitude" || key == "Make" || key == "Model")
    {
        if (index.column() > 2) //not for label, type and diff column
            updateGeneratedName(index);

        QString fileName = index.model()->headerData(index.column(), Qt::Horizontal).toString();


        if (!isLoading && fileName != "")
        {
            QVariant value = item->index().data();

//            metadata->diffData(QModelIndex());
            qDebug()<<"onPropertyChanged"<<item->row()<<item->column()<<key<<value<<fileName<<index.data(Qt::EditRole)<<index.data(Qt::DisplayRole)<<isLoading;

            if (value != "")
            {
                emit fileDelete(fileName); //to stop the video to free the resource in windows/os

                QString valueString = value.toString();
                if (key == "CreateDate")
                    valueString = "\"" + value.toDateTime().toString("yyyy-MM-dd HH:mm:ss") + "\"";
                else if (key == "GPSAltitude")
                {
                    qDebug()<<"GPSAltitude"<<value<<value.toDouble();
                    if (valueString.right(2) == " m")
                        valueString = valueString.left(valueString.length()-2);
                    if (valueString.left(1) == "-")
                        valueString += " -GPSAltitudeRef=\"Below Sea Level\"";
                    else
                        valueString += " -GPSAltitudeRef=\"Above Sea Level\"";
                }
                else {
                    valueString = "\"" + valueString + "\"";
                }

                QString *directoryString  = new QString();
                onGetPropertyValue(index.model()->headerData(index.column(), Qt::Horizontal).toString(), "Directory", directoryString);
//                ui->statusBar->showMessage("Update metadata for " + directory.toString() + "//" + fileName + "...");
                emit addLogEntry("Update metadata " + fileName);

                QMap<QString, QString> parameters;
                parameters["fileName"] = fileName;
                QString code = "exiftool -" + key + "=" + valueString + "  -overwrite_original \"" + *directoryString + "//" + fileName + "\"";

                processManager->startProcess(code, parameters, nullptr, [] (QWidget *parent, QString, QMap<QString, QString> parameters, QStringList result)
                {
                    FPropertyTreeView *propertyTreeView = qobject_cast<FPropertyTreeView *>(parent);
                    emit propertyTreeView->addLogToEntry("Update metadata " + parameters["fileName"], result.join("\n"));
                });
            }
        }
    }
} //on onPropertyChanged

void FPropertyTreeView::onFileDelete(QString fileName)
{
    qDebug()<<"FPropertyTreeView::onFileDelete"<<fileName;

    for (int column=0; column<propertyItemModel->columnCount();column++)
    {
        if (propertyItemModel->headerData(column,Qt::Horizontal).toString() == fileName)
            propertyItemModel->takeColumn(column);
    }

}

void FPropertyTreeView::onFileRename()
{
    QString lastFolder = QSettings().value("LastFolder").toString();
    qDebug()<<"FPropertyTreeView::onFileRename"<<lastFolder;
    loadModel(lastFolder);
}

void FPropertyTreeView::onTrim()
{
    QString lastFolder = QSettings().value("LastFolder").toString();
    qDebug()<<"FPropertyTreeView::onFileRename"<<lastFolder;
    loadModel(lastFolder);
}
