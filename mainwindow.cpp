#include "avideowidget.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QStyleFactory>

#include <QDesktopServices>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QNetworkReply>
#include <QShortcut>
#include <QSslConfiguration>
#include <QTimer>
#include <QVersionNumber>
#include <QtDebug>

#include "aglobal.h"

#include <QtMath>

#include <QMovie>

#include <QToolTip>

#include <QCloseEvent>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    changeUIProperties();

    allConnects();

    loadSettings();

    onClipsFilterChanged(); //crash if removed...

    ui->folderTreeView->onIndexClicked(QModelIndex()); //initial load

//    emit timelineWidgetsChanged(ui->transitionTimeSpinBox->value(), ui->transitionComboBox->currentText(), ui->clipsTableView);

    allTooltips();

    on_actionDebug_mode_triggered(false); //no debug mode

    QTimer::singleShot(0, this, [this]()->void
    {
                           QString theme = QSettings().value("theme").toString();
                           if (theme == "White")
                               on_actionWhite_theme_triggered();
                           else
                               on_actionBlack_theme_triggered();

//                           if (ui->videoFilesTreeView->selectionModel()->selectedIndexes().count() == 0)
//                           {
//                               ui->statusBar->showMessage("Please select a file", 3000);

//                               QPoint point = ui->videoFilesTreeView->mapToGlobal(QPoint(10,10));

//                               qDebug()<<"point"<<point;

//                               QToolTip::showText(point, "test1");
//                           }

                           showUpgradePrompt();
                       });
}

MainWindow::~MainWindow()
{

//    qDebug()<<"Destructor"<<geometry();

    QSettings().setValue("Geometry", geometry());
    QSettings().sync();

    delete ui;
}

void MainWindow::on_actionQuit_triggered()
{
    if (checkExit())
        qApp->quit();
}

void MainWindow::closeEvent (QCloseEvent *event)
{
    if (checkExit())
        event->accept();
    else
        event->ignore();
}

bool MainWindow::checkExit()
{
    bool exitYes = false;

    qDebug()<<"MainWindow::checkExit"<<ui->progressBar->value();

    if (ui->progressBar->value() > 0 && ui->progressBar->value() < 100)
    {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Quit", tr("Export in progress, are you sure you want to quit?"), QMessageBox::Yes|QMessageBox::No);
        if (reply == QMessageBox::Yes)
            exitYes = true;
    }
    else
        exitYes = true;

    if (exitYes)
    {
        if (ui->clipsTableView->checkSaveIfClipsChanged())
            on_actionSave_triggered();
    }

    return exitYes;
}

void MainWindow::changeUIProperties()
{
    //added designer settings
    ui->propertyDiffCheckBox->setCheckState(Qt::PartiallyChecked);

    ui->tagFilter1ListView->setMaximumHeight(ui->resetSortButton->height());
    ui->tagFilter2ListView->setMaximumHeight(ui->resetSortButton->height());

    ui->resetSortButton->setIcon(style()->standardIcon(QStyle::SP_DialogCancelButton));

    tagFilter1Model = new QStandardItemModel(this);
    ui->tagFilter1ListView->setModel(tagFilter1Model);
    ui->tagFilter1ListView->setFlow(QListView::LeftToRight);
    ui->tagFilter1ListView->setWrapping(true);
    ui->tagFilter1ListView->setDragDropMode(QListView::DragDrop);
    ui->tagFilter1ListView->setDefaultDropAction(Qt::MoveAction);
    ui->tagFilter1ListView->setSpacing(4);

    tagFilter2Model = new QStandardItemModel(this);
    ui->tagFilter2ListView->setModel(tagFilter2Model);
    ui->tagFilter2ListView->setFlow(QListView::LeftToRight);
    ui->tagFilter2ListView->setWrapping(true);
    ui->tagFilter2ListView->setDragDropMode(QListView::DragDrop);
    ui->tagFilter2ListView->setDefaultDropAction(Qt::MoveAction);
    ui->tagFilter2ListView->setSpacing(4);

    ui->transitionDial->setNotchesVisible(true);
    ui->positionDial->setNotchesVisible(true);

    //https://joekuan.files.wordpress.com/2015/09/screen3.png
    ui->actionSave->setIcon(style()->standardIcon(QStyle::SP_DriveFDIcon));
    ui->actionQuit->setIcon(style()->standardIcon(QStyle::SP_DialogCancelButton));
    ui->actionBlack_theme->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
    ui->actionWhite_theme->setIcon(style()->standardIcon(QStyle::SP_TitleBarMaxButton));
    ui->actionPrevious_in_out->setIcon(style()->standardIcon(QStyle::SP_MediaSkipBackward));
    ui->actionNext_in_out->setIcon(style()->standardIcon(QStyle::SP_MediaSkipForward));
    ui->actionPlay_Pause->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    ui->actionNext_frame->setIcon(style()->standardIcon(QStyle::SP_MediaSeekForward));
    ui->actionPrevious_frame->setIcon(style()->standardIcon(QStyle::SP_MediaSeekBackward));
    ui->actionIn->setIcon(style()->standardIcon(QStyle::SP_ArrowLeft));
    ui->actionOut->setIcon(style()->standardIcon(QStyle::SP_ArrowRight));
    ui->actionAlike->setIcon(style()->standardIcon(QStyle::SP_DialogApplyButton));
    ui->actionExport->setIcon(style()->standardIcon(QStyle::SP_FileDialogStart));
    ui->actionHelp->setIcon(style()->standardIcon(QStyle::SP_DialogHelpButton));

    ui->videoFilesTreeView->setType("Video");
    ui->audioFilestreeView->setType("Audio");
    ui->exportFilesTreeView->setType("Export");

//    AStarRating starRating = qvariant_cast<AStarRating>(3);
////    starRating.
//    QPainter *painter = new QPainter();

////    painter->fillRect(QRect(10,10,10,10), qApp->palette.highlight());

//    starRating.paint(painter, QRect(10,10,10,10), qApp->palette(),
//                     AStarRating::EditMode::ReadOnly);

//    ui->ratingFilterComboBox->setMinimumWidth(100);
//    ui->ratingFilterComboBox->setMinimumHeight(100);
    for (int i=20; i<=5; i++)
    {
        QPixmap pixmap(":/RatingW" + QString::number(i) + ".png");

        ui->ratingFilterComboBox->setItemIcon(i, QIcon(pixmap));
        ui->ratingFilterComboBox->setItemText(i,"");
    }


}

