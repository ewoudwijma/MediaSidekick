#include "apropertyitemdelegate.h"
#include "apropertytreeview.h"

#include <QHeaderView>
#include <QDebug>
#include <QSettings>
#include <QUrl>
#include <QScrollBar>
#include "aglobal.h"
#include <QDateTime>

static const int deltaIndex = 2;
static const int firstFileColumnIndex = 3;

APropertyTreeView::APropertyTreeView(QWidget *parent) : QTreeView(parent)
{
    propertyItemModel = new QStandardItemModel(this);
    QStringList labels;
    labels << "Property" << "Type" << "Diff";
    propertyItemModel->setHorizontalHeaderLabels(labels);

    propertyProxyModel = new APropertySortFilterProxyModel(this);

    propertyProxyModel->setSourceModel(propertyItemModel);

    setModel(propertyProxyModel);

    header()->setSectionResizeMode(QHeaderView::Interactive);
//    header()->setSectionsMovable(true);
//    header()->setSectionsClickable(true);
//    horizontalHeader()->stretchSectionCount();
//    setSortingEnabled(true);
    setItemsExpandable(false);
    setColumnWidth(0,columnWidth(0) * 2);

    APropertyItemDelegate *propertyItemDelegate = new APropertyItemDelegate(this);
    setItemDelegate(propertyItemDelegate);
    setColumnHidden(1, true); //type column
    setColumnHidden(2, true); //diffcolumn

    setSelectionMode(QAbstractItemView::MultiSelection);

    frozenTableView = new QTreeView(this);

    init();

    //connect the headers and scrollbars of both tableviews together
    connect(header(),&QHeaderView::sectionResized, this,
            &APropertyTreeView::updateSectionWidth);
//      connect(verticalHeader(),&QHeaderView::sectionResized, this,
//              &FreezeTableWidget::updateSectionHeight);

    connect(frozenTableView->verticalScrollBar(), &QAbstractSlider::valueChanged,
            verticalScrollBar(), &QAbstractSlider::setValue);
    connect(verticalScrollBar(), &QAbstractSlider::valueChanged,
            frozenTableView->verticalScrollBar(), &QAbstractSlider::setValue);

    connect(propertyItemModel, &QStandardItemModel::itemChanged, this, &APropertyTreeView::onPropertyChanged);
    isLoading = false;

    processManager = new AProcessManager(this);

//    QString lastFolder = QSettings().value("LastFolder").toString();
//    if (lastFolder != ""  && lastFolder.length()>3) //not the root folder
//    {
//        loadModel(QSettings().value("LastFolder").toString());
//    }
}

APropertyTreeView::~APropertyTreeView()
{
      delete frozenTableView;
}

void APropertyTreeView::onFolderIndexClicked(QModelIndex index)
{
    QString lastFolder = QSettings().value("LastFolder").toString();
//    qDebug()<<"APropertyTreeView::onFolderIndexClicked"<<index.data().toString()<<lastFolder;
    loadModel(lastFolder);
}

