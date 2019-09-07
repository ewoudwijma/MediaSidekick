#include "fpropertyitemdelegate.h"
#include "fpropertytreeview.h"

#include <QHeaderView>
#include <QDebug>
#include <QSettings>
#include <QUrl>
#include <QScrollBar>

static const int deltaIndex = 2;
static const int firstFileColumnIndex = 3;

FPropertyTreeView::FPropertyTreeView(QWidget *parent) : QTreeView(parent)
{
    propertyItemModel = new QStandardItemModel(this);
    pivotItemModel = new QStandardItemModel(this);
    QStringList labels;
    labels << "Property" << "Type" << "Diff";
    propertyItemModel->setHorizontalHeaderLabels(labels);

    propertyProxyModel = new FPropertySortFilterProxyModel(this);

    propertyProxyModel->setSourceModel(propertyItemModel);

    setModel(propertyProxyModel);

    header()->setSectionResizeMode(QHeaderView::Interactive);
//    header()->setSectionsMovable(true);
    header()->setSectionsClickable(true);
//    horizontalHeader()->stretchSectionCount();
//    setSortingEnabled(true);
    setItemsExpandable(false);
    setColumnWidth(0,columnWidth(0) * 2);

//    FPropertyItemDelegate *propertyItemDelegate = new FPropertyItemDelegate(this);
//    setItemDelegate(propertyItemDelegate);
    setColumnHidden(1, true); //type column
    setColumnHidden(2, true); //diffcolumn

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
    qDebug()<<"FPropertyTreeView::onFolderIndexClicked"<<index.data().toString()<<lastFolder;
    loadModel(lastFolder);
}


void FPropertyTreeView::diffData(QModelIndex parent)
{
    for (int i=0; i<propertyItemModel->rowCount(parent); i++)
    {
        bool delta = false;
        QString sameValue = propertyItemModel->index(i,firstFileColumnIndex, parent).data().toString();
        for (int j=0; j<propertyItemModel->columnCount(parent); j++)
        {
            QModelIndex index = propertyItemModel->index(i,j, parent);
            if (j > firstFileColumnIndex && sameValue != index.data().toString() && sameValue != "" && index.data().toString() != "")
            {
                delta = true;
//                qDebug()<<"setting bold"<<i<<j<<parent.data().toString()<<index.data().toString();
            }
            if( propertyItemModel->hasChildren(index) )
                diffData(index);
        }
        propertyItemModel->itemFromIndex(propertyItemModel->index(i,deltaIndex, parent))->setData(QVariant(delta), Qt::DisplayRole);

//        if (delta)
        {
            for (int j=firstFileColumnIndex; j<propertyItemModel->columnCount(parent); j++) //make all columns in row bold
            {
                QFont boldFont;
                boldFont.setBold(delta);
                QModelIndex index = propertyItemModel->index(i,j, parent);
                if (delta)
                    qDebug()<<"setting bold"<<i<<j<<index.data();
                propertyItemModel->itemFromIndex(index)->setData(boldFont, Qt::FontRole);
            }
        }
    }
}

void FPropertyTreeView::loadModel(QString folderName)
{
    propertyItemModel->removeRows(0, propertyItemModel->rowCount());
    while (propertyItemModel->columnCount()>firstFileColumnIndex) //remove old columns
        propertyItemModel->removeColumn(propertyItemModel->columnCount()-1);

    QString command = "exiftool -s -c \"%02.6f\" \"" + folderName.replace("/", "//") + "*\""; ////
//    qDebug()<<"FPropertyTreeView::loadModel"<<folderName<<command<<processManager;

    processManager->startProcess(command
                                   , nullptr
                                   , [] (QWidget *parent, QString , QStringList result)
     {
        FPropertyTreeView *propertyTreeView = qobject_cast<FPropertyTreeView *>(parent);

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

//        qDebug()<<"loadModel2"<<parent<<result;
        QString folderFileName;
        QUrl folderFile;

        QMap<QString, QMap<QString, QString>> valueMap;
        QMap<QString, QString> fileMediaMap;
        QMap<QString, QStandardItem *> labelMap;

        for (int resultIndex=0;resultIndex<result.count();resultIndex++)
        {
            int indexOf = result[resultIndex].indexOf("======== ");
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

                if (labelString == "FileName" || labelString == "GeneratedName" || labelString == "CreateDate" || labelString == "GPSLatitude" || labelString == "GPSLongitude" || labelString == "GPSAltitude" || labelString == "Make" || labelString == "Model" || labelString == "Location" || labelString == "toName" || labelString == "toMeta")
                    labelMap[labelString] = topLevelItems["General"];
                else if (labelString == "ImageWidth" || labelString == "ImageHeight"  || labelString == "Duration")
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

        QMapIterator<QString, QString> iFile(fileMediaMap);
        QStringList labels;
        labels << "Property" << "Type" << "Diff";

        while (iFile.hasNext()) //all files
        {
            iFile.next();
            labels<<iFile.key();
        }
        propertyTreeView->propertyItemModel->setHorizontalHeaderLabels(labels);

        for (int i=firstFileColumnIndex; i< propertyTreeView->propertyItemModel->columnCount(); i++)
        {
//            propertyTreeView->setColumnWidth(i, propertyTreeView->columnWidth(i) * 2);
        }

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
            sublevelItems.append(new QStandardItem( "QString" ));
            sublevelItems.append(new QStandardItem( "false" )); //diff

            bool valueFound = false; //in case label is added by a non media file
            QString previousValue = "";
            QMapIterator<QString, QString> iFile(fileMediaMap);
            while (iFile.hasNext()) //all files
            {
                iFile.next();
                QString value = valueMap[iLabel.key()][iFile.key()];
                sublevelItems.append(new QStandardItem( value ));
                valueFound = valueFound || value != "";
                if (previousValue != "" && value != "" && value != previousValue)
                    sublevelItems[2] = new QStandardItem( "true" );
                previousValue = value;
//                qDebug()<< iFile.key() << ": " << iFile.value();
            }
            if (valueFound)
                iLabel.value()->appendRow(sublevelItems);
        }

        for (int col = 1; col < propertyTreeView->model()->columnCount(); ++col)
              propertyTreeView->frozenTableView->setColumnHidden(col, true);

        propertyTreeView->propertyProxyModel->setFilterRegExp(QRegExp(";1", Qt::CaseInsensitive,
                                                    QRegExp::FixedString));
        propertyTreeView->propertyProxyModel->setFilterKeyColumn(-1);

        propertyTreeView->expandAll();
        propertyTreeView->frozenTableView->expandAll();

//        propertyTreeView->scrollTo(propertyTreeView->propertyItemModel->index(3,propertyTreeView->propertyItemModel->columnCount()-1), QAbstractItemView::EnsureVisible);

//        propertyTreeView->diffData(QModelIndex());
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