void MainWindow::allConnects()
{
    ui->graphicsView1->addNode("folder", 0, 0);
    ui->graphicsView1->addNode("prop", 5, 0);
    ui->graphicsView1->addNode("main", 8, 0);
    ui->graphicsView1->addNode("tf1", 13, 0);

    ui->graphicsView1->addNode("time", 2, 3);
    ui->graphicsView1->addNode("video", 5, 3);
    ui->graphicsView1->addNode("star", 11, 3);
    ui->graphicsView1->addNode("tf2", 13, 3);

    ui->graphicsView1->addNode("files", 0, 6);
    ui->graphicsView1->addNode("clip", 8, 6);
    ui->graphicsView1->addNode("tags", 13, 6);

//    ui->graphicsView2->addNode("folder", 0, 0);
//    ui->graphicsView2->addNode("prop", 5, 0);
    ui->graphicsView2->addNode("main", 12, 3);

    ui->graphicsView2->addNode("time", 6, 6);
    ui->graphicsView2->addNode("video", 6, 0);

//    ui->graphicsView2->addNode("files", 0, 6);
    ui->graphicsView2->addNode("clip", 0, 3);

    connect(ui->folderTreeView, &AFolderTreeView::indexClicked, ui->propertyTreeView, &APropertyTreeView::onFolderIndexClicked);
    ui->graphicsView1->connectNodes("folder", "prop", "folder");


    connect(ui->folderTreeView, &AFolderTreeView::indexClicked, ui->clipsTableView, &AClipsTableView::onFolderIndexClicked);
    ui->graphicsView1->connectNodes("folder", "clip", "folder");
    connect(ui->folderTreeView, &AFolderTreeView::indexClicked, ui->videoWidget, &AVideoWidget::onFolderIndexClicked);
    ui->graphicsView1->connectNodes("folder", "video", "folder");

    connect(ui->propertyTreeView, &APropertyTreeView::propertiesLoaded, ui->clipsTableView, &AClipsTableView::onPropertiesLoaded);

    QList<AFilesTreeView *> filesTreeList;
    filesTreeList <<ui->videoFilesTreeView;
    filesTreeList << ui->audioFilestreeView;
    filesTreeList << ui->exportFilesTreeView;
    foreach (AFilesTreeView* filesTree, filesTreeList)
    {
        connect(ui->folderTreeView, &AFolderTreeView::indexClicked, filesTree, &AFilesTreeView::onFolderIndexClicked);
        ui->graphicsView1->connectNodes("folder", "files", "folder");

        connect(filesTree, &AFilesTreeView::indexClicked, ui->clipsTableView, &AClipsTableView::onFileIndexClicked);
        ui->graphicsView1->connectNodes("files", "clip", "file");
        connect(filesTree, &AFilesTreeView::fileDelete, ui->videoWidget, &AVideoWidget::onReleaseMedia); //stop and release
        ui->graphicsView1->connectNodes("files", "video", "delete");
    //    connect(filesTree, &AFilesTreeView::fileRename, ui->videoWidget, &AVideoWidget::onFileRename); //stop and release
    //    ui->graphicsView1->connectNodes("files", "video", "rename");
        connect(filesTree, &AFilesTreeView::fileDelete, ui->clipsTableView, &AClipsTableView::onClipsDelete); //remove from clips
        ui->graphicsView1->connectNodes("files", "clip", "delete");
        connect(filesTree, &AFilesTreeView::clipsDelete, ui->clipsTableView, &AClipsTableView::onClipsDelete); //remove from clips
        ui->graphicsView1->connectNodes("files", "clip", "delete");
        connect(filesTree, &AFilesTreeView::fileRename, ui->clipsTableView, &AClipsTableView::onReloadClips); //reload
        ui->graphicsView1->connectNodes("files", "clip", "rename");
        connect(filesTree, &AFilesTreeView::fileDelete, ui->propertyTreeView, &APropertyTreeView::onRemoveFile); //remove from column
        ui->graphicsView1->connectNodes("files", "prop", "delete");
        connect(filesTree, &AFilesTreeView::fileRename, ui->propertyTreeView, &APropertyTreeView::onReloadProperties); //reload
        ui->graphicsView1->connectNodes("files", "prop", "rename");
        connect(filesTree, &AFilesTreeView::trimF, ui->clipsTableView, &AClipsTableView::onTrimF);
        ui->graphicsView1->connectNodes("files", "clip", "trim");
        connect(filesTree, &AFilesTreeView::getPropertyValue, ui->propertyTreeView, &APropertyTreeView::onGetPropertyValue);

        connect(ui->clipsTableView, &AClipsTableView::indexClicked, filesTree, &AFilesTreeView::onClipIndexClicked);
        ui->graphicsView1->connectNodes("clip", "files", "clip");
    }

    connect(ui->clipsTableView, &AClipsTableView::folderIndexClickedItemModel, ui->tagsListView, &ATagsListView::onFolderIndexClicked);
    ui->graphicsView1->connectNodes("clip", "tags", "folder");
    connect(ui->clipsTableView, &AClipsTableView::folderIndexClickedProxyModel, this,  &MainWindow::onFolderIndexClicked); //to set clip counter
    ui->graphicsView1->connectNodes("clip", "main", "folder");

    connect(ui->clipsTableView, &AClipsTableView::fileIndexClicked, ui->videoWidget, &AVideoWidget::onFileIndexClicked); //setmedia
    ui->graphicsView1->connectNodes("clip", "video", "file");
    connect(ui->clipsTableView, &AClipsTableView::fileIndexClicked, ui->propertyTreeView, &APropertyTreeView::onFileIndexClicked);
    ui->graphicsView1->connectNodes("clip", "prop", "file");
    connect(ui->clipsTableView, &AClipsTableView::fileIndexClicked, this, &MainWindow::onFileIndexClicked); //trigger onclipfilterchanged
    ui->graphicsView1->connectNodes("clip", "prop", "file");

    connect(ui->clipsTableView, &AClipsTableView::indexClicked, ui->videoWidget, &AVideoWidget::onClipIndexClicked);
    ui->graphicsView1->connectNodes("clip", "video", "clip");
    connect(ui->clipsTableView, &AClipsTableView::indexClicked, ui->propertyTreeView, &APropertyTreeView::onClipIndexClicked);
    ui->graphicsView1->connectNodes("clip", "video", "clip");

    connect(ui->clipsTableView, &AClipsTableView::clipsChangedToVideo, ui->videoWidget, &AVideoWidget::onClipsChangedToVideo);
    ui->graphicsView2->connectNodes("clip", "video", "clipchangevideo");
    connect(ui->clipsTableView, &AClipsTableView::clipsChangedToVideo, this,  &MainWindow::onClipsChangedToVideo);
    ui->graphicsView2->connectNodes("clip", "main", "clipchangevideo");
    connect(ui->clipsTableView, &AClipsTableView::clipsChangedToTimeline, ui->timelineWidget,  &ATimeline::onClipsChangedToTimeline);
    ui->graphicsView2->connectNodes("clip", "time", "clipchangetimeline");
    connect(ui->clipsTableView, &AClipsTableView::getPropertyValue, ui->propertyTreeView, &APropertyTreeView::onGetPropertyValue);
//    ui->graphicsView1->connectNodes("clip", "prop", "get");
    connect(ui->clipsTableView, &AClipsTableView::addJobsEntry, ui->logTableView, &AJobTableView::onAddJob);
    ui->graphicsView1->connectNodes("video", "prop", "get");
    connect(ui->clipsTableView, &AClipsTableView::addToJob, ui->logTableView, &AJobTableView::onAddLogToJob);

    connect(ui->clipsTableView, &AClipsTableView::reloadProperties, ui->propertyTreeView, &APropertyTreeView::onReloadProperties);

    connect(ui->clipsTableView, &AClipsTableView::setIn, ui->videoWidget, &AVideoWidget::onSetIn);
    connect(ui->clipsTableView, &AClipsTableView::setOut, ui->videoWidget, &AVideoWidget::onSetOut);

//    connect(ui->clipsTableView, &AClipsTableView::frameRateChanged, this, &MainWindow::onFrameRateChanged);

    connect(ui->clipsTableView, &AClipsTableView::propertiesLoaded, this, &MainWindow::onPropertiesLoaded);

    connect(ui->clipsTableView, &AClipsTableView::propertyUpdate, ui->exportWidget, &AExport::onPropertyUpdate);
    connect(ui->clipsTableView, &AClipsTableView::trimC, ui->exportWidget, &AExport::onTrimC);
    connect(ui->clipsTableView, &AClipsTableView::reloadAll, ui->exportWidget, &AExport::onReloadAll);

    connect(ui->videoWidget, &AVideoWidget::videoPositionChanged, ui->clipsTableView, &AClipsTableView::onVideoPositionChanged);
    ui->graphicsView1->connectNodes("video", "clip", "pos");
    connect(ui->videoWidget, &AVideoWidget::videoPositionChanged, ui->timelineWidget, &ATimeline::onVideoPositionChanged);
    ui->graphicsView2->connectNodes("video", "time", "pos");
    connect(ui->videoWidget, &AVideoWidget::videoPositionChanged, this, &MainWindow::onVideoPositionChanged);
    ui->graphicsView2->connectNodes("video", "main", "pos");
    connect(ui->videoWidget, &AVideoWidget::scrubberInChanged, ui->clipsTableView, &AClipsTableView::onScrubberInChanged);
    ui->graphicsView1->connectNodes("video", "clip", "in");
    connect(ui->videoWidget, &AVideoWidget::scrubberOutChanged, ui->clipsTableView, &AClipsTableView::onScrubberOutChanged);
    ui->graphicsView1->connectNodes("video", "clip", "out");
    connect(ui->videoWidget, &AVideoWidget::getPropertyValue, ui->propertyTreeView, &APropertyTreeView::onGetPropertyValue);
//    ui->graphicsView1->connectNodes("video", "prop", "get");
    connect(ui->videoWidget, &AVideoWidget::createNewEdit, this, &MainWindow::onCreateNewEdit);

    connect(ui->timelineWidget, &ATimeline::timelinePositionChanged, ui->videoWidget, &AVideoWidget::onTimelinePositionChanged);
    ui->graphicsView2->connectNodes("time", "video", "pos");
//    connect(ui->timelineWidget, &ATimeline::getPropertyValue, ui->propertyTreeView, &APropertyTreeView::onGetPropertyValue);
//    ui->graphicsView1->connectNodes("time", "prop", "get");
    connect(ui->timelineWidget, &ATimeline::clipsChangedToVideo, ui->videoWidget, &AVideoWidget::onClipsChangedToVideo);
    ui->graphicsView2->connectNodes("time", "video", "clipchangevideo");
    connect(ui->timelineWidget, &ATimeline::clipsChangedToTimeline, this, &MainWindow::onClipsChangedToTimeline);
    ui->graphicsView2->connectNodes("time", "main", "clipchangetimeline");

    connect(ui->timelineWidget, &ATimeline::adjustTransitionTime, this, &MainWindow::onAdjustTransitionTime);

    connect(ui->propertyTreeView, &APropertyTreeView::addJob, ui->logTableView, &AJobTableView::onAddJob);
    ui->graphicsView1->connectNodes("video", "prop", "get");
    connect(ui->propertyTreeView, &APropertyTreeView::addToJob, ui->logTableView, &AJobTableView::onAddLogToJob);
    connect(ui->propertyTreeView, &APropertyTreeView::fileDelete, ui->videoWidget, &AVideoWidget::onReleaseMedia); // on property change, stop video

    connect(this, &MainWindow::propertyFilterChanged, ui->propertyTreeView, &APropertyTreeView::onPropertyFilterChanged);
    ui->graphicsView1->connectNodes("main", "prop", "filter");
    connect(this, &MainWindow::clipsFilterChanged, ui->clipsTableView, &AClipsTableView::onClipsFilterChanged);
    ui->graphicsView1->connectNodes("main", "clip", "filter");

    connect(this, &MainWindow::timelineWidgetsChanged, ui->timelineWidget, &ATimeline::onTimelineWidgetsChanged);
    ui->graphicsView2->connectNodes("main", "video", "widgetchanged");

    connect(ui->exportWidget, &AExport::addJobsEntry, ui->logTableView, &AJobTableView::onAddJob);
    connect(ui->exportWidget, &AExport::addToJob, ui->logTableView, &AJobTableView::onAddLogToJob);
    connect(ui->exportWidget, &AExport::getPropertyValue, ui->propertyTreeView, &APropertyTreeView::onGetPropertyValue);
    connect(ui->exportWidget, &AExport::reloadClips, ui->clipsTableView, &AClipsTableView::onReloadClips); //reload
    connect(ui->exportWidget, &AExport::reloadProperties, ui->propertyTreeView, &APropertyTreeView::onReloadProperties); //reload

    connect(tagFilter1Model, &QStandardItemModel::dataChanged,  this, &MainWindow::onTagFiltersChanged);
    connect(tagFilter1Model, &QStandardItemModel::rowsRemoved,  this, &MainWindow::onTagFiltersChanged); //datachanged not signalled when removing
    ui->graphicsView1->connectNodes("tf1", "main", "filter");

    connect(tagFilter2Model, &QStandardItemModel::dataChanged,  this, &MainWindow::onTagFiltersChanged);
    connect(tagFilter2Model, &QStandardItemModel::rowsRemoved,  this, &MainWindow::onTagFiltersChanged); //datachanged not signalled when removing
    ui->graphicsView1->connectNodes("tf2", "main", "filter");

    //do not show the graph tab (only in debug mode)
    graphWidget1 = ui->clipsTabWidget->widget(3);
    graphWidget2 = ui->clipsTabWidget->widget(4);

    connect(&m_network, SIGNAL(finished(QNetworkReply*)), SLOT(onUpgradeCheckFinished(QNetworkReply*)));

} //allConnects