void APropertyTreeView::setCellStyle(QStringList fileNames)
{
//    qDebug()<<"APropertyTreeView::setCellStyle"<<fileNames.count()<<propertyProxyModel->rowCount();
    isLoading = true;
    QFont boldFont;
    boldFont.setBold(true);
    for (int parentRow=0; parentRow<propertyProxyModel->rowCount(); parentRow++)
    {
        QModelIndex parentIndex = propertyProxyModel->index(parentRow, 0);
//        propertyProxyModel->setData(parentIndex, boldFont, Qt::FontRole);

//        qDebug()<<"APropertyTreeView::setCellStyle"<<propertyProxyModel->rowCount(parentIndex)<<propertyProxyModel->columnCount(parentIndex);
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

void APropertyTreeView::onFileIndexClicked(QModelIndex index, QModelIndexList selectedIndices)
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
//    qDebug()<<"APropertyTreeView::onFileIndexClicked"<<index.data().toString()<<selectedIndices.count()<<selectedFileNames.count();
//    QModelIndexList *selectedIndices = index.model().selectionModel->sele;
//    QModelIndex modelIndex = propertyProxyModel->index(propertyProxyModel->rowCount(),0);
    setCellStyle(selectedFileNames);

}

void APropertyTreeView::onClipIndexClicked(QModelIndex index)
{
//    qDebug()<<"AFilesTreeView::onClipIndexClicked"<<index;
    QString fileName = index.model()->index(index.row(),fileIndex).data().toString();
    QStringList selectedFileNames;
    selectedFileNames <<fileName;
    setCellStyle(selectedFileNames);
}

void APropertyTreeView::loadModel(QString folderName)
{
//    qDebug() << "APropertyTreeView::loadModel" << folderName;
    propertyItemModel->removeRows(0, propertyItemModel->rowCount());
    while (propertyItemModel->columnCount()>firstFileColumnIndex) //remove old columns
        propertyItemModel->removeColumn(propertyItemModel->columnCount()-1);

    QString command = "exiftool -s -c \"%02.6f\" \"" + folderName + "*\""; ////
//    qDebug()<<"APropertyTreeView::loadModel"<<folderName<<command<<processManager;

    QMap<QString, QString> parameters;

    QString *processId = new QString();
    emit addLogEntry(folderName, "All", "Property load", processId);
    parameters["processId"] = *processId;

    isLoading = true;
    processManager->startProcess(command, parameters
                                   , [] (QWidget *parent, QMap<QString, QString> parameters, QString result)
    {
        APropertyTreeView *propertyTreeView = qobject_cast<APropertyTreeView *>(parent);
        emit propertyTreeView->addLogToEntry(parameters["processId"], result);
    }
                                   , [] (QWidget *parent, QString , QMap<QString, QString> parameters, QStringList result)
     {
        APropertyTreeView *propertyTreeView = qobject_cast<APropertyTreeView *>(parent);

        //create topLevelItems
        QStringList topLevelItemNames;
        topLevelItemNames << "General" << "Location" << "Camera" << "Author" << "Video" << "Audio" << "Media" << "File" << "Date" << "Other";
        QMap<QString, QStandardItem *> topLevelItems;
        for (int i=0; i<topLevelItemNames.count();i++)
        {
            QStandardItem *topLevelItem =  new QStandardItem( topLevelItemNames[i] );
            topLevelItem->setEditable(false);
            propertyTreeView->propertyItemModel->setItem(i,0,topLevelItem);
            topLevelItems[topLevelItemNames[i]] = topLevelItem;
        }

        QString folderFileName;
        QUrl folderFile;

        QMap<QString, QMap<QString, QString>> valueMap;
        QMap<QString, QString> fileMediaMap;
        QMap<QString, QStandardItem *> labelMap;

        for (int resultIndex=0;resultIndex<result.count();resultIndex++)
        {
//            emit propertyTreeView->addLogToEntry(parameters["processId"], result[resultIndex]);
            int indexOf = result[resultIndex].indexOf("======== "); //next file
            if (indexOf > -1)//next file found
            {
                folderFileName = result[resultIndex].mid(indexOf+9);
                folderFile = QUrl(folderFileName);
                emit propertyTreeView->addLogToEntry(parameters["processId"], "Processing " + folderFileName + "\n");
            }
            else
            {
                int pos = result[resultIndex].indexOf(":");
                QString labelString = result[resultIndex].left(pos).trimmed();
                QString valueString = result[resultIndex].mid(pos+1).trimmed();

                if (labelString == "MIMEType")
                {
                    if ((valueString.contains("video") || valueString.contains("audio")))
                        fileMediaMap[folderFile.fileName()] = "MediaFile";
                    else
                        fileMediaMap[folderFile.fileName()] = "OtherTypeOfFile";
                }

                if (fileMediaMap[folderFile.fileName()] == "" || fileMediaMap[folderFile.fileName()] == "MediaFile") //not identied yet or identified as media
                {
                    valueMap[labelString][folderFile.fileName()] = valueString; //sets the value

//                    if (labelString == "FileName" || labelString == "SuggestedName" || labelString == "CreateDate" ||  labelString == "Make" || labelString == "Model" || labelString == "Director")
//                        labelMap[labelString] = topLevelItems["General"];
//                    else if ( labelString == "GPSLatitude" || labelString == "GPSLongitude" || labelString == "GPSAltitude")
//                        labelMap[labelString] = topLevelItems["Location"];
                    if (labelString == "ImageWidth" || labelString == "ImageHeight" || labelString == "CompressorID" || labelString == "ImageWidth" || labelString == "ImageHeight" || labelString == "VideoFrameRate" || labelString == "AvgBitrate" || labelString == "BitDepth" )
                        labelMap[labelString] = topLevelItems["Video"];
                    else if (labelString.contains("Audio"))
                        labelMap[labelString] = topLevelItems["Audio"];
                    else if (labelString.contains("Duration") || labelString.contains("Image") || labelString.contains("Video") || labelString.contains("Compressor") || labelString == "TrackDuration" )
                        labelMap[labelString] = topLevelItems["Media"];
                    else if (labelString.contains("File") || labelString.contains("Directory"))
                        labelMap[labelString] = topLevelItems["File"];
                    else if (labelString.contains("Date"))
                        labelMap[labelString] = topLevelItems["Date"];
                    else
                        labelMap[labelString] = topLevelItems["Other"];
                }
            }
        }
        labelMap["CreateDate"] = topLevelItems["General"];
        labelMap["FileName"] = topLevelItems["General"];
        labelMap["SuggestedName"] = topLevelItems["General"];

        labelMap["GPSLatitude"] = topLevelItems["Location"];
        labelMap["GPSLongitude"] = topLevelItems["Location"];
        labelMap["GPSAltitude"] = topLevelItems["Location"];

        labelMap["Make"] = topLevelItems["Camera"];
        labelMap["Model"] = topLevelItems["Camera"];

        labelMap["Director"] = topLevelItems["Author"];
        labelMap["Producer"] = topLevelItems["Author"];
        labelMap["Publisher"] = topLevelItems["Author"];

        QMapIterator<QString, QString> iFile(fileMediaMap);
        QStringList labels;
        labels << "Property" << "Type" << "Diff";

        emit propertyTreeView->addLogToEntry(parameters["processId"], "add all files as labels\n");

        while (iFile.hasNext()) //add all files as labels
        {
            iFile.next();

            if (fileMediaMap[iFile.key()] == "MediaFile")
            {
                labels<<iFile.key();
            }
        }
        propertyTreeView->propertyItemModel->setHorizontalHeaderLabels(labels);

        emit propertyTreeView->addLogToEntry(parameters["processId"], "add all properties\n");

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
            while (iFile.hasNext()) //all files add the label values
            {
                iFile.next();

                if (fileMediaMap[iFile.key()] == "MediaFile")
                {
                    QString value = valueMap[iLabel.key()][iFile.key()];

                    QStandardItem *item;
                    item = new QStandardItem( value );
                    item->setEditable(iLabel.value() == topLevelItems["General"] || iLabel.value() == topLevelItems["Location"] || iLabel.value() == topLevelItems["Camera"] || iLabel.value() == topLevelItems["Author"]); //only these labels are editable.
                    sublevelItems.append(item);

                    valueFound = valueFound || value != "";
                    if (previousValue != "" && value != "" && value != previousValue) //diff
                        sublevelItems[2] = new QStandardItem( "true" );
                    previousValue = value;
    //                qDebug()<< iFile.key() << ": " << iFile.value();
                }
            }
            if (true)
                iLabel.value()->appendRow(sublevelItems);
        } // all labels

        emit propertyTreeView->addLogToEntry(parameters["processId"], "Update suggested names\n");

        for (int col = 1; col < propertyTreeView->model()->columnCount(); ++col)
        {
              propertyTreeView->frozenTableView->setColumnHidden(col, true);
              if (col > 2)
              {
                  QStandardItem *item = topLevelItems["General"];
                  QModelIndex parentIndex = propertyTreeView->propertyItemModel->indexFromItem(item);
                  QModelIndex childIndex = parentIndex.model()->index(0,col,parentIndex); //0 is first child
                  propertyTreeView->updateSuggestedName(childIndex); //ok to take it from general as only the model and the column of childindex will be used.
              }
        }

        emit propertyTreeView->addLogToEntry(parameters["processId"], "Setfilter, expand and set diff values\n");

        propertyTreeView->propertyProxyModel->setFilterRegExp(QRegExp(";1", Qt::CaseInsensitive,
                                                    QRegExp::FixedString));
        propertyTreeView->propertyProxyModel->setFilterKeyColumn(-1);

        propertyTreeView->expandAll();
        propertyTreeView->frozenTableView->expandAll();

        propertyTreeView->setCellStyle(QStringList()); //to set diff values
        propertyTreeView->isLoading = false;
        emit propertyTreeView->propertiesLoaded();
        emit propertyTreeView->addLogToEntry(parameters["processId"], "Completed");

    });
} //loadmodel

