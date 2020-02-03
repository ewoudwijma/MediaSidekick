#include "apropertyeditordialog.h"
#include "astarrating.h"
#include "ui_propertyeditordialog.h"

#include <QtDebug>

#include <QSettings>

#include <QQmlApplicationEngine>
#include <QQuickView>
#include <QTimer>

#include <QQuickView>
#include <QQuickItem>

#include <aglobal.h>

APropertyEditorDialog::APropertyEditorDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::propertyEditorDialog)
{
    ui->setupUi(this);

    QStatusBar *bar = new QStatusBar(this);
    ui->statusBarLayout->addWidget(bar);

    ui->propertyTreeView->editMode = true;

    ui->updateProgressBar->setValue(0);

    //loadSettings
    Qt::CheckState checkState;
    if (QSettings().value("locationCheckBox").toBool())
        checkState = Qt::Checked;
    else
        checkState = Qt::Unchecked;
    ui->locationCheckBox->setCheckState(checkState);

    if (QSettings().value("cameraCheckBox").toBool())
        checkState = Qt::Checked;
    else
        checkState = Qt::Unchecked;
    ui->cameraCheckBox->setCheckState(checkState);

    if (QSettings().value("artistCheckBox").toBool())
        checkState = Qt::Checked;
    else
        checkState = Qt::Unchecked;
    ui->artistCheckBox->setCheckState(checkState);

    //tooltips
    ui->filterColumnsLineEdit->setToolTip(tr("<p><b>Property filters</b></p>"
                                              "<p><i>Filters on files shown</i></p>"
                                              "<ul>"
                                              "<li>Enter text to filter on properties</li>"
                                              "</ul>"));

    ui->locationCheckBox->setToolTip(tr("<p><b>Location, camera and artist</b></p>"
                                        "<p><i>Check what is part of the suggestedname</i></p>"
                                        "<ul>"
                                        "<li>Createdate: Mandatory part of suggested name</li>"
                                        "<li>Location: Geo coordinates and altitude</li>"
                                        "<li>Camera: Model, if no model then Make</li>"
                                        "<li>Artist: The creator of the media file</li>"
                                        "</ul>"));
    ui->cameraCheckBox->setToolTip(ui->locationCheckBox->toolTip());
    ui->artistCheckBox->setToolTip(ui->locationCheckBox->toolTip());
    ui->updateProgressBar->setToolTip(tr("<p><b>Progress bar</b></p>"
                                         "<p><i>Shows progress when updating properties</i></p>"
                                         ));
    ui->propertyTreeView->setToolTip(tr("<p><b>Property list</b></p>"
                                     "<p><i>Show the editable properties for the files of the selected folder</i></p>"
                                     "<ul>"
                                        "<li>Only files applying to the <u>filter</u> are shown</li>"
                                        "<li>Move over the column headers to see <u>column tooltips</u></li>"
                                        "<li><u>Rows</u></li>"
                                        "<ul>"
                                        "<li><u>General, Location, Camera and Labels</u>: Can be <u>updated</u>. Use Minimum, Delta and Maximum to update all the selected file or edit each file separately (press the update button to perform the update)</li>"
                                        "<ul>"
                                        "<li><u>CreateDate</u>: Format Delta: day;hour;minute;second</li>"
                                        "<li><u>GeoCoordinate</u>: Format Minimum and Maximum: Latitude(°);Longitude(°);Altitute(m)</li>. Format Delta: distance(m);bearing(°);altitude(m)"
                                        "<li><u>Make, Model and Artist: Drop down</u> shows all the values of the files in current file selection</li>"
                                        "<li><u>Keywords</u>: Multiple keywords seperated by semicolon (;)</li>"
                                        "<li><u>Rating</u>: One to five stars. </li>"
                                        "<li>Note: Currently the keywords and ratings of media files are not synchronised with keywords and ratings of Clips.</li>"
                                        "</ul>"
                                        "<li><u>Status</u>: <u>information properties</u></li>"
                                        "<ul>"
                                        "<li><u>Suggested name</u>: If suggested name differs from the filename it will be shown (press the rename button to rename the file)</li>"
                                        "</ul>"
                                        "<li><u>Artists, Keywords and Ratings</u>: <u>derived</u> from the properties in the labels section and will be updated if the corresponding labels property will be updated.</li>"
                                        "</ul>"
                                        "</ul>"
                                     ));

    ui->updateButton->setToolTip(tr("<p><b>%1</b></p>"
                                    "<p><i>Write properties to files</i></p>"
                                    "<ul>"
                                    "<li>Values are not directly written to the media file. This update button will do this. The following color coding is used</li>"
                                    "<ul>"
                                    "<li>Orange: Before updating: a value is updated but not yet written</li>"
                                    "<li>Green: After updating: The value is succesfully updated to the media file</li>"
                                    "<li>Red: After updating: The value is not updated to the media file. See the Status propery of each file to see what went wrong</li>"
                                    "</ul>"
                                    "<li>Derived values (Artists, Keywords and Ratings): Derived values are updated so other media viewing tools show the keywords, ratings and artists correctly. E.g. Windows Explorer properties, ACDSee etc. As there is no unified standard (each tool will use their own properties) this will be updated / improved in future releases of ACVC</li>"
                                    "<li>Currently writing to AVI and MP3 files is not supported. The status property will show if this attempted (Rename to suggested names is supported)</li>"
                                    "</ul>").arg(ui->updateButton->text()));
    ui->renameButton->setToolTip(tr("<p><b>%1</b></p>"
                                    "<p><i>Rename selected files to suggested name</i></p>"
                                    "<ul>"
                                    "<li></li>"
                                    "</ul>").arg(ui->renameButton->text()));
    ui->closeButton->setToolTip(tr("<p><b>%1</b></p>"
                                   "<p><i>Close the property editor</i></p>"
                                   ).arg(ui->closeButton->text()));

    //geocoding

    ui->mapQuickWidget->setToolTip(tr("<p><b>Map Viewer</b></p>"
                                      "<p><i>Shows all selected locations on a map</i></p>"
                                      ));

    ui->geocodingGroupBox->setToolTip(tr("<p><b>Geocoding</b></p>"
                                         "<p><i>Gets location coordinates from an address and enter the result in the GeoCoordinate minimum value</i></p>"
                                         ));

    //connects
    connect(ui->propertyTreeView, &APropertyTreeView::addJob, this, &APropertyEditorDialog::onAddJob);
    connect(ui->propertyTreeView, &APropertyTreeView::addToJob, this, &APropertyEditorDialog::onAddToJob);
    connect(ui->propertyTreeView, &APropertyTreeView::redrawMap, this, &APropertyEditorDialog::redrawMap);

    connect(ui->geocodingWidget, &AGeocoding::finished, this, &APropertyEditorDialog::onGeoCodingFinished);

//    spinnerLabel = new ASpinnerLabel(this);
//    spinnerLabel->start();

    QTimer::singleShot(0, this, [this]()->void
    {

    //initial processing

//                           if (propertyItemModel != nullptr)
//                                ui->propertyTreeView->propertyItemModel = propertyItemModel;
//                           else
//                                ui->propertyTreeView->onFolderIndexClicked(QModelIndex());

                           ui->propertyTreeView->onSuggestedNameFiltersClicked(ui->locationCheckBox, ui->cameraCheckBox, ui->artistCheckBox);

                       });
}