void MainWindow::loadSettings()
{
    QRect savedGeometry = QSettings().value("Geometry").toRect();
    if (savedGeometry != geometry() && savedGeometry.width() != 0) //initial
        setGeometry(savedGeometry);

//    qDebug()<<"tagFilter1"<<QSettings().value("tagFilter1").toString();
//    qDebug()<<"tagFilter2"<<QSettings().value("tagFilter2").toString();
    QStringList tagList1 = QSettings().value("tagFilter1").toString().split(";", QString::SkipEmptyParts);

    for (int j=0; j < tagList1.count(); j++)//tbd: add as method of tagslistview
    {
        QList<QStandardItem *> items;
        QStandardItem *item = new QStandardItem(tagList1[j].toLower());
        item->setBackground(QBrush(Qt::red));
//                item->setFont(QFont(font().family(), 8 * devicePixelRatio()));
        items.append(item);
        items.append(new QStandardItem("I")); //nr of occurrences
        tagFilter1Model->appendRow(items);
    }
    QStringList tagList2 = QSettings().value("tagFilter2").toString().split(";", QString::SkipEmptyParts);

    for (int j=0; j < tagList2.count(); j++)
    {
//        qDebug()<<"tagList2[j].toLower()"<<j<<tagList2[j].toLower();
        QList<QStandardItem *> items;
        QStandardItem *item = new QStandardItem(tagList2[j].toLower());
        item->setBackground(QBrush(Qt::red));
//                item->setFont(QFont(font().family(), 8 * devicePixelRatio()));
        items.append(item);
        items.append(new QStandardItem("I")); //nr of occurrences
        tagFilter2Model->appendRow(items);
    }

    Qt::CheckState checkState;
    if (QSettings().value("alikeCheckBox").toBool())
        checkState = Qt::Checked;
    else
        checkState = Qt::Unchecked;
    ui->alikeCheckBox->setCheckState(checkState);

    if (QSettings().value("fileOnlyCheckBox").toBool())
        checkState = Qt::Checked;
    else
        checkState = Qt::Unchecked;
    ui->fileOnlyCheckBox->setCheckState(checkState);

//    ui->progressBar->setRange(0, 100);
    ui->progressBar->setValue(0);


    //    QPixmap pixmap(":/acvc.ico");
//        QLabel *mylabel = new QLabel (this);
//        ui->logoLabel->setPixmap( QPixmap (":/acvc.ico"));
//        mylabel->show();
//        mylabel->update();
    //    m_durationLabel->setPixmap(pixmap);
    //    mylabel->show();
    //    toolbar->addWidget(mylabel);

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

    if (QSettings().value("authorCheckBox").toBool())
        checkState = Qt::Checked;
    else
        checkState = Qt::Unchecked;
    ui->authorCheckBox->setCheckState(checkState);

    ui->filesTabWidget->setCurrentIndex(QSettings().value("filesTabIndex").toInt());
    ui->clipsTabWidget->setCurrentIndex(QSettings().value("clipTabIndex").toInt());

    ui->ratingFilterComboBox->setCurrentIndex(QSettings().value("ratingFilterComboBox").toInt());

    ui->transitionTimeSpinBox->setValue(QSettings().value("transitionTime").toInt());
    ui->transitionComboBox->setCurrentText(QSettings().value("transitionType").toString());

//    connect(ui->exportTargetComboBox, &QComboBox::currentTextChanged, this, &MainWindow::on_exportTargetComboBox_currentTextChanged);
    ui->exportTargetComboBox->setCurrentText(QSettings().value("exportTarget").toString());
    on_exportTargetComboBox_currentTextChanged(ui->exportTargetComboBox->currentText());

    if (QSettings().value("exportSize").toString() == "")
        ui->exportSizeComboBox->setCurrentText("Source");
    else
        ui->exportSizeComboBox->setCurrentText(QSettings().value("exportSize").toString());

    if (QSettings().value("exportFrameRate").toString() == "")
        ui->exportFramerateComboBox->setCurrentText("Source");
    else
        ui->exportFramerateComboBox->setCurrentText(QSettings().value("exportFrameRate").toString());

    int lframeRate = QSettings().value("frameRate").toInt();
    if (lframeRate < 1)
    {
            lframeRate = 25;
    }
//    qDebug()<<"Set framerate"<<lframeRate;
    ui->clipsFramerateComboBox->setCurrentText(QString::number(lframeRate));

    if (QSettings().value("exportVideoAudioSlider") == "")
        ui->exportVideoAudioSlider->setValue(20);
    else
        ui->exportVideoAudioSlider->setValue(QSettings().value("exportVideoAudioSlider").toInt());

    watermarkFileName = QSettings().value("watermarkFileName").toString();
    watermarkFileNameChanged(watermarkFileName);
} //loadSettings

