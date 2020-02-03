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

#include <QToolTip>

#include <QCloseEvent>

#ifdef Q_OS_WIN
    #include <QWinTaskbarProgress>
#endif

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    qApp->installEventFilter(this);

    changeUIProperties();

    allConnects();

    loadSettings();

    onClipsFilterChanged(false); //crash if removed...

    allTooltips();

    on_actionDebug_mode_triggered(false); //no debug mode

    QTimer::singleShot(0, this, [this]()->void
    {
                           QString theme = QSettings().value("theme").toString();

                           if (theme == "White")
                               on_actionWhite_theme_triggered();
                           else
                               on_actionBlack_theme_triggered();

                           ui->folderTreeView->onIndexClicked(QModelIndex()); //initial load

//                           qDebug()<<"contextSensitiveHelpOn"<<QSettings().value("contextSensitiveHelpOn");
                           if (QSettings().value("contextSensitiveHelpOn").toString() == "" || QSettings().value("contextSensitiveHelpOn").toBool())
                               ui->actionContext_Sensitive_Help->setChecked(true);

                           showUpgradePrompt();
                       });

    //tooltips after 10 seconds (to show hints first)
}

MainWindow::~MainWindow()
{

//    qDebug()<<"Destructor"<<geometry();

    QSettings().setValue("Geometry", geometry());
//    QSettings().setValue("windowState", saveState());
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

//    qDebug()<<"MainWindow::checkExit"<<ui->progressBar->value();

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

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
//    qDebug()<<"MainWindow::eventFilter"<<event;

    if (!ui->actionTooltips->isChecked())
    {
        if (event->type() == QEvent::ToolTip)
        {
            return true;
        }
        else
        {
            return QMainWindow::eventFilter(obj, event);
        }
    }
    else
    {
        return QMainWindow::eventFilter(obj, event);
    }
}

