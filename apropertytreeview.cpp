#include "apropertyitemdelegate.h"
#include "apropertytreeview.h"
#include "astarrating.h"

#include <QHeaderView>
#include <QDebug>
#include <QSettings>
#include <QUrl>
#include <QScrollBar>
#include "aglobal.h"
#include <QDateTime>
#include <QGeoCoordinate>
#include <QTimer>

APropertyTreeView::APropertyTreeView(QWidget *parent) : QTreeView(parent)
{
    propertyItemModel = new QStandardItemModel(this);
    QStringList headerLabels = QStringList() << "Property" << "Minimum" << "Delta" << "Maximum" << "Type" << "Diff";

    propertyItemModel->setHorizontalHeaderLabels(headerLabels);

    propertyProxyModel = new APropertySortFilterProxyModel(this);

//    propertyProxyModel->setSourceModel(propertyItemModel);

    setModel(propertyProxyModel);

    frozenTableView = new QTreeView(this);

    initModel(propertyItemModel);

    header()->setSectionResizeMode(QHeaderView::Interactive);
    setItemsExpandable(false);
    setColumnWidth(propertyIndex, columnWidth(propertyIndex) * 1.6);
    setColumnHidden(typeIndex, true);
    setColumnHidden(diffIndex, true);

    APropertyItemDelegate *propertyItemDelegate = new APropertyItemDelegate(this);
    setItemDelegate(propertyItemDelegate);

    setSelectionMode(QAbstractItemView::NoSelection);

    //connect the headers and scrollbars of both tableviews together
    connect(header(),&QHeaderView::sectionResized, this,
            &APropertyTreeView::updateSectionWidth);
//      connect(verticalHeader(),&QHeaderView::sectionResized, this,
//              &FreezeTableWidget::updateSectionHeight);

    connect(frozenTableView->verticalScrollBar(), &QAbstractSlider::valueChanged,
            verticalScrollBar(), &QAbstractSlider::setValue);
    connect(verticalScrollBar(), &QAbstractSlider::valueChanged,
            frozenTableView->verticalScrollBar(), &QAbstractSlider::setValue);

    isLoading = false;

    processManager = new AProcessManager(this);

    editMode = false;

    QTimer::singleShot(0, this, [this]()->void
    {
        spinnerLabel = new ASpinnerLabel(this); //otherwise not centered!
                       });

}

APropertyTreeView::~APropertyTreeView()
{
    qDebug()<<"destructor APropertyTreeView::~APropertyTreeView";
    disconnect(propertyItemModel, &QStandardItemModel::itemChanged, this, &APropertyTreeView::onPropertyChanged);

    delete frozenTableView;
}

void APropertyTreeView::onFolderIndexClicked(QModelIndex index)//
{
    setColumnHidden(minimumIndex, !editMode);
    setColumnHidden(deltaIndex, !editMode);
    setColumnHidden(maximumIndex, !editMode);

    QString lastFolder = QSettings().value("LastFolder").toString();
//    qDebug()<<"APropertyTreeView::onFolderIndexClicked"<<index.data().toString()<<lastFolder<<editMode;
    loadModel(lastFolder);
}

void APropertyTreeView::setCellStyle(QStringList fileNames)
{
//    qDebug()<<"APropertyTreeView::setCellStyle"<<fileNames.count()<<propertyProxyModel->rowCount();
    isLoading = true;
    QFont boldFont;
    boldFont.setBold(true);
    QModelIndex scrollToIndex = QModelIndex();
    for (int parentRow = 0; parentRow < propertyProxyModel->rowCount(); parentRow++)
    {
        QModelIndex parentIndex = propertyProxyModel->index(parentRow, propertyIndex);

//        qDebug()<<"APropertyTreeView::setCellStyle"<<propertyProxyModel->rowCount(parentIndex)<<propertyProxyModel->columnCount(parentIndex);
        for (int childRow = 0; childRow < propertyProxyModel->rowCount(parentIndex); childRow++)
        {
            for (int childColumn = 0; childColumn < propertyProxyModel->columnCount(parentIndex); childColumn++)
            {
                QModelIndex childIndex = propertyProxyModel->index(childRow, childColumn, parentIndex);
                QVariant hItem = propertyItemModel->headerData(childColumn, Qt::Horizontal);

                if (propertyProxyModel->index(childRow, diffIndex, parentIndex).data().toBool() && childColumn == propertyIndex) //&& childIndex.data(Qt::FontRole) != boldFont)
                {
                    propertyProxyModel->setData(childIndex, boldFont, Qt::FontRole);
                }
                if (fileNames.contains(hItem.toString()))
                {
                    propertyProxyModel->setData(childIndex,  QVariant(palette().highlight()) , Qt::BackgroundRole);
                    if (childRow == 0 && parentIndex.row() == 0) //first item of first toplevelitem (createdate within general)
                        scrollToIndex = childIndex;
                }
                else if (childIndex.data(Qt::BackgroundRole) == QVariant(palette().highlight()))
                {
                    propertyProxyModel->setData(childIndex,  QVariant(palette().base()) , Qt::BackgroundRole);
                }
            }
        }
    }
    isLoading = false;

    if (scrollToIndex != QModelIndex())
    {
//        qDebug()<<""<<scrollToIndex.row()<<scrollToIndex.column()<<scrollToIndex.data().toString();
        scrollTo(scrollToIndex, QAbstractItemView::PositionAtCenter);
    }
}