void MainWindow::allTooltips()
{
    //folder and file
    ui->folderTreeView->setToolTip(tr("<p><b>Folder</b></p>"
                                      "<p><i>Select a folder containing video files</i></p>"
                                      "<ul>"
                                      "<li>Select folder: After selecting a folder, the files tab is shown and clips, tags and properties of all files are loaded</li>"
                                      "<li>Warning: For movie files with more than 100 clips and if more than 100 files with clips are loaded, a warning will be given with the option to skip or cancel.</li>"
                                      "</ul>"));

    ui->videoFilesTreeView->setToolTip(tr("<p><b>File</b></p>"
                                     "<p><i>Files within the selected folder</i></p>"
                                     "<ul>"
                                     "<li>Click on file: Show the clips of this file on the timeline and highlight the clips of the file in the clips tab</li>"
                                     "</ul>"
                                     "<p>Right mouse click:</p>"
                                      "<ul>"
                                     "<li>Trim: create new video file based on the clips of the video file. Rating, alike and tags as well as relevant properties are copied</li>"
                                     "<li>Rename: Rename the file to the suggested file name (go to the properties tab to manage selected file names</li>"
                                     "<li>Delete file(s): Delete the video file and its supporting files (clips)</li>"
                                     "<li>Delete clips: Delete the clips of the file</li>"
                                      "</ul>"));

    //tt clip filters
    ui->alikeCheckBox->setToolTip(tr("<p><b>Alike</b></p>"
                                     "<p><i>Check if other clips are 'alike' this clip. This is typically the case with Action-cam footage: multiple shots which are similar.</i></p>"
                                     "<p><i>This is used to exclude similar clips from the exported video.</i></p>"
                                     "<ul>"
                                     "<li>Alike filter checkbox: Show only alike clips</li>"
                                     "<li>Alike column: Set if this clip is like another clip <CTRL-A></li>"
                                     "<li>Timeline: Only clips which meet the filter criteria are shown in the timeline</li>"
                                     "<li>Hint: Give the best of the alikes a higher rating than the others</li>"
                                     "<li>Hint: Give alike clips the same tags to filter on them later</li>"
                                     "</ul>"
                                  ));

    ui->ratingFilterComboBox->setToolTip(tr("<p><b>Ratings</b></p>"
                                              "<p><i>Give a rating to an clip (0 to 5 stars)</i></p>"
                                              "<ul>"
                                                "<li>Rating filter: Select 0 to 5 stars. All clips with same or higher rating will be shown</li>"
                                                "<li>Rating column: Double click to change the rating (or CTRL-0 to CTRL-5 to rate the current clip)</li>"
                                                "<li>Timeline: Only clips which meet the filter criteria are shown in the timeline</li>"
                                              "</ul>"));

    ui->tagFilter1ListView->setToolTip(tr("<p><b>Tag filters</b></p>"
                                          "<p><i>Define which clips are shown based on their tags</i></p>"
                                          "<ul>"
                                          "<li>Tag fields: The following logical condition applies: (Left1 or left2 or left3 ...) and (right1 or right2 or right3 ...)</li>"
                                          "<li>Timeline: Only clips which meet the filter criteria are shown in the timeline</li>"
                                          "</ul>"));

    ui->tagFilter2ListView->setToolTip(ui->tagFilter1ListView->toolTip());

    ui->fileOnlyCheckBox->setToolTip(tr("<p><b>File only</b></p>"
                                        "<p><i>Show only clips of the selected file</i></p>"
                                        "<ul>"
                                        "<li>Timeline: Only clips which meet the filter criteria are shown in the timeline</li>"
                                        "</ul>"));

    ui->clipsFramerateComboBox->setToolTip(tr("<p><b>Framerate</b></p>"
                                        "<p><i>The framerate used in the clips</i></p>"
                                        "<ul>"
                                        "<li>Normally the same as the video files used. However sometimes different video files have different framerates and one of them should be selected.</li>"
                                        "</ul>"));

    //tt clips table
    ui->resetSortButton->setToolTip(tr("<p><b>Reset sort</b></p>"
                                       "<p><i>Set the order of clips back to file default</i></p>"
                                       ));

    ui->clipsTableView->setToolTip(tr("<p><b>Clips list</b></p>"
                                     "<p><i>Show the clips for the files in the selected folder</i></p>"
                                     "<ul>"
                                     "<li>Only clips applying to the filters above this table are shown</li>"
                                     "<li>Move over the column headers to see column tooltips</li>"
                                     "<li>Clips belonging to the selected file are highlighted gray</li>"
                                     "<li>The clip currently shown in the video window is highlighted blue</li>"
                                     "<li>To delete a clip: right mouse click</li>"
                                     "<li>To change the order of clips: drag the number on the left of this table up or down</li>"
                                     "</ul>"
                                     ));

    ui->clipsTableView->clipsItemModel->horizontalHeaderItem(inIndex)->setToolTip(tr("<p><b>Title</b></p>"
                                                                                   "<p><i>Description</i></p>"
                                                                                   "<ul>"
                                                                                   "<li>Feature 1</li>"
                                                                                   "</ul>"));
    ui->clipsTableView->clipsItemModel->horizontalHeaderItem(outIndex)->setToolTip( ui->clipsTableView->clipsItemModel->horizontalHeaderItem(inIndex)->toolTip());
    ui->clipsTableView->clipsItemModel->horizontalHeaderItem(durationIndex)->setToolTip( ui->clipsTableView->clipsItemModel->horizontalHeaderItem(inIndex)->toolTip());
    ui->clipsTableView->clipsItemModel->horizontalHeaderItem(ratingIndex)->setToolTip(ui->ratingFilterComboBox->toolTip());
    ui->clipsTableView->clipsItemModel->horizontalHeaderItem(alikeIndex)->setToolTip(ui->alikeCheckBox->toolTip());
    ui->clipsTableView->clipsItemModel->horizontalHeaderItem(tagIndex)->setToolTip(tr("<p><b>Tags per clip</b></p>"
                                                                                    "<p><i>Show the tags per clip</i></p>"
                                                                                    "<ul>"
                                                                                    "<li>To Add: drag tag on the right to Tags field</li>"
                                                                                    "<li>To delete: Double click on a tag field, then drag tag to Tags field on the right</li>"
                                                                                    "</ul>"
                                                                                    ));

    //tt tags
    ui->newTagLineEdit->setToolTip(tr("<p><b>Tag field</b></p>"
                                      "<p><i>Create and remove tags</i></p>"
                                      "<ul>"
                                      "<li>Add new tag to tag list: fill in tag name and press return</li>"
                                      "<li>Delete tags from filter, clips and taglist: drag tag and drop in the tag field</li>"
                                      "</ul>"
                                      ));
    ui->tagsListView->setToolTip(tr("<p><b>Tag list</b></p>"
                                    "<p><i>List of tags used in clips or filter</i></p>"
                                    "<ul>"
                                    "<li>Add tag to clip: Drag and drop to tag column of clips</li>"
                                    "<li>Add tag to filter: Drag and drop to filters</li>"
                                    "</ul>"
                                    ));

    //properties
    ui->propertyTreeView->setToolTip(tr("<p><b>Property list</b></p>"
                                     "<p><i>Show the properties for all the files of the selected folder</i></p>"
                                     "<ul>"
                                        "<li>Only properties applying to the filters above this table are shown</li>"
                                        "<li>Properties belonging to the selected file are highlighted in blue</li>"
                                        "<li>The properties in the general, location, camera and author section can be updated (except for filename and suggested name).</li>"
                                        "<li>If a suggested name differs from the filename it will be bold (indicating you can rename it in the files tab)</li>"
                                     "</ul>"
                                     ));

    ui->propertyFilterLineEdit->setToolTip(tr("<p><b>Property filter</b></p>"
                                              "<p><i>Filters on properties shown</i></p>"
                                              "<ul>"
                                              "<li>Enter text to filter on properties. E.g. date shows all the date properties</li>"
                                              "</ul>"));
    ui->propertyDiffCheckBox->setToolTip(tr("<p><b>Diff</b></p>"
                                            "<p><i>Determines which properties are shown based on if they are different or the same accross the movie files</i></p>"
                                            "<ul>"
                                            "<li>Unchecked: Only properties which are the same for all files</li>"
                                            "<li>Checked: Only properties which are different between files</li>"
                                            "<li>Partially checked (default): All properties</li>"
                                            "</ul>"));
    ui->locationCheckBox->setToolTip(tr("<p><b>Location, camera, author</b></p>"
                                        "<p><i>Check what is part of the suggestedname</i></p>"
                                        "<ul>"
                                        "<li>Createdate: Not optional</li>"
                                        "<li>Location: GPS coordinates and altitude</li>"
                                        "<li>Camera: Model, if no model then Make</li>"
                                        "<li>Author: Director, if not, Producer, if not, Publisher, if not, Writer</li>"
                                        "</ul>"));
    ui->cameraCheckBox->setToolTip(ui->locationCheckBox->toolTip());
    ui->authorCheckBox->setToolTip(ui->locationCheckBox->toolTip());

    //timeline
    ui->transitionDial->setToolTip(tr("<p><b>Transitions</b></p>"
                                      "<p><i>Sets the transition time and type for the exported video</i></p>"
                                      "<ul>"
                                      "<li>No transitions: set the time to 0</li>"
                                      "<li>Transtion type: curently only Cross Dissolve supported</li>"
                                      "<li>Remark: Lossless and encode do not support transitions yet, the transition time is however subtracted from the exported video</li>"
                                      "</ul>"));
    ui->transitionComboBox->setToolTip( ui->transitionDial->toolTip());
    ui->transitionTimeSpinBox->setToolTip( ui->transitionDial->toolTip());

    //export
    ui->exportTargetComboBox->setToolTip(tr("<p><b>Export target</b></p>"
                                 "<p><i>Determines what will be exported</i></p>"
                                 "<ul>"
                                              "<li>Lossless: FFMpeg generated video file. Very fast!!! If codec, size and framerate of clips is not the same the result might not play correctly</li>"
                                            "<ul>"
                                                         "<li>Excluded: transitions (duration preserved by cut in the middle), watermark, audio fade in/out</li>"
                                            "</ul>"
                                              "<li>Encode: FFMpeg generated video file</li>"
    "<ul>"
                 "<li>Excluded: transitions (duration preserved by cut in the middle), audio fade in/out</li>"
    "</ul>"
                                              "<li>Premiere: Final cut XML project file for Adobe Premiere</li>"
                                              "<li>Shotcut: Mlt project file</li>"
                                 "</ul>"));

    ui->exportSizeComboBox->setToolTip(tr("<p><b>Export video size</b></p>"
                                          "<p><i>The video size of the exported files</i></p>"
                                          "<ul>"
                                          "<li>Source: if source selected than the video size of the source will be used (see clips window)</li>"
                                          "</ul>"));

    ui->exportFramerateComboBox->setToolTip(tr("<p><b>Export framerate</b></p>"
                                                 "<p><i>The framerate of the exported files</i></p>"
                                                 "<ul>"
                                                 "<li>Source: if source selected than the framerate of the source will be used (see clips window)</li>"
                                                 "</ul>"));

    ui->exportVideoAudioSlider->setToolTip(tr("<p><b>Video Audio volume</b></p>"
                                              "<p><i>Sets the volume of the original video</i></p>"
                                              "<ul>"
                                              "<li>Remark: This only applies to the videos. Audio files are always played at 100%</li>"
                                              "<li>Remark: Volume adjustments are also audible in ACVC when playing video files</li>"
                                              "</ul>"));

    ui->watermarkLabel->setToolTip(tr("<p><b>Watermark</b></p>"
                                      "<p><i>Adds a watermark / logo on the bottom right of the exported video</i></p>"
                                      "<ul>"
                                      "<li>Button: If no watermark then browse for an image. If a watermark is already selected, clicking the button will remove the watermark</li>"
                                      "<li>Remark: No watermark on lossless encoding</li>"
                                      "</ul>"));

    ui->watermarkButton->setToolTip(ui->watermarkLabel->toolTip());

    //log
    ui->logTableView->setToolTip(tr("<p><b>Log items</b></p>"
                                    "<p><i>Show details of background processes for Exiftool and FFMpeg</i></p>"
                                    "<ul>"
                                    "<li>Click on a row to see details</li>"
                                    "</ul>"));
} //tooltips

void MainWindow::on_actionBlack_theme_triggered()
{
//    qDebug()<<"on_actionBlack_theme_triggered 1"<<qApp->style()<<QStyleFactory::keys();

    qApp->setStyle(QStyleFactory::create("Fusion"));
//    qApp->setPalette(QApplication::style()->standardPalette());
//    qDebug()<<"on_actionBlack_theme_triggered 1.1";
    QColor mainColor = QColor(45,45,45);
    QColor disabledColor = QColor(127,127,127);
    QColor baseColor = QColor(18,18,18);
    QColor linkColor = QColor(42, 130, 218);
    QColor linkVisitedColor = QColor(255, 0, 255);
    QColor highlightColor = QColor(42, 130, 218);

    QPalette palette;
    palette.setColor(QPalette::Window, mainColor);
    palette.setColor(QPalette::WindowText, Qt::white);
    palette.setColor(QPalette::Base, baseColor);
    palette.setColor(QPalette::AlternateBase, mainColor);
    palette.setColor(QPalette::ToolTipBase, Qt::white);
    palette.setColor(QPalette::ToolTipText, Qt::white);
    palette.setColor(QPalette::Text, Qt::white);
    palette.setColor(QPalette::Disabled, QPalette::Text, disabledColor);
    palette.setColor(QPalette::Button, mainColor);
    palette.setColor(QPalette::ButtonText, Qt::white);
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, disabledColor);
    palette.setColor(QPalette::BrightText, Qt::red);
    palette.setColor(QPalette::Link, linkColor);
    palette.setColor(QPalette::Link, linkVisitedColor);

    palette.setColor(QPalette::Highlight, highlightColor);
    palette.setColor(QPalette::HighlightedText, Qt::black);
    palette.setColor(QPalette::Disabled, QPalette::HighlightedText, disabledColor);

    palette.setColor(QPalette::PlaceholderText, Qt::black);

    qApp->setPalette(palette);

//    qDebug()<<"MainWindow::on_actionBlack_theme_triggered set palette"<<palette;

    qApp->setStyleSheet("QToolTip { color: #ffffff; background-color: #2a82da; border: 1px solid white; }");

    ui->clipsTableView->selectClips(); //to set the colors right

    QSettings().setValue("theme", "Black");
    QSettings().sync();

    ui->clipsTableView->update();
}