void MainWindow::changeUIProperties()
{
#ifdef Q_OS_WIN
    taskbarButton = new QWinTaskbarButton(this);
    taskbarButton->setWindow(windowHandle());
//       taskbarButton->setOverlayIcon(QIcon(":/loading.png"));
#endif

    //added designer settings
    ui->propertyDiffCheckBox->setCheckState(Qt::PartiallyChecked);

    ui->tagFilter1ListView->setMaximumHeight(ui->resetSortButton->height());
    ui->tagFilter2ListView->setMaximumHeight(ui->resetSortButton->height());

    ui->resetSortButton->setIcon(style()->standardIcon(QStyle::SP_DialogCancelButton));

//    tagFilter1Model = new QStandardItemModel(this);
//    ui->tagFilter1ListView->setModel(tagFilter1Model);
//    ui->tagFilter1ListView->setFlow(QListView::LeftToRight);
//    ui->tagFilter1ListView->setWrapping(true);
//    ui->tagFilter1ListView->setDragDropMode(QListView::DragDrop);
//    ui->tagFilter1ListView->setDefaultDropAction(Qt::MoveAction);
//    ui->tagFilter1ListView->setSpacing(4);
//    ui->tagFilter1ListView->setEditTriggers(QAbstractItemView::NoEditTriggers);

//    connect(ui->tagFilter1ListView, &ATagsListView::doubleClicked, this, &MainWindow::onTagFilter1ListViewDoubleClicked);


//    tagFilter2Model = new QStandardItemModel(this);
//    ui->tagFilter2ListView->setModel(tagFilter2Model);
//    ui->tagFilter2ListView->setFlow(QListView::LeftToRight);
//    ui->tagFilter2ListView->setWrapping(true);
//    ui->tagFilter2ListView->setDragDropMode(QListView::DragDrop);
//    ui->tagFilter2ListView->setDefaultDropAction(Qt::MoveAction);
//    ui->tagFilter2ListView->setSpacing(4);
//    ui->tagFilter2ListView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    ui->positionDial->setNotchesVisible(true);

    ui->videoFilesTreeView->setType("Video");
    ui->audioFilestreeView->setType("Audio");
    ui->exportFilesTreeView->setType("Export");

    for (int i=20; i<=5; i++)
    {
        QPixmap pixmap(":/RatingW" + QString::number(i) + ".png");

        ui->ratingFilterComboBox->setItemIcon(i, QIcon(pixmap));
        ui->ratingFilterComboBox->setItemText(i,"");
    }

    //transition
    ui->transitionDial->setNotchesVisible(true);
    ui->transitionTimeSpinBox->setFixedWidth(120);

    //video/media window

    ui->positionSpinBox->setKeyboardTracking(false);
    ui->positionSpinBox->setFixedWidth(120);
    ui->durationLabel->setText(" / --:--:--:--");
    ui->durationLabel->setFixedWidth(120);

    //https://joekuan.files.wordpress.com/2015/09/screen3.png
    ui->actionSave->setIcon(style()->standardIcon(QStyle::SP_DriveFDIcon));
    ui->actionQuit->setIcon(style()->standardIcon(QStyle::SP_DialogCancelButton));
    ui->actionBlack_theme->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
    ui->actionWhite_theme->setIcon(style()->standardIcon(QStyle::SP_TitleBarMaxButton));
    ui->actionPrevious_in_out->setIcon(style()->standardIcon(QStyle::SP_MediaSkipBackward));
    ui->actionNext_in_out->setIcon(style()->standardIcon(QStyle::SP_MediaSkipForward));
    ui->actionPlay_Pause->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
//    ui->actionPlay_Pause->setDisabled(true);

    ui->actionNext_frame->setIcon(style()->standardIcon(QStyle::SP_MediaSeekForward));
    ui->actionPrevious_frame->setIcon(style()->standardIcon(QStyle::SP_MediaSeekBackward));
    ui->actionIn->setIcon(style()->standardIcon(QStyle::SP_ArrowLeft));
    ui->actionOut->setIcon(style()->standardIcon(QStyle::SP_ArrowRight));
    ui->actionAlike->setIcon(style()->standardIcon(QStyle::SP_DialogApplyButton));
    ui->actionExport->setIcon(style()->standardIcon(QStyle::SP_FileDialogStart));
    ui->actionHelp->setIcon(style()->standardIcon(QStyle::SP_DialogHelpButton));
    ui->actionRepeat_context_sensible_help->setIcon(style()->standardIcon(QStyle::SP_MessageBoxQuestion));

    QList<QPushButton *> playerControls;
    playerControls<<ui->skipBackwardButton<<ui->seekBackwardButton<<ui->playButton<<ui->seekForwardButton<<ui->skipForwardButton<<ui->stopButton<<ui->muteButton<<ui->setInButton<<ui->setOutButton;
    foreach (QPushButton *widget, playerControls)
    {
        widget->setText("");
        widget->setMaximumWidth(widget->height());
    }

    ui->skipBackwardButton->setIcon(style()->standardIcon(QStyle::SP_MediaSkipBackward));
    ui->seekBackwardButton->setIcon(style()->standardIcon(QStyle::SP_MediaSeekBackward));
    ui->playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    ui->seekForwardButton->setIcon(style()->standardIcon(QStyle::SP_MediaSeekForward));
    ui->skipForwardButton->setIcon(style()->standardIcon(QStyle::SP_MediaSkipForward));

    ui->stopButton->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
    ui->muteButton->setIcon(style()->standardIcon(QStyle::SP_MediaVolume));

    ui->setInButton->setText("{");
    ui->setOutButton->setText("}");

    ui->speedComboBox->addItem("-2x");
    ui->speedComboBox->addItem("-1x");
    ui->speedComboBox->addItem("1 fps");
    ui->speedComboBox->addItem("2 fps");
    ui->speedComboBox->addItem("4 fps");
    ui->speedComboBox->addItem("8 fps");
    ui->speedComboBox->addItem("16 fps");
    ui->speedComboBox->addItem("1x");
    ui->speedComboBox->addItem("2x");
    ui->speedComboBox->addItem("4x");
    ui->speedComboBox->addItem("8x");
    ui->speedComboBox->addItem("16x");

    ui->speedComboBox->setCurrentText("1x");

    ui->cancelButton->setIcon(style()->standardIcon(QStyle::SP_DialogCancelButton));
    ui->cancelButton->setEnabled(false);
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

    connect(ui->folderTreeView, &AFolderTreeView::indexClicked, ui->clipsTableView, &AClipsTableView::onFolderIndexClicked); //this before propertiesLoaded because save check!
    ui->graphicsView1->connectNodes("folder", "clip", "folder");

    connect(ui->folderTreeView, &AFolderTreeView::indexClicked, ui->videoWidget, &AVideoWidget::onFolderIndexClicked);
    ui->graphicsView1->connectNodes("folder", "video", "folder");

    connect(ui->folderTreeView, &AFolderTreeView::indexClicked, ui->propertyTreeView, &APropertyTreeView::onFolderIndexClicked);
    ui->graphicsView1->connectNodes("folder", "prop", "folder");

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
        connect(filesTree, &AFilesTreeView::releaseMedia, ui->videoWidget, &AVideoWidget::onReleaseMedia); //stop and release
        ui->graphicsView1->connectNodes("files", "video", "release");
        connect(filesTree, &AFilesTreeView::clipsDelete, ui->clipsTableView, &AClipsTableView::onClipsDelete); //remove from clips
        ui->graphicsView1->connectNodes("files", "clip", "delete");
        connect(filesTree, &AFilesTreeView::reloadClips, ui->clipsTableView, &AClipsTableView::onReloadClips); //reload
        ui->graphicsView1->connectNodes("files", "clip", "reload");
        connect(filesTree, &AFilesTreeView::removeFile, ui->propertyTreeView, &APropertyTreeView::onRemoveFile); //remove from column
        ui->graphicsView1->connectNodes("files", "prop", "remove");
        connect(filesTree, &AFilesTreeView::reloadProperties, ui->propertyTreeView, &APropertyTreeView::onReloadProperties);
        ui->graphicsView1->connectNodes("files", "prop", "reload");
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
    connect(ui->clipsTableView, &AClipsTableView::addJob, ui->logTableView, &AJobTableView::onAddJob);
    ui->graphicsView1->connectNodes("video", "prop", "get");
    connect(ui->clipsTableView, &AClipsTableView::addToJob, ui->logTableView, &AJobTableView::onAddToJob);

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
    connect(ui->videoWidget, &AVideoWidget::durationChanged, this, &MainWindow::onDurationChanged);
    connect(ui->videoWidget, &AVideoWidget::scrubberInChanged, ui->clipsTableView, &AClipsTableView::onScrubberInChanged);
    ui->graphicsView1->connectNodes("video", "clip", "in");
    connect(ui->videoWidget, &AVideoWidget::scrubberOutChanged, ui->clipsTableView, &AClipsTableView::onScrubberOutChanged);
    ui->graphicsView1->connectNodes("video", "clip", "out");
    connect(ui->videoWidget, &AVideoWidget::getPropertyValue, ui->propertyTreeView, &APropertyTreeView::onGetPropertyValue);
//    ui->graphicsView1->connectNodes("video", "prop", "get");
    connect(ui->videoWidget, &AVideoWidget::createNewEdit, this, &MainWindow::onCreateNewEdit);

    connect(ui->videoWidget, &AVideoWidget::playerStateChanged, this, &MainWindow::onPlayerStateChanged);
    connect(ui->videoWidget, &AVideoWidget::mutedChanged, this, &MainWindow::onMutedChanged);
    connect(ui->videoWidget, &AVideoWidget::playbackRateChanged, this, &MainWindow::onPlaybackRateChanged);

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
    connect(ui->propertyTreeView, &APropertyTreeView::addToJob, ui->logTableView, &AJobTableView::onAddToJob);
    connect(ui->propertyTreeView, &APropertyTreeView::releaseMedia, ui->videoWidget, &AVideoWidget::onReleaseMedia); // on property change, stop video

    connect(this, &MainWindow::propertyFilterChanged, ui->propertyTreeView, &APropertyTreeView::onPropertyFilterChanged);
    ui->graphicsView1->connectNodes("main", "prop", "filter");
    connect(this, &MainWindow::clipsFilterChanged, ui->clipsTableView, &AClipsTableView::onClipsFilterChanged);
    ui->graphicsView1->connectNodes("main", "clip", "filter");

    connect(this, &MainWindow::timelineWidgetsChanged, ui->timelineWidget, &ATimeline::onTimelineWidgetsChanged);
    ui->graphicsView2->connectNodes("main", "video", "widgetchanged");

    connect(ui->exportWidget, &AExport::addJob, ui->logTableView, &AJobTableView::onAddJob);
    connect(ui->exportWidget, &AExport::addToJob, ui->logTableView, &AJobTableView::onAddToJob);
    connect(ui->exportWidget, &AExport::getPropertyValue, ui->propertyTreeView, &APropertyTreeView::onGetPropertyValue);
    connect(ui->exportWidget, &AExport::reloadClips, ui->clipsTableView, &AClipsTableView::onReloadClips);
    connect(ui->exportWidget, &AExport::reloadProperties, ui->propertyTreeView, &APropertyTreeView::onReloadProperties);
    connect(ui->exportWidget, &AExport::exportCompleted, this, &MainWindow::onExportCompleted); //reload

    connect(ui->tagFilter1ListView, &ATagsListView::tagChanged,  this, &MainWindow::onTagFilter1ListViewChanged);
    connect(ui->tagFilter2ListView, &ATagsListView::tagChanged,  this, &MainWindow::onTagFilter2ListViewChanged);

    ui->graphicsView1->connectNodes("tf1", "main", "filter");

    ui->graphicsView1->connectNodes("tf2", "main", "filter");

    //do not show the graph tab (only in debug mode)
    graphWidget1 = ui->clipsTabWidget->widget(3);
    graphWidget2 = ui->clipsTabWidget->widget(4);
    graphicsWidget = ui->clipsTabWidget->widget(5);

    connect(&m_network, SIGNAL(finished(QNetworkReply*)), SLOT(onUpgradeCheckFinished(QNetworkReply*)));

    connect(ui->positionSpinBox, SIGNAL(valueChanged(int)), ui->videoWidget, SLOT(onSpinnerPositionChanged(int))); //using other syntax not working...
} //allConnects