void APropertyTreeView::onFileIndexClicked(QModelIndex , QModelIndexList selectedIndices)//index
{
    QStringList selectedFileNames;
    for (int i = 0; i < selectedIndices.count(); i++)
    {
        QModelIndex index = selectedIndices[i];
        if (index.column() == propertyIndex)
        {
            selectedFileNames << index.data().toString();
        }
    }

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
    spinnerLabel->start();

    isLoading = true;

    if (editMode)
    {
        setColumnWidth(minimumIndex, columnWidth(propertyIndex) * 1.0);
        setColumnWidth(deltaIndex, columnWidth(propertyIndex) * 0.5);
        setColumnWidth(maximumIndex, columnWidth(propertyIndex) * 1.0);
    }

    propertyItemModel->removeRows(0, propertyItemModel->rowCount());
    while (propertyItemModel->columnCount() > firstFileColumnIndex) //remove old columns
        propertyItemModel->removeColumn(propertyItemModel->columnCount() - 1);

    QString exiftool = "exiftool";
#ifndef Q_OS_WIN
    exiftool = "/usr/local/bin/" + exiftool;
#endif
    QString command = exiftool + " -s -c \"%02.6f\" \"" + folderName + "\""; ////

    QMap<QString, QString> parameters;

    QString *processId = new QString();
    emit addJob(folderName, "All", "Property load", processId);
    emit addToJob(*processId, command + "\n");
    parameters["processId"] = *processId;

    processManager->startProcess(command, parameters
                                   , [] (QWidget *parent, QMap<QString, QString> parameters, QString result)
    {
        APropertyTreeView *propertyTreeView = qobject_cast<APropertyTreeView *>(parent);
        emit propertyTreeView->addToJob(parameters["processId"], result);
    }
                                   , [] (QWidget *parent, QString , QMap<QString, QString> parameters, QStringList result)
     {
        APropertyTreeView *propertyTreeView = qobject_cast<APropertyTreeView *>(parent);

        //create topLevelItems
        QStringList toplevelPropertyNames;

        toplevelPropertyNames << "General" << "Location" << "Camera" << "Labels" << "Status" << "Video" << "Audio" << "Media" << "File" << "Date" << "Artists" << "Keywords" << "Ratings"<< "Other";

        QMap<QString, QStandardItem *> topLevelItems;
        for (int i = 0; i < toplevelPropertyNames.count(); i++)
        {
            QStandardItem *topLevelItem =  new QStandardItem( toplevelPropertyNames[i] );
            topLevelItem->setEditable(false);
            propertyTreeView->propertyItemModel->setItem(i, propertyIndex,topLevelItem);
            topLevelItems[toplevelPropertyNames[i]] = topLevelItem;
        }

        QString folderFileName;
        QUrl folderFile;

        QMap<QString, QMap<QString, QString>> valueMap;
        QMap<QString, QString> fileMap;
        QMap<QString, QStandardItem *> propertyMap;

        for (int resultIndex = 0; resultIndex < result.count(); resultIndex++)
        {
            int indexOf = result[resultIndex].indexOf("======== "); //next file
            if (indexOf > -1)//next file found
            {
                folderFileName = result[resultIndex].mid(indexOf+9);
                folderFile = QUrl(folderFileName);
                emit propertyTreeView->addToJob(parameters["processId"], "Processing " + folderFileName + "\n");
            }
            else
            {
                int pos = result[resultIndex].indexOf(":");
                QString propertyName = result[resultIndex].left(pos).trimmed();
                QString valueString = result[resultIndex].mid(pos+1).trimmed();


                if (propertyName == "MIMEType")
                {
                    if (valueString.contains("video") || valueString.contains("image") || (valueString.contains("audio")))
                    {
                        fileMap[folderFile.fileName()] = "MediaFile";
//                        qDebug()<<"MediaFile"<<folderFileName<<valueString;
                    }
                    else
                    {
                        fileMap[folderFile.fileName()] = "OtherTypeOfFile";
//                        qDebug()<<"OtherTypeOfFile"<<folderFileName<<valueString;
                    }
                }

                if (fileMap[folderFile.fileName()] == "" || fileMap[folderFile.fileName()] == "MediaFile") //not identied yet or identified as media
                {
                    valueMap[propertyName][folderFile.fileName()] = valueString; //sets the value

                    //assign to topLevel
                    if (propertyName == "CreateDate")//|| propertyName == "SuggestedName"
                        propertyMap[propertyName] = topLevelItems["General"];
                    else if (propertyName == "Directory")
                        propertyMap[propertyName] = topLevelItems["Status"];
                    else if ( propertyName == "GPSLatitude" || propertyName == "GPSLongitude" || propertyName == "GPSAltitude")
                    {
                        propertyMap["GeoCoordinate"] = topLevelItems["Location"];

                        //change value for Geo data
                        QStringList positiveValues = QStringList() << " m Above Sea Level" << " N" << " E";
                        QStringList negativeValues = QStringList() << " m Below Sea Level" << " S" << " W";

                        foreach (QString value, positiveValues)
                            if (valueString.contains(value))
                                valueString = QString::number(valueString.left(valueString.length() - value.length()).toDouble());

                        foreach (QString value, negativeValues)
                            if (valueString.contains(value))
                                valueString = QString::number(-valueString.left(valueString.length() - value.length()).toDouble());

                        //init if not done
                        if (valueMap["GeoCoordinate"][folderFile.fileName()] == "")
                            valueMap["GeoCoordinate"][folderFile.fileName()] = ";;";

                        QStringList geoStringList = valueMap["GeoCoordinate"][folderFile.fileName()].split(";");
                        if (propertyName == "GPSLatitude")
                            geoStringList[0] = valueString;
                        else if (propertyName == "GPSLongitude")
                            geoStringList[1] = valueString;
                        else if (propertyName == "GPSAltitude")
                            geoStringList[2] = valueString;

                        valueMap["GeoCoordinate"][folderFile.fileName()] = geoStringList.join(";"); //sets the value
                    }
                    else if ( propertyName == "Make" || propertyName == "Model" )
                        propertyMap[propertyName] = topLevelItems["Camera"];
                    else if (propertyName == "Artist")
                        propertyMap[propertyName] = topLevelItems["Labels"];
                    else if (propertyName == "Rating")
                    {
                        propertyMap[propertyName] = topLevelItems["Labels"];
                    }
                    else if (propertyName == "Keywords")
                    {
                        propertyMap[propertyName] = topLevelItems["Labels"];
                        valueMap[propertyName][folderFile.fileName()] = valueMap[propertyName][folderFile.fileName()].replace(",", ";"); //sets the value
                    }
                    else if (propertyName == "RatingPercent" || propertyName == "SharedUserRating") //video
                    {
//                        qDebug()<<"SharedUserRating"<<valueString;
                        propertyMap[propertyName] = topLevelItems["Ratings"];
                        if (valueString.toInt() == 1 ) //== 1)
                            valueMap[propertyName][folderFile.fileName()] = "1";
                        else if (valueString.toInt() ==25)// == 25)
                            valueMap[propertyName][folderFile.fileName()] = "2";
                        else if (valueString.toInt() ==50)//== 50)
                            valueMap[propertyName][folderFile.fileName()] = "3";
                        else if (valueString.toInt() ==75)// 75)
                            valueMap[propertyName][folderFile.fileName()] = "4";
                        else if (valueString.toInt() ==99)//== 99)
                            valueMap[propertyName][folderFile.fileName()] = "5";
                    }

                    else if (propertyName.contains("Rating"))
                        propertyMap[propertyName] = topLevelItems["Ratings"];
                    else if (propertyName.contains("Keyword") || propertyName == "Category" || propertyName == "Comment" || propertyName == "Subject")
                        propertyMap[propertyName] = topLevelItems["Keywords"];
                    else if ( propertyName == "Director" || propertyName == "Producer" || propertyName == "Publisher" || propertyName == "Creator" || propertyName == "Writer" || propertyName.contains("Author")|| propertyName == "ContentDistributor")
                        propertyMap[propertyName] = topLevelItems["Artists"];
                    else // if (!propertyTreeView->editMode)
                    {
                        if (propertyName == "ImageWidth" || propertyName == "ImageHeight" || propertyName == "CompressorID" || propertyName == "ImageWidth" || propertyName == "ImageHeight" || propertyName == "VideoFrameRate" || propertyName == "AvgBitrate" || propertyName == "BitDepth" )
                            propertyMap[propertyName] = topLevelItems["Video"];
                        else if (propertyName.contains("Audio"))
                            propertyMap[propertyName] = topLevelItems["Audio"];
                        else if (propertyName.contains("Duration") || propertyName.contains("Image") || propertyName.contains("Video") || propertyName.contains("Compressor") || propertyName == "TrackDuration" )
                            propertyMap[propertyName] = topLevelItems["Media"];
                        else if (propertyName.contains("File") || propertyName.contains("Directory"))
                            propertyMap[propertyName] = topLevelItems["File"];
                        else if (propertyName.contains("Date"))
                            propertyMap[propertyName] = topLevelItems["Date"];
                        else
                            propertyMap[propertyName] = topLevelItems["Other"];
                    }
                } //mediafiles
            } //if nextfile
        } //for resultIndex

        //add mandatory properties if no values occured for this in above for loop
        propertyMap["CreateDate"] = topLevelItems["General"];

        propertyMap["GeoCoordinate"] = topLevelItems["Location"];

        propertyMap["Make"] = topLevelItems["Camera"];
        propertyMap["Model"] = topLevelItems["Camera"];

        propertyMap["Artist"] = topLevelItems["Labels"];
        propertyMap["Rating"] = topLevelItems["Labels"];
        propertyMap["Keywords"] = topLevelItems["Labels"];

        propertyMap["SuggestedName"] = topLevelItems["Status"];
        propertyMap["Status"] = topLevelItems["Status"];

        propertyMap["Author"] = topLevelItems["Artists"];
        propertyMap["Creator"] = topLevelItems["Artists"];
        propertyMap["XPAuthor"] = topLevelItems["Artists"];

        propertyMap["LastKeywordIPTC"] = topLevelItems["Keywords"];
        propertyMap["LastKeywordXMP"] = topLevelItems["Keywords"];
        propertyMap["Subject"] = topLevelItems["Keywords"];
        propertyMap["XPKeywords"] = topLevelItems["Keywords"];
        propertyMap["Comment"] = topLevelItems["Keywords"];
        propertyMap["Category"] = topLevelItems["Keywords"];

        propertyMap["RatingPercent"] = topLevelItems["Ratings"];
        propertyMap["SharedUserRating"] = topLevelItems["Ratings"];

        QMapIterator<QString, QString> fileIterator(fileMap);
        QStringList headerLabels = QStringList() << "Property" << "Minimum" << "Delta" << "Maximum" << "Type" << "Diff";

        emit propertyTreeView->addToJob(parameters["processId"], "add all files as labels\n");

        while (fileIterator.hasNext()) //add all files as labels
        {
            fileIterator.next();
            QString fileName = fileIterator.key();

            if (fileMap[fileName] == "MediaFile")
            {
                headerLabels << fileName;
            }
        }
        propertyTreeView->propertyItemModel->setHorizontalHeaderLabels(headerLabels);

        emit propertyTreeView->addToJob(parameters["processId"], "add all properties\n");

        QMapIterator<QString, QStandardItem *> propertyIterator(propertyMap);
        while (propertyIterator.hasNext()) //all labels
        {
            propertyIterator.next();
            QString propertyName = propertyIterator.key();

            bool isEditable = propertyName == "CreateDate" || propertyName == "GeoCoordinate";
            bool isDeltaAndMaximumEditable = isEditable;
            if (propertyName == "Make" || propertyName == "Model" || propertyName == "Artist" || propertyName == "Rating" || propertyName == "Keywords")
            {
                    isEditable = true;
                    isDeltaAndMaximumEditable = false;
            }

            QList<QStandardItem *> sublevelItems;

            QStandardItem *item = new QStandardItem( propertyName );
            item->setEditable(false);
            sublevelItems.append(item); //property

            for (int i=0; i < 3; i++)
            {
                QStandardItem *item;

                if (propertyName == "Rating" || propertyName == "RatingPercent" || propertyName == "SharedUserRating")
                {
//                    qDebug()<<"Rating"<<i;
                    item = new QStandardItem;
                    QVariant starVar = QVariant::fromValue(AStarRating(0));
                    item->setData(starVar, Qt::EditRole);
                }
                else
                {
                    item = new QStandardItem("");
                }
                if (i==0) //Minimum
                    item->setEditable(isEditable);
                else //delta and Maximum
                    item->setEditable(isDeltaAndMaximumEditable);

                sublevelItems.append(item); //Minimum delta and to
            }

            //type
            if (propertyName == "CreateDate")
                sublevelItems.append(new QStandardItem( "QDateTime" ));
            else if (propertyName == "GeoCoordinate")
                sublevelItems.append(new QStandardItem( "QGeoCoordinate" ));
            else if (propertyName == "Keywords" || propertyName == "Category" || propertyName == "LastKeywordXMP" || propertyName == "LastKeywordIPTC" || propertyName == "XPKeywords" || propertyName == "Comment" || propertyName == "Subject")
                sublevelItems.append(new QStandardItem( "ATags" ));
            else if (propertyName == "Rating" || propertyName == "RatingPercent" || propertyName == "SharedUserRating")
                sublevelItems.append(new QStandardItem( "ARating" ));
            else if (propertyName == "Make" || propertyName == "Model" || propertyName == "Artist")
                sublevelItems.append(new QStandardItem( "QComboBox" ));
            else
                sublevelItems.append(new QStandardItem( "QString" ));
            //not needed for derived artists as comboboxes not needed

            sublevelItems.append(new QStandardItem( "false" )); //diff

            bool valueFound = false; //in case label is added by a non media file

            QString previousValue = "";

            QMapIterator<QString, QString> fileIterator(fileMap);
            while (fileIterator.hasNext()) //all files add the label values
            {
                fileIterator.next();
                QString fileName = fileIterator.key();

                if (fileMap[fileName] == "MediaFile") // only add for mediafiles
                {
                    QString value = valueMap[propertyName][fileName];
                    QStandardItem *item;

                    if (propertyName == "Rating" || propertyName == "RatingPercent" || propertyName == "SharedUserRating")
                    {
                        item = new QStandardItem;
                        QVariant starVar = QVariant::fromValue(AStarRating(value.toInt()));
                        item->setData(starVar, Qt::EditRole);
                    }
                    else
                    {
                        item = new QStandardItem(value);

                    }
                    sublevelItems.append(item);

                    valueFound = valueFound || value != "";
                    if (previousValue != "" && value != "" && value != previousValue) //diff
                        sublevelItems[diffIndex] = new QStandardItem("true");

                    previousValue = value;
                }
            }

            propertyIterator.value()->appendRow(sublevelItems);
        } // all labels

        emit propertyTreeView->addToJob(parameters["processId"], "Update suggested names\n");

        propertyTreeView->setupModel();

        emit propertyTreeView->addToJob(parameters["processId"], "Completed");

        propertyTreeView->spinnerLabel->stop();
    });
} //loadmodel