APropertyEditorDialog::~APropertyEditorDialog()
{
    qDebug()<<"destructor APropertyEditorDialog::~APropertyEditorDialog";
    delete ui;
}

void APropertyEditorDialog::on_closeButton_clicked()
{
    close();
}

void APropertyEditorDialog::closeEvent (QCloseEvent *event)
{
    if (checkExit())
        event->accept();
    else
        event->ignore();
}

bool APropertyEditorDialog::checkExit()
{
    bool exitYes = false;

    if (ui->propertyTreeView->changedIndexesMap.count() != 0)
    {
        QMessageBox::StandardButton reply;
         reply = QMessageBox::question(this, "Close", tr("There are %1 files with changes which are not saved yet. Are you sure you wanna quit?").arg(ui->propertyTreeView->changedIndexesMap.count()),
                                       QMessageBox::Yes|QMessageBox::No);

        if (reply == QMessageBox::Yes)
        {
            exitYes = true;
        }
    }
    else
        exitYes = true;

    if (exitYes)
    {
        ui->propertyTreeView->isLoading = true; //to avoid updates from mainwindow
        ui->propertyTreeView->propertyItemModel = new QStandardItemModel(); //to avoid updates from mainwindow
        accept();
    }

    return exitYes;
}

void APropertyEditorDialog::setProperties(QStandardItemModel *itemModel)
{
//    qDebug()<<"APropertyEditorDialog::setProperties"<<itemModel->rowCount();

    ui->propertyTreeView->onSuggestedNameFiltersClicked(ui->locationCheckBox, ui->cameraCheckBox, ui->artistCheckBox);

    ui->propertyTreeView->initModel(itemModel);

    onPropertiesLoaded();
}