void MainWindow::loadSettings()
{
    QRect savedGeometry = QSettings().value("Geometry").toRect();
    if (savedGeometry != geometry() && savedGeometry.width() != 0) //initial
        setGeometry(savedGeometry);

//    restoreState(QSettings().value("windowState").toByteArray());

    ui->tagFilter1ListView->stringToModel(QSettings().value("tagFilter1").toString());
    ui->tagFilter2ListView->stringToModel(QSettings().value("tagFilter2").toString());

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

    if (QSettings().value("exportVideoAudioSlider").toString() == "")
        ui->exportVideoAudioSlider->setValue(20);
    else
        ui->exportVideoAudioSlider->setValue(QSettings().value("exportVideoAudioSlider").toInt());

    watermarkFileName = QSettings().value("watermarkFileName").toString();
    watermarkFileNameChanged(watermarkFileName);

    if (QSettings().value("tooltipsOn").toString() == "")
        ui->actionTooltips->setChecked(true);
    else
        ui->actionTooltips->setChecked(QSettings().value("tooltipsOn").toBool());

    if (QSettings().value("muteOn").toBool())
        ui->videoWidget->onMute();

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

    ui->folderButton->setToolTip(tr("<p><b>Selected folder</b></p>"
                                    "<p><i>Click here to open the folder</i></p>"
                                    ));

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

    //video
    ui->videoWidget->setToolTip(tr("<p><b>Media window</b></p>"
                                   "<p><i>Video and audio can be played here</i></p>"
                                   "<ul>"
                                   "<li>Player controls...</li>"
                                   "<li>Add clips...</li>"
                                   "</ul>"
                                ));

    ui->playButton->setToolTip(tr("<p><b>Play or pause</b></p>"
                              "<p><i>Play or pause the video</i></p>"
                              "<ul>"
                              "<li>Shortcut: Space</li>"
                              "</ul>"));
    ui->stopButton->setToolTip(tr("<p><b>Stop the video</b></p>"
                                "<p><i>Stop the video</i></p>"
                                ));

    ui->skipForwardButton->setToolTip(tr("<p><b>Go to next or previous clip</b></p>"
                              "<p><i>Go to next or previous clip</i></p>"
                              "<ul>"
                              "<li>Shortcut: ctrl-up and ctrl-down</li>"
                              "</ul>"));
    ui->skipBackwardButton->setToolTip(ui->skipForwardButton->toolTip());

    ui->seekBackwardButton->setToolTip(tr("<p><b>Go to next or previous frame</b></p>"
                              "<p><i>Go to next or previous frame</i></p>"
                              "<ul>"
                              "<li>Shortcut: ctrl-left and ctrl-right</li>"
                              "</ul>"));
    ui->seekForwardButton->setToolTip(ui->seekBackwardButton->toolTip());

    ui->setInButton->setToolTip(tr("<p><b>Set in- and out- point</b></p>"
                                  "<p><i>Set the inpoint of a new clip or change the in- or outpoint of the current clip to the current position on the timeline</i></p>"
                                  "<ul>"
                                  "<li>Change: inpoint before outpoint of last selected clip or outpoint after inpoint of last selected clip</li>"
//                                  "<li>Set: No last selected clip or inpoint after outpoint of last selected clip and outpoint before inpoint of last selected clip</li>"
                                  "<li>Shortcut: ctrl-i and ctrl-o</li>"
                                  "</ul>"));
    ui->setOutButton->setToolTip(ui->setInButton->toolTip());

    ui->muteButton->setToolTip(tr("<p><b>Mute or unmute</b></p>"
                              "<p><i>Mute or unmute sound (toggle)</i></p>"
                              "<ul>"
                              "<li>Video files will be muted if selected. Audio files will be unmuted if selected</li>"
                              "<li>Shortcut mute toggle: ctrl-m</li>"
                              "</ul>"));
    ui->speedComboBox->setToolTip(tr("<p><b>Speed</b></p>"
                                 "<p><i>Change the play speed of the video</i></p>"
                                 "<ul>"
                                 "<li>Supported speeds are depending on the codec</li>"
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
                                          "<li>Double click: Remove a tag</li>"
                                          "</ul>"));

    ui->tagFilter2ListView->setToolTip(ui->tagFilter1ListView->toolTip());

    ui->fileOnlyCheckBox->setToolTip(tr("<p><b>File only</b></p>"
                                        "<p><i>Show only clips of the selected file</i></p>"
                                        "<ul>"
                                        "<li>Timeline: Only clips which meet the filter criteria are shown in the timeline</li>"
                                        "</ul>"));

    ui->clipsSizeComboBox->setToolTip(tr("<p><b>Video size</b></p>"
                                         "<p><i>Shows the sizes found in the media used for clips</i></p>"
                                         "<ul>"
                                         "<li>One of them should be selected.</li>"
                                         "</ul>"));

    ui->clipsFramerateComboBox->setToolTip(tr("<p><b>Framerate</b></p>"
                                        "<p><i>Shows the framerates found in the media used for clips</i></p>"
                                        "<ul>"
                                        "<li>Ones of them should be selected.</li>"
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
                                                                                    "<li>To delete: First click to select item, second click to go into edit mode, double click to delete a tag</li>"
                                                                                    "</ul>"
                                                                                    ));


    //tt tags
    ui->newTagLineEdit->setToolTip(tr("<p><b>Tag field</b></p>"
                                      "<p><i>Create and remove tags</i></p>"
                                      "<ul>"
                                      "<li>Add new tag to tag list: fill in tag name and press return</li>"
                                      "</ul>"
                                      ));
    ui->tagsListView->setToolTip(tr("<p><b>Tag list</b></p>"
                                    "<p><i>List of tags used in clips or filter</i></p>"
                                    "<ul>"
                                    "<li>Add tag to clip: Drag and drop to tag column of clips</li>"
                                    "<li>Add tag to filter: Drag and drop to filters</li>"
                                    "<li>Delete tags: double click (or drag tag and drop in the tag field)</li>"
                                    "</ul>"
                                    ));

    //properties
    ui->propertyTreeView->setToolTip(tr("<p><b>Property list</b></p>"
                                     "<p><i>Show the properties (Metadata / EXIF data) for files of the selected folder</i></p>"
                                     "<ul>"
                                        "<li>Only properties and files applying to the <u>filters</u> are shown</li>"
                                        "<ul>"
                                        "<li>Filter files: only the columns matching are shown</li>"
                                        "<li>Filter properties: only the rows matching are shown</li>"
                                        "<li><u>Diff</u>: show only properties which differs between the files, are equal or both</li>"
                                        "</ul>"
                                        "<li>Properties belonging to the <u>selected file</u> are highlighted in blue</li>"
                                     "</ul>"
                                     ));

    ui->propertyEditorPushButton->setToolTip(tr("<p><b>Property editor</b></p>"
                                                "<p><i>Opens a new window to edit properties</i></p>"
                                                ));

    ui->propertyFilterLineEdit->setToolTip(tr("<p><b>Property filters</b></p>"
                                              "<p><i>Filters on properties or files shown</i></p>"
                                              "<ul>"
                                              "<li>Enter text to filter on properties. E.g. date shows all the date properties</li>"
                                              "</ul>"));
    ui->filterColumnsLineEdit->setToolTip(ui->propertyFilterLineEdit->toolTip());

    ui->propertyDiffCheckBox->setToolTip(tr("<p><b>Diff</b></p>"
                                            "<p><i>Determines which properties are shown based on if they are different or the same accross the movie files</i></p>"
                                            "<ul>"
                                            "<li>Unchecked: Only properties which are the same for all files</li>"
                                            "<li>Checked: Only properties which are different between files</li>"
                                            "<li>Partially checked (default): All properties</li>"
                                            "</ul>"));
    ui->refreshButton->setToolTip(tr("<p><b>Refresh</b></p>"
                                     "<p><i>Reloads all the properties</i></p>"
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
                                              "<li>Lossless: FFMpeg generated video file. Very fast! If codec, size and framerate of clips is not the same the result might not play correctly</li>"
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

    ui->cancelButton->setToolTip(tr("<p><b>Cancel export</b></p>"
                                    "<p><i>Current export will be aborted</i></p>"
                                    ));

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

    palette.setColor(QPalette::PlaceholderText, Qt::darkGray);

//    palette.setColor(ui->menuBar->backgroundRole(), Qt::red);

//    QPalette pal = ui->videoFilesTreeView->fileContextMenu->palette();
//    qDebug()<<"basecolor"<<pal.base().color()<<qApp->palette().base().color();
//    pal.setColor(QPalette::Base, qApp->palette().window().color());
//    fileContextMenu->setPalette(pal);


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

    palette.setColor(QPalette::PlaceholderText, Qt::lightGray);

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
               "<p>Copyright © 2019-2020</p>"
               "<p><a href=\"https://actioncamvideocompanion.com\">ACVC Website</a></p>"
               "<p>This program proudly uses the following projects:</p>"
               "<ul>"
               "<li><a href=\"https://www.qt.io/\">Qt</a> application and UI framework</li>"
               "<li><a href=\"https://www.shotcut.org/\">Shotcut</a> Open source video editor (timeline and version check)</li>"
               "<li><a href=\"https://www.ffmpeg.org/\">FFmpeg</a> multimedia format and codec libraries (lossless and encoded previews)</li>"
               "<li><a href=\"https://exiftool.org/\">Exiftool</a> Read, Write and Edit Meta Information (Properties)</li>"
               "</ul>"
               "<p>This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.</p>"
               "<p>As ACVC may contain bugs, BACKUP your video files before editing!</p>"
               "<p>ACVC issuetracker on GitHub <a href=\"https://github.com/ewoudwijma/ActionCamVideoCompanion/issues\">GitHub ACVC issues</a></p>"
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

    ui->exportButton->setEnabled(ui->clipsTableView->model()->rowCount() > 0);

    createContextSensitiveHelp("New Edit");
}

void MainWindow::on_propertyFilterLineEdit_textChanged(const QString &)//arg1
{
    emit propertyFilterChanged(ui->propertyFilterLineEdit, ui->propertyDiffCheckBox);
}

void MainWindow::on_propertyDiffCheckBox_stateChanged(int )//arg1
{
    emit propertyFilterChanged(ui->propertyFilterLineEdit, ui->propertyDiffCheckBox);
}

void MainWindow::onClipsChangedToVideo(QAbstractItemModel *itemModel)
{
    ui->clipRowsCounterLabel->setText(QString::number(itemModel->rowCount()) + " / " + QString::number(ui->clipsTableView->clipsItemModel->rowCount()));
}

void MainWindow::onFolderIndexClicked(QAbstractItemModel *itemModel)
{
//    qDebug()<<"MainWindow::onFolderIndexClicked"<<itemModel->rowCount();

//    ui->spinnerLabel->start();

    spinnerLabel = new ASpinnerLabel(ui->filesTabWidget);
    spinnerLabel->start();

    ui->clipRowsCounterLabel->setText(QString::number(itemModel->rowCount()) + " / " + QString::number(ui->clipsTableView->clipsItemModel->rowCount()));

    ui->filesTabWidget->setCurrentIndex(1); //go to files tab

    QString folderName = ui->folderTreeView->directoryModel->fileInfo(ui->folderTreeView->currentIndex()).absoluteFilePath();

    ui->folderButton->setText(folderName);
    ui->folderButton->setIcon(style()->standardIcon(QStyle::SP_DirIcon).pixmap(QSize(ui->folderButton->size().height(),ui->folderButton->size().height())));

    ui->timelineWidget->transitiontimeLastGood = -1;

    onClipsFilterChanged(false);
    emit timelineWidgetsChanged(ui->transitionTimeSpinBox->value(), ui->transitionComboBox->currentText(), ui->clipsTableView);

    createContextSensitiveHelp("Folder selected", folderName);
}

void MainWindow::showContextSensitiveHelp(int index)
{
    if (requestList[index].tabWidget != nullptr)
        requestList[index].tabWidget->setCurrentIndex(requestList[index].tabIndex);

    QString widgetNamePlus = requestList[index].widgetName;

    if (requestList.count() > 1)
        widgetNamePlus +=  + " (" + QString::number(index+1) + " of " + QString::number(requestList.count()) + ")";

    QString text = QStringLiteral("<div style=\"background-color: magenta\"><p>▲%1</p><p><i>%2</i></p></div>").arg(requestList[index].context + ": " + widgetNamePlus, requestList[index].helpText);//

//    qDebug()<<"MainWindow::showContextSensitiveHelp"<<requestList[index].context<<requestList[index].helpText.mid(0,80);
    QToolTip::showText( requestList[index].widget->mapToGlobal( QPoint( requestList[index].widget->width() / 2, requestList[index].widget->height() ) ), text); //, this, QRect(),10000

//    if (index + 1 < requestList.count())
//    {
//        QTimer::singleShot(5000, this, [index, this]()->void //duration depends on the amount of text shown requestList[index].helpText.length() * 60
//        {
////            showContextSensitiveHelp(index + 1);
//            showContextSensitiveHelp(currentRequestNumber);
//            if (currentRequestNumber + 1 < requestList.count())
//                 currentRequestNumber++;
//            else
//             currentRequestNumber = 0;

//        });
//    }
}

void MainWindow::createContextSensitiveHelp(QString context, QString arg1)
{
    if (!ui->actionContext_Sensitive_Help->isChecked() && context != "ByeBye")
        return;

    requestList.clear();
    currentRequestNumber = 0;

//    qDebug()<<"MainWindow::createContextSensitiveHelp"<<context<<arg1<<ui->videoFilesTreeView->currentIndex().data();
    QString name = qgetenv("USER");
    if (name.isEmpty())
        name = qgetenv("USERNAME");
    name[0] = name.at(0).toTitleCase();

    AContextSensitiveHelpRequest contextSensitiveHelpRequest;
    if (context == "ACVC started" || context == "Folder selected")
    {
        if (ui->folderTreeView->currentIndex() == QModelIndex())
            requestList.append({context, ui->folderTreeView, "Folder view", tr("<p>Select folder</p>"
                                "<p><i>Select a folder with media files.</i></p>"
                                "<ul>"
                                "<li>Supported video files:  .mp4, .avi, .wmv, .mts</li>"
                                "<li>Supported audio files:  .mp3</li>"
                                "</ul>"), ui->filesTabWidget, 0});
        else if (ui->videoWidget->selectedFileName == "")
            requestList.append({context, ui->videoFilesTreeView, "Video files", tr("<p>Select a file</p>"
                                                            "<p><i>If a video or audio file is selected, the file can be played in the media window, it's edits are shown and edits van be changed</i></p>"
                                "<ul>"
                                "<li>Next Context Sensitive Help Message: Press ? or CTRL-R</li>"
                                "</ul>"), ui->filesTabWidget, 1});

        if (context == "ACVC started")
        {
            requestList.append({context, ui->videoWidget, "Welcome message", tr("<p>Context sensitive help</p>"
                                "<p><i>Context sensitive help is enabled and will guide you through ACVC.</i></p>"
                                "<ul>"
                                "<li>For a specific event, more then one help message can be shown, this is shown in the title bar as (x of y)</li>"
                                "<li>Show next message: Press the ? in the toolbar repeatedly</li>"
                                "<li>Switch off: Menu / Help / Context sensitive help</li>"
                                "<li>Next Context Sensitive Help Message: Press ? or CTRL-R</li>"
                                "</ul>"), nullptr, -1});
        }

        if (context == "Folder selected")
        {
            requestList.append({context, ui->propertyTreeView, "Properties", tr("<p>View properties</p>"
                                                            "<p><i>View and update the properties of the mediafiles in the selected folder</i></p>"
                                                            "<ul>"
                                                            "<li>Selected folder: %1</li>"
                                                            "</ul>").arg(arg1), ui->clipsTabWidget, 1});
        }

    }
    else if (context == "File selected" || context == "Clips filter changed" || context == "New Edit")
    {

//        qDebug()<<"MainWindow::createContextSensitiveHelp"<<context;

        if (context == "New Edit")
        {
            requestList.append({context, ui->newTagLineEdit, "Tags", tr("<p>Create tags</p>"
                                                            "<p><i>Create tags. Drag and drop created tabs to clips. Tags can be used for filtering</i></p>"
                                                            ), ui->clipsTabWidget, 0});

            if (ui->clipsTableView->clipsProxyModel->rowCount() != 0)
                requestList.append({context, ui->ratingFilterComboBox, "Filters", tr("<p>Select filters</p>"
                                                        "<p><i>%1 Clips ready to be exported</i></p>"
                                                        "<ul>"
                                                        "<li>Use filters (rating, alike, tags, file only) to select clips to be exported</li>"
                                                        "</ul>").arg(QString::number(ui->clipsTableView->clipsProxyModel->rowCount())), ui->clipsTabWidget, 0});
        }

        if (ui->clipsTableView->clipsItemModel->rowCount() == 0)
        {
            requestList.append({context, ui->setInButton, "In point", tr("<p>Set in point</p>"
                                                            "<p><i>No clips created yet, you can add clips here by setting in and outpoints</i></p>"
                                                            ), ui->clipsTabWidget, 0});
            requestList.append({context, ui->propertyEditorPushButton, "Properties", tr("<p>Edit properties</p>"
                                                            "<p><i>Edit the properties of the media files and rename the files</i></p>"
                                                            ), ui->clipsTabWidget, 1});
        }
        else if (ui->clipsTableView->clipsProxyModel->rowCount() == 0)
            requestList.append({context, ui->ratingFilterComboBox, "Filters", tr("<p>Select filters</p>"
                                                            "<p><i>%1 Clips available but not selected</i></p>"
                                                            "<ul>"
                                                            "<li>Use filters (rating, alike, tags, file only) to select clips to be exported</li>"
                                                            "</ul>").arg(QString::number(ui->clipsTableView->clipsItemModel->rowCount())), ui->clipsTabWidget, 0});
        else //clips selected
        {
            QString audioDuration = AGlobal().msec_to_time(AGlobal().frames_to_msec(ui->timelineWidget->maxAudioDuration));
            QString videoDuration = AGlobal().msec_to_time(AGlobal().frames_to_msec(ui->timelineWidget->maxVideoDuration));
            QString combinedDuration = AGlobal().msec_to_time(AGlobal().frames_to_msec(ui->timelineWidget->maxCombinedDuration));
//            qDebug()<<"MainWindow::onClipFilterChanged"<<ui->clipsTableView->clipsProxyModel->rowCount()<<audioDuration<<videoDuration;

            if (ui->timelineWidget->maxAudioDuration != ui->timelineWidget->maxVideoDuration)
                requestList.append({context, ui->exportButton, "Export", tr("<p>Run export</p>"
                                                                "<p><i>%1 clips selected, export possible but audio duration not equal to video duration</i></p>"
                                                                "<ul>"
                                    "<li>Video duration: %2</li>"
                                    "<li>Audio duration: %3</li>"
                                                                "</ul>").arg(QString::number(ui->clipsTableView->clipsProxyModel->rowCount()), videoDuration, audioDuration), ui->clipsTabWidget, 0});
            else
                requestList.append({context, ui->exportButton, "Export", tr("<p>Run export</p>"
                                                                "<p><i>File selected, %1 clips selected, export possible, video and audio same length</i></p>"
                                                                "<ul>"
                                                                "<li>Video / Audio length: %2</li>"
                                                                "</ul>").arg(QString::number(ui->clipsTableView->clipsProxyModel->rowCount()), combinedDuration), ui->clipsTabWidget, 0});

            if (ui->timelineWidget->maxAudioDuration == 0)
            {
                requestList.append({context, ui->audioFilestreeView, "Audio files", tr("<p>List of audio files in folder</p>"
                                                                "<p><i>%1 clips selected but no audio clips selected. Add audio files here and create clips for them</i></p>"
                                                                ).arg(QString::number(ui->clipsTableView->clipsProxyModel->rowCount())), ui->clipsTabWidget, 0});
            }

        }
//        qDebug()<<"MainWindow::createContextSensitiveHelp done"<<context;
    }
    else if (context == "Export started") //to do use target as arg1
    {
        requestList.append({context, ui->logTableView, "Jobs", tr("<p>Overview of jobs</p>"
                                                        "<p><i>Exporting involves running of one or more processes</i></p>"
                                                        "<ul>"
                                                        "<li>Processes and process details are shown here</li>"
                                                        "</ul>"), ui->clipsTabWidget, 2});
    }
    else if (context == "Export completed")
    {
        if (arg1 == "") //no error
            requestList.append({context, ui->exportFilesTreeView, "Export files",  tr("<p>List of exported files</p>"
                                                            "<p><i>Exported file can be found here</i></p>"
                                                            ), ui->filesTabWidget, 1});
        else
            requestList.append({context, ui->logTableView, "Jobs", tr("<p>Overview of jobs</p>"
                                                            "<p><i>Export completed with error, check the log to find out what went wrong</i></p>"
                                                            "<ul>"
                                                            "<li>Error message: %1</li>"
                                                            "</ul>").arg(arg1), ui->clipsTabWidget, 2});
    }
    else if (context == "ByeBye")
    {
        requestList.append({context, ui->videoWidget, "Context Sensitive Help", tr("Bye bye %1.").arg(name), nullptr, -1});
    }

//    QTimer *timer = new QTimer(this);
//            QObject::connect(timer, SIGNAL(timeout()), this, SLOT(showGPS()));
//            timer->start(1000); //time specified in ms

    if (requestList.count() > 0)
    {
        QTimer::singleShot(100, this, [this]()->void //timer needed to show first message
        {
                               showContextSensitiveHelp(currentRequestNumber);
                               if (currentRequestNumber + 1 < requestList.count())
                                    currentRequestNumber++;
                               else
                                currentRequestNumber = 0;
        });
    }
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

    onClipsFilterChanged(false);

    if (ui->exportFilesTreeView->model() != index.model())
        createContextSensitiveHelp("File selected");
}

void MainWindow::onClipsFilterChanged(bool fromFilters)
{
    ui->clipRowsCounterLabel->setText(QString::number(ui->clipsTableView->model()->rowCount()) + " / " + QString::number(ui->clipsTableView->clipsItemModel->rowCount()));

//    qDebug()<<"MainWindow::onClipFilterChanged"<<QSettings().value("ratingFilterComboBox")<<ui->ratingFilterComboBox->currentText();

    emit clipsFilterChanged(ui->ratingFilterComboBox, ui->alikeCheckBox, ui->tagFilter1ListView, ui->tagFilter2ListView, ui->fileOnlyCheckBox);

    ui->exportButton->setEnabled(ui->clipsTableView->model()->rowCount() > 0);

    if (fromFilters)
        createContextSensitiveHelp("Clips filter changed");
}

void MainWindow::onTagFilter1ListViewChanged()
{
    qDebug()<<"MainWindow::on_tagFilter1ListView_indexesMoved";

    QString string1 = ui->tagFilter1ListView->modelToString();

    if (QSettings().value("tagFilter1") != string1)
    {
        QSettings().setValue("tagFilter1", string1);
        QSettings().sync();
    }

    onClipsFilterChanged(true);

}

void MainWindow::onTagFilter2ListViewChanged()
{
    qDebug()<<"MainWindow::on_tagFilter2ListView_indexesMoved";

    QString string1 = ui->tagFilter2ListView->modelToString();

    if (QSettings().value("tagFilter2") != string1)
    {
        QSettings().setValue("tagFilter2", string1);
        QSettings().sync();
    }

    onClipsFilterChanged(true);
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

    ui->statusBar->showMessage(tr("%1 clip changes and %2 deletions will be saved").arg(QString::number(changeCount), QString::number(ui->clipsTableView->nrOfDeletedItems)), 5000);
//        ui->statusBar->showMessage("No clip changes to save", 5000);

    ui->clipsTableView->saveModels();
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

void MainWindow::on_newTagLineEdit_returnPressed()
{
    if (ui->tagsListView->addTag(ui->newTagLineEdit->text()))
        ui->newTagLineEdit->clear();
    else
        QMessageBox::information(this, "Add tag", "Tag " + ui->newTagLineEdit->text() + " already in list");
}

void MainWindow::on_exportButton_clicked()
{
    ui->cancelButton->setEnabled(true);
    createContextSensitiveHelp("Export started");

    spinnerLabel = new ASpinnerLabel(ui->exportGroupBox);

    int transitionTime = 0;
//    if (ui->transitionComboBox->currentText() != "No transition")
    transitionTime = ui->transitionTimeSpinBox->value();
//    qDebug()<<"MainWindow::on_exportButton_clicked";
    ui->exportWidget->exportClips(ui->clipsTableView->clipsProxyModel, ui->exportTargetComboBox->currentText(), ui->exportSizeComboBox->currentText(), ui->exportFramerateComboBox->currentText(), transitionTime, ui->progressBar, ui->exportVideoAudioSlider, spinnerLabel, watermarkFileName, ui->exportButton, ui->clipsFramerateComboBox, ui->clipsSizeComboBox, ui->statusBar);

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
        reply = QMessageBox::question(this, "Donate", tr("If you like this software, consider to make a donation. Go to the Donate page on the ACVC website for more information (%1 exports made)").arg(QSettings().value("exportCounter").toString()), QMessageBox::Yes|QMessageBox::No);
        if (reply == QMessageBox::Yes)
            QDesktopServices::openUrl(QUrl("https://www.actioncamvideocompanion.com/support"));
    }
}

void MainWindow::onExportCompleted(QString error)
{
    createContextSensitiveHelp("Export completed", error);

#ifdef Q_OS_WIN
    QWinTaskbarProgress *progress = taskbarButton->progress();

    if (error == "")
        progress->setVisible(false);
    else
    {
        progress->stop();

        QTimer::singleShot(5000, this, [progress]()->void //timer needed to show first message
        {
                               progress->resume();
                               progress->setVisible(false);
        });

    }
#endif
    ui->cancelButton->setEnabled(false);
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
    onClipsFilterChanged(true);
}

void MainWindow::on_fileOnlyCheckBox_clicked(bool checked)
{
    if (QSettings().value("fileOnlyCheckBox").toBool() != checked)
    {
        QSettings().setValue("fileOnlyCheckBox", checked);
        QSettings().sync();
    }
    onClipsFilterChanged(true);
}

void MainWindow::on_actionDebug_mode_triggered(bool checked)
{
//    qDebug()<<"on_actionDebug_mode_triggered"<<checked;

    if (checked)
    {
        ui->clipsTabWidget->insertTab(3, graphWidget1, "Graph1");
        ui->clipsTabWidget->insertTab(4, graphWidget2, "Graph2");
        ui->clipsTabWidget->insertTab(5, graphicsWidget, "Timeline2.0");
    }
    else
    {
        ui->clipsTabWidget->removeTab(5);
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
//            qDebug()<<"MainWindow::on_resetSortButton_clicked1"<<row<<ui->clipsTableView->clipsItemModel->index(row, orderBeforeLoadIndex).data().toInt() * 10 << ui->clipsTableView->clipsItemModel->index(row, orderAtLoadIndex).data().toInt()<<ui->clipsTableView->clipsItemModel;
            ui->clipsTableView->clipsItemModel->setData(ui->clipsTableView->clipsItemModel->index(row, orderAtLoadIndex), ui->clipsTableView->clipsItemModel->index(row, orderBeforeLoadIndex).data().toInt() * 10);
            ui->clipsTableView->clipsItemModel->setData(ui->clipsTableView->clipsItemModel->index(row, changedIndex), "yes");
        }
        if (ui->clipsTableView->clipsItemModel->index(row, orderBeforeLoadIndex).data().toInt() * 10 != ui->clipsTableView->clipsItemModel->index(row, orderAfterMovingIndex).data().toInt())
        {
//            qDebug()<<"MainWindow::on_resetSortButton_clicked2"<<row<<ui->clipsTableView->clipsItemModel->index(row, orderBeforeLoadIndex).data().toInt() * 10 << ui->clipsTableView->clipsItemModel->index(row, orderAfterMovingIndex).data().toInt();
            ui->clipsTableView->clipsItemModel->setData(ui->clipsTableView->clipsItemModel->index(row, orderAfterMovingIndex), ui->clipsTableView->clipsItemModel->index(row, orderBeforeLoadIndex).data().toInt() * 10);
            ui->clipsTableView->clipsItemModel->setData(ui->clipsTableView->clipsItemModel->index(row, changedIndex), "yes");
        }
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

void MainWindow::on_transitionDial_sliderMoved(int )//position
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

        if (ui->positionSpinBox->value() + delta <= AGlobal().msec_to_frames(ui->videoWidget->playerDuration))
            ui->positionSpinBox->setValue(ui->positionSpinBox->value() + delta);
//        positionValueChangedBy = "";
    }
    positiondialOldValue = value;
}

void MainWindow::on_positionDial_sliderMoved(int )//position
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

    ui->positionSpinBox->blockSignals(true); //do not fire valuechanged signal
    ui->positionSpinBox->setValue(AGlobal().msec_to_frames(progress));
    ui->positionSpinBox->blockSignals(false);
}