//! [init part1]
void APropertyTreeView::init()
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
void APropertyTreeView::updateSectionWidth(int logicalIndex, int /* oldSize */, int newSize)
{
      if (logicalIndex == 0){
            frozenTableView->setColumnWidth(logicalIndex, newSize);
            updateFrozenTableGeometry();
      }
}

void APropertyTreeView::updateSectionHeight(int logicalIndex, int , int newSize)
{
//      frozenTableView->setRowHeight(logicalIndex, newSize);
}
//! [sections]


//! [resize]
void APropertyTreeView::resizeEvent(QResizeEvent * event)
{
      QTreeView::resizeEvent(event);
      updateFrozenTableGeometry();
 }
//! [resize]


//! [navigate]
QModelIndex APropertyTreeView::moveCursor(CursorAction cursorAction,
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

void APropertyTreeView::scrollTo (const QModelIndex & index, ScrollHint hint){
    if (index.column() > 0)
        QTreeView::scrollTo(index, hint);
}

//! [geometry]
void APropertyTreeView::updateFrozenTableGeometry()
{
      frozenTableView->setGeometry(frameWidth(),
                                   frameWidth(), columnWidth(0),
                                   viewport()->height()+header()->height());
}
//! [geometry]

void APropertyTreeView::onPropertyFilterChanged(QLineEdit *propertyFilterLineEdit, QCheckBox *propertyDiffCheckBox, QCheckBox *locationCheckBox, QCheckBox *cameraCheckBox, QCheckBox *authorCheckBox)
{
//    qDebug()<<"APropertyTreeView::onPropertyFilterChanged"<<propertyFilterLineEdit->text()<<propertyDiffCheckBox->checkState()<<locationCheckBox->checkState()<<cameraCheckBox->checkState();
    QString diffString;
    if (propertyDiffCheckBox->checkState() == Qt::Unchecked)
        diffString = "0";
    else if (propertyDiffCheckBox->checkState() == Qt::Checked)
        diffString = "2";
    else
        diffString = "1";

    locationInName = locationCheckBox->checkState() == Qt::Checked;
    cameraInName = cameraCheckBox->checkState() == Qt::Checked;
    authorInName = authorCheckBox->checkState() == Qt::Checked;

    for (int col = 1; col < model()->columnCount(); ++col)
    {
          if (col > 2)
          {
//              QStandardItem *item = topLevelItems["General"];
//              QModelIndex parentIndex = propertyItemModel->indexFromItem(item);
              QModelIndex parentIndex = propertyItemModel->index(0,0);
              QModelIndex childIndex = parentIndex.model()->index(0,col,parentIndex); //0 is first child
              updateSuggestedName(childIndex);
          }
    }

//    qDebug()<<"on_propertySearchLineEdit_textChanged"<<arg1<<ui->diffOnlyCheckBox->checkState();
    propertyProxyModel->setFilterRegExp(QRegExp(propertyFilterLineEdit->text() + ";" + diffString, Qt::CaseInsensitive,
                                                QRegExp::FixedString));
    propertyProxyModel->setFilterKeyColumn(-1);

    expandAll();
    frozenTableView->expandAll();
}

void APropertyTreeView::onGetPropertyValue(QString fileName, QString key, QString *value)
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
//    qDebug()<<"APropertyTreeView::onGetPropertyValue"<<fileName<<fileColumnNr<<key<<propertyItemModel->rowCount();

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
//                    qDebug()<<  "APropertyTreeView::onGetPropertyValue"<<topLevelIndex<<childRowIndex<<sublevelIndex.data().toString()<<*value;
                    return;
                }
            }

        }
        *value = "";
    }
    else
        *value = "";
}