void MainWindow::on_actionWhite_theme_triggered()
{
//    qDebug()<<"on_actionWhite_theme_triggered 1";
    qApp->setStyle(QStyleFactory::create("Fusion"));
//    qDebug()<<"on_actionWhite_theme_triggered 1.1";

    QColor mainColor = QColor(240,240,240);
    QColor disabledColor = QColor(127,127,127);
    QColor baseColor = QColor(255,255,255);
    QColor linkColor = QColor(0, 0, 255);
    QColor linkVisitedColor = QColor(255, 0, 255);
    QColor highlightColor = QColor(0,120,215);

//    qDebug()<<""<<qApp->palette().color(QPalette::Window);
//    qDebug()<<""<<qApp->palette().color(QPalette::Base);
//    qDebug()<<""<<qApp->palette().color(QPalette::Disabled, QPalette::Text);
//    qDebug()<<""<<qApp->palette().color(QPalette::Link);
//    qDebug()<<""<<qApp->palette().color(QPalette::LinkVisited);
//    qDebug()<<""<<qApp->palette().color(QPalette::Highlight);
//    qDebug()<<""<<qApp->palette().color(QPalette::PlaceholderText);

    QPalette palette;
    palette.setColor(QPalette::Window, mainColor);
    palette.setColor(QPalette::WindowText, Qt::black);
    palette.setColor(QPalette::Base, baseColor);
    palette.setColor(QPalette::AlternateBase, mainColor);
    palette.setColor(QPalette::ToolTipBase, Qt::black);
    palette.setColor(QPalette::ToolTipText, Qt::black);
    palette.setColor(QPalette::Text, Qt::black);
    palette.setColor(QPalette::Disabled, QPalette::Text, disabledColor);
    palette.setColor(QPalette::Button, mainColor);
    palette.setColor(QPalette::ButtonText, Qt::black);
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, disabledColor);
    palette.setColor(QPalette::BrightText, Qt::red);
    palette.setColor(QPalette::Link, linkColor);
    palette.setColor(QPalette::LinkVisited, linkVisitedColor);

    palette.setColor(QPalette::Highlight, highlightColor);
    palette.setColor(QPalette::HighlightedText, Qt::white);
    palette.setColor(QPalette::Disabled, QPalette::HighlightedText, disabledColor);

    palette.setColor(QPalette::PlaceholderText, Qt::white);

    qApp->setPalette(palette);

    qApp->setStyleSheet("QToolTip { color: #ffffff; background-color: #2a82da; border: 1px solid white; }");

    ui->clipsTableView->selectClips(); //to set the colors right

    QSettings().setValue("theme", "White");
    QSettings().sync();

    ui->clipsTableView->update();
}

void MainWindow::on_actionAbout_triggered()
{
    QMessageBox::about(this, tr("About ACVC"),
            tr("<p><b>ACVC</b> is a free desktop application aimed at minimizing the time between shooting and publishing videos. ACVC prepares raw video for efficient editing in your preferred video editor. ACVC is tuned for action camera footage having a lot of raw video which needs to compressed to a small video showing the best of it.</p>"
               "<p>Version  %1</p>"
               "<p>Licensed under the: <a href=\"https://www.gnu.org/licenses/lgpl-3.0.html\">GNU Lesser General Public License v3.0</a></p>"
               "<p>Copyright © 2019</p>"
               "<p><a href=\"http://bit.ly/ACVCHome\">ACVC Website</a></p>"
               "<p>This program proudly uses the following projects:</p>"
               "<ul>"
               "<li><a href=\"https://www.qt.io/\">Qt</a> application and UI framework</li>"
               "<li><a href=\"https://www.shotcut.org/\">Shotcut</a> Open source video editor (timeline and version check)</li>"
               "<li><a href=\"https://www.ffmpeg.org/\">FFmpeg</a> multimedia format and codec libraries (lossless and encoded previews)</li>"
               "<li><a href=\"https://www.sno.phy.queensu.ca/~phil/exiftool/\">Exiftool</a> Read, Write and Edit Meta Information (Properties)</li>"
               "</ul>"
               "<p>This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.</p>"
               "<p>As ACVC may contain bugs, BACKUP your video files before editing!</p>"
               "<p>ACVC issuetracker on GitHub <a href=\"http://bit.ly/ACVCGithubIssues\">GitHub ACVC issues</a></p>"
               "<p>ACVC is created by <a href=\"https://nl.linkedin.com/in/ewoudwijma\">Ewoud Wijma</a>.</p>"
               ).arg(qApp->applicationVersion()));
}

void MainWindow::on_actionAbout_Qt_triggered()
{
    qApp->aboutQt();
}

void MainWindow::onCreateNewEdit()
{
    ui->clipsTableView->addClip(ui->ratingFilterComboBox->currentIndex(), ui->alikeCheckBox->checkState() == Qt::Checked, ui->tagFilter1ListView->model(), ui->tagFilter2ListView->model());
}

void MainWindow::on_propertyFilterLineEdit_textChanged(const QString &)//arg1
{
    emit propertyFilterChanged(ui->propertyFilterLineEdit, ui->propertyDiffCheckBox, ui->locationCheckBox, ui->cameraCheckBox, ui->authorCheckBox);
}

void MainWindow::on_propertyDiffCheckBox_stateChanged(int )//arg1
{
    emit propertyFilterChanged(ui->propertyFilterLineEdit, ui->propertyDiffCheckBox, ui->locationCheckBox, ui->cameraCheckBox, ui->authorCheckBox);
}

void MainWindow::onClipsChangedToVideo(QAbstractItemModel *itemModel)
{
    ui->clipRowsCounterLabel->setText(QString::number(itemModel->rowCount()) + " / " + QString::number(ui->clipsTableView->clipsItemModel->rowCount()));
}

void MainWindow::onFolderIndexClicked(QAbstractItemModel *itemModel)
{
//    qDebug()<<"MainWindow::onFolderIndexClicked"<<itemModel->rowCount();

    ui->clipRowsCounterLabel->setText(QString::number(itemModel->rowCount()) + " / " + QString::number(ui->clipsTableView->clipsItemModel->rowCount()));

    ui->filesTabWidget->setCurrentIndex(1); //go to files tab

    ui->folderIconLabel->setPixmap(style()->standardIcon(QStyle::SP_DirIcon).pixmap(QSize(ui->folderIconLabel->size().height(),ui->folderIconLabel->size().height())));
    ui->folderIconLabel->setMaximumWidth(ui->folderIconLabel->size().height());
    ui->folderLabel->setText(ui->folderTreeView->directoryModel->fileInfo(ui->folderTreeView->currentIndex()).absoluteFilePath());

//    ui->folderLabel->show();

//    Qt::CheckState checkState;

//    ui->alikeCheckBox->setCheckState(Qt::Unchecked);
//    on_alikeCheckBox_clicked(false); //save in QSettings

//    tagFilter1Model->clear();
//    tagFilter2Model->clear();
//    onTagFiltersChanged();

//    ui->fileOnlyCheckBox->setCheckState(Qt::Unchecked);
//    on_fileOnlyCheckBox_clicked(false); //save in QSettings

    ui->timelineWidget->transitiontimeLastGood = -1;

    onClipsFilterChanged();
    emit timelineWidgetsChanged(ui->transitionTimeSpinBox->value(), ui->transitionComboBox->currentText(), ui->clipsTableView);
}

void MainWindow::onFileIndexClicked(QModelIndex index)
{
//    qDebug()<<"MainWindow::onFileIndexClicked"<<index.data().toString();

    if (ui->videoFilesTreeView->model() != index.model())
        ui->videoFilesTreeView->clearSelection();
    if (ui->audioFilestreeView->model() != index.model())
        ui->audioFilestreeView->clearSelection();
    if (ui->exportFilesTreeView->model() != index.model())
        ui->exportFilesTreeView->clearSelection();

    onClipsFilterChanged();
}

void MainWindow::onClipsFilterChanged()
{
    ui->clipRowsCounterLabel->setText(QString::number(ui->clipsTableView->model()->rowCount()) + " / " + QString::number(ui->clipsTableView->clipsItemModel->rowCount()));

//    qDebug()<<"MainWindow::onClipFilterChanged"<<QSettings().value("ratingFilterComboBox")<<ui->ratingFilterComboBox->currentText();

    emit clipsFilterChanged(ui->ratingFilterComboBox, ui->alikeCheckBox, ui->tagFilter1ListView, ui->tagFilter2ListView, ui->fileOnlyCheckBox);
//    qDebug()<<"MainWindow::onClipsFilterChanged"<<QSettings().value("ratingFilterComboBox")<<ui->ratingFilterComboBox->currentText();

    ui->exportButton->setEnabled(ui->clipsTableView->model()->rowCount() > 0);
}

void MainWindow::onTagFiltersChanged()
{
    QString string1 = "";
    QString sep = "";
    for (int i=0; i < tagFilter1Model->rowCount();i++)
    {
        string1 += sep + tagFilter1Model->index(i,0).data().toString();
        sep = ";";
    }

    QString string2 = "";
    sep = "";
    for (int i=0; i < tagFilter2Model->rowCount();i++)
    {
        string2 += sep + tagFilter2Model->index(i,0).data().toString();
        sep = ";";
    }

    if (QSettings().value("tagFilter1") != string1)
    {
        QSettings().setValue("tagFilter1", string1);
        QSettings().sync();
    }
    if (QSettings().value("tagFilter2") != string2)
    {
        QSettings().setValue("tagFilter2", string2);
        QSettings().sync();
    }
    onClipsFilterChanged();
}

void MainWindow::on_action5_stars_triggered()
{
    ui->clipsTableView->giveStars(5);
}

void MainWindow::on_action4_stars_triggered()
{
    ui->clipsTableView->giveStars(4);
}

void MainWindow::on_action1_star_triggered()
{
    ui->clipsTableView->giveStars(1);
}

void MainWindow::on_action2_stars_triggered()
{
    ui->clipsTableView->giveStars(2);
}