void MainWindow::onDurationChanged(int duration)
{
    ui->durationLabel->setText(AGlobal().msec_to_time(duration).prepend(" / "));

    ui->skipBackwardButton->setEnabled(duration > 0);
    ui->seekBackwardButton->setEnabled(duration > 0);
    ui->playButton->setEnabled(duration > 0);
    ui->seekForwardButton->setEnabled(duration > 0);
    ui->skipForwardButton->setEnabled(duration > 0);
    ui->muteButton->setEnabled(duration > 0);
    ui->stopButton->setEnabled(duration > 0);
    ui->setInButton->setEnabled(duration > 0);
    ui->setOutButton->setEnabled(duration > 0);
    ui->speedComboBox->setEnabled(duration > 0);
}

void MainWindow::onPlayerStateChanged(QMediaPlayer::State state)
{
    if (state == QMediaPlayer::PlayingState)
        ui->playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
    if (state == QMediaPlayer::PausedState)
        ui->playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    if (state == QMediaPlayer::StoppedState)
        ui->playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
}

void MainWindow::onMutedChanged(bool muted)
{
//    qDebug()<<"AVideoWidget::onMutedChanged"<<muted;
    ui->muteButton->setIcon(style()->standardIcon(muted
            ? QStyle::SP_MediaVolumeMuted
            : QStyle::SP_MediaVolume));

    if (QSettings().value("muteOn").toBool() != muted)
    {
        QSettings().setValue("muteOn", muted);
        QSettings().sync();
    }

}

