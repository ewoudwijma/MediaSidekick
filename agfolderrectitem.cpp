#include "agfolderrectitem.h"

#include <QDesktopServices>

#include "apropertyeditordialog.h"
#include "apropertytreeview.h"
#include "aexport.h"

#include <QDir>
#include <QFormLayout>
#include <QInputDialog>
#include <QPushButton>
#include <QSettings>
#include <QTextBrowser>
#include <QVBoxLayout>

#include <qmath.h>

#include <QMessageBox>

AGFolderRectItem::AGFolderRectItem(QGraphicsItem *parent, QFileInfo fileInfo) :
    AGViewRectItem(parent, fileInfo)
{
    this->mediaType = "Folder";
    this->itemType = "Base";

    QPen pen(Qt::transparent);
    setPen(pen);

    setRect(QRectF(0, 0, 200 * 9.0 / 16.0 + 4, 200 * 9.0 / 16.0)); //+2 is minimum to get dd-mm-yyyy not wrapped (+2 extra to be sure)

    setData(itemTypeIndex, itemType);
    setData(mediaTypeIndex, mediaType);

    updateToolTip();

    pictureItem = new QGraphicsPixmapItem(this);
    QImage image = QImage(":/images/Folder.png");
    QPixmap pixmap = QPixmap::fromImage(image);//.scaled(QSize(200,200 * myImage.height() / myImage.width()))
    pictureItem->setPixmap(pixmap);
    if (image.height() != 0)
        pictureItem->setScale(200.0 * 9.0 / 16.0 / image.height() * 0.8);

    pictureItem->setData(mediaTypeIndex, "Folder");
    pictureItem->setData(itemTypeIndex, "SubPicture");

    setTextItem(QTime(), QTime());
}

void AGFolderRectItem::setTextItem(QTime time, QTime totalTime)
{
    if (subLogItem == nullptr)
    {
        newSubLogItem();
    }

    if (subLogItem != nullptr)
    {
        if (time != QTime())
            AGViewRectItem::setTextItem(time, totalTime);
        else
        {
            duration = 0;
            int foundGroups = 0;

            foreach (MGroupRectItem *groupItem, groups)
            {
                if (groupItem->timelineGroupItem->clips.count() > 0)
                {
                    duration = max(duration, groupItem->duration);
                    foundGroups++;
                }
            }

//            qDebug()<<__func__<<"AGFolderRectItem"<<fileInfo.fileName()<<duration<<time<<totalTime;

            subLogItem->setHtml(tr("<p>%1</p><p><small><i>%2 %3</i></small></p><p><small><i>%4</i></small></p>").arg(
                                    fileInfo.fileName(),
                                    QString::number(foundGroups) + " track" + (foundGroups>1?"s":""),
                                    QTime::fromMSecsSinceStartOfDay(duration).toString(),
                                    lastOutputString));
        }
    }
}


//https://stackoverflow.com/questions/34036848/how-to-suppress-unicode-characters-in-qstring-or-convert-to-latin1
QString cleanQString(QString toClean) {

    QString toReturn="";

    for(int i=0;i<toClean.size();i++){
        if(toClean.at(i).unicode()<256){
            toReturn.append(toClean.at(i));
        }
    }

    return toReturn;


}