void APropertyTreeView::initModel(QStandardItemModel *itemModel)
{
    isLoading = true;

    propertyItemModel = itemModel;

    propertyProxyModel->setSourceModel(propertyItemModel);

    init();

    connect(propertyItemModel, &QStandardItemModel::itemChanged, this, &APropertyTreeView::onPropertyChanged);
}

void APropertyTreeView::setupModel()
{
    //model() is propertyProxyModel (APropertySortFilterProxyModel)
    isLoading = true;

    //set propertyProxyModel equal to propertyItemModel
    propertyProxyModel->setFilterRegExp(QRegExp(";1", Qt::CaseInsensitive,
                                                QRegExp::FixedString));
    propertyProxyModel->setFilterKeyColumn(-1);

    expandAll();
    frozenTableView->expandAll();

//    qDebug()<<"APropertyTreeView::setupModel"<<model()<<propertyItemModel<<propertyProxyModel<<model()->rowCount()<<propertyItemModel->rowCount()<<propertyProxyModel->rowCount()<<isLoading;

    //set all parents to non editable
    for (int parentRow = 0; parentRow< propertyProxyModel->rowCount(); parentRow++)
    {
        QModelIndex toplevelPropertyIndex = propertyProxyModel->index(parentRow, propertyIndex);
//        QModelIndex toplevelPropertyProxyIndex = toplevelPropertyIndex;// propertyProxyModel->mapFromSource(toplevelPropertyIndex);

        //set toplevels not editable
        for (int column = 0; column < propertyProxyModel->columnCount(); column++)
        {
            QModelIndex toplevelColumnindex = propertyProxyModel->index(parentRow, column);
            propertyItemModel->itemFromIndex(propertyProxyModel->mapToSource(toplevelColumnindex))->setEditable(false);

            //set frozen columns hidden
            if (column >= minimumIndex)
            {
                if (!editMode)
                    frozenTableView->setColumnHidden(column, true);
                else
                    frozenTableView->setColumnHidden(column, column >= typeIndex);
            }
        }

        if (toplevelPropertyIndex.data().toString() == "Video" || toplevelPropertyIndex.data().toString() == "Audio" || toplevelPropertyIndex.data().toString() == "Media" || toplevelPropertyIndex.data().toString() == "File"  || toplevelPropertyIndex.data().toString() == "Date" || toplevelPropertyIndex.data().toString() == "Other")
        {
            frozenTableView->setRowHidden(parentRow, QModelIndex(), editMode);
            setRowHidden(parentRow, QModelIndex(), editMode);
        }
        if (toplevelPropertyIndex.data().toString() == "Status")
        {
            frozenTableView->setRowHidden(parentRow, QModelIndex(), !editMode);
            setRowHidden(parentRow, QModelIndex(), !editMode);
        }

        for (int childRow = 0; childRow <propertyProxyModel->rowCount(toplevelPropertyIndex); childRow++)
        {
            QModelIndex sublevelPropertyIndex = propertyProxyModel->index(childRow, propertyIndex, toplevelPropertyIndex);

            //Hide directory in edit mode
            if (sublevelPropertyIndex.data().toString() == "Directory")
            {
                frozenTableView->setRowHidden(childRow, toplevelPropertyIndex, editMode);
                setRowHidden(childRow, toplevelPropertyIndex, editMode);
            }

            //set fields editable
            for (int column = firstFileColumnIndex; column < model()->columnCount(); column++)
            {
                QModelIndex sublevelIndex = propertyProxyModel->index(childRow, column, toplevelPropertyIndex);

//                qDebug()<<"set editable"<<sublevelIndex.data().toString()<<editMode<<isLoading;
                if (toplevelPropertyIndex.data().toString() == "General" || toplevelPropertyIndex.data().toString() == "Location" || toplevelPropertyIndex.data().toString() == "Camera" || toplevelPropertyIndex.data().toString() == "Labels")
                    propertyItemModel->itemFromIndex(propertyProxyModel->mapToSource(sublevelIndex))->setEditable(editMode);
                else
                    propertyItemModel->itemFromIndex(propertyProxyModel->mapToSource(sublevelIndex))->setEditable(false);
            }
        }
    }

    //update values
    for (int column = 0; column < propertyProxyModel->columnCount(); column++)
    {
        if (column >= typeIndex)
        {
            frozenTableView->setColumnHidden(column, true);
        }
        if (column >= firstFileColumnIndex)
        {
            if (editMode)
                updateSuggestedNames(column); //ok to take it from general as only the model and the column of childindex will be used.

            QString fileName = model()->headerData(column, Qt::Horizontal).toString();
            onSetPropertyValue(fileName, "Status", "Loaded");
            if (editMode)
                setColumnWidth(column, columnWidth(propertyIndex) * 1.0);
            else
                setColumnWidth(column, columnWidth(propertyIndex) * 1.0);
        }
    }

    if (editMode)
        calculateMinimumDeltaMaximum();

    isLoading = false;
    setCellStyle(QStringList()); //to set diff values

    emit propertiesLoaded();
} //setupmodel