void APropertyEditorDialog::onAddJob(QString folder, QString file, QString action, QString* id)
{
    emit addJob(folder, file, action, id);
//    qDebug()<<"APropertyEditorDialog::onAddJob"<<folder << file << action<<*id;
}

void APropertyEditorDialog::onAddToJob(QString id, QString log)
{
//    qDebug()<<"APropertyEditorDialog::onAddToJob"<<id<<log;
    emit addToJob(id, log);
}

void APropertyEditorDialog::on_filterColumnsLineEdit_textChanged(const QString &arg1)
{
    ui->propertyTreeView->onPropertyColumnFilterChanged(arg1);
    redrawMap();
}

void APropertyEditorDialog::on_locationCheckBox_clicked(bool checked)
{
    if (QSettings().value("locationCheckBox").toBool() != checked)
    {
        QSettings().setValue("locationCheckBox", checked);
        QSettings().sync();
        ui->propertyTreeView->onSuggestedNameFiltersClicked(ui->locationCheckBox, ui->cameraCheckBox, ui->artistCheckBox);
    }
}

void APropertyEditorDialog::on_cameraCheckBox_clicked(bool checked)
{
    if (QSettings().value("cameraCheckBox").toBool() != checked)
    {
        QSettings().setValue("cameraCheckBox", checked);
        QSettings().sync();
        ui->propertyTreeView->onSuggestedNameFiltersClicked(ui->locationCheckBox, ui->cameraCheckBox, ui->artistCheckBox);
    }
}

void APropertyEditorDialog::on_artistCheckBox_clicked(bool checked)
{
    if (QSettings().value("artistCheckBox").toBool() != checked)
    {
        QSettings().setValue("artistCheckBox", checked);
        QSettings().sync();
        ui->propertyTreeView->onSuggestedNameFiltersClicked(ui->locationCheckBox, ui->cameraCheckBox, ui->artistCheckBox);
    }
}

void APropertyEditorDialog::onPropertiesLoaded()
{
//    qDebug()<<"APropertyEditorDialog::onPropertiesLoaded"<< ui->propertyTreeView->propertyItemModel->columnCount();
    ui->propertyTreeView->setupModel(); //to set hidden rows, other stuff already done by mainwindow property treeview

    redrawMap();

    for (int column = firstFileColumnIndex; column < ui->propertyTreeView->model()->columnCount(); column++)
    {


        ui->propertyTreeView->propertyItemModel->horizontalHeaderItem(column)->setToolTip(tr("<p><b>File properties</b></p>"
                                                                                               "<p><i>Update property of each files seperately</i></p>"
                                                                                               "<ul>"
                                                                                               "<li></li>"
                                                                                               "</ul>"
                                                                                               ));
    }

    checkAndMatchPicasa();
}

void APropertyEditorDialog::redrawMap()
{
//    qDebug()<<"APropertyEditorDialog::redrawMap";
    QObject *target = qobject_cast<QObject *>(ui->mapQuickWidget->rootObject());

    QMetaObject::invokeMethod(target, "clearMapItems", Qt::AutoConnection);

    for(int col = firstFileColumnIndex; col < ui->propertyTreeView->propertyProxyModel->columnCount(); col++)
    {
        if (!ui->propertyTreeView->isColumnHidden(col))
        {
            QString fileName =  ui->propertyTreeView->propertyProxyModel->headerData(col, Qt::Horizontal).toString();

            QVariant *coordinate = new QVariant();
            ui->propertyTreeView->onGetPropertyValue(fileName, "GeoCoordinate", coordinate);

            QGeoCoordinate geoCoordinate = AGlobal().csvToGeoCoordinate(coordinate->toString());

//            qDebug()<<"coordinateString"<<coordinate->toString();

            QMetaObject::invokeMethod(target, "addMarker", Qt::AutoConnection, Q_ARG(QVariant, fileName), Q_ARG(QVariant, geoCoordinate.latitude()), Q_ARG(QVariant, geoCoordinate.longitude()));
        }
    }
    QMetaObject::invokeMethod(target, "fitViewportToMapItems", Qt::AutoConnection);
}