void MainWindow::onPlaybackRateChanged(qreal rate)
{
    ui->speedComboBox->setCurrentText(QString::number(rate) + "x");
}

void MainWindow::showUpgradePrompt()
{
    ui->statusBar->showMessage("Version check...", 15000);
    QNetworkRequest request(QUrl("http://www.actioncamvideocompanion.com/version.json"));
    QSslConfiguration sslConfig = request.sslConfiguration();
    sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
    request.setSslConfiguration(sslConfig);
    m_network.get(request);
}

void MainWindow::onUpgradeCheckFinished(QNetworkReply* reply)
{
//    qDebug()<<"MainWindow::onUpgradeCheckFinished";
    QString m_upgradeUrl = "https://actioncamvideocompanion.com/download/";

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
                reply = QMessageBox::question(this, "Version check", tr("ACVC version %1 is available! Click Yes to get it.").arg(latest), QMessageBox::Yes|QMessageBox::No);
                if (reply == QMessageBox::Yes)
                    QDesktopServices::openUrl(QUrl(m_upgradeUrl));

            } else {
                ui->statusBar->showMessage(tr("Version check: You are running the latest version of ACVC."), 15000);
            }
            reply->deleteLater();
            return;
        } else {
            ui->statusBar->showMessage("Version check: failed to parse version.json "+ error->errorString());
        }
    } else {
        ui->statusBar->showMessage("Network error " + reply->errorString());
    }
    QMessageBox::StandardButton mreply;
    mreply = QMessageBox::question(this, "Version check", tr("Failed to read version.json when checking. Click here to go to the Web site."), QMessageBox::Yes|QMessageBox::No);
    if (mreply == QMessageBox::Yes)
        QDesktopServices::openUrl(QUrl(m_upgradeUrl));
    reply->deleteLater();
}