void MainWindow::on_action3_stars_triggered()
{
    ui->clipsTableView->giveStars(3);
}

void MainWindow::on_action0_stars_triggered()
{
    ui->clipsTableView->giveStars(0);

}

void MainWindow::on_actionAlike_triggered()
{
    ui->clipsTableView->toggleAlike();
}

void MainWindow::on_actionSave_triggered()
{
    int changeCount = 0;
    for (int row = 0; row<ui->clipsTableView->clipsItemModel->rowCount(); row++)
    {
        if (ui->clipsTableView->clipsItemModel->index(row, changedIndex).data().toString() == "yes")
            changeCount++;
    }

//    if (changeCount > 0) //if any changes in any file
    {
        ui->clipsTableView->onSectionMoved(-1,-1,-1); //to reorder the items

        for (int row = 0; row < ui->clipsTableView->srtFileItemModel->rowCount(); row++) //go through all srt files
        {
            QString folderName = ui->clipsTableView->srtFileItemModel->index(row, 0).data().toString();
            QString fileName = ui->clipsTableView->srtFileItemModel->index(row, 1).data().toString();
//            qDebug()<<"MainWindow::on_actionSave_triggered"<<ui->clipsTableView->srtFileItemModel->rowCount()<<row<<folderName<<fileName;
            ui->clipsTableView->saveModel(folderName, fileName);
        }
    }
//    else
    ui->statusBar->showMessage(QString::number(changeCount) + " clip changes saved", 5000);
//        ui->statusBar->showMessage("No clip changes to save", 5000);
}

void MainWindow::on_actionPlay_Pause_triggered()
{
    ui->videoWidget->togglePlayPaused();
}

void MainWindow::on_actionIn_triggered()
{
//    qDebug()<<"MainWindow::on_actionIn_triggered"<<ui->clipsTableView->highLightedRow<<ui->videoWidget->m_position;
    ui->videoWidget->onSetIn();
}

void MainWindow::on_actionOut_triggered()
{
    qDebug()<<"MainWindow::on_actionOut_triggered"<<ui->clipsTableView->highLightedRow<<ui->videoWidget->m_position;
    ui->videoWidget->onSetOut();
}

void MainWindow::on_actionPrevious_frame_triggered()
{
    ui->videoWidget->rewind();
}

void MainWindow::on_actionNext_frame_triggered()
{
    ui->videoWidget->fastForward();
}

void MainWindow::on_actionPrevious_in_out_triggered()
{
     ui->videoWidget->skipPrevious();
}

void MainWindow::on_actionNext_in_out_triggered()
{
    ui->videoWidget->skipNext();
}

//void MainWindow::on_actionAdd_tag_triggered()
//{
//    QList<QStandardItem *> items;
//    QStandardItem *item = new QStandardItem("ewoud");
//    item->setBackground(QBrush(Qt::red));

//    items.append(item);
//    items.append(new QStandardItem("I"));
//    ui->tagsListView->tagsItemModel->appendRow(items);
//}

void MainWindow::on_newTagLineEdit_returnPressed()
{
    if (ui->tagsListView->addTag(ui->newTagLineEdit->text()))
        ui->newTagLineEdit->clear();
    else
        QMessageBox::information(this, "Add tag", "Tag " + ui->newTagLineEdit->text() + " already in list");
}

void MainWindow::on_exportButton_clicked()
{
    int transitionTime = 0;
//    if (ui->transitionComboBox->currentText() != "No transition")
    transitionTime = ui->transitionTimeSpinBox->value();
//    qDebug()<<"MainWindow::on_exportButton_clicked";
    ui->exportWidget->exportClips(ui->clipsTableView->clipsProxyModel, ui->exportTargetComboBox->currentText(), ui->exportSizeComboBox->currentText(), ui->exportFramerateComboBox->currentText(), transitionTime, ui->progressBar, ui->exportVideoAudioSlider, ui->spinnerLabel, watermarkFileName, ui->exportButton, ui->clipsFramerateComboBox, ui->clipsSizeComboBox, ui->statusBar);

//    if (QSettings().value("firstUsedDate") == QVariant())
//    {
//        QSettings().setValue("firstUsedDate", QDateTime::currentDateTime().addDays(-1));
//        QSettings().sync();
//    }
//    qint64 days = QSettings().value("firstUsedDate").toDateTime().daysTo(QDateTime::currentDateTime());

    QSettings().setValue("exportCounter", QSettings().value("exportCounter").toInt()+1);
    QSettings().sync();
//    qDebug()<<"exportCounter"<<QSettings().value("exportCounter").toInt();
    if (QSettings().value("exportCounter").toInt() == 50)
    {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Donate", tr("If you like this software, consider to make a donation. Go to the Donate page on the ACVC website for more information") + " " + QSettings().value("exportCounter").toString(), QMessageBox::Yes|QMessageBox::No);
        if (reply == QMessageBox::Yes)
            QDesktopServices::openUrl(QUrl("http://bit.ly/ACVCSupport"));
    }
}

void MainWindow::on_exportTargetComboBox_currentTextChanged(const QString &arg1)
{
//    qDebug()<<"MainWindow::on_exportTargetComboBox_currentTextChanged"<<arg1;
    if (QSettings().value("exportTarget") != arg1)
    {
        QSettings().setValue("exportTarget", arg1);
        QSettings().sync();
    }

    ui->exportSizeComboBox->setEnabled(arg1 != "Lossless");
    ui->exportFramerateComboBox->setEnabled(arg1 != "Lossless");
    ui->watermarkButton->setEnabled(arg1 != "Lossless");
    ui->watermarkLabel->setEnabled(arg1 != "Lossless");
    ui->transitionComboBox->setEnabled(arg1 != "Lossless" && arg1 != "Encode");

}

void MainWindow::on_exportSizeComboBox_currentTextChanged(const QString &arg1)
{
    if (QSettings().value("exportSize") != arg1)
    {
        QSettings().setValue("exportSize", arg1);
        QSettings().sync();
    }
}

void MainWindow::on_exportFramerateComboBox_currentTextChanged(const QString &arg1)
{
//    ui->transitionDial->setRange(0, ui->exportFramerateComboBox->currentText().toInt() * 4);
    if (QSettings().value("exportFramerate") != arg1)
    {
        QSettings().setValue("exportFramerate", arg1);
        QSettings().sync();
    }
}

void MainWindow::on_actionExport_triggered()
{
    ui->exportButton->click();
}

void MainWindow::on_alikeCheckBox_clicked(bool checked)
{
    if (QSettings().value("alikeCheckBox").toBool() != checked)
    {
        QSettings().setValue("alikeCheckBox", checked);
        QSettings().sync();
    }
    onClipsFilterChanged();
}

void MainWindow::on_fileOnlyCheckBox_clicked(bool checked)
{
    if (QSettings().value("fileOnlyCheckBox").toBool() != checked)
    {
        QSettings().setValue("fileOnlyCheckBox", checked);
        QSettings().sync();
    }
    onClipsFilterChanged();
}

void MainWindow::on_actionDebug_mode_triggered(bool checked)
{
//    qDebug()<<"on_actionDebug_mode_triggered"<<checked;

    if (checked)
    {
        ui->clipsTabWidget->insertTab(3, graphWidget1, "Graph1");
        ui->clipsTabWidget->insertTab(4, graphWidget2, "Graph2");
    }
    else
    {
        ui->clipsTabWidget->removeTab(4);
        ui->clipsTabWidget->removeTab(3);
    }

    ui->clipsTableView->setColumnHidden(orderBeforeLoadIndex, !checked);
    ui->clipsTableView->setColumnHidden(orderAtLoadIndex, !checked);
    ui->clipsTableView->setColumnHidden(orderAfterMovingIndex, !checked);
    ui->clipsTableView->setColumnHidden(fpsIndex, !checked);
    ui->clipsTableView->setColumnHidden(fileDurationIndex, !checked);
    ui->clipsTableView->setColumnHidden(changedIndex, !checked);
}

void MainWindow::on_resetSortButton_clicked()
{
    for (int row=0;row<ui->clipsTableView->clipsItemModel->rowCount();row++)
    {
        //https://uvesway.wordpress.com/2013/01/08/qheaderview-sections-visualindex-vs-logicalindex/
        ui->clipsTableView->verticalHeader()->moveSection(ui->clipsTableView->verticalHeader()->visualIndex(row), row);
        if (ui->clipsTableView->clipsItemModel->index(row, orderBeforeLoadIndex).data().toInt() * 10 != ui->clipsTableView->clipsItemModel->index(row, orderAtLoadIndex).data().toInt())
        {
            qDebug()<<"MainWindow::on_resetSortButton_clicked1"<<row<<ui->clipsTableView->clipsItemModel->index(row, orderBeforeLoadIndex).data().toInt() * 10 << ui->clipsTableView->clipsItemModel->index(row, orderAtLoadIndex).data().toInt()<<ui->clipsTableView->clipsItemModel;
            ui->clipsTableView->clipsItemModel->setData(ui->clipsTableView->clipsItemModel->index(row, orderAtLoadIndex), ui->clipsTableView->clipsItemModel->index(row, orderBeforeLoadIndex).data().toInt() * 10);
            ui->clipsTableView->clipsItemModel->setData(ui->clipsTableView->clipsItemModel->index(row, changedIndex), "yes");
        }
        if (ui->clipsTableView->clipsItemModel->index(row, orderBeforeLoadIndex).data().toInt() * 10 != ui->clipsTableView->clipsItemModel->index(row, orderAfterMovingIndex).data().toInt())
        {
            qDebug()<<"MainWindow::on_resetSortButton_clicked2"<<row<<ui->clipsTableView->clipsItemModel->index(row, orderBeforeLoadIndex).data().toInt() * 10 << ui->clipsTableView->clipsItemModel->index(row, orderAfterMovingIndex).data().toInt();
            ui->clipsTableView->clipsItemModel->setData(ui->clipsTableView->clipsItemModel->index(row, orderAfterMovingIndex), ui->clipsTableView->clipsItemModel->index(row, orderBeforeLoadIndex).data().toInt() * 10);
            ui->clipsTableView->clipsItemModel->setData(ui->clipsTableView->clipsItemModel->index(row, changedIndex), "yes");
        }
    }
}