void APropertyEditorDialog::on_updateButton_clicked()
{
    ui->updateProgressBar->setValue(0);
    ui->updateProgressBar->setStyleSheet("QProgressBar::chunk {background: " + palette().highlight().color().name() + "}");

    redrawMap();
    ui->propertyTreeView->saveChanges(ui->updateProgressBar);
//    onPropertiesLoaded();
}

void APropertyEditorDialog::onGeoCodingFinished(QGeoCoordinate geoCoordinate)
{
    QObject *target = qobject_cast<QObject *>(ui->mapQuickWidget->rootObject());

//    qDebug()<<"APropertyEditorDialog::onGeoCodingFinished"<<geoCoordinate;
    ui->propertyTreeView->onSetPropertyValue("Minimum", "GeoCoordinate", QString::number(geoCoordinate.latitude()) + ";" + QString::number(geoCoordinate.longitude()) + ";" + QString::number(geoCoordinate.altitude()));
    QMetaObject::invokeMethod(target, "center", Qt::AutoConnection, Q_ARG(QVariant, geoCoordinate.latitude()), Q_ARG(QVariant, geoCoordinate.longitude()));
    redrawMap();
}

void APropertyEditorDialog::on_renameButton_clicked()
{
    QStringList fileNameList;
    QStringList newFileNameList;
    bool uniqueFileNames = true;

    for(int col = firstFileColumnIndex; col < ui->propertyTreeView->propertyProxyModel->columnCount(); col++)
    {
        if (!ui->propertyTreeView->isColumnHidden(col))
        {
            QString fileName =  ui->propertyTreeView->propertyProxyModel->headerData(col, Qt::Horizontal).toString();
            if (!fileName.contains(".mlt") && !fileName.contains("*.xml"))
            {
                QVariant *suggestedName = new QVariant();

                ui->propertyTreeView->onGetPropertyValue(fileName, "SuggestedName", suggestedName);

                if (suggestedName->toString() != "")
                {
                    uniqueFileNames = !fileNameList.contains(fileName);
                    fileNameList << fileName;
                    newFileNameList << suggestedName->toString();
                }
            }
        }
    }

    if (!uniqueFileNames)
    {
        QMessageBox::information(this, "Rename", "Not all filenames unique. Cannot rename");
    }
    else if (fileNameList.count() > 0)
    {
        QString folderName = QSettings().value("LastFolder").toString();

        QMessageBox::StandardButton reply;
         reply = QMessageBox::question(this, "Rename " + QString::number(fileNameList.count()) + " File(s)", "Are you sure you want to rename " + fileNameList.join(", ") + " and its supporting files (srt and txt) to " + newFileNameList.join(", ") + ".* ?",
                                       QMessageBox::Yes|QMessageBox::No);

         if (reply == QMessageBox::Yes)
         {

             for (int i=0; i< fileNameList.count();i++)
             {
    //             QString fileName = fileNameList[i];
                 emit releaseMedia(fileNameList[i]); //to stop the video
                 QFile file(folderName + fileNameList[i]);
                 QString extensionString = fileNameList[i].mid(fileNameList[i].lastIndexOf(".")); //.avi ..mp4 etc.
//                 qDebug()<<"Rename"<<fileNameList[i]<<newFileNameList[i] + extensionString;
                 if (file.exists())
                    file.rename(folderName + newFileNameList[i] + extensionString);

                 int lastIndex = fileNameList[i].lastIndexOf(".");
                 if (lastIndex > -1)
                 {
                     QFile *file = new QFile(folderName + fileNameList[i].left(fileNameList[i].lastIndexOf(".")) + ".srt");
                     if (file->exists())
                        file->rename(folderName + newFileNameList[i] + ".srt");

                     file = new QFile(folderName + fileNameList[i].left(fileNameList[i].lastIndexOf(".")) + ".txt");
                     if (file->exists())
                        file->rename(folderName + newFileNameList[i] + ".txt");
                 }
//                 qDebug()<<"on_renameButton_clicked colorChanged";
                 ui->propertyTreeView->colorChanged = "yes";
                 ui->propertyTreeView->onSetPropertyValue(fileNameList[i], "SuggestedName", QBrush(QColor(34,139,34, 50)), Qt::BackgroundRole);

             }
             emit reloadClips();
             ui->propertyTreeView->isLoading = true; //to avoid mainwindow propertytreeview setupmodel to change properties here
             emit reloadProperties();
         }
    }
    else
    {
            QMessageBox::information(this, "Rename", "Nothing to do");
    }

} //on_renameButton_clicked