void MainWindow::onPropertiesLoaded()
{
//    qDebug()<<"MainWindow::onPropertiesLoaded";
    emit propertyFilterChanged(ui->propertyFilterLineEdit, ui->propertyDiffCheckBox);

    ui->clipsFramerateComboBox->blockSignals(true); // do not fire on_clipsFramerateComboBox_currentTextChanged

    ui->clipsFramerateComboBox->clear();
    ui->clipsFramerateComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);

    ui->clipsSizeComboBox->clear();
    ui->clipsSizeComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);

    bool foundCurrentClipsFramerate = false;

    if (ui->clipsTableView->clipsItemModel->rowCount() == 0) //no clips created yet, select from the properties
    {
        for (int column = firstFileColumnIndex; column < ui->propertyTreeView->propertyItemModel->columnCount(); column++)
        {
            QString fileName = ui->propertyTreeView->propertyItemModel->headerData(column, Qt::Horizontal).toString();

            if (!fileName.toLower().contains(".mp3") && !fileName.toLower().contains("lossless") && !fileName.toLower().contains("encode") && !fileName.toLower().contains("shotcut") && !fileName.toLower().contains("premiere"))
            {
                QVariant *frameratePointer = new QVariant();
                ui->propertyTreeView->onGetPropertyValue(fileName, "VideoFrameRate", frameratePointer);

                int fpsSuggested = qRound(frameratePointer->toDouble());

                if (ui->clipsFramerateComboBox->findText(QString::number(fpsSuggested)) < 0)
                {
                    ui->clipsFramerateComboBox->addItem(QString::number(fpsSuggested));
                }

                if (QString::number(fpsSuggested) == QSettings().value("frameRate"))
                    foundCurrentClipsFramerate = true;

                QVariant *widthValue = new QVariant();
                QVariant *heightValue = new QVariant();
                ui->propertyTreeView->onGetPropertyValue(fileName, "ImageWidth", widthValue);
                ui->propertyTreeView->onGetPropertyValue(fileName, "ImageHeight", heightValue);
                QString size = widthValue->toString() + " x " + heightValue->toString();
//                qDebug()<<fileName<<"size"<<size;
                if (ui->clipsSizeComboBox->findText(size) < 0)
                    ui->clipsSizeComboBox->addItem(size);
            }
        }
    }
    else //select from the clips
    {
        for (int row = 0; row < ui->clipsTableView->clipsItemModel->rowCount(); row++)
        {
            QString fileName = ui->clipsTableView->clipsItemModel->index(row, fileIndex).data().toString();

            if (!fileName.toLower().contains(".mp3") && !fileName.toLower().contains("lossless") && !fileName.toLower().contains("encode") && !fileName.toLower().contains("shotcut") && !fileName.toLower().contains("premiere"))
            {
                QVariant *frameratePointer = new QVariant();
                ui->propertyTreeView->onGetPropertyValue(fileName, "VideoFrameRate", frameratePointer);

                int fpsSuggested = qRound(frameratePointer->toDouble());

                if (ui->clipsFramerateComboBox->findText(QString::number(fpsSuggested)) < 0)
                {
                    ui->clipsFramerateComboBox->addItem(QString::number(fpsSuggested));
                }

                if (QString::number(fpsSuggested) == QSettings().value("frameRate"))
                    foundCurrentClipsFramerate = true;

                QVariant *widthValue = new QVariant();
                QVariant *heightValue = new QVariant();
                ui->propertyTreeView->onGetPropertyValue(fileName, "ImageWidth", widthValue);
                ui->propertyTreeView->onGetPropertyValue(fileName, "ImageHeight", heightValue);
                QString size = widthValue->toString() + " x " + heightValue->toString();
                if (ui->clipsSizeComboBox->findText(size) < 0)
                    ui->clipsSizeComboBox->addItem(size);
            }
        }
    }

    ui->clipsFramerateComboBox->blockSignals(false);

    if (foundCurrentClipsFramerate)
        ui->clipsFramerateComboBox->setCurrentText(QSettings().value("frameRate").toString());
    else if (ui->clipsFramerateComboBox->currentText() != "" && ui->clipsFramerateComboBox->currentText() != "0")
    {
        ui->statusBar->showMessage("Clips framerate changed from " + QSettings().value("frameRate").toString() + " to " + ui->clipsFramerateComboBox->currentText(), 10000);
        QSettings().setValue("frameRate", ui->clipsFramerateComboBox->currentText());
    }