//! [init part1]
void APropertyTreeView::init()
{
      frozenTableView->setModel(model());
      APropertyItemDelegate *propertyItemDelegate = new APropertyItemDelegate(this);
      frozenTableView->setItemDelegate(propertyItemDelegate);

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

      frozenTableView->setSelectionMode(selectionMode());

      frozenTableView->setColumnWidth(propertyIndex, columnWidth(propertyIndex) );
//      frozenTableView->setColumnWidth(typeIndex, columnWidth(typeIndex) );

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
//    qDebug()<<"APropertyTreeView::updateSectionWidth"<<logicalIndex;
      if (logicalIndex == propertyIndex || logicalIndex == minimumIndex || logicalIndex == maximumIndex)
      {
            frozenTableView->setColumnWidth(logicalIndex, newSize);
            updateFrozenTableGeometry();
      }
}

void APropertyTreeView::updateSectionHeight(int , int , int )//logicalIndex, newSize
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

      if (cursorAction == MoveLeft && current.column() >= firstFileColumnIndex
              && visualRect(current).topLeft().x() < frozenTableView->columnWidth(propertyIndex) + frozenTableView->columnWidth(minimumIndex) + frozenTableView->columnWidth(deltaIndex) + frozenTableView->columnWidth(maximumIndex) )
      {
            const int newValue = horizontalScrollBar()->value() + visualRect(current).topLeft().x()
                                 - frozenTableView->columnWidth(propertyIndex) - frozenTableView->columnWidth(minimumIndex) - frozenTableView->columnWidth(deltaIndex) - frozenTableView->columnWidth(maximumIndex);
            qDebug()<<"APropertyTreeView::moveCursor"<<newValue;
            horizontalScrollBar()->setValue(newValue);
      }
      return current;
}
//! [navigate]

void APropertyTreeView::scrollTo (const QModelIndex & index, ScrollHint hint)
{
//    qDebug()<<"APropertyTreeView::scrollTo"<<index<<hint;
    if (index.column() >= firstFileColumnIndex)
        QTreeView::scrollTo(index, hint);
}

//! [geometry]
void APropertyTreeView::updateFrozenTableGeometry()
{
//    qDebug()<<"APropertyTreeView::updateFrozenTableGeometry";
    frozenTableView->setGeometry(frameWidth(),
                                 frameWidth(), columnWidth(propertyIndex) + columnWidth(minimumIndex) + columnWidth(deltaIndex) + columnWidth(maximumIndex),
                                 viewport()->height()+header()->height());
}
//! [geometry]

void APropertyTreeView::onPropertyFilterChanged(QLineEdit *propertyFilterLineEdit, QCheckBox *propertyDiffCheckBox)
{
//    qDebug()<<"APropertyTreeView::onPropertyFilterChanged"<<propertyFilterLineEdit->text()<<propertyDiffCheckBox->checkState();
    QString diffString;
    if (propertyDiffCheckBox->checkState() == Qt::Unchecked)
        diffString = "0";
    else if (propertyDiffCheckBox->checkState() == Qt::Checked)
        diffString = "2";
    else
        diffString = "1";

//    qDebug()<<"on_propertySearchLineEdit_textChanged"<<arg1<<ui->diffOnlyCheckBox->checkState();
    propertyProxyModel->setFilterRegExp(QRegExp(propertyFilterLineEdit->text() + ";" + diffString, Qt::CaseInsensitive,
                                                QRegExp::FixedString));
    propertyProxyModel->setFilterKeyColumn(-1);

    expandAll();
    frozenTableView->expandAll();
}

QModelIndex APropertyTreeView::findIndex(QString fileName, QString propertyName)
{
    int fileColumnNr = -1;
    for(int col = 0; col < propertyItemModel->columnCount(); col++)
    {
      if (propertyItemModel->headerData(col, Qt::Horizontal).toString() == fileName)
      {
          fileColumnNr = col;
      }
    }
//    qDebug()<<"APropertyTreeView::onSetPropertyValue"<<fileName<<fileColumnNr<<propertyName<<propertyItemModel->rowCount();

    if (fileColumnNr != -1)
    {
        //get row/ item value
        for(int rowIndex = 0; rowIndex < propertyItemModel->rowCount(); rowIndex++)
        {
            QModelIndex topLevelIndex = propertyItemModel->index(rowIndex,propertyIndex);

            for (int childRowIndex = 0; childRowIndex < propertyItemModel->rowCount(topLevelIndex); childRowIndex++)
            {
                QModelIndex sublevelIndex = propertyItemModel->index(childRowIndex,propertyIndex, topLevelIndex);

                if (sublevelIndex.data().toString() == propertyName)
                {
                    return propertyItemModel->index(childRowIndex, fileColumnNr, topLevelIndex);
                }
            }

        }
    }
    return QModelIndex();
}

bool APropertyTreeView::onGetPropertyValue(QString fileName, QString propertyName, QVariant *value)
{
    QModelIndex index = findIndex(fileName, propertyName);

    if (index != QModelIndex())
    {
        *value = index.data();
        return true;
    }
    else
    {
        *value = "";
        return false;
    }
}

bool APropertyTreeView::onSetPropertyValue(QString fileName, QString propertyName, QVariant value, int role)
{
//     qDebug()<<"APropertyTreeView::onSetPropertyValue"<<fileName<<propertyName<<value;
    QModelIndex index = findIndex(fileName, propertyName);

    if (index != QModelIndex())
    {
        QModelIndex proxyIndex = propertyProxyModel->mapFromSource(index);

//        scrollTo(proxyIndex, QAbstractItemView::PositionAtCenter);

//        propertyItemModel->setData(index, value);
        propertyProxyModel->setData(proxyIndex, value, role);
        return true;
    }
    else
        return false;
}