void APropertyTreeView::updateSuggestedName(QModelIndex index)
{
    QString fileName = index.model()->headerData(index.column(), Qt::Horizontal).toString();

    QString suggestedName = "";

        QString createDate;// = index.model()->index(0,index.column(),index.parent()).data().toString();
        QString gpsAltitude;// = index.model()->index(2,index.column(),index.parent()).data().toString();
        QString gpsLatitude;// = index.model()->index(3,index.column(),index.parent()).data().toString();
        QString gpsLongitude;// = index.model()->index(4,index.column(),index.parent()).data().toString();
        QString make;// = index.model()->index(5,index.column(),index.parent()).data().toString();
        QString model;// = index.model()->index(6,index.column(),index.parent()).data().toString();
        QString director, producer, publisher;
        QModelIndex suggestedNameIndex;// = index.model()->index(7,index.column(),index.parent());

        for (int parentRow=0;parentRow<index.model()->rowCount();parentRow++)
        {
            QModelIndex parentIndex = index.model()->index(parentRow, 0);


        for (int childRow=0;childRow<index.model()->rowCount(parentIndex);childRow++)
        {
//            qDebug()<<"APropertyTreeView::updateSuggestedName"<<childRow<<index.model()->index(childRow,0, parentIndex).data().toString();
            if (index.model()->index(childRow,0, parentIndex).data().toString() == "CreateDate")
                createDate = index.model()->index(childRow,index.column(),parentIndex).data().toString();
            if (index.model()->index(childRow,0, parentIndex).data().toString() == "GPSAltitude")
                gpsAltitude = index.model()->index(childRow,index.column(),parentIndex).data().toString();
            if (index.model()->index(childRow,0, parentIndex).data().toString() == "GPSLatitude")
                gpsLatitude = index.model()->index(childRow,index.column(),parentIndex).data().toString();
            if (index.model()->index(childRow,0, parentIndex).data().toString() == "GPSLongitude")
                gpsLongitude = index.model()->index(childRow,index.column(),parentIndex).data().toString();
            if (index.model()->index(childRow,0, parentIndex).data().toString() == "Make")
                make = index.model()->index(childRow,index.column(),parentIndex).data().toString();
            if (index.model()->index(childRow,0, parentIndex).data().toString() == "Model")
                model = index.model()->index(childRow,index.column(),parentIndex).data().toString();
            if (index.model()->index(childRow,0, parentIndex).data().toString() == "Director")
                director = index.model()->index(childRow,index.column(),parentIndex).data().toString();
            if (index.model()->index(childRow,0, parentIndex).data().toString() == "Producer")
                producer = index.model()->index(childRow,index.column(),parentIndex).data().toString();
            if (index.model()->index(childRow,0, parentIndex).data().toString() == "Publisher")
                publisher = index.model()->index(childRow,index.column(),parentIndex).data().toString();
            if (index.model()->index(childRow,0, parentIndex).data().toString() == "SuggestedName")
                suggestedNameIndex = index.model()->index(childRow,index.column(),parentIndex);
        }
        }

//        qDebug()<<"APropertyTreeView::updateSuggestedName"<<createDate<<locationInName<<cameraInName;
        if (createDate != "0000:00:00 00:00:00") // && createDate.right(8) != "00:00:00"
        {
            suggestedName = createDate.replace(":", "-");

            if (gpsLatitude != "" && locationInName)
            {
                if (gpsLatitude.right(1) == "N")
                    suggestedName += " [" + gpsLatitude.left(gpsLatitude.length()-5);
                else
                    suggestedName += " [-" + gpsLatitude.left(gpsLatitude.length()-5);
                if (gpsLongitude != "")
                {
                    if (gpsLongitude.right(1) == "E")
                        suggestedName += "," + gpsLongitude.left(gpsLongitude.length()-5) + "]";
                    else
                        suggestedName += ",-" + gpsLongitude.left(gpsLongitude.length()-5) + "]";
                    if (gpsAltitude != "")
                    {
                        int indexAbove = gpsAltitude.indexOf("Above");
                        int indexBelow = gpsAltitude.indexOf("Below");
                        if (indexAbove > 0)
                            suggestedName += "@" + gpsAltitude.left(indexAbove).replace(" ", "");
                        else if (indexBelow > 0)
                            suggestedName += "@-" + gpsAltitude.left(indexBelow).replace(" ", "");
                    }
                }
            }

            if (cameraInName)
            {
                if (model != "")
                    suggestedName += " " + model;
                else if (make != "")
                    suggestedName += " " + make;
            }

            if (authorInName)
            {
                if (director != "")
                    suggestedName += " " + director;
                else if (producer != "")
                    suggestedName += " " + producer;
                else if (publisher != "")
                    suggestedName += " " + publisher;
            }

            int pos = fileName.indexOf("+"); //first +
            int extPos = fileName.lastIndexOf(".");
            QString msString = "";
            if (pos > 0)
            {
                msString = fileName.mid(pos, extPos - pos);
    //            qDebug()<<"msString"<<msString;
                suggestedName += " " + msString;
            }

            propertyItemModel->setData(suggestedNameIndex, suggestedName, Qt::EditRole);

//            if (!fileName.contains(suggestedName + "."))
//                propertyItemModel->setData(suggestedNameIndex, QBrush("#FF4500"), Qt::ForegroundRole); //orange
//            else
//                propertyItemModel->setData(suggestedNameIndex, QVariant(palette().windowText()), Qt::ForegroundRole); //orange
        }
}

