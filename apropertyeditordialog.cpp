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
    ui->propertyTreeView->setToolTip(tr("<p><b>Property list by Exiftool</b></p>"
                                     "<p><i>Show the editable properties for the files of the selected folder</i></p>"
                                     "<ul>"
                                        "<li>Only files applying to the <u>filter</u> are shown</li>"
                                        "<li>Move over the column headers to see <u>column tooltips</u></li>"
                                        "<li><b>right click</b>: show cell contents in separate window</li>"
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
                                        "<li><u>Suggested name</u>: Shown, if suggested name differs from the filename (press the rename button to rename the file)</li>"
                                        "</ul>"
                                        "<li><u>Artists, Keywords and Ratings</u>: <u>derived</u> from the properties in the labels section and is updated if the corresponding labels property is updated.</li>"
                                        "</ul>"
                                        "</ul>"
                                     ));

    ui->propertyTreeView->propertyItemModel->horizontalHeaderItem(minimumIndex)->setToolTip(tr("<p><b>Minimum, delta and max</b></p>"
                                                                                               "<p><i>Update property of multiple files at once</i></p>"
                                                                                               "<ul>"
                                                                                                  "<li>Only files applying to the <u>filter</u> are updated</li>"
                                                                                               "<li>Types</li>"
                                                                                               "<ul>"
                                                                                               "<li><u>Range</u> from Minimum via Delta to Maximum (CreateDate and GeoCoordinate)</li>"
                                                                                               "<ul>"
                                                                                                   "<li><u>Minimum</u>: value of first file</li>"
                                                                                                   "<li><u>Delta</u>: increase value with delta for subsequent files</li>"
                                                                                                   "<li><u>Maximum</u>: value of last file</li>"
                                                                                               "</ul>"
                                                                                               "<li><u>Drop down</u> (Make, Model and Artist): Shows all values found in selected files and choose one to update all the selected files with</li>"
                                                                                               "<li><u>Multiple values</u> (Keywords): Shows all values found in selected files. Edit Minumum value (seperate value by semicolon (;)) to update all selected files with new values</li>"
                                                                                               "<li><u>Stars</u> (Rating): Shows the Minumum and Maximum of all stars found in selected files. Change stars to update all selected files with new stars</li>"
                                                                                               "</ul>"
                                                                                               "</ul>"
                                                                                               ));
    ui->propertyTreeView->propertyItemModel->horizontalHeaderItem(deltaIndex)->setToolTip(ui->propertyTreeView->propertyItemModel->horizontalHeaderItem(minimumIndex)->toolTip());
    ui->propertyTreeView->propertyItemModel->horizontalHeaderItem(maximumIndex)->setToolTip(ui->propertyTreeView->propertyItemModel->horizontalHeaderItem(minimumIndex)->toolTip());

    ui->updateButton->setToolTip(tr("<p><b>%1</b></p>"
                                    "<p><i>Write properties to files</i></p>"
                                    "<ul>"
                                    "<li>Values are not directly written to the media file. This update button will do this. The following <b>color coding</b> is used</li>"
                                    "<ul>"
                                    "<li>Orange: Before updating: a value is updated but not yet written</li>"
                                    "<li>Green: After updating: The value is succesfully updated to the media file</li>"
                                    "<li>Red: After updating: The value is not updated to the media file. See the Status propery of each file to see what went wrong</li>"
                                    "</ul>"
                                    "<li><b>Derived values (Artists, Keywords and Ratings)</b>: Derived values are updated so other media viewing tools show the keywords, ratings and artists correctly. E.g. Windows Explorer properties, ACDSee etc. As there is no unified standard (each tool will use their own properties). (this will be updated / improved in future releases of Media Sidekick)</li>"
                                    "<li>Currently writing to <b>AVI and MP3 files is not supported</b>. The status property will show if this attempted (<b>Rename to suggested names is supported</b>)</li>"
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
    connect(ui->propertyTreeView, &APropertyTreeView::redrawMap, this, &APropertyEditorDialog::onRedrawMap);

    connect(ui->geocodingWidget, &AGeocoding::finished, this, &APropertyEditorDialog::onGeoCodingFinished);

    connect(this, &APropertyEditorDialog::loadProperties, ui->propertyTreeView, &APropertyTreeView::onloadProperties);

//    connect(ui->propertyTreeView, &APropertyTreeView::mousePressEvent, this, &APropertyEditorDialog::propertyTreeMousePressEvent);

    QTimer::singleShot(0, this, [this]()->void
    {
                           ui->propertyTreeView->onSuggestedNameFiltersClicked(ui->locationCheckBox, ui->cameraCheckBox, ui->artistCheckBox);

                       });
}

APropertyEditorDialog::~APropertyEditorDialog()
{
    qDebug()<<"APropertyEditorDialog::~APropertyEditorDialog";
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

void APropertyEditorDialog::setProperties(QStandardItemModel *itemModel, QStringList filesMap)
{
//    qDebug()<<"APropertyEditorDialog::setProperties"<<itemModel->rowCount();

    ui->propertyTreeView->onSuggestedNameFiltersClicked(ui->locationCheckBox, ui->cameraCheckBox, ui->artistCheckBox);

    ui->propertyTreeView->initModel(itemModel);

    ui->propertyTreeView->filesMap = filesMap;

    onPropertiesLoaded();
}

void APropertyEditorDialog::on_filterColumnsLineEdit_textChanged(const QString &arg1)
{
    ui->propertyTreeView->onPropertyColumnFilterChanged(arg1);
    onRedrawMap();
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

    onRedrawMap();

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

void APropertyEditorDialog::onRedrawMap()
{
//    qDebug()<<"APropertyEditorDialog::onRedrawMap";
    QObject *target = qobject_cast<QObject *>(ui->mapQuickWidget->rootObject());

    QMetaObject::invokeMethod(target, "clearMapItems", Qt::AutoConnection);

    for(int col = firstFileColumnIndex; col < ui->propertyTreeView->propertyProxyModel->columnCount(); col++)
    {
        if (!ui->propertyTreeView->isColumnHidden(col))
        {
            QString folderFileName = ui->propertyTreeView->filesMap[col - firstFileColumnIndex];

            QVariant *coordinate = new QVariant();
            ui->propertyTreeView->onGetPropertyValue(folderFileName, "GeoCoordinate", coordinate);

            QGeoCoordinate geoCoordinate = AGlobal().csvToGeoCoordinate(coordinate->toString());

//            qDebug()<<"coordinateString"<<coordinate->toString();

            QMetaObject::invokeMethod(target, "addMarker", Qt::AutoConnection, Q_ARG(QVariant, folderFileName), Q_ARG(QVariant, geoCoordinate.latitude()), Q_ARG(QVariant, geoCoordinate.longitude()));
        }
    }
    QMetaObject::invokeMethod(target, "fitViewportToMapItems", Qt::AutoConnection);
}

void APropertyEditorDialog::on_updateButton_clicked()
{
    ui->updateProgressBar->setValue(0);
    ui->updateProgressBar->setStyleSheet("QProgressBar::chunk {background: " + palette().highlight().color().name() + "}");

    onRedrawMap();
    ui->propertyTreeView->saveChanges(ui->updateProgressBar);
//    onPropertiesLoaded();
}

void APropertyEditorDialog::onGeoCodingFinished(QGeoCoordinate geoCoordinate)
{
    QObject *target = qobject_cast<QObject *>(ui->mapQuickWidget->rootObject());

    qDebug()<<"APropertyEditorDialog::onGeoCodingFinished"<<geoCoordinate;
    ui->propertyTreeView->onSetPropertyValue("Minimum", "GeoCoordinate", QString::number(geoCoordinate.latitude()) + ";" + QString::number(geoCoordinate.longitude()) + ";" + QString::number(geoCoordinate.altitude()));
    QMetaObject::invokeMethod(target, "center", Qt::AutoConnection, Q_ARG(QVariant, geoCoordinate.latitude()), Q_ARG(QVariant, geoCoordinate.longitude()));
    onRedrawMap();
}

void APropertyEditorDialog::on_renameButton_clicked()
{
    QStringList folderFileNameList;
    QStringList suggestedFolderFileNameList;
    bool uniqueFileNames = true;

    for(int col = firstFileColumnIndex; col < ui->propertyTreeView->propertyProxyModel->columnCount(); col++)
    {
        if (!ui->propertyTreeView->isColumnHidden(col))
        {
            QString folderFileName = ui->propertyTreeView->filesMap[col - firstFileColumnIndex];
            QString fileNameLow = folderFileName.toLower();
            int lastIndexOf = fileNameLow.lastIndexOf(".");
            QString extension = fileNameLow.mid(lastIndexOf + 1);

            if (!AGlobal().projectExtensions.contains(extension, Qt::CaseInsensitive))
            {
                QVariant *suggestedFolderFileName = new QVariant();

                ui->propertyTreeView->onGetPropertyValue(folderFileName, "SuggestedName", suggestedFolderFileName);

                if (suggestedFolderFileName->toString() != "")
                {
                    if (suggestedFolderFileNameList.contains(suggestedFolderFileName->toString()))
                            uniqueFileNames = false;
                    else
                    {
                        folderFileNameList << folderFileName;
                        suggestedFolderFileNameList << suggestedFolderFileName->toString();
                    }
                }
            }
        }
    }

    if (!uniqueFileNames)
    {
        QMessageBox::information(this, "Rename", "Not all filenames unique. Cannot rename");
    }
    else if (folderFileNameList.count() > 0)
    {
        QMessageBox::StandardButton reply;
         reply = QMessageBox::question(this, "Rename " + QString::number(folderFileNameList.count()) + " File(s)", "Are you sure you want to rename " + folderFileNameList.join(", ") + " and its supporting files (srt and txt) to " + suggestedFolderFileNameList.join(", ") + ".* ?",
                                       QMessageBox::Yes|QMessageBox::No);

         if (reply == QMessageBox::Yes)
         {

             for (int i=0; i< folderFileNameList.count();i++)
             {
                 int lastIndexOf = folderFileNameList[i].lastIndexOf("/");
                 QString folderName = folderFileNameList[i].left(lastIndexOf + 1);
                 QString fileName = folderFileNameList[i].mid(lastIndexOf + 1);

                 emit releaseMedia(folderName, fileName); //to stop the video
                 QFile file(folderFileNameList[i]);
                 QString extensionString = fileName.mid(fileName.lastIndexOf(".")); //.avi ..mp4 etc.

//                 qDebug()<<"Rename"<<fileNameList[i]<<suggestedFileNameList[i] + extensionString;
                 if (file.exists())
                    file.rename(suggestedFolderFileNameList[i] + extensionString);

                 int lastIndex = fileName.lastIndexOf(".");
                 if (lastIndex > -1)
                 {
                     QFile *file = new QFile(folderFileNameList[i].left(folderFileNameList[i].lastIndexOf(".")) + ".srt");
                     if (file->exists())
                        file->rename(suggestedFolderFileNameList[i] + ".srt");

                     file = new QFile(folderFileNameList[i].left(folderFileNameList[i].lastIndexOf(".")) + ".txt");
                     if (file->exists())
                        file->rename(suggestedFolderFileNameList[i] + ".txt");
                 }
//                 qDebug()<<"on_renameButton_clicked colorChanged";
                 ui->propertyTreeView->colorChanged = "yes";
                 ui->propertyTreeView->onSetPropertyValue(folderFileNameList[i], "SuggestedName", QBrush(QColor(34,139,34, 50)), Qt::BackgroundRole);

             }

             ui->propertyTreeView->isLoading = true; //to avoid mainwindow propertytreeview setupmodel to change properties here
             emit loadProperties();
         }
    }
    else
    {
            QMessageBox::information(this, "Rename", "Nothing to do");
    }

} //on_renameButton_clicked

void APropertyEditorDialog::checkAndMatchPicasa()
{
    QString selectedFolderName = QSettings().value("selectedFolderName").toString();
    QString fileName = ".picasa.ini";
    QFile file(selectedFolderName + fileName);


    if (file.open(QIODevice::ReadOnly))
    {
//        QString *processId = new QString();
        QStringList filesImported;
        QStringList filesNotImported;
        bool propertiesSet = false;

//        onAddJob(folderName, fileName, "Import picasa", processId);
        QString fileName;
        bool filePropertiesExist;
        QTextStream in(&file);
        while (!in.atEnd())
           {
              QString line = in.readLine();
              if (line.indexOf("[") == 0)
              {
                  fileName = line.mid(1,line.length() -2); //picasa file has only filenames not foldernames.
                  QVariant *directoryName = new QVariant;
                  filePropertiesExist = ui->propertyTreeView->onGetPropertyValue(selectedFolderName + fileName, "Directory", directoryName);
                  if (filePropertiesExist)
                  {
//                      onAddToJob(*processId, fileName + " added\n");
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
                      ui->propertyTreeView->onGetPropertyValue(selectedFolderName + fileName, "Keywords", keywords);

                      if (keywords->toString() != line.mid(9))
                      {
                        ui->propertyTreeView->onSetPropertyValue(selectedFolderName + fileName, "Keywords", line.mid(9));
                        propertiesSet = true;
                      }
//                      onAddToJob(*processId, " - " + line + " => Keywords: " + line.mid(9) + "\n");
                  }
                  else if (line.contains("star=yes"))
                  {
                      QVariant *rating = new QVariant;
                      ui->propertyTreeView->onGetPropertyValue(selectedFolderName + fileName, "Rating", rating);
                      AStarRating starRating = qvariant_cast<AStarRating>( *rating);
//                      qDebug()<<"Picasa star"<<starRating.starCount();
                      if (starRating.starCount() != 3)
                      {
                        ui->propertyTreeView->onSetPropertyValue(selectedFolderName + fileName, "Rating", QVariant::fromValue(AStarRating(3)));
                        propertiesSet = true;
                      }
//                      onAddToJob(*processId, " - " + line + " => Rating: 3\n");
                  }
//                  else
//                    onAddToJob(*processId, " - " + line + " ignored\n");
              }
           }

        if (filesNotImported.count() > 0)
        {
//            onAddToJob(*processId, "\nFiles not found in current folder:\n");

        }
        foreach (QString line, filesNotImported)
        {
//            onAddToJob(*processId, " - " + line + "\n");
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
    emit loadProperties();
    ui->updateProgressBar->setValue(0);
    ui->updateProgressBar->setStyleSheet("QProgressBar::chunk {background: " + palette().highlight().color().name() + "}");

//    ui->propertyTreeView->isLoading = false;
}

void APropertyEditorDialog::onLoadProperties()
{
    ui->propertyTreeView->exiftoolMap = exifToolMap;

    ui->propertyTreeView->isLoading = true; //to avoid mainwindow propertytreeview setupmodel to change properties here
    emit loadProperties();
    ui->updateProgressBar->setValue(0);
    ui->updateProgressBar->setStyleSheet("QProgressBar::chunk {background: " + palette().highlight().color().name() + "}");

//    ui->propertyTreeView->isLoading = false;
}