void AGFolderRectItem::onItemRightClicked(QPoint pos)
{

    fileContextMenu->clear();

    fileContextMenu->addAction(new QAction("Property manager",fileContextMenu));
    fileContextMenu->actions().last()->setIcon(qApp->style()->standardIcon(QStyle::SP_DirOpenIcon));
    connect(fileContextMenu->actions().last(), &QAction::triggered, [=]()
    {
        QDialog *propertyViewerDialog = new QDialog();

        propertyViewerDialog->setWindowTitle("Media Sidekick Property viewer");

        propertyViewerDialog->setWindowFlags( propertyViewerDialog->windowFlags() |  Qt::WindowMaximizeButtonHint);

        QRect savedGeometry = QSettings().value("Geometry").toRect();
        savedGeometry.setX(savedGeometry.x() + savedGeometry.width() * .05);
        savedGeometry.setY(savedGeometry.y() + savedGeometry.height() * .05);
        savedGeometry.setWidth(savedGeometry.width() * .9);
        savedGeometry.setHeight(savedGeometry.height() * .9);
        propertyViewerDialog->setGeometry(savedGeometry);

        QVBoxLayout *mainLayout = new QVBoxLayout(propertyViewerDialog);

        QHBoxLayout *filesLayout = new QHBoxLayout();
        mainLayout->addLayout(filesLayout);

        QLineEdit *filterFilesLineEdit = new QLineEdit();
        filterFilesLineEdit->setPlaceholderText("filter files...");
        filesLayout->addWidget(filterFilesLineEdit);

        QPushButton *editorButton = new QPushButton();
        editorButton->setText("Editor");
        filesLayout->addWidget(editorButton);

        QCheckBox *diffCheckBox = new QCheckBox();
        diffCheckBox->setText("Diff");
        diffCheckBox->setTristate(true);
        diffCheckBox->setCheckState(Qt::PartiallyChecked);
        filesLayout->addWidget(diffCheckBox);

        QPushButton *refreshButton = new QPushButton();
        refreshButton->setText("Refresh");
        filesLayout->addWidget(refreshButton);

        filesLayout->addStretch();

        QHBoxLayout *propertiesLayout = new QHBoxLayout();
        mainLayout->addLayout(propertiesLayout);

        QLineEdit *filterPropertiesLineEdit = new QLineEdit();
        filterPropertiesLineEdit->setPlaceholderText("filter properties...");
        propertiesLayout->addWidget(filterPropertiesLineEdit);

        APropertyTreeView *propertyTreeView = new APropertyTreeView();

        //properties
//        propertyTreeView->setToolTip(tr("<p><b>Property list by Exiftool</b></p>"
//                                         "<p><i>Show the properties (Metadata / EXIF data) for files of the selected folder</i></p>"
//                                         "<ul>"
//                                            "<li>Only properties and files applying to the <b>filters</b> are shown</li>"
//                                            "<ul>"
//                                            "<li><b>Filter files</b>: only the columns matching are shown</li>"
//                                            "<li><b>Filter properties</b>: only the rows matching are shown</li>"
//                                            "<li><b>Diff</b>: show only properties which differs between the files, are equal or both</li>"
//                                            "</ul>"
//                                            "<li>Properties belonging to the <b>selected file</b> are highlighted in blue</li>"
//                                         "</ul>"
//                                         ));

        editorButton->setToolTip(tr("<p><b>Property editor</b></p>"
                                                    "<p><i>Opens a new window to edit properties</i></p>"
                                                    ));

        filterPropertiesLineEdit->setToolTip(tr("<p><b>Property filters</b></p>"
                                                  "<p><i>Filters on properties or files shown</i></p>"
                                                  "<ul>"
                                                  "<li>Enter text to filter on properties. E.g. date shows all the date properties</li>"
                                                  "</ul>"));
        filterFilesLineEdit->setToolTip(filterPropertiesLineEdit->toolTip());

        diffCheckBox->setToolTip(tr("<p><b>Diff</b></p>"
                                                "<p><i>Determines which properties are shown based on if they are different or the same accross the movie files</i></p>"
                                                "<ul>"
                                                "<li>Unchecked: Only properties which are the same for all files</li>"
                                                "<li>Checked: Only properties which are different between files</li>"
                                                "<li>Partially checked (default): All properties</li>"
                                                "</ul>"));
        refreshButton->setToolTip(tr("<p><b>Refresh</b></p>"
                                         "<p><i>Reloads all the properties</i></p>"
                                         ));

        connect(filterPropertiesLineEdit, &QLineEdit::textChanged, [=]
        {
            propertyTreeView->onPropertyFilterChanged(filterPropertiesLineEdit, diffCheckBox);
        });

        connect(diffCheckBox, &QCheckBox::clicked, [=]
        {
            propertyTreeView->onPropertyFilterChanged(filterPropertiesLineEdit, diffCheckBox);
        });

        connect(filterFilesLineEdit, &QLineEdit::textChanged, [=]
        {
            propertyTreeView->onPropertyColumnFilterChanged(filterFilesLineEdit->text());
        });

        connect(editorButton, &QPushButton::clicked, [=]
        {
//            statusBar->showMessage("Opening editor window", 5000);

//            videoWidget->onReleaseMedia(videoWidget->selectedFolderName, videoWidget->selectedFileName);
//            spinnerLabel->start();

            APropertyEditorDialog *propertyEditorDialog = new APropertyEditorDialog();

            QRect savedGeometry = QSettings().value("Geometry").toRect();
            savedGeometry.setX(savedGeometry.x() + savedGeometry.width() * .05);
            savedGeometry.setY(savedGeometry.y() + savedGeometry.height() * .05);
            savedGeometry.setWidth(savedGeometry.width() * .9);
            savedGeometry.setHeight(savedGeometry.height() * .9);
            propertyEditorDialog->setGeometry(savedGeometry);

        //    propertyEditorDialog->propertyItemModel = propertyTreeView->propertyItemModel;

//            connect(propertyEditorDialog, &APropertyEditorDialog::releaseMedia, videoWidget, &AVideoWidget::onReleaseMedia);
            connect(propertyEditorDialog, &APropertyEditorDialog::loadProperties, propertyTreeView, &APropertyTreeView::onloadProperties);

//            connect(this, &MainWindow::propertiesLoaded, propertyEditorDialog, &APropertyEditorDialog::onPropertiesLoaded);

            propertyEditorDialog->setProperties(propertyTreeView->propertyItemModel, propertyTreeView->filesList);

//            spinnerLabel->stop();

            propertyEditorDialog->setWindowModality(Qt::NonModal);

            propertyEditorDialog->show();
        });

        connect(refreshButton, &QPushButton::clicked, [=]
        {
            propertyTreeView->onloadProperties();
        });

        foreach (QGraphicsItem *item, scene()->items())
        {
            if (item->data(mediaTypeIndex).toString() == "MediaFile" && item->data(itemTypeIndex).toString() == "Base")
            {
                AGMediaFileRectItem *mediaItem = (AGMediaFileRectItem *)item;

                QMapIterator<QString, QMap<QString, MMetaDataStruct>> categoryIterator(mediaItem->exiftoolCategoryProperyMap);
                while (categoryIterator.hasNext()) //for each category
                {
                    categoryIterator.next();

                    foreach (MMetaDataStruct metaDataStruct, categoryIterator.value()) //for each property
                    {
                        propertyTreeView->exiftoolCategoryProperyFileMap[categoryIterator.key()][metaDataStruct.propertyName][mediaItem->fileInfo.absoluteFilePath()] = metaDataStruct;
                    }
                }
            }
        }

        propertyTreeView->onloadProperties();

        mainLayout->addWidget(propertyTreeView);

        propertyViewerDialog->setWindowModality(Qt::NonModal);

        propertyViewerDialog->show();

    });

    fileContextMenu->actions().last()->setToolTip(tr("<p><b>%1</b></p>"
                                        "<p><i>Property manager for %2</i></p>"
                                              ).arg(fileContextMenu->actions().last()->text(), fileInfo.fileName())); //not effective!


    fileContextMenu->addAction(new QAction("Download media",fileContextMenu));
    fileContextMenu->actions().last()->setIcon(qApp->style()->standardIcon(QStyle::SP_DirOpenIcon));
    connect(fileContextMenu->actions().last(), &QAction::triggered, [=]()
    {
//        bool ok;
//        QString defaultValue = QSettings().value("DownloadMediaURL").toString();
//        QString text = QInputDialog::getText(this->scene()->views().first(), "Download Media",
//                                             "URL:                                                                             ", QLineEdit::Normal,
//                                             "", &ok); //add spaces in the label as workaround to make the dialog wider.

        QDialog *dialog = new QDialog(this->scene()->views().first());
        dialog->setWindowTitle("Download Media");
        dialog->setMinimumWidth(500);

        QVBoxLayout *mainLayout = new QVBoxLayout(dialog);

        QFormLayout *formLayout = new QFormLayout();
        mainLayout->addLayout(formLayout);

        QLineEdit *urlLineEdit = new QLineEdit();
        urlLineEdit->setPlaceholderText("URL...");
//        mainLayout->addWidget(urlLineEdit);
        formLayout->addRow("URL:", urlLineEdit);

        QCheckBox *audioOnlyCheckBox = new QCheckBox();
        formLayout->addRow("Audio Only:", audioOnlyCheckBox);

        QCheckBox *mp3Mp4CheckBox = new QCheckBox();
        formLayout->addRow("Export to mp3/mp4:", mp3Mp4CheckBox);
        mp3Mp4CheckBox->setToolTip("Find the best mp3/mp4 codec available. If not checked default youtube formats are used (e.g. Webm, Opus)");

        QHBoxLayout *buttonLayout = new QHBoxLayout();
        mainLayout->addLayout(buttonLayout);

        QPushButton *okButton = new QPushButton();
        okButton->setText("OK");
        buttonLayout->addWidget(okButton);

        QPushButton *cancelButton = new QPushButton();
        cancelButton->setText("Cancel");
        buttonLayout->addWidget(cancelButton);

        connect(okButton, &QPushButton::clicked, [=] (bool checked)
        {
//            QPushButton *button = qobject_cast<QPushButton *)sender();
//            qDebug()<<"Download Media Clicked"<<checked<<urlLineEdit->text();

            //            QSettings().setValue("DownloadMediaURL", text);
            //            QSettings().sync();

            QString recycleFolderName = QSettings().value("selectedFolderName").toString() + "MSKRecycleBin/";
            QDir recycleDir(recycleFolderName);
            if (!recycleDir.exists())
                recycleDir.mkpath(".");

            AGProcessAndThread *process = new AGProcessAndThread(this);
                        //https://ostechnix.com/youtube-dl-tutorial-with-examples-for-beginners/

            QString command = "youtube-dl";

            if (audioOnlyCheckBox->isChecked())
            {
                if (mp3Mp4CheckBox->isChecked())
                    command += " -x --audio-format mp3";
                else
                    command += " -x";
            }
            else
            {
                if (mp3Mp4CheckBox->isChecked())
                    command += " -f \"bestvideo[ext=mp4]+bestaudio[ext=mp3]/mp4\"";
            }

            command += " -o \"" + fileInfo.absoluteFilePath() + "/MSKRecycleBin/%(title)s.%(ext)s\" " + urlLineEdit->text();

            process->command("Download media", command);

//            if (audioOnlyCheckBox->isChecked())
//                process->command("Download media", "youtube-dl -x -o \"" + fileInfo.absoluteFilePath() + "/MSKRecycleBin/%(title)s.%(ext)s\" " + urlLineEdit->text());
////            process->command("Download media", "youtube-dl.exe -x --audio-format mp3 -o \"" + fileInfo.absoluteFilePath() + "/%(title)s.%(ext)s\" " + urlLineEdit->text());
//            else
//                process->command("Download media", "youtube-dl -o \"" + fileInfo.absoluteFilePath() + "/MSKRecycleBin/%(title)s.%(ext)s\" " + urlLineEdit->text());
////                process->command("Download media", "youtube-dl.exe -f \"bestvideo[ext=mp4]+bestaudio[ext=m4a]/mp4\" -o \"" + fileInfo.absoluteFilePath() + "/%(title)s.%(ext)s\" " + urlLineEdit->text());

//            //            process->command("Download media", "youtube-dl.exe -f \"bestvideo+bestaudio/mp4\" -o \"" + fileInfo.absoluteFilePath() + "/%(title)s.%(ext)s\" " + text);

            processes<<process;
            connect(process, &AGProcessAndThread::processOutput, this, &AGFolderRectItem::onProcessOutput);
            connect(process, &AGProcessAndThread::processOutput, [=] (QTime time, QTime totalTime, QString event, QString outputString)
            {
                QStringList searchStrings;
                searchStrings << "[ffmpeg] Merging formats into ";
                searchStrings << "[ffmpeg] Destination: ";
                searchStrings << "[ffmpeg] Correcting container in ";
                searchStrings << "[download] Destination: "; //for osx

                foreach (QString searchString, searchStrings)
                    if (outputString.contains(searchString))
                        transitionValueChangedBy = outputString.replace(searchString, "").replace("\"", "");//.replace("//","/");

                if (event == "finished")
                {
                    if (process->errorMessage == "")
                    {
                    }
                    else
                        QMessageBox::information(scene()->views().first(), "Error " + process->name, process->errorMessage);

                    if (transitionValueChangedBy != "")
                    {
                        QFileInfo fileInfo(transitionValueChangedBy);

                        QDir recycleDir(fileInfo.absolutePath());
                        {
//                            qDebug()<<"onItemRightClicked download media finished"<<transitionValueChangedBy<<fileInfo.absolutePath();

                            recycleDir.setFilter( QDir::NoDotAndDotDot | QDir::Files );
                            foreach( QFileInfo dirInfoItem, recycleDir.entryInfoList() )
                            {
//                                qDebug()<<__func__<<dirInfoItem.fileName().toUtf8()<<cleanQString(dirInfoItem.fileName())<<fileInfo.fileName()<<(cleanQString(dirInfoItem.fileName()) == fileInfo.fileName());

                                if (cleanQString(dirInfoItem.fileName()) == fileInfo.fileName())
                                {
                                    QFile file(dirInfoItem.absoluteFilePath());
                                    bool success = file.rename(fileInfo.absoluteFilePath().replace("/MSKRecycleBin", ""));

//                                    qDebug()<<"onItemRightClicked download media finished"<<"move from msk recycle bin"<<transitionValueChangedBy<<fileInfo<<dirInfoItem<<success;
                                }
                            }
                        }
                    }
                }
            });
            process->start();

            dialog->close();
        });

        connect(cancelButton, &QPushButton::clicked, [=] ()
        {
            dialog->close();
        });

        dialog->show();

//        if (ok && !text.isEmpty())
//        {
//        }

    });

    fileContextMenu->addSeparator();

    AGViewRectItem::onItemRightClicked(pos);

    QPointF map = scene()->views().first()->mapToGlobal(QPoint(pos.x()+10, pos.y()));
    fileContextMenu->popup(QPoint(map.x(), map.y()));
}