void APropertyTreeView::onPropertyChanged(QStandardItem *item)
{
    QModelIndex index = item->index();
    QString key = item->model()->index(item->row(),0,index.parent()).data().toString();

    if (key == "CreateDate" || key == "GPSLongitude" || key == "GPSLatitude" || key == "GPSAltitude" || key == "Make" || key == "Model" || key == "Director" || key == "Producer" || key == "Publisher")
    {
        if (index.column() > 2) //not for label, type and diff column
            updateSuggestedName(index);

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

                QString *processId = new QString();
                emit addLogEntry(*directoryString, fileName, "Update properties", processId);

                QMap<QString, QString> parameters;
                parameters["processId"] = *processId;

                QString code = "exiftool -" + key + "=" + valueString + "  -overwrite_original \"" + *directoryString + "//" + fileName + "\"";

                processManager->startProcess(code, parameters, nullptr, [] (QWidget *parent, QString, QMap<QString, QString> parameters, QStringList result)
                {
                    APropertyTreeView *propertyTreeView = qobject_cast<APropertyTreeView *>(parent);
                    emit propertyTreeView->addLogToEntry(parameters["processId"], result.join("\n"));
                });
            }
        }
    }
} //on onPropertyChanged

void APropertyTreeView::onRemoveFile(QString fileName)
{
//    qDebug()<<"APropertyTreeView::onRemoveFile"<<fileName;

    for (int column=0; column<propertyItemModel->columnCount();column++)
    {
        if (propertyItemModel->headerData(column,Qt::Horizontal).toString() == fileName)
            propertyItemModel->takeColumn(column);
    }
}

void APropertyTreeView::onReloadProperties()
{
    QString lastFolder = QSettings().value("LastFolder").toString();
//    qDebug()<<"APropertyTreeView::onReloadProperties"<<lastFolder;
    loadModel(lastFolder);
}