//    emit timelineWidgetsChanged(ui->transitionTimeSpinBox->value(), ui->transitionComboBox->currentText(), ui->clipsTableView);

    spinnerLabel->stop();

    emit propertiesLoaded();

} //onPropertiesLoaded

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
    QDesktopServices::openUrl(QUrl("https://www.actioncamvideocompanion.com/support"));
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

    onClipsFilterChanged(true);
}

void MainWindow::on_actionHelp_triggered()
{
    QDesktopServices::openUrl(QUrl("https://actioncamvideocompanion.com/features"));

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
#ifdef Q_OS_WIN
        watermarkFileName = QFileDialog::getOpenFileName(this,
            tr("Open Image"), ui->folderTreeView->directoryModel->rootPath(), tr("Image Files (*.png *.jpg *.bmp *.ico)"));
#else
        watermarkFileName = QFileDialog::getOpenFileName(this,
            tr("Open Image"), QDir::home().homePath(), tr("Image Files (*.png *.jpg *.bmp *.ico)"));
#endif
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
    QDesktopServices::openUrl(QUrl("https://github.com/ewoudwijma/ActionCamVideoCompanion/issues"));
}

void MainWindow::on_actionMute_triggered()
{
    ui->videoWidget->onMute();
}

void MainWindow::on_clipsFramerateComboBox_currentTextChanged(const QString &arg1)
{
//    qDebug()<<"MainWindow::on_clipsFramerateComboBox_currentTextChanged"<<arg1<<QSettings().value("frameRate");

    ui->transitionDial->setRange(0, arg1.toInt() * 4);

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

void MainWindow::on_skipBackwardButton_clicked()
{
    ui->videoWidget->skipPrevious();
}

void MainWindow::on_seekBackwardButton_clicked()
{
    ui->videoWidget->rewind();
}

void MainWindow::on_playButton_clicked()
{
    ui->videoWidget->togglePlayPaused();
}

void MainWindow::on_seekForwardButton_clicked()
{
    ui->videoWidget->fastForward();
}

void MainWindow::on_skipForwardButton_clicked()
{
    ui->videoWidget->skipNext();
}

void MainWindow::on_stopButton_clicked()
{
    ui->videoWidget->onStop();
}

void MainWindow::on_muteButton_clicked()
{
    ui->videoWidget->onMute();
}

void MainWindow::on_setInButton_clicked()
{
    ui->videoWidget->onSetIn();
}

void MainWindow::on_setOutButton_clicked()
{
    ui->videoWidget->onSetOut();
}

void MainWindow::on_speedComboBox_currentTextChanged(const QString &arg1)
{
    double playbackRate = arg1.left(arg1.lastIndexOf("x")).toDouble();
    if (arg1.indexOf(" fps")  > 0)
        playbackRate = arg1.left(arg1.lastIndexOf(" fps")).toDouble() / QSettings().value("frameRate").toInt();
//    qDebug()<<"AVideoWidget::onSpeedChanged"<<speed<<playbackRate<<speed.lastIndexOf("x")<<speed.left(speed.lastIndexOf("x"));
    ui->videoWidget->setPlaybackRate(playbackRate);
}

void MainWindow::on_actionContext_Sensitive_Help_changed()
{
//    qDebug()<<"MainWindow::on_actionContext_Sensitive_Help_changed"<<QSettings().value("contextSensitiveHelpOn").toBool()<<ui->actionContext_Sensitive_Help->isChecked();
    if (QSettings().value("contextSensitiveHelpOn").toBool() != ui->actionContext_Sensitive_Help->isChecked())
    {
        QSettings().setValue("contextSensitiveHelpOn", ui->actionContext_Sensitive_Help->isChecked());
        QSettings().sync();
    }

    if (ui->actionContext_Sensitive_Help->isChecked())
        createContextSensitiveHelp("ACVC started");
    else
        createContextSensitiveHelp("ByeBye");
}

void MainWindow::on_actionTooltips_changed()
{
//    qDebug()<<"MainWindow::on_actionTooltips_changed"<<ui->actionTooltips->isChecked();

    if (QSettings().value("tooltipsOn").toBool() != ui->actionTooltips->isChecked())
    {
        QSettings().setValue("tooltipsOn", ui->actionTooltips->isChecked());
        QSettings().sync();
    }
}

void MainWindow::on_actionRepeat_context_sensible_help_triggered()
{
    if (!ui->actionContext_Sensitive_Help->isChecked())
    {
        ui->actionContext_Sensitive_Help->setChecked(true);
    }

    if (requestList.count() > 0)
    {
        QTimer::singleShot(100, this, [this]()->void //timer needed to show first message
        {
                               showContextSensitiveHelp(currentRequestNumber);
                               if (currentRequestNumber + 1 < requestList.count())
                                    currentRequestNumber++;
                               else
                                    currentRequestNumber = 0;
        });
    }
}

void MainWindow::on_folderButton_clicked()
{
    QString folderName = ui->folderTreeView->directoryModel->fileInfo(ui->folderTreeView->currentIndex()).absoluteFilePath();
    QDesktopServices::openUrl( QUrl::fromLocalFile( folderName ) );
}

void MainWindow::on_progressBar_valueChanged(int value)
{
#ifdef Q_OS_WIN
    QWinTaskbarProgress *progress = taskbarButton->progress();
    progress->setVisible(true);
    progress->setValue(value);
#endif
}

void MainWindow::on_cancelButton_clicked()
{
    ui->exportWidget->stopAllProcesses();
    ui->exportButton->setEnabled(true);

    spinnerLabel->stop();

    ui->progressBar->setValue(0);
    ui->cancelButton->setEnabled(false);
}

void MainWindow::on_propertyEditorPushButton_clicked()
{
    spinnerLabel->start();

    propertyEditorDialog = new APropertyEditorDialog(this);
//    propertyEditorDialog->setModal(true);


    QRect savedGeometry = QSettings().value("Geometry").toRect();
    savedGeometry.setX(savedGeometry.x() + savedGeometry.width() * .05);
    savedGeometry.setY(savedGeometry.y() + savedGeometry.height() * .05);
    savedGeometry.setWidth(savedGeometry.width() * .9);
    savedGeometry.setHeight(savedGeometry.height() * .9);
    propertyEditorDialog->setGeometry(savedGeometry);

//    propertyEditorDialog->propertyItemModel = ui->propertyTreeView->propertyItemModel;

    connect(propertyEditorDialog, &APropertyEditorDialog::addJob, ui->logTableView, &AJobTableView::onAddJob);
    connect(propertyEditorDialog, &APropertyEditorDialog::addToJob, ui->logTableView, &AJobTableView::onAddToJob);

    connect(propertyEditorDialog, &APropertyEditorDialog::releaseMedia, ui->videoWidget, &AVideoWidget::onReleaseMedia);
    connect(propertyEditorDialog, &APropertyEditorDialog::reloadClips, ui->clipsTableView, &AClipsTableView::onReloadClips);
    connect(propertyEditorDialog, &APropertyEditorDialog::reloadProperties, ui->propertyTreeView, &APropertyTreeView::onReloadProperties);

    connect(propertyEditorDialog, &APropertyEditorDialog::finished, this, &MainWindow::onPropertyEditorDialogFinished);

    connect(this, &MainWindow::propertiesLoaded, propertyEditorDialog, &APropertyEditorDialog::onPropertiesLoaded);

    propertyEditorDialog->setProperties(ui->propertyTreeView->propertyItemModel);

    spinnerLabel->stop();

    propertyEditorDialog->setWindowModality(Qt::NonModal);

    propertyEditorDialog->show();
}

void MainWindow::onPropertyEditorDialogFinished(int result)
{
//    qDebug()<<"MainWindow::onPropertyEditorDialogFinished"<<result;
    disconnect(this, &MainWindow::propertiesLoaded, propertyEditorDialog, &APropertyEditorDialog::onPropertiesLoaded);
    propertyEditorDialog = nullptr; //garbagecollection?
}

void MainWindow::on_filterColumnsLineEdit_textChanged(const QString &arg1)
{
    ui->propertyTreeView->onPropertyColumnFilterChanged(arg1);
}

void MainWindow::on_refreshButton_clicked()
{
    ui->propertyTreeView->onReloadProperties();
//    emit reloadProperties;
}