void MainWindow::on_locationCheckBox_clicked(bool checked)
{
    if (QSettings().value("locationCheckBox").toBool() != checked)
    {
        QSettings().setValue("locationCheckBox", checked);
        QSettings().sync();
        emit propertyFilterChanged(ui->propertyFilterLineEdit, ui->propertyDiffCheckBox, ui->locationCheckBox, ui->cameraCheckBox, ui->authorCheckBox);
    }
}

void MainWindow::on_authorCheckBox_clicked(bool checked)
{
    if (QSettings().value("authorCheckBox").toBool() != checked)
    {
        QSettings().setValue("authorCheckBox", checked);
        QSettings().sync();
        emit propertyFilterChanged(ui->propertyFilterLineEdit, ui->propertyDiffCheckBox, ui->locationCheckBox, ui->cameraCheckBox, ui->authorCheckBox);
    }
}


void MainWindow::on_cameraCheckBox_clicked(bool checked)
{
    if (QSettings().value("cameraCheckBox").toBool() != checked)
    {
        QSettings().setValue("cameraCheckBox", checked);
        QSettings().sync();
        emit propertyFilterChanged(ui->propertyFilterLineEdit, ui->propertyDiffCheckBox, ui->locationCheckBox, ui->cameraCheckBox, ui->authorCheckBox);

    //    QString fileName = QFileDialog::getOpenFileName(this,
    //          tr("Open Image"), "/home/", tr("MediaFiles (*.mp4)"));

//        QFileDialog dialog(this);
//        dialog.setFileMode(QFileDialog::AnyFile);
//        dialog.setNameFilter(tr("MediaFiles (*.mp4)"));
//        dialog.setViewMode(QFileDialog::List);
//        QStringList fileNames;
//         if (dialog.exec())
//             fileNames = dialog.selectedFiles();
    }
}

void MainWindow::on_transitionComboBox_currentTextChanged(const QString &arg1)
{
    if (QSettings().value("transitionType") != arg1)
    {
//        qDebug()<<"MainWindow::on_transitionComboBox_currentTextChanged"<<arg1;
        QSettings().setValue("transitionType", arg1);
        QSettings().sync();
        emit timelineWidgetsChanged(ui->transitionTimeSpinBox->value(), ui->transitionComboBox->currentText(), ui->clipsTableView);
    }
}

void MainWindow::onClipsChangedToTimeline(QAbstractItemModel *) //itemModel
{
    if (transitionValueChangedBy != "Dial")//do not change the dial if the dial triggers the change
    {
        double result;
        result = ui->transitionTimeSpinBox->value();

//        qDebug()<<"MainWindow::onClipsChangedToTimeline transition"<<int(result);

        transitionValueChangedBy = "SpinBox";
        ui->transitionDial->setValue( int(result ));
//        transitionValueChangedBy = "";

//        qDebug()<<"MainWindow::onClipsChangedToTimeline transition after"<<int(result);
    }
}

void MainWindow::on_transitionDial_valueChanged(int value)
{
    if (transitionValueChangedBy != "SpinBox") //do not change the spinbox if the spinbox triggers the change
    {
        double result;
        if (value < 50)
            result = qSin(M_PI * value / 100);
        else
            result = 1 / qSin(M_PI * value / 100);
        result = ( value);

//        qDebug()<<"MainWindow::on_transitionDial_valueChanged"<<value<<result;

        ui->transitionTimeSpinBox->setValue(value);
//        transitionValueChangedBy = "";
    }
}

void MainWindow::on_transitionTimeSpinBox_valueChanged(int arg1)
{
    if (QSettings().value("transitionTime") != arg1)
    {
//        qDebug()<<"MainWindow::on_transitionTimeSpinBox_valueChanged"<<arg1<<transitionValueChangedBy;
        QSettings().setValue("transitionTime", arg1);
        QSettings().sync();

        emit timelineWidgetsChanged(ui->transitionTimeSpinBox->value(), ui->transitionComboBox->currentText(), ui->clipsTableView);
    }
}

void MainWindow::onAdjustTransitionTime(int transitionTime)
{
//    qDebug()<<"MainWindow::onAdjustTransitionTime"<<transitionTime;

    if (transitionValueChangedBy != "SpinBox")
    {
//        transitionValueChangedBy = "Timeline";
        ui->transitionTimeSpinBox->setValue(transitionTime);
//        transitionValueChangedBy = "";
    }
}

void MainWindow::on_transitionDial_sliderMoved(int position)
{
    transitionValueChangedBy = "Dial";
}


void MainWindow::on_positionDial_valueChanged(int value)
{
    if (positionValueChangedBy != "SpinBox") //do not change the spinbox if the spinbox triggers the change
    {

//        if (positiondialOldValue == 99 && value == 0)
//            positiondialOldValue = -1;
//        if (positiondialOldValue == 0 && value == 99)
//            positiondialOldValue = 100;

        int delta = (value - positiondialOldValue + 100) % 100;

        if (delta > 50)
            delta -= 100;

        if (delta < 0)
            delta = -1;
        if (delta > 0)
            delta = 1;

//        qDebug()<<"incrementSlider"<<delta<<ui->incrementSlider->value()<<AGlobal().msec_to_frames(ui->videoWidget->playerDuration) * ui->incrementSlider->value() / 200.0 + 1;
        delta *= (AGlobal().msec_to_frames(ui->videoWidget->playerDuration) * ui->incrementSlider->value() / 200.0 / 100.0 + 1);

//        qDebug()<<"MainWindow::on_positionDial_valueChanged"<<positiondialOldValue<<value<<delta<< ui->videoWidget->m_positionSpinner->value()<<ui->videoWidget->playerDuration;

        if (ui->videoWidget->m_positionSpinner->value() + delta <= AGlobal().msec_to_frames(ui->videoWidget->playerDuration))
            ui->videoWidget->m_positionSpinner->setValue(ui->videoWidget->m_positionSpinner->value() + delta);
//        positionValueChangedBy = "";
    }
    positiondialOldValue = value;
}

void MainWindow::on_positionDial_sliderMoved(int position)
{
//    qDebug()<<"MainWindow::on_positionDial_sliderMoved"<<position;
    positionValueChangedBy = "Dial";
}

void MainWindow::onVideoPositionChanged(int progress, int , int )//row, relativeProgress
{
    if (positionValueChangedBy != "Dial")
    {
//        qDebug()<<"MainWindow::onVideoPositionChanged"<<progress<<positionValueChangedBy;
        ui->positionDial->setValue(AGlobal().msec_to_frames(progress)%100);
//        positionValueChangedBy = "";
    }
    positionValueChangedBy = "SpinBox";
}

void MainWindow::showUpgradePrompt()
{
//    qDebug()<<"MainWindow::showUpgradePrompt";
//    QSettings().setValue("checkUpgradeAutomatic", false);
//    if (QSettings().value("checkUpgradeAutomatic").toBool())
//    {
        ui->statusBar->showMessage("Checking for upgrade...", 15000);
        QNetworkRequest request(QUrl("http://bit.ly/ACVCVersionCheck"));
        QSslConfiguration sslConfig = request.sslConfiguration();
        sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
        request.setSslConfiguration(sslConfig);
        m_network.get(request);
//    } else {
//        m_network.setStrictTransportSecurityEnabled(false);
//        QMessageBox::StandardButton reply;
//        reply = QMessageBox::question(this, "Upgrade", tr("Click here to check for a new version of ACVC."), QMessageBox::Yes|QMessageBox::No);
//        if (reply == QMessageBox::Yes)
//            on_actionUpgrade_triggered();
//    }
}

//void MainWindow::on_actionUpgrade_triggered()
//{
//    qDebug()<<"MainWindow::on_actionUpgrade_triggered";
//    if (QSettings().value("askUpgradeAutomatic", true).toBool())
//    {
//        QMessageBox dialog(QMessageBox::Question, qApp->applicationName(), tr("Do you want to automatically check for updates in the future?"), QMessageBox::No | QMessageBox::Yes, this);
//        dialog.setWindowModality(Qt::ApplicationModal);//QmlApplication::dialogModality()
//        dialog.setDefaultButton(QMessageBox::Yes);
//        dialog.setEscapeButton(QMessageBox::No);
//        dialog.setCheckBox(new QCheckBox(tr("Do not show this anymore.", "Automatic upgrade check dialog")));
//        QSettings().setValue("checkUpgradeAutomatic", dialog.exec() == QMessageBox::Yes);
//        if (dialog.checkBox()->isChecked())
//            QSettings().setValue("askUpgradeAutomatic", false);
//    }
//    ui->statusBar->showMessage("Checking for upgrade2...", 15000);
//    m_network.get(QNetworkRequest(QUrl("http://actioncamvideocompanion.com/version.json")));
//}