void APropertyEditorDialog::checkAndMatchPicasa()
{
    QString folderName = QSettings().value("LastFolder").toString();
    QString fileName = ".picasa.ini";
    QFile file(folderName + fileName);


    if (file.open(QIODevice::ReadOnly))
    {
        QString *processId = new QString();
        QStringList filesImported;
        QStringList filesNotImported;
        bool propertiesSet = false;

        onAddJob(folderName, fileName, "Import picasa", processId);
        QString fileName;
        bool filePropertiesExist;
        QTextStream in(&file);
        while (!in.atEnd())
           {
              QString line = in.readLine();
              if (line.indexOf("[") == 0)
              {
                  fileName = line.mid(1,line.length() -2);
                  QVariant *directoryName = new QVariant;
                  filePropertiesExist = ui->propertyTreeView->onGetPropertyValue(fileName, "Directory", directoryName);
                  if (filePropertiesExist)
                  {
                      onAddToJob(*processId, fileName + " added\n");
                      filesImported << fileName;
                  }
                  else
                  {
                      if (fileName != "Picasa") //first lines of ini file not in log
                        filesNotImported << fileName;
                  }
              }
              else if (filePropertiesExist)
              {
                  if (line.contains("keywords="))
                  {
                      QVariant *keywords = new QVariant;
                      ui->propertyTreeView->onGetPropertyValue(fileName, "Keywords", keywords);

                      if (keywords->toString() != line.mid(9))
                      {
                        ui->propertyTreeView->onSetPropertyValue(fileName, "Keywords", line.mid(9));
                        propertiesSet = true;
                      }
                      onAddToJob(*processId, " - " + line + " => Keywords: " + line.mid(9) + "\n");
                  }
                  else if (line.contains("star=yes"))
                  {
                      QVariant *rating = new QVariant;
                      ui->propertyTreeView->onGetPropertyValue(fileName, "Rating", rating);
                      AStarRating starRating = qvariant_cast<AStarRating>( *rating);
//                      qDebug()<<"Picasa star"<<starRating.starCount();
                      if (starRating.starCount() != 3)
                      {
                        ui->propertyTreeView->onSetPropertyValue(fileName, "Rating", QVariant::fromValue(AStarRating(3)));
                        propertiesSet = true;
                      }
                      onAddToJob(*processId, " - " + line + " => Rating: 3\n");
                  }
                  else
                    onAddToJob(*processId, " - " + line + " ignored\n");
              }
           }

        if (filesNotImported.count() > 0)
        {
            onAddToJob(*processId, "\nFiles not found in current folder:\n");

        }
        foreach (QString line, filesNotImported)
        {
            onAddToJob(*processId, " - " + line + "\n");
        }

        QStatusBar *bar = qobject_cast<QStatusBar *>(ui->statusBarLayout->itemAt(0)->widget());
        if (filesImported.count() > 0)
        {
            if (propertiesSet)
                bar->showMessage(tr("Picasa found properties for %1 and suggested updates. Go to Jobs to see details").arg(filesImported.join(", ")));
            else
                bar->showMessage(tr("Picasa found properties for %1 but no updates needed. Go to Jobs to see details").arg(filesImported.join(", ")));
        }
        else
            bar->showMessage(tr("Picasa file found but no matching files. Go to Jobs to see details"), 10000);

        file.close();
    }
//    else
//        onAddToJob(*processId, "Could not open " + file.fileName() + "\n");

}

void APropertyEditorDialog::on_refreshButton_clicked()
{
    ui->propertyTreeView->isLoading = true; //to avoid mainwindow propertytreeview setupmodel to change properties here
    emit reloadProperties();
    ui->updateProgressBar->setValue(0);
    ui->updateProgressBar->setStyleSheet("QProgressBar::chunk {background: " + palette().highlight().color().name() + "}");

//    ui->propertyTreeView->isLoading = false;
}