void APropertyTreeView::updateSuggestedNames(int column)
{
    QModelIndex index = model()->index(0,column); // general top level row. Index will be used to access all variables in row
    QString fileName = model()->headerData(index.column(), Qt::Horizontal).toString();

//    qDebug()<<"APropertyTreeView::updateSuggestedNames"<<fileName<<column <<locationInName<<cameraInName<<artistInName;

    QString suggestedName = "";

    QString createDate;// = index.model()->index(0,index.column(),index.parent()).data().toString();
    QString geoCoordinate;// = index.model()->index(2,index.column(),index.parent()).data().toString();
    QString make;// = index.model()->index(5,index.column(),index.parent()).data().toString();
    QString model;// = index.model()->index(6,index.column(),index.parent()).data().toString();
    QString artist;
    QModelIndex suggestedNameIndex;// = index.model()->index(7,index.column(),index.parent());

    for (int parentRow = 0; parentRow < index.model()->rowCount(); parentRow++)
    {
        QModelIndex parentIndex = index.model()->index(parentRow, propertyIndex);


        for (int childRow = 0; childRow < index.model()->rowCount(parentIndex); childRow++)
        {
//            qDebug()<<"APropertyTreeView::updateSuggestedNames"<<childRow<<index.model()->index(childRow, propertyIndex, parentIndex).data().toString();
            if (index.model()->index(childRow, propertyIndex, parentIndex).data().toString() == "CreateDate")
                createDate = index.model()->index(childRow, index.column(), parentIndex).data().toString();
            if (index.model()->index(childRow, propertyIndex, parentIndex).data().toString() == "GeoCoordinate")
                geoCoordinate = index.model()->index(childRow, index.column(), parentIndex).data().toString();
            if (index.model()->index(childRow, propertyIndex, parentIndex).data().toString() == "Make")
                make = index.model()->index(childRow, index.column(), parentIndex).data().toString();
            if (index.model()->index(childRow, propertyIndex, parentIndex).data().toString() == "Model")
                model = index.model()->index(childRow, index.column(), parentIndex).data().toString();
            if (index.model()->index(childRow, propertyIndex, parentIndex).data().toString() == "Artist")
                artist = index.model()->index(childRow, index.column(), parentIndex).data().toString();
            if (index.model()->index(childRow, propertyIndex, parentIndex).data().toString() == "SuggestedName")
                suggestedNameIndex = index.model()->index(childRow, index.column(), parentIndex);
        }
    }


//    qDebug()<<"APropertyTreeView::updateSuggestedNames"<<createDate<<locationInName<<cameraInName<<suggestedNameIndex;
    if (createDate != "0000:00:00 00:00:00") // && createDate.right(8) != "00:00:00"
    {
        suggestedName = createDate.replace(":", "-");

        if (geoCoordinate != "" && locationInName)
        {
            QString text = "";
            QStringList valueList = geoCoordinate.split(";");

            if (valueList.count() > 0 && valueList[0].toDouble() != 0)
                text += "[" + QString::number(valueList[0].toDouble(), 'f', 6);
            if (valueList.count() > 1 && valueList[1].toDouble() != 0)
                text += "," + QString::number(valueList[1].toDouble(), 'f', 6);
            if (valueList.count() > 0)
                text += "]";
            if (valueList.count() > 2 && valueList[2].toInt() != 0)
                text += "@" + valueList[2] + "m";

            suggestedName += " " + text;
        }

        if (cameraInName)
        {
            if (model != "")
                suggestedName += " " + model;
            else if (make != "")
                suggestedName += " " + make;
        }

        if (artistInName)
        {
            if (artist != "")
                suggestedName += " " + artist;
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

//        qDebug()<<"APropertyTreeView::updateSuggestedNames"<<suggestedName;

        if (fileName.mid(0,extPos) == suggestedName)
            propertyProxyModel->setData(suggestedNameIndex, "", Qt::EditRole);
        else
            propertyProxyModel->setData(suggestedNameIndex, suggestedName, Qt::EditRole);

//            if (!fileName.contains(suggestedName + "."))
//                propertyItemModel->setData(suggestedNameIndex, QBrush("#FF4500"), Qt::ForegroundRole); //orange
//            else
//                propertyItemModel->setData(suggestedNameIndex, QVariant(palette().windowText()), Qt::ForegroundRole); //orange
    }
} //updateSuggestedNames

void APropertyTreeView::onPropertyChanged(QStandardItem *item)
{
    QModelIndex index = item->index();
    QModelIndex proxyIndex = propertyProxyModel->mapFromSource(index);

    QString propertyName = item->model()->index(item->row(), propertyIndex, index.parent()).data().toString();
    QString typeName = item->model()->index(item->row(), typeIndex, index.parent()).data().toString();

    if (colorChanged == "yes" || !editMode)
    {
//        qDebug()<<"onPropertyChanged colorChanged reset";
        colorChanged = "";
        return;
    }

    if (index.parent() != QModelIndex()) //not toplevelobjects
    {

        if (!isLoading)
        {
//            qDebug()<<"APropertyTreeView::onPropertyChanged"<<index.row()<<index.column()<<propertyName<<typeName<<index.data()<<valueChangedBy<<isLoading<<colorChanged<<editMode;

            if (index.column() >= firstFileColumnIndex) //not for label, type and diff and Minimum/Maximum column index.column() == minimumIndex || index.column() == maximumIndex ||
                updateSuggestedNames(index.column());

            if (index.column() >= firstFileColumnIndex) //updating individual files
            {
                QString fileName =  index.model()->headerData(index.column(), Qt::Horizontal).toString();

                QVariant brush;
                if (index.parent().data().toString() == "Status")
                {
                    if (propertyName == "Status")
                    {
                        if (index.data().toString() == "Changed")
                            brush = QBrush(QColor(255, 140, 0, 50)); //orange causes recursive call
                        else if (index.data().toString().contains("1 image files updated"))
                            if (index.data().toString().contains("Warning"))
                                brush = QBrush(QColor(255, 140, 0, 50)); //orange causes recursive call
                            else
                                brush = QBrush(QColor(34,139,34, 50)); //darkgreen causes recursive call
                        else
                            brush = QBrush(QColor(139,0,0, 50)); //dark red causes recursive call
                    }
                    else if (propertyName == "SuggestedName")
                    {
                        if (index.data().toString() != "")
                            brush = QBrush(QColor(255, 140, 0, 50)); //orange causes recursive call
                        else
                            brush = QVariant(palette().base()); //causes recursive call
                    }
                    else
                        brush = QVariant(palette().base()); //causes recursive call
                 }
                else
                {
                    brush = QBrush(QColor(255, 140, 0, 50)); //orange causes recursive call
                    changedIndexesMap[fileName][propertyName] = proxyIndex;
                    onSetPropertyValue(fileName, "Status", "Changed");
                }

//                qDebug()<<"onPropertyChanged colorChanged"<<propertyName<<proxyIndex.data(Qt::BackgroundRole)<<brush;
                if (proxyIndex.data(Qt::BackgroundRole) != brush)
                {
                    colorChanged = "yes";
                    propertyProxyModel->setData(proxyIndex, brush , Qt::BackgroundRole); //orange causes recursive call
                }

                if (valueChangedBy != "MinimumDeltaMaximum")
                {
                    isLoading = true; //to avoid loop
                    calculateMinimumDeltaMaximum();
                    isLoading = false;
                }

                if (propertyName == "Artist")
                {
                    onSetPropertyValue(fileName, "Author", index.data().toString());
                    onSetPropertyValue(fileName, "Creator", index.data().toString());
                    onSetPropertyValue(fileName, "XPAuthor", index.data().toString());
                }
                if (propertyName == "Keywords")
                {
                    onSetPropertyValue(fileName, "LastKeywordIPTC", index.data().toString());
                    onSetPropertyValue(fileName, "LastKeywordXMP", index.data().toString());
                    onSetPropertyValue(fileName, "Subject", index.data().toString());
                    onSetPropertyValue(fileName, "XPKeywords", index.data().toString());
                    onSetPropertyValue(fileName, "Comment", index.data().toString());
                    onSetPropertyValue(fileName, "Category", index.data().toString());
                }
                if (propertyName == "Rating")
                {
                    onSetPropertyValue(fileName, "RatingPercent", index.data());
                }
                if (propertyName == "GeoCoordinate")
                {
                    emit redrawMap();
                }
            }
            else if (index.column() >= minimumIndex && index.column() <= maximumIndex) //updating Minimum delta Maximum
            {
                QModelIndex minimumValue = propertyProxyModel->index(proxyIndex.row(), minimumIndex, proxyIndex.parent());
                QModelIndex deltaValue = propertyProxyModel->index(proxyIndex.row(), deltaIndex, proxyIndex.parent());
                QModelIndex maximumValue = propertyProxyModel->index(proxyIndex.row(), maximumIndex, proxyIndex.parent());

                int nrOfNonHiddenColumns = 0;
                for (int column = firstFileColumnIndex; column < propertyProxyModel->columnCount();column++)
                {
                    if (!isColumnHidden(column))
                        nrOfNonHiddenColumns ++;
                }

                if (typeName == "QDateTime" && valueChangedBy != "MinimumDeltaMaximum")
                {
                    valueChangedBy = "MinimumDeltaMaximum";

                    QDateTime minimumTime = QDateTime::fromString(minimumValue.data().toString(), "yyyy:MM:dd HH:mm:ss");
                    QString deltaString = deltaValue.data().toString();
                    QDateTime maximumTime;

//                    qDebug()<<"APropertyTreeView::onPropertyChanged delta"<<proxyIndex.row()<<index.column();

                    int seconds = 0;

                    if (index.column() == deltaIndex || index.column() == minimumIndex) //updating delta and Minimum, set Maximum
                    {
                        seconds = AGlobal().csvToSeconds(deltaString);
                        maximumTime = minimumTime.addSecs(seconds);

                        propertyProxyModel->setData(propertyProxyModel->index(proxyIndex.row(), maximumIndex, proxyIndex.parent()), maximumTime.toString("yyyy:MM:dd HH:mm:ss"));
                    }
                    else //updating Maximum, set delta
                    {
                        maximumTime = QDateTime::fromString(maximumValue.data().toString(), "yyyy:MM:dd HH:mm:ss");
                        seconds = minimumTime.secsTo(maximumTime);
                        deltaString = AGlobal().secondsToCSV(seconds);
                        qDebug()<<"secs and days"<<minimumTime<<maximumTime<<minimumTime.secsTo(maximumTime)<<deltaString;
                        propertyProxyModel->setData(propertyProxyModel->index(proxyIndex.row(), deltaIndex, proxyIndex.parent()), deltaString);
                    }

                    maximumTime = minimumTime;

//                    qDebug()<<"delta time"<<nrOfNonHiddenColumns<<deltaString;

                    double deltaAxi = 0;
                    double delta = double(seconds) / double(nrOfNonHiddenColumns-1);
                    for (int column = firstFileColumnIndex; column < propertyProxyModel->columnCount();column++)
                    {
                        if (!isColumnHidden(column))
                        {
                            propertyProxyModel->setData(propertyProxyModel->index(proxyIndex.row(), column, proxyIndex.parent()), maximumTime.toString("yyyy:MM:dd HH:mm:ss"));

                            maximumTime = maximumTime.addSecs(qRound(delta + deltaAxi));
                            deltaAxi += delta - double(qRound(delta + deltaAxi));
    //                        qDebug()<<"deltaAxi"<<delta<<deltaAxi<<qRound(delta + deltaAxi);
                        }
                    }
//                    qDebug()<<"delta time done"<<nrOfNonHiddenColumns<<deltaString<<seconds;
                    valueChangedBy = "";
                }
                else if (typeName == "QGeoCoordinate" && valueChangedBy != "MinimumDeltaMaximum")
                {
//                    qDebug()<<"APropertyTreeView::onPropertyChanged delta"<<proxyIndex.row()<<index.column();
                    valueChangedBy = "MinimumDeltaMaximum";

                    QGeoCoordinate minimumCoordinate;
                    QString deltaString;
                    QGeoCoordinate maximumCoordinate;

                    minimumCoordinate = AGlobal().csvToGeoCoordinate(minimumValue.data().toString());

                    deltaString = deltaValue.data().toString();

//                    qDebug()<<"APropertyTreeView::onPropertyChanged delta"<<proxyIndex.row()<<index.column()<<minimumCoordinate<<deltaString<<toCoordinate;

                    int distance = 0;
                    int bearing = 0;
                    int altitude = 0;
                    if (index.column() == deltaIndex || index.column() == minimumIndex) //updating delta and minimum, set Maximum
                    {
                        QStringList valueList = deltaString.split(";");
                        if (valueList.count() > 0 && valueList[0] != "0")
                            distance = valueList[0].toInt();
                        if (valueList.count() > 1 && valueList[1] != "0")
                            bearing = valueList[1].toInt();
                        if (valueList.count() > 2 && valueList[2] != "0")
                            altitude = valueList[2].toInt();

                        maximumCoordinate = minimumCoordinate.atDistanceAndAzimuth(distance, bearing, altitude);
                        propertyProxyModel->setData(propertyProxyModel->index(proxyIndex.row(), maximumIndex, proxyIndex.parent()), QString::number(maximumCoordinate.latitude()) + ";" + QString::number(maximumCoordinate.longitude()) + ";" + QString::number(maximumCoordinate.altitude()));
                    }
                    else //updating Maximum, set delta
                    {
                        minimumCoordinate = AGlobal().csvToGeoCoordinate(maximumValue.data().toString());

//                        toCoordinate = QGeoCoordinate(toValue.data().toString().split(";")[0].toDouble(), toValue.data().toString().split(";")[1].toDouble(), toValue.data().toString().split(";")[2].toInt());
                        distance = minimumCoordinate.distanceTo(maximumCoordinate);
                        bearing = minimumCoordinate.azimuthTo(maximumCoordinate);
                        altitude = maximumCoordinate.altitude() - minimumCoordinate.altitude();
                        deltaString = QString::number(distance) + ";" + QString::number(bearing) + ";" + QString::number(altitude);

                        qDebug()<<"geo delta"<<minimumCoordinate<<maximumCoordinate<<deltaString;
                        propertyProxyModel->setData(propertyProxyModel->index(proxyIndex.row(), deltaIndex, proxyIndex.parent()), deltaString);
                    }

                    maximumCoordinate = minimumCoordinate;

//                    qDebug()<<"delta geo"<<nrOfNonHiddenColumns<<deltaString<<distance<<bearing<<altitude;

//                    if (deltaString != "")
                    {
                        double deltaAxi = 0;
                        double delta = double(distance) / double(nrOfNonHiddenColumns-1);
                        double total = 0;

                        for (int column = firstFileColumnIndex; column < propertyProxyModel->columnCount();column++)
                        {
                            if (!isColumnHidden(column))
                            {
                                    propertyProxyModel->setData(propertyProxyModel->index(proxyIndex.row(), column, proxyIndex.parent()), QString::number(maximumCoordinate.latitude()) + ";" + QString::number(maximumCoordinate.longitude()) + ";" + QString::number(maximumCoordinate.altitude()));

                                    maximumCoordinate = maximumCoordinate.atDistanceAndAzimuth(qRound(delta + deltaAxi), bearing, double(altitude) / double(nrOfNonHiddenColumns -1));
                                    total += qRound(delta + deltaAxi);
//                                    qDebug()<<"deltaAxi"<<delta<<deltaAxi<<qRound(delta + deltaAxi)<<total;
                                    deltaAxi += delta - double(qRound(delta + deltaAxi));
                            }
                        }
                    }

                    valueChangedBy = "";
                }
                else if (typeName == "QComboBox" && valueChangedBy != "MinimumDeltaMaximum")
                {
                    valueChangedBy = "MinimumDeltaMaximum";

                    if (index.column() == minimumIndex && !index.data().toString().contains("<Multiple>***")) //update all files
                    {
                        QString selectedValue = index.data().toString();
                        QStringList values = selectedValue.split(";");
                        for (int i=0; i < values.count();i++)
                        {
                            if (values[i].contains("***"))
                                selectedValue = values[i].replace("***","");
                        }

//                        qDebug()<<"delta combo"<<index.data().toString();

                        for (int column = firstFileColumnIndex; column < propertyProxyModel->columnCount();column++)
                        {
                            if (!isColumnHidden(column))
                            {
                                propertyProxyModel->setData(propertyProxyModel->index(proxyIndex.row(), column, proxyIndex.parent()), selectedValue);
                            }
                        }
                    }
                    valueChangedBy = "";
                }
                else if (typeName == "ARating" && valueChangedBy != "MinimumDeltaMaximum")
                {
                    valueChangedBy = "MinimumDeltaMaximum";
                    for (int column = firstFileColumnIndex; column < propertyProxyModel->columnCount();column++)
                    {
                        if (!isColumnHidden(column))
                        {
                            QVariant old = propertyProxyModel->index(proxyIndex.row(), column, proxyIndex.parent()).data();
                            AStarRating starRating = qvariant_cast<AStarRating>( minimumValue.data());
                            AStarRating oldStarRating = qvariant_cast<AStarRating>( old);

//                                qDebug()<<"rateeee"<<starRating.starCount()<<oldStarRating.starCount();

                            if (starRating.starCount() != oldStarRating.starCount())
                            {
                                propertyProxyModel->setData(propertyProxyModel->index(proxyIndex.row(), column, proxyIndex.parent()), minimumValue.data());
                            }
                        }
                    }
                    valueChangedBy = "";
                } //typeName
                else if (typeName == "ATags" && valueChangedBy != "MinimumDeltaMaximum")
                {
                    valueChangedBy = "MinimumDeltaMaximum";
                    for (int column = firstFileColumnIndex; column < propertyProxyModel->columnCount();column++)
                    {
                        if (!isColumnHidden(column))
                        {
                            propertyProxyModel->setData(propertyProxyModel->index(proxyIndex.row(), column, proxyIndex.parent()), minimumValue.data());
                        }
                    }
                    valueChangedBy = "";
                } //typeName
            }
        } //!isLoading
    } //if propertyName
} //on onPropertyChanged

void APropertyTreeView::saveChanges(QProgressBar *pprogressBar)
{
    spinnerLabel->start();

    progressBar = pprogressBar;

    QMapIterator<QString, QMap<QString, QModelIndex>> changedFilesIterator(changedIndexesMap);
    while (changedFilesIterator.hasNext()) //all files
    {
        changedFilesIterator.next();

        QString fileName = changedFilesIterator.key();
        QStringList propertyNames;
        QStringList values;

        QMapIterator<QString, QModelIndex> changedPropertyIterator(changedFilesIterator.value());
        while (changedPropertyIterator.hasNext()) //all files
        {
            changedPropertyIterator.next();

            QModelIndex proxyIndex = changedPropertyIterator.value();

            QString propertyName = propertyProxyModel->index(proxyIndex.row(), propertyIndex, proxyIndex.parent()).data().toString();

            //update properties
            if (proxyIndex.column() >= firstFileColumnIndex && proxyIndex.parent().data().toString()!= "Status")
            {
                QString valueString = "\"" + proxyIndex.data().toString() + "\"";

                if (propertyName == "CreateDate")
                    valueString = "\"" + proxyIndex.data().toDateTime().toString("yyyy-MM-dd HH:mm:ss") + "\"";
                else if (propertyName == "GeoCoordinate")
                {
//                    QStringList geoValues = proxyIndex.data().toString().split(";");

                    QGeoCoordinate geoCoordinate = AGlobal().csvToGeoCoordinate(proxyIndex.data().toString());

                    propertyNames << "GPSLatitude";
                    values << QString::number(geoCoordinate.latitude());

                    propertyNames << "GPSLatitudeRef";
                    if (geoCoordinate.latitude() < 0)
                        values << "\"South\"";
                    else
                        values << "\"North\"";

                    propertyNames << "GPSLongitude";
                    values << QString::number(geoCoordinate.longitude());

                    propertyNames << "GPSLongitudeRef";
                    if (geoCoordinate.longitude() < 0)
                        values << "\"West\"";
                    else
                        values << "\"East\"";

                    propertyName = "GPSAltitude";
                    valueString = QString::number(geoCoordinate.altitude());
                    if (geoCoordinate.altitude() < 0)
                        valueString += " -GPSAltitudeRef=\"Below Sea Level\"";
                    else
                        valueString += " -GPSAltitudeRef=\"Above Sea Level\"";
                }
                else if (propertyName == "Rating")
                {
                    AStarRating starRating = qvariant_cast<AStarRating>(proxyIndex.data());

                    valueString = "\"" + QString::number(starRating.starCount()) + "\"";
                }
                else if (propertyName == "RatingPercent")
                {
                    AStarRating starRating = qvariant_cast<AStarRating>(proxyIndex.data());

                    if (starRating.starCount() == 0)
                        valueString = "\"0\"";
                    else if (starRating.starCount() == 1)
                        valueString = "\"1\"";
                    else if (starRating.starCount() == 2)
                        valueString = "\"25\"";
                    else if (starRating.starCount() == 3)
                        valueString = "\"50\"";
                    else if (starRating.starCount() == 4)
                        valueString = "\"75\"";
                    else if (starRating.starCount() == 5)
                        valueString = "\"99\"";
                }

                propertyNames << propertyName;
                values << valueString;
            } //gt firstFileColumnIndex
        }//properties

        QVariant *directoryName  = new QVariant();
        onGetPropertyValue(fileName, "Directory", directoryName);
//                ui->statusBar->showMessage("Update metadata for " + directory.toString() + "//" + fileName + "...");

        emit releaseMedia(fileName); //to stop the video to free the resource in windows/os

        QString *processId = new QString();
        emit addJob(directoryName->toString(), fileName, "Update properties", processId);

        QMap<QString, QString> parameters;
        parameters["processId"] = *processId;
        parameters["fileName"] = fileName;
        parameters["propertyNames"] = propertyNames.join(";");

        QString exiftool = "exiftool";
    #ifndef Q_OS_WIN
        exiftool = "/usr/local/bin/" + exiftool;
    #endif
        QString code = exiftool;

//        qDebug()<<"APropertyTreeView::saveChanges"<<propertyNames.count()<<values.count();

        for (int i=0; i<propertyNames.count(); i++)
            code += " -" + propertyNames[i] + "=" + values[i];

        code += " -overwrite_original \"" + directoryName->toString() + "//" + fileName + "\"";

//        qDebug()<<"APropertyTreeView::saveChanges"<<code;

        if (code.contains("exiftool"))
            onSetPropertyValue(parameters["fileName"], "Status", "Updating metadata");
        else
            onSetPropertyValue(parameters["fileName"], "Status", code);

        processManager->startProcess(code, parameters, nullptr, [] (QWidget *parent, QString, QMap<QString, QString> parameters, QStringList result)
        {
            APropertyTreeView *propertyTreeView = qobject_cast<APropertyTreeView *>(parent);

            qDebug()<<"APropertyTreeView::saveChanges"<<result.join("\n");
            QString resultJoin = result.join(" ").trimmed();

            propertyTreeView->onSetPropertyValue(parameters["fileName"], "Status", resultJoin);//color done in propertychanged

            QStringList propertyNames = parameters["propertyNames"].split(";");
            foreach (QString propertyName, propertyNames)
            {
//                qDebug()<<"savechanges colorChanged" <<propertyName;

                QString propName;
                if (propertyName.contains("GPS"))
                    propName = "GeoCoordinate";
                else
                    propName = propertyName;

                propertyTreeView->colorChanged = "yes";
                if (!resultJoin.contains("1 image files updated") || resultJoin.contains(propertyName)) //property mentioned in error message
                    propertyTreeView->onSetPropertyValue(parameters["fileName"], propName, QBrush(QColor(255, 140, 0, 50)), Qt::BackgroundRole); //orange
                else
                    propertyTreeView->onSetPropertyValue(parameters["fileName"], propName, QBrush(QColor(34,139,34, 50)), Qt::BackgroundRole); //darkgreen
            }

//            qDebug()<<""<<propertyNames.count()<<propertyTreeView->changedIndexesMap.count()<<100.0 / (propertyTreeView->changedIndexesMap.count());
            propertyTreeView->progressBar->setValue(propertyTreeView->progressBar->value() + 100.0 / propertyTreeView->changedIndexesMap.count());

            emit propertyTreeView->addToJob(parameters["processId"], result.join("\n"));
        });

    }//files

    QMap<QString, QString> parameters;
    processManager->startProcess(parameters, [] (QWidget *parent, QString , QMap<QString, QString> , QStringList ) //command, parameters, result
    {
        APropertyTreeView *propertyTreeView = qobject_cast<APropertyTreeView *>(parent);

        propertyTreeView->progressBar->setValue(100);
        propertyTreeView->progressBar->setStyleSheet("QProgressBar::chunk {background: green}");

        propertyTreeView->changedIndexesMap.clear();

        propertyTreeView->spinnerLabel->stop();
    });
} //saveChanges

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

void APropertyTreeView::onPropertyColumnFilterChanged(QString filter)
{
    for (int column = firstFileColumnIndex; column < propertyItemModel->columnCount(); column++)
    {
        setColumnHidden(column, !propertyItemModel->headerData(column,Qt::Horizontal).toString().contains(filter, Qt::CaseInsensitive));
    }
    if (editMode)
    {
        valueChangedBy = "MinimumDeltaMaximum";
        calculateMinimumDeltaMaximum();
        valueChangedBy = "";
    }
}

void APropertyTreeView::onSuggestedNameFiltersClicked(QCheckBox *locationCheckBox, QCheckBox *cameraCheckBox, QCheckBox *artistCheckBox)
{
//    qDebug()<<"APropertyTreeView::onSuggestedNameFiltersClicked"<<locationCheckBox->checkState()<<cameraCheckBox->checkState()<<artistCheckBox->checkState();

    locationInName = locationCheckBox->checkState() == Qt::Checked;
    cameraInName = cameraCheckBox->checkState() == Qt::Checked;
    artistInName = artistCheckBox->checkState() == Qt::Checked;

    for (int col = firstFileColumnIndex; col < propertyProxyModel->columnCount(); ++col)
    {
//              qDebug()<<"APropertyTreeView::onSuggestedNameFiltersClicked2"<<col;
        updateSuggestedNames(col);
    }
}

void APropertyTreeView::calculateMinimumDeltaMaximum()
{
//    qDebug()<<"APropertyTreeView::calculateMinimumDeltaMaximum"<<isLoading;
    for (int parentRow = 0; parentRow < propertyProxyModel->rowCount(); parentRow++)
    {
        QModelIndex parentIndex = propertyProxyModel->index(parentRow, propertyIndex);

        for (int childRow = 0; childRow < propertyProxyModel->rowCount(parentIndex); childRow++)
        {
            QModelIndex typeValue = propertyProxyModel->index(childRow, typeIndex, parentIndex);

            QString minValue = "";
            QString maxValue = "";
            QStringList values;

            for (int childColumn = firstFileColumnIndex; childColumn < propertyProxyModel->columnCount(parentIndex); childColumn++)
            {
                if (!isColumnHidden(childColumn))
                {
                    QModelIndex childIndex = propertyProxyModel->index(childRow, childColumn, parentIndex);
                    QVariant hItem = propertyItemModel->headerData(childColumn, Qt::Horizontal);

                    QString value = childIndex.data().toString();

                    if (typeValue.data().toString() == "ARating")
                    {
                        AStarRating starRating = qvariant_cast<AStarRating>( childIndex.data());
                        if (starRating.starCount() > 0)
                            value = QString::number( starRating.starCount());
//                        minValue = "0";
//                        maxValue = "0";
                    }

                    if (value != "")
                    {
                        if (typeValue.data().toString() == "GeoCoordinate")
                        {
    //                            qDebug()<<"geoCoordinate"<<geoCoordinate.toString();
                                QGeoCoordinate minGeoCoordinate(minValue.split(";")[0].toDouble(), minValue.split(";")[1].toDouble(), minValue.split(";")[2].toInt());
                                QGeoCoordinate maxGeoCoordinate(maxValue.split(";")[0].toDouble(), maxValue.split(";")[1].toDouble(), maxValue.split(";")[2].toInt());
                                QGeoCoordinate geoCoordinate(value.split(";")[0].toDouble(), value.split(";")[1].toDouble(), value.split(";")[2].toInt());

                            if (geoCoordinate.isValid())
                            {
                                if (minValue == "" || geoCoordinate.azimuthTo(minGeoCoordinate) + 90 > 270) //SW
                                    minValue = QString::number(geoCoordinate.latitude()) + ";" + QString::number(geoCoordinate.longitude()) + ";" + QString::number(geoCoordinate.altitude());
                                if (maxValue == "" || geoCoordinate.azimuthTo(maxGeoCoordinate) < 90) //NE
                                    maxValue = QString::number(geoCoordinate.latitude()) + ";" + QString::number(geoCoordinate.longitude()) + ";" + QString::number(geoCoordinate.altitude());
                            }
                        }
                        else if (typeValue.data().toString() == "ARating")
                        {
                            AStarRating starRating = qvariant_cast<AStarRating>( childIndex.data());

//                            qDebug()<<"starCount"<<value<<starRating.starCount();

                            if (minValue == "" || starRating.starCount() < minValue.toInt())
                                minValue = QString::number(starRating.starCount());
                            if (maxValue == "" || starRating.starCount() > maxValue.toInt())
                                maxValue = QString::number(starRating.starCount());
                        }
                        else
                        {
                            if (minValue == "" || value < minValue)
                                minValue = value;
                            if (maxValue == "" || value > maxValue)
                                maxValue = value;
                            if (!values.contains(value)) //for QComboBox
                                values = values << value;
                        }

//                        qDebug()<<"APropertyTreeView::calculateMinimumDeltaMaximum"<<childIndex.row()<<childIndex.column()<<childIndex.data().toString()<<typeValue.data().toString()<<minValue<<maxValue;
                    } //value != ""
                }
            } //for columns

//            qDebug()<<"APropertyTreeView::calculateMinimumDeltaMaximum1"<<typeValue.data().toString()<<minValue;

            if (minValue != "")
            {
                //Minimums
                if (typeValue.data().toString() == "QComboBox")
                {
                    if (values.count() > 1)
                        propertyProxyModel->setData(propertyProxyModel->index(childRow, minimumIndex, parentIndex), "<Multiple>***;" + values.join(";"));
                    else
                        propertyProxyModel->setData(propertyProxyModel->index(childRow, minimumIndex, parentIndex), values.join(";"));
                }
                else if (typeValue.data().toString() == "ATags")
                {
                    propertyProxyModel->setData(propertyProxyModel->index(childRow, minimumIndex, parentIndex), values.join(";"));
                }
                else if (typeValue.data().toString() == "QGeoCoordinate")
                {
                    propertyProxyModel->setData(propertyProxyModel->index(childRow, minimumIndex, parentIndex), minValue);
                }
                else if (typeValue.data().toString() == "QDateTime")
                    propertyProxyModel->setData(propertyProxyModel->index(childRow, minimumIndex, parentIndex), minValue);
                else if (typeValue.data().toString() == "ARating")
                {
//                    qDebug()<<"APropertyTreeView::calculateMinimumDeltaMaximum min"<<typeValue.data().toString()<<minValue;
                    propertyProxyModel->setData(propertyProxyModel->index(childRow, minimumIndex, parentIndex), QVariant::fromValue(AStarRating(minValue.toInt())));
                }
            }
            else
                propertyProxyModel->setData(propertyProxyModel->index(childRow, minimumIndex, parentIndex), "");

            if (maxValue != "")
            {
                //Maximum's
                if (typeValue.data().toString() == "QGeoCoordinate")
                    propertyProxyModel->setData(propertyProxyModel->index(childRow, maximumIndex, parentIndex), maxValue);
                else if (typeValue.data().toString() == "QDateTime")
                    propertyProxyModel->setData(propertyProxyModel->index(childRow, maximumIndex, parentIndex), maxValue);
                else if (typeValue.data().toString() == "ARating")
                    propertyProxyModel->setData(propertyProxyModel->index(childRow, maximumIndex, parentIndex), QVariant::fromValue(AStarRating(maxValue.toInt())));
            }
            else
                propertyProxyModel->setData(propertyProxyModel->index(childRow, maximumIndex, parentIndex), "");

            if (minValue != "" && maxValue != "")
            {
                //deltas
                if (typeValue.data().toString() == "QGeoCoordinate")
                {
                    QGeoCoordinate minGeoCoordinate = AGlobal().csvToGeoCoordinate(minValue);
                    QGeoCoordinate maxGeoCoordinate = AGlobal().csvToGeoCoordinate(maxValue);

                    propertyProxyModel->setData(propertyProxyModel->index(childRow, deltaIndex, parentIndex), QString::number(qRound(minGeoCoordinate.distanceTo(maxGeoCoordinate))) + ";" + QString::number(qRound(minGeoCoordinate.azimuthTo(maxGeoCoordinate))) + ";" + QString::number(maxGeoCoordinate.altitude() - minGeoCoordinate.altitude()));
                }
                else if (typeValue.data().toString() == "QDateTime")
                {
                    QDateTime minimumTime = QDateTime::fromString(minValue, "yyyy:MM:dd HH:mm:ss");
                    QDateTime maximumTime = QDateTime::fromString(maxValue, "yyyy:MM:dd HH:mm:ss");

                    propertyProxyModel->setData(propertyProxyModel->index(childRow, deltaIndex, parentIndex), AGlobal().secondsToCSV(minimumTime.secsTo(maximumTime)));
                }
            }
            else
                propertyProxyModel->setData(propertyProxyModel->index(childRow, deltaIndex, parentIndex), "");
        }
    }
} //calculateMinimumDeltaMaximum