void MainWindow::onUpgradeCheckFinished(QNetworkReply* reply)
{
//    qDebug()<<"MainWindow::onUpgradeCheckFinished";
    QString m_upgradeUrl = "http://bit.ly/ACVCDownload";

    if (!reply->error())
    {
        QByteArray response = reply->readAll();
//        qDebug() << "response: " << response;

        if (response.contains("Bitly"))
        {
            int indexOfAOpen = response.indexOf("<a href=\"");
            int indexOfAClose = response.indexOf("\">");
            QString httpString = response.mid(indexOfAOpen + 9, indexOfAClose - indexOfAOpen + 1 - 10);
//            qDebug() << "response: " << httpString;

            QNetworkRequest request(httpString);
//            QNetworkRequest request(QUrl("http://www.actioncamvideocompanion.com/version.json"));
            m_network.get(request);
            return;
        }

        QJsonParseError *error = new QJsonParseError();
        QJsonDocument json = QJsonDocument::fromJson(response, error);
        QString current = qApp->applicationVersion();

        if (!json.isNull() && json.object().value("version_string").type() == QJsonValue::String)
        {
            QString latest = json.object().value("version_string").toString();
            if (current != "adhoc" && QVersionNumber::fromString(current) < QVersionNumber::fromString(latest))
            {
                if (!json.object().value("url").isUndefined())
                    m_upgradeUrl = json.object().value("url").toString();

                QMessageBox::StandardButton reply;
                reply = QMessageBox::question(this, "Upgrade", tr("ACVC version %1 is available! Click Yes to get it.").arg(latest), QMessageBox::Yes|QMessageBox::No);
                if (reply == QMessageBox::Yes)
                    QDesktopServices::openUrl(QUrl(m_upgradeUrl));

            } else {
                ui->statusBar->showMessage(tr("You are running the latest version of ACVC."), 15000);
            }
            reply->deleteLater();
            return;
        } else {
            ui->statusBar->showMessage("failed to parse version.json "+ error->errorString());
        }
    } else {
        ui->statusBar->showMessage("Network error " + reply->errorString());
    }
    QMessageBox::StandardButton mreply;
    mreply = QMessageBox::question(this, "Upgrade", tr("Failed to read version.json when checking. Click here to go to the Web site."), QMessageBox::Yes|QMessageBox::No);
    if (mreply == QMessageBox::Yes)
        QDesktopServices::openUrl(QUrl(m_upgradeUrl));
    reply->deleteLater();
}

void MainWindow::onPropertiesLoaded()
{
    emit propertyFilterChanged(ui->propertyFilterLineEdit, ui->propertyDiffCheckBox, ui->locationCheckBox, ui->cameraCheckBox, ui->authorCheckBox);

    ui->clipsFramerateComboBox->blockSignals(true); // do not fire on_clipsFramerateComboBox_currentTextChanged

    ui->clipsFramerateComboBox->clear();
    ui->clipsFramerateComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);

    ui->clipsSizeComboBox->clear();
    ui->clipsSizeComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);

    bool foundCurrentClipsFramerate = false;

//    for (int col = 3; col < ui->propertyTreeView->propertyItemModel->columnCount(); col++)
//    {
//        QString fileName = ui->propertyTreeView->propertyItemModel->headerData(col, Qt::Horizontal).toString();

//        if (!fileName.toLower().contains(".mp3") && !fileName.toLower().contains("lossless") && !fileName.toLower().contains("encode") && !fileName.toLower().contains("shotcut") && !fileName.toLower().contains("premiere"))
//        {
//            QString *frameratePointer = new QString();
//            ui->propertyTreeView->onGetPropertyValue(fileName, "VideoFrameRate", frameratePointer);

//            int fpsSuggested = qRound((*frameratePointer).toDouble());

//            if (ui->clipsFramerateComboBox->findText(QString::number(fpsSuggested)) < 0)
//            {
//                ui->clipsFramerateComboBox->addItem(QString::number(fpsSuggested));
//            }

//            if (QString::number(fpsSuggested) == QSettings().value("frameRate"))
//                foundCurrentClipsFramerate = true;

//            QString *widthValue = new QString();
//            QString *heightValue = new QString();
//            ui->propertyTreeView->onGetPropertyValue(fileName, "ImageWidth", widthValue);
//            ui->propertyTreeView->onGetPropertyValue(fileName, "ImageHeight", heightValue);
//            QString size = *widthValue + " x " + *heightValue;
//            if (ui->clipsSizeComboBox->findText(size) < 0)
//                ui->clipsSizeComboBox->addItem(size);
//        }
//    }

    for (int row = 0; row < ui->clipsTableView->clipsItemModel->rowCount(); row++)
    {
        QString fileName = ui->clipsTableView->clipsItemModel->index(row, fileIndex).data().toString();

        if (!fileName.toLower().contains(".mp3") && !fileName.toLower().contains("lossless") && !fileName.toLower().contains("encode") && !fileName.toLower().contains("shotcut") && !fileName.toLower().contains("premiere"))
        {
            QString *frameratePointer = new QString();
            ui->propertyTreeView->onGetPropertyValue(fileName, "VideoFrameRate", frameratePointer);

            int fpsSuggested = qRound((*frameratePointer).toDouble());

            if (ui->clipsFramerateComboBox->findText(QString::number(fpsSuggested)) < 0)
            {
                ui->clipsFramerateComboBox->addItem(QString::number(fpsSuggested));
            }

            if (QString::number(fpsSuggested) == QSettings().value("frameRate"))
                foundCurrentClipsFramerate = true;

            QString *widthValue = new QString();
            QString *heightValue = new QString();
            ui->propertyTreeView->onGetPropertyValue(fileName, "ImageWidth", widthValue);
            ui->propertyTreeView->onGetPropertyValue(fileName, "ImageHeight", heightValue);
            QString size = *widthValue + " x " + *heightValue;
            if (ui->clipsSizeComboBox->findText(size) < 0)
                ui->clipsSizeComboBox->addItem(size);
        }
    }

    ui->clipsFramerateComboBox->blockSignals(false);

//    qDebug()<<"MainWindow::onPropertiesLoaded"<<foundCurrentClipsFramerate<<QSettings().value("frameRate")<<ui->clipsFramerateComboBox->currentText();

    if (foundCurrentClipsFramerate)
        ui->clipsFramerateComboBox->setCurrentText(QSettings().value("frameRate").toString());
    else
    {
        QMessageBox::information(this, "Framerate", "Clips framerate changed from " + QSettings().value("frameRate").toString() + " to " + ui->clipsFramerateComboBox->currentText());
        QSettings().setValue("frameRate", ui->clipsFramerateComboBox->currentText());
    }

//    emit timelineWidgetsChanged(ui->transitionTimeSpinBox->value(), ui->transitionComboBox->currentText(), ui->clipsTableView);
}

void MainWindow::on_clipsTabWidget_currentChanged(int index)
{
    if (QSettings().value("clipTabIndex").toInt() != index)
    {
        QSettings().setValue("clipTabIndex", index);
        QSettings().sync();
    }
}

void MainWindow::on_filesTabWidget_currentChanged(int index)
{
    if (QSettings().value("filesTabIndex").toInt() != index)
    {
        QSettings().setValue("filesTabIndex", index);
        QSettings().sync();
    }
}

void MainWindow::on_actionDonate_triggered()
{
    QDesktopServices::openUrl(QUrl("http://bit.ly/ACVCSupport"));
}

void MainWindow::on_actionCheck_for_updates_triggered()
{
    showUpgradePrompt();
}

void MainWindow::on_ratingFilterComboBox_currentIndexChanged(int index)
{
    if (QSettings().value("ratingFilterComboBox").toInt() != index)
    {
        QSettings().setValue("ratingFilterComboBox", index);
        QSettings().sync();
    }

    onClipsFilterChanged();
}

void MainWindow::on_actionHelp_triggered()
{
    QDesktopServices::openUrl(QUrl("http://bit.ly/ACVCFeatures"));

//    QMessageBox::about(this, tr("About ACVC"),
//            tr("<p><h1>ACVC process flow</b></p>"
//               "<p>Version  %1</p>"
//               "<ul>"
//               "<li>Select a folder: %2</li>"
//               "<li>Create clips: %3 clips created</li>"
//               "</ul>"
//               ).arg(qApp->applicationVersion(), ui->folderLabel->text(), QString::number(ui->clipsTableView->clipsItemModel->rowCount())));

}

void MainWindow::on_watermarkButton_clicked()
{
    if (watermarkFileName == "")
    {
        watermarkFileName = QFileDialog::getOpenFileName(this,
            tr("Open Image"), ui->folderTreeView->directoryModel->rootPath(), tr("Image Files (*.png *.jpg *.bmp *.ico)"));
    }
    else
    {
        watermarkFileName = "";
    }
    watermarkFileNameChanged(watermarkFileName);

}

void MainWindow::watermarkFileNameChanged(QString newFileName)
{
    if (newFileName != QSettings().value("watermarkFileName"))
    {
        QSettings().setValue("watermarkFileName", newFileName);
        QSettings().sync();
    }

    if (newFileName != "")
    {

        QImage myImage;
        myImage.load(newFileName);

        ui->watermarkLabel->setPixmap(QPixmap::fromImage(myImage).scaled(QSize(50,50)));

        ui->watermarkLabel->show();
    }
    else
    {
        ui->watermarkLabel->setText("No Watermark");
    }
}

void MainWindow::on_actionGithub_ACVC_Issues_triggered()
{
    QDesktopServices::openUrl(QUrl("http://bit.ly/ACVCGithubIssues"));
}

void MainWindow::on_actionMute_triggered()
{
    ui->videoWidget->onMute();
}

void MainWindow::on_clipsFramerateComboBox_currentTextChanged(const QString &arg1)
{
    qDebug()<<"MainWindow::on_clipsFramerateComboBox_currentTextChanged"<<arg1<<QSettings().value("frameRate");

    ui->transitionDial->setRange(0, arg1.toInt() * 4);
//    QSettings().setValue("frameRate", arg1); //as used globally
    if (QSettings().value("frameRate") != arg1)
    {
        QSettings().setValue("frameRate", arg1);
        QSettings().sync();
    }
}

void MainWindow::on_exportVideoAudioSlider_valueChanged(int value)
{
//    qDebug()<<"MainWindow::on_exportVideoAudioSlider_valueChanged"<<value;

    ui->videoWidget->setSourceVideoVolume(value);

    if (QSettings().value("exportVideoAudioSlider").toInt() != value)
    {
        QSettings().setValue("exportVideoAudioSlider", value);
        QSettings().sync();
    }
}

void MainWindow::on_clearJobsButton_clicked()
{
    while (ui->logTableView->jobItemModel->rowCount()>0)
        ui->logTableView->jobItemModel->takeRow(0);
}
