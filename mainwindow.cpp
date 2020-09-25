#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QStyleFactory>

#include <QDesktopServices>
#include <QFileDialog>
#include <QGraphicsVideoItem>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QNetworkReply>
#include <QShortcut>
#include <QSslConfiguration>
#include <QTimer>
#include <QVersionNumber>
#include <QtDebug>

#include "agfilesystem.h"
#include "aglobal.h"
#include "agview.h"
#include <QtMath>

#include <QToolTip>

#include <QCloseEvent>

#ifdef Q_OS_WIN
    #include <QWinTaskbarProgress>
#endif
//https://stackoverflow.com/questions/43347722/qt-show-progress-bar-in-dock-macos
//https://code.qt.io/cgit/qt-creator/qt-creator.git/tree/src/plugins/coreplugin/progressmanager

#include <QScrollBar>
#include <QGraphicsScene>
#include <QTextEdit>
#include <QTextBrowser>
#include <QHostAddress>
#include <QNetworkInterface>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
//    qDebug()<<"MainWindow::MainWindow"<<thread();
    ui->setupUi(this);

    qApp->installEventFilter(this);

    agFileSystem = new AGFileSystem(this);

    changeUIProperties();

    allConnects();

    loadSettings();

//    onClipsFilterChanged(false); //crash if removed...

    allTooltips();

    QTimer::singleShot(0, this, [this]()->void
    {
                           QString theme = QSettings().value("theme").toString();

                           if (theme == "White")
                               on_actionWhite_theme_triggered();
                           else
                               on_actionBlack_theme_triggered();

                           QString selectedFolderName = QSettings().value("selectedFolderName").toString();

                           QDir recycleDir(selectedFolderName);
                           if (!recycleDir.exists())
                                selectedFolderName = "";

                            //here as otherwise openfolder not opened
                           if (selectedFolderName == "")
                               on_actionOpen_Folder_triggered();
                           else
                           {
                               checkAndOpenFolder(selectedFolderName);
                           }

//                           ui->folderTreeView->onIndexClicked(QModelIndex()); //initial load

//                           qDebug()<<"contextSensitiveHelpOn"<<QSettings().value("contextSensitiveHelpOn");
//                           if (QSettings().value("contextSensitiveHelpOn").toString() == "" || QSettings().value("contextSensitiveHelpOn").toBool())
//                               ui->actionContext_Sensitive_Help->setChecked(true);

                           showUpgradePrompt();
                       });

    //tooltips after 10 seconds (to show hints first)
}

MainWindow::~MainWindow()
{

//    qDebug()<<"Destructor"<<geometry();

    if (geometry() != QSettings().value("Geometry").toRect())
    {
        qDebug()<<"MainWindow::~MainWindow"<<geometry();
        QSettings().setValue("Geometry", geometry());
//    QSettings().setValue("windowState", saveState());
        QSettings().sync();
    }

    delete ui;
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
   QMainWindow::resizeEvent(event);

//   qDebug()<<"MainWindow::resizeEvent"<<geometry()<<event<<QSettings().value("Geometry").toRect();
   if (geometry() != QSettings().value("Geometry").toRect())
   {
       if (ui->graphicsView->rootItem != nullptr)
           ui->graphicsView->arrangeItems(nullptr, __func__);

       QSettings().setValue("Geometry", geometry());
   //    QSettings().setValue("windowState", saveState());
       QSettings().sync();
   }
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

//    if (ui->progressBar->value() > 0 && ui->progressBar->value() < 100)
//    {
//        QMessageBox::StandardButton reply;
//        reply = QMessageBox::question(this, "Quit", tr("Export in progress, are you sure you want to quit?"), QMessageBox::Yes|QMessageBox::No);
//        if (reply == QMessageBox::Yes)
//            exitYes = true;
//    }
//    else
        exitYes = true;

    if (exitYes)
    {
        //check unsaved changes
        if (ui->graphicsView->undoIndex != ui->graphicsView->undoSavePoint)
        {
            QMessageBox::StandardButton reply;
            reply = QMessageBox::question(this, "Check changes", tr("There are %1 changes. Do you want to save these changes").arg(QString::number(qAbs(ui->graphicsView->undoIndex - ui->graphicsView->undoSavePoint))),
                                          QMessageBox::Yes|QMessageBox::No);

            if (reply == QMessageBox::Yes)
                on_actionSave_triggered();
        }
//        if (ui->clipsTableView->checkSaveIfClipsChanged())

//        ui->graphicsView->clearAll();

        foreach (AGProcessAndThread *process, processes)
        {
            if ((process->process != nullptr && process->process->state() != QProcess::NotRunning) || (process->jobThread != nullptr && process->jobThread->isRunning()))
            {
                qDebug()<<"MainWindow::checkExit Killing process"<<"Main"<<process->name<<process->process<<process->jobThread;
                process->kill();
            }
        }

//        //check gracefull completion
//        int nrOfActiveJobs = 1;
//        while (nrOfActiveJobs > 0)
//        {
//            nrOfActiveJobs = 0;
//            foreach (AGProcessAndThread *process, processes)
//            {
//                if ((process->process != nullptr && process->process->state() != QProcess::NotRunning) || (process->jobThread != nullptr && process->jobThread->isRunning()))
//                {
////                    qDebug()<<"Runnning"<<process->name;
//                     nrOfActiveJobs ++;
//                }
//            }
//        }
    }

//    qDebug()<<"checkexit done"<<exitYes;
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
    //added designer settings

    //video/media window

    //https://joekuan.files.wordpress.com/2015/09/screen3.png
    ui->actionSave->setIcon(style()->standardIcon(QStyle::SP_DriveFDIcon));
    ui->actionQuit->setIcon(style()->standardIcon(QStyle::SP_DialogCancelButton));
    ui->actionOpen_Folder->setIcon(style()->standardIcon(QStyle::SP_DirOpenIcon));
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
    ui->actionWhatIsNew->setIcon(QIcon(QPixmap::fromImage(QImage(":/MediaSidekick.ico"))));

    ui->mediaFileScaleSlider->setStyleSheet("QSlider::sub-page:Horizontal { background-color: #9F2425; }"
                  "QSlider::add-page:Horizontal { background-color: #333333; }"
                  "QSlider::groove:Horizontal { background: transparent; height:4px; }"
                  "QSlider::handle:Horizontal { width:10px; border-radius:5px; background:#9F2425; margin: -5px 0px -5px 0px; }");

    ui->clipScaleSlider->setStyleSheet("QSlider::sub-page:Horizontal { background-color: #249f55; }"
                  "QSlider::add-page:Horizontal { background-color: #333333; }"
                  "QSlider::groove:Horizontal { background: transparent; height:4px; }"
                  "QSlider::handle:Horizontal { width:10px; border-radius:5px; background:#249f55; margin: -5px 0px -5px 0px; }");

//https://forum.qt.io/topic/87256/how-to-set-qslider-handle-to-round/3
    ui->actionRefresh->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));//SP_DialogCancelButton

    ui->actionUndo->setIcon(QIcon(QPixmap::fromImage(QImage(":/images/undo.png"))));
    ui->actionRedo->setIcon(QIcon(QPixmap::fromImage(QImage(":/images/redo.png"))));

    ui->actionZoom_In->setIcon(QIcon(QPixmap::fromImage(QImage(":/images/zoomin.png"))));
    ui->actionZoom_Out->setIcon(QIcon(QPixmap::fromImage(QImage(":/images/zoomout.png"))));
}

void MainWindow::allConnects()
{
//    ui->graphicsView1->addNode("folder", 0, 0);
//    ui->graphicsView1->addNode("prop", 5, 0);
//    ui->graphicsView1->addNode("main", 8, 0);
//    ui->graphicsView1->addNode("tf1", 13, 0);

//    ui->graphicsView1->addNode("time", 2, 3);
//    ui->graphicsView1->addNode("video", 5, 3);
//    ui->graphicsView1->addNode("star", 11, 3);
//    ui->graphicsView1->addNode("tf2", 13, 3);

//    ui->graphicsView1->addNode("files", 0, 6);
//    ui->graphicsView1->addNode("clip", 8, 6);
//    ui->graphicsView1->addNode("tags", 13, 6);

//    ui->graphicsView2->addNode("main", 12, 3);

//    ui->graphicsView2->addNode("time", 6, 6);
//    ui->graphicsView2->addNode("video", 6, 0);

//    ui->graphicsView2->addNode("clip", 0, 3);

//    //do not show the graph tab (only in debug mode)
//    graphWidget1 = ui->clipsTabWidget->widget(1);
//    graphWidget2 = ui->clipsTabWidget->widget(2);
//    graphicsWidget = ui->clipsTabWidget->widget(3);

    connect(&m_network, SIGNAL(finished(QNetworkReply*)), SLOT(onUpgradeCheckFinished(QNetworkReply*)));

    connect(agFileSystem, &AGFileSystem::addItem, ui->graphicsView, &AGView::onAddItem, Qt::BlockingQueuedConnection); //to draw item on scene immediately
    connect(agFileSystem, &AGFileSystem::deleteItem, ui->graphicsView, &AGView::onDeleteItem);
    connect(agFileSystem, &AGFileSystem::fileChanged, ui->graphicsView, &AGView::onFileChanged);

    connect(ui->graphicsView, &AGView::fileWatch, agFileSystem, &AGFileSystem::onFileWatch);
    connect(ui->graphicsView, &AGView::showInStatusBar, this, &MainWindow::onShowInStatusBar);

} //allConnects

void MainWindow::loadSettings()
{
    QRect savedGeometry = QSettings().value("Geometry").toRect();
    if (savedGeometry != geometry())
    {
        if (savedGeometry.width() != 0)
            setGeometry(savedGeometry);
    }

//    restoreState(QSettings().value("windowState").toByteArray());

    if (QSettings().value("tooltipsOn").toString() == "")
        ui->actionTooltips->setChecked(true);
    else
        ui->actionTooltips->setChecked(QSettings().value("tooltipsOn").toBool());

    QString viewMode = QSettings().value("viewMode").toString(); //default
    bool syncNeeded = false;
    if (viewMode == "")
    {
        QSettings().setValue("viewMode", "SpotView");
        syncNeeded = true;
    }
    QString viewDirection = QSettings().value("viewDirection").toString();
    if (viewDirection == "")
    {
        QSettings().setValue("viewDirection", "Return");
        syncNeeded = true;
    }

    if (syncNeeded)
        QSettings().sync();

    ui->timelineViewButton->setEnabled(QSettings().value("viewMode") != "TimelineView");
    ui->spotviewDownButton->setEnabled((QSettings().value("viewMode") != "SpotView" || QSettings().value("viewDirection") != "Down"));
    ui->spotviewRightButton->setEnabled((QSettings().value("viewMode") != "SpotView" || QSettings().value("viewDirection") != "Right"));
    ui->spotviewReturnButton->setEnabled((QSettings().value("viewMode") != "SpotView" || QSettings().value("viewDirection") != "Return"));

    if (QSettings().value("orderBy").toString() == "")
    {
        QSettings().setValue("orderBy", "Date");
        QSettings().sync();
    }
    ui->graphicsView->setOrderBy(QSettings().value("orderBy").toString());

    ui->orderByDateButton->setEnabled(QSettings().value("orderBy").toString() != "Date");
    ui->orderByNameButton->setEnabled(QSettings().value("orderBy").toString() != "Name");

    QSettings().value("playerInDialogcheckBox");
    if (QSettings().value("playerInDialogcheckBox").toString() == "")
#ifdef Q_OS_WIN
        ui->playerInDialogcheckBox->setChecked(false);
#else
        ui->playerInDialogcheckBox->setChecked(true);
#endif
    else
        ui->playerInDialogcheckBox->setChecked(QSettings().value("playerInDialogcheckBox").toBool());
    ui->graphicsView->setPlayInDialog(ui->playerInDialogcheckBox->isChecked());

    ui->searchLineEdit->setText(QSettings().value("searchLineEdit").toString());
    on_searchLineEdit_textChanged(QSettings().value("searchLineEdit").toString());

    if (QSettings().value("mediaFileScaleSlider").toInt() == 0)
        QSettings().setValue("mediaFileScaleSlider", ui->mediaFileScaleSlider->value()); //default

    if (QSettings().value("mediaFileScaleSlider").toInt() != ui->mediaFileScaleSlider->value())
        ui->mediaFileScaleSlider->setValue(QSettings().value("mediaFileScaleSlider").toInt());
    else
        on_mediaFileScaleSlider_valueChanged(QSettings().value("mediaFileScaleSlider").toInt()); //force update of label anyway

    if (QSettings().value("clipScaleSlider").toInt() == 0)
        QSettings().setValue("clipScaleSlider", ui->clipScaleSlider->value()); //default

    if (QSettings().value("clipScaleSlider").toInt() != ui->clipScaleSlider->value())
        ui->clipScaleSlider->setValue(QSettings().value("clipScaleSlider").toInt());
    else
        on_clipScaleSlider_valueChanged(QSettings().value("clipScaleSlider").toInt()); //force update of label anyway

} //loadSettings

void MainWindow::allTooltips()
{
    QString commandControl = "Ctrl-";
#ifdef Q_OS_MAC
    commandControl = "⌘-";
#endif
    //folder and file
    ui->mainToolBar->setToolTip(tr("<p><b>Folder</b></p>"
                                      "<p><i>Select a folder containing video files</i></p>"
                                      "<ul>"
                                      "<li>Select folder: After selecting a folder, the files tab is shown and clips, tags and properties of all files are loaded</li>"
                                      "<li>Warning: For movie files with more than 100 clips and if more than 100 files with clips are loaded, a warning is given with the option to skip or cancel.</li>"
                                      "</ul>"));

//    ui->videoFilesTreeView->setToolTip(tr("<p><b>File</b></p>"
//                                     "<p><i>Files within the selected folder</i></p>"
//                                     "<ul>"
//                                     "<li><b>Click on file</b>: Show the clips of this file on the timeline and in the clips tab</li>"
//                                     "</ul>"
//                                          ));

    //video
//    ui->videoWidget->setToolTip(tr("<p><b>Media window</b></p>"
//                                   "<p><i>Video and audio is played here</i></p>"
//                                   "<ul>"
//                                   "<li>Player controls...</li>"
//                                   "<li>Add clips...</li>"
//                                   "</ul>"
//                                ));

//    ui->playButton->setToolTip(tr("<p><b>Play or pause</b></p>"
//                              "<p><i>Play or pause the video</i></p>"
//                              "<ul>"
//                              "<li>Shortcut: Space</li>"
//                              "</ul>"));
//    ui->stopButton->setToolTip(tr("<p><b>Stop the video</b></p>"
//                                "<p><i>Stop the video</i></p>"
//                                ));

//    ui->skipForwardButton->setToolTip(tr("<p><b>Go to next or previous clip</b></p>"
//                              "<p><i>Go to next or previous clip</i></p>"
//                              "<ul>"
//                              "<li>Shortcut: %1up and %1down</li>"
//                              "</ul>").arg(commandControl));
//    ui->skipBackwardButton->setToolTip(ui->skipForwardButton->toolTip());

//    ui->seekBackwardButton->setToolTip(tr("<p><b>Go to next or previous frame</b></p>"
//                              "<p><i>Go to next or previous frame</i></p>"
//                              "<ul>"
//                              "<li>Shortcut: %1left and %1right</li>"
//                              "</ul>").arg(commandControl));
//    ui->seekForwardButton->setToolTip(ui->seekBackwardButton->toolTip());

//    ui->setInButton->setToolTip(tr("<p><b>Set in- and out- point</b></p>"
//                                  "<p><i>Set the inpoint of a new clip or change the in- or outpoint of the current clip to the current position on the timeline</i></p>"
//                                  "<ul>"
//                                  "<li>Change: inpoint before outpoint of last selected clip or outpoint after inpoint of last selected clip</li>"
////                                  "<li>Set: No last selected clip or inpoint after outpoint of last selected clip and outpoint before inpoint of last selected clip</li>"
//                                  "<li>Shortcut: %1i and %1o</li>"
//                                  "</ul>").arg(commandControl));
//    ui->setOutButton->setToolTip(ui->setInButton->toolTip());

//    ui->muteButton->setToolTip(tr("<p><b>Mute or unmute</b></p>"
//                              "<p><i>Mute or unmute sound (toggle)</i></p>"
//                              "<ul>"
//                              "<li>Video files are muted if selected. Audio files are unmuted if selected</li>"
//                              "<li>Shortcut mute toggle: %1m</li>"
//                              "</ul>").arg(commandControl));
//    ui->speedComboBox->setToolTip(tr("<p><b>Speed</b></p>"
//                                 "<p><i>Change the play speed of the video</i></p>"
//                                 "<ul>"
//                                 "<li>Supported speeds are depending on the media file codec installed on your computer</li>"
//                                 "</ul>"));

    //tt clip filters

    foreach (QAction *toolBarAction, ui->mainToolBar->actions())
    {
        if (toolBarAction->text() == "A&like")
            toolBarAction->setToolTip(tr("<p><b>Alike</b></p>"
                                                                              "<p><i>Check if other clips are 'alike' this clip. This is typically the case with Action-cam footage: multiple shots which are similar.</i></p>"
                                                                              "<p><i>This is used to exclude similar clips from the exported video.</i></p>"
                                                                              "<ul>"
                                                                              "<li>Alike filter checkbox: Show only alike clips</li>"
                                                                              "<li>Alike column: Set if this clip is like another clip <%1 A></li>"
                                                                              "<li>Timeline: Only clips which meet the filter criteria are shown in the timeline</li>"
                                                                              "<li>Hint: Give the best of the alikes a higher rating than the others</li>"
                                                                              "<li>Hint: Give alike clips the same tags to filter on them later</li>"
                                                                              "</ul>"
                                                                           ).arg(commandControl));
        else if (toolBarAction->toolTip().contains("star"))
                 toolBarAction->setToolTip(tr("<p><b>Ratings</b></p>"
                                                                                            "<p><i>Give a rating to a clip (0 to 5 stars)</i></p>"
                                                                                            "<ul>"
                                                                                              "<li>Rating filter: Select 0 to 5 stars. All clips with same or higher rating are shown</li>"
                                                                                              "<li>Rating column: Double click to change the rating (or %1 0 to %1 5 to rate the current clip)</li>"
                                                                                              "<li>Timeline: Only clips which meet the filter criteria are shown in the timeline</li>"
                                                                                            "</ul>").arg(commandControl));
    }

//    ui->tagFilter1ListView->setToolTip(tr("<p><b>Tag filters</b></p>"
//                                          "<p><i>Define which clips are shown based on their tags</i></p>"
//                                          "<ul>"
//                                          "<li>Tag fields: The following logical condition applies: (Left1 or left2 or left3 ...) and (right1 or right2 or right3 ...)</li>"
//                                          "<li>Timeline: Only clips which meet the filter criteria are shown in the timeline</li>"
//                                          "<li>Double click: Remove a tag</li>"
//                                          "</ul>"));

//    ui->tagFilter2ListView->setToolTip(ui->tagFilter1ListView->toolTip());

//    ui->fileOnlyCheckBox->setToolTip(tr("<p><b>File only</b></p>"
//                                        "<p><i>Show only clips of the selected file</i></p>"
//                                        "<ul>"
//                                        "<li><b>Timeline</b>: If file only is checked, the timeline only shows clips of the selected file</li>"
//                                        "</ul>"));

    //tt clips table
//    ui->resetSortButton->setToolTip(tr("<p><b>Reset sort</b></p>"
//                                       "<p><i>Set the order of clips back to file order</i></p>"
//                                       ));

//    ui->clipsTableView->setToolTip(tr("<p><b>Clips list</b></p>"
//                                     "<p><i>Show the clips for the files in the selected folder</i></p>"
//                                     "<ul>"
//                                     "<li>Only clips applying to the <b>filters</b> above this table are shown</li>"
//                                     "<li>Move over the column headers to see <b>column tooltips</b></li>"
//                                     "<li>Clips belonging to the <b>selected file</b> are highlighted gray</li>"
//                                     "<li>The clip <b>currently shown</b> in the video window is highlighted blue</li>"
//                                     "<li>To <b>delete</b> a clip: right mouse click</li>"
//                                     "<li>To change the <b>order</b> of clips: drag the number on the left of this table up or down</li>"
//                                      "<li>Note: Clips are saved on your filesystems as <b>.srt files</b>. They have the same name as the media file for which the edits are made.</li>"
//                                     "</ul>"
//                                     ));

    //tt tags
//    ui->newTagLineEdit->setToolTip(tr("<p><b>Tag field</b></p>"
//                                      "<p><i>Create and remove tags</i></p>"
//                                      "<ul>"
//                                      "<li>Add new tag to tag list: fill in tag name and press return</li>"
//                                      "</ul>"
//                                      ));
//    ui->tagsListView->setToolTip(tr("<p><b>Tag list</b></p>"
//                                    "<p><i>List of tags used in clips or filter</i></p>"
//                                    "<ul>"
//                                    "<li>Add tag to clip: Drag and drop to tag column of clips</li>"
//                                    "<li>Add tag to filter: Drag and drop to filters</li>"
//                                    "<li>Delete tags: double click (or drag tag and drop in the tag field)</li>"
//                                    "</ul>"
//                                    ));

    ui->spotviewDownButton->setToolTip(tr("<p><b>Spot view</b></p>"
                                      "<p><i>Arranges clips below their mediafiles. Best for spotting of clips in mediafiles</i></p>"
                                      "<ul>"
                                          "<li><b>Down ↓</b>: Media files arranged vertically</li>"
                                          "<li><b>Right →</b>: Media files arranged horizontally</li>"
                                          "<li><b>Return ↵</b>: Media files arranged in rows</li>"
                                          "<li><b>Media file duration line</b>: In spot view mode the size of the media file item is aligned with the size of the corresponding clips (with a minimum)</li>"
                                      "</ul>"));
    ui->spotviewRightButton->setToolTip(ui->spotviewDownButton->toolTip());
    ui->spotviewReturnButton->setToolTip(ui->spotviewDownButton->toolTip());

    ui->timelineViewButton->setToolTip(tr("<p><b>Timeline view</b></p>"
                                      "<p><i>Arranges clips next to eachother as they will appear in exports. Best for preparing export</i></p>"
                                      "<ul>"
                                      "<li><b>Transitions</b>: Clips will overlap if there is a transition time (currently defined in classical view)</li>"
                                      "<li><b>Media file duration line</b>: In timeline mode the size of the media file item is the same as the duration line (with a minimum)</li>"
                                      "</ul>"));

    ui->orderByDateButton->setToolTip(tr("<p><b>Order mediaitems</b></p>"
                                         "<p><i>Orders media items based on their createdate (if present) or filename</i></p>"
                                         "<ul>"
                                             "<li><b>Date ↓</b>: Order by createdate. If no createdate, order by name. Files without createdate are shown first</li>"
                                             "<li><b>Name ↓</b>: Order by file name</li>"
                                         "</ul>"));
    ui->orderByNameButton->setToolTip(ui->orderByDateButton->toolTip());

    ui->searchLineEdit->setToolTip(tr("<p><b>Search</b></p>"
                                      "<p><i>Shows only items which meet the search criteria</i></p>"
                                      "<ul>"
                                      "<li><b>Matching</b>: match on the tags of clips and on filename</li>"
                                      "</ul>"));

    ui->actionReload->setToolTip(tr("<p><b>(Re)load media</b></p>"
                                      "<p><i>Clears the screen and (re)loads all videos, audios and images from the selected folder %1</i></p>"
                                      "<ul>"
                                        "<li><b>Cancel</b>: During loading, the load process can be cancelled</li>"
                                        "<li><b>File changes</b>: file changes are notified by Media Sidekick and updated in the view directly, no reload needed.</li>"
                                      "</ul>").arg(QSettings().value("selectedFolderName").toString()));

    ui->actionRefresh->setToolTip(tr("<p><b>Refresh screen</b></p>"
                                      "<p><i>Redraw all the items of selected folder %1 on the screen</i></p>"
                                      "<ul>"
                                        "<li>Refresh the screen if something is not shown correctly</li>"
                                      "</ul>").arg(QSettings().value("selectedFolderName").toString()));

    ui->playerInDialogcheckBox->setToolTip(tr("<p><b>Show video in window</b></p>"
                                              "<p><i>Video can be played in a separate window or at the place where it is located on the screen</i></p>"
                                              "<ul>"
                                                 "<li><b>Performace on Mac / OSX</b>: Currently performance of video windows in a graphical view on Mac / OSX is very bad (very high CPU load)</li>"
                                              "</ul>"));
    ui->mediaFileScaleSlider->setToolTip(tr("<p><b>Media file scale</b></p>"
                                   "<p><i>Sets the size of the video and audio files based on their duration (pixels per minute)</i></p>"
                                   ));
    ui->clipScaleSlider->setToolTip(tr("<p><b>Clip and export scale</b></p>"
                                   "<p><i>Sets the size of the clips and exported files based on their duration (pixels per minute)</i></p>"
                                   ));

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
    QColor menuColor = QColor(80,80,80);

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

    qApp->setPalette(palette);

//    qDebug()<<"MainWindow::on_actionBlack_theme_triggered set palette"<<palette;

    qApp->setStyleSheet("QToolTip { color: #ffffff; background-color: #2a82da; border: 1px solid white; }");

    QSettings().setValue("theme", "Black");
    QSettings().sync();

    ui->graphicsView->setThemeColors(Qt::white);
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
    QColor menuColor = mainColor;

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

    QSettings().setValue("theme", "White");
    QSettings().sync();

    ui->graphicsView->setThemeColors(Qt::black);
}

void MainWindow::on_actionAbout_triggered()
{
    QMessageBox::about(this, tr("About Media Sidekick"),
            tr("<p><b>Media Sidekick</b> is a free desktop application aimed at minimizing the time between shooting and publishing videos. Media Sidekick prepares raw video for efficient editing in your preferred video editor. Media Sidekick is tuned for action camera footage having a lot of raw video which needs to compressed to a small video showing the best of it.</p>"
               "<p>Version  %1</p>"
               "<p>Licensed under the: <a href=\"https://www.gnu.org/licenses/lgpl-3.0.html\">GNU Lesser General Public License v3.0</a></p>"
               "<p>Copyright © 2019-2020</p>"
               "<p><a href=\"https://mediasidekick.org\">Media Sidekick Website</a></p>"
               "<p>This program proudly uses the following projects:</p>"
               "<ul>"
               "<li><a href=\"https://www.qt.io/\">Qt</a> application and UI framework</li>"
//               "<li><a href=\"https://www.shotcut.org/\">Shotcut</a> Open source video editor (timeline and version check)</li>"
               "<li><a href=\"https://www.ffmpeg.org/\">FFmpeg</a> multimedia format and codec libraries (lossless and encoded previews)</li>"
               "<li><a href=\"https://exiftool.org/\">Exiftool</a> Read, Write and Edit Meta Information (Properties)</li>"
               "<li><a href=\"https://github.com/banelle/derperview\">Derperview by Banelle</a> Perform non-linear stretch of 4:3 video to make it 16:9. See also <a href=\"https://intofpv.com/t-derperview-a-command-line-superview-alternative\">Derperview - A Command Line Superview Alternative</a></li>"
               "<li><a href=\"https://github.com/ytdl-org/youtube-dl\">Youtube-dl</a>  download videos from youtube.com or other video platforms</li>"
               "</ul>"
               "<p>This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.</p>"
               "<p>As Media Sidekick may contain bugs, BACKUP your video files before editing!</p>"
               "<p>Media Sidekick issuetracker on GitHub <a href=\"https://github.com/ewoudwijma/MediaSidekick/issues\">GitHub Media Sidekick issues</a></p>"
               "<p>Media Sidekick is created by <a href=\"https://www.linkedin.com/in/ewoudwijma\">Ewoud Wijma</a>.</p>"
               ).arg(qApp->applicationVersion()));
}

void MainWindow::on_actionAbout_Qt_triggered()
{
    qApp->aboutQt();
}

void MainWindow::on_action5_stars_triggered()
{
    ui->graphicsView->processAction("Key_*****");
}

void MainWindow::on_action4_stars_triggered()
{
    ui->graphicsView->processAction("Key_****");
}

void MainWindow::on_action1_star_triggered()
{
    ui->graphicsView->processAction("Key_*");
}

void MainWindow::on_action2_stars_triggered()
{
    ui->graphicsView->processAction("Key_**");
}

void MainWindow::on_action3_stars_triggered()
{
    ui->graphicsView->processAction("Key_***");
}

void MainWindow::on_action0_stars_triggered()
{
    ui->graphicsView->processAction("Key_*0");
}

void MainWindow::on_actionAlike_triggered()
{
    ui->graphicsView->processAction("Key_✔");
}

void MainWindow::on_actionSave_triggered()
{
    ui->statusBar->showMessage(tr("%1 changes will be saved").arg(QString::number(qAbs(ui->graphicsView->undoIndex - ui->graphicsView->undoSavePoint))), 5000);

    ui->graphicsView->saveModels();

//    qDebug()<<"MainWindow::on_actionSave_triggered done";
}

void MainWindow::on_actionPlay_Pause_triggered()
{
    ui->graphicsView->processAction("actionPlay_Pause");
}

void MainWindow::on_actionIn_triggered()
{
//    qDebug()<<"MainWindow::on_actionIn_triggered"<<ui->clipsTableView->highLightedRow<<ui->videoWidget->m_position;
//    ui->videoWidget->onSetIn();

    ui->graphicsView->processAction("actionIn");
}

void MainWindow::on_actionOut_triggered()
{
//    qDebug()<<"MainWindow::on_actionOut_triggered"<<ui->clipsTableView->highLightedRow<<ui->videoWidget->m_position;
//    ui->videoWidget->onSetOut();
    ui->graphicsView->processAction("actionOut");
}

void MainWindow::on_actionPrevious_frame_triggered()
{
    ui->graphicsView->processAction("actionPrevious_frame");
//    ui->videoWidget->rewind();
}

void MainWindow::on_actionNext_frame_triggered()
{
    ui->graphicsView->processAction("actionNext_frame");
//    ui->videoWidget->fastForward();
}

void MainWindow::on_actionPrevious_in_out_triggered()
{
    ui->graphicsView->processAction("actionPrevious_in_out");
//     ui->videoWidget->skipPrevious();
}

void MainWindow::on_actionNext_in_out_triggered()
{
    ui->graphicsView->processAction("actionNext_in_out");
//    ui->videoWidget->skipNext();
}

void MainWindow::on_actionExport_triggered()
{
//    ui->exportButton->click();
    ui->graphicsView->processAction(__func__);
}

void MainWindow::showUpgradePrompt()
{
    QString currentPath = QDir::currentPath();

    if (!currentPath.contains("Debug", Qt::CaseInsensitive) && !currentPath.contains("Release", Qt::CaseInsensitive))
    {
        //    ui->statusBar->showMessage("Version check...", 15000);
            QNetworkRequest request(QUrl("http://www.mediasidekick.org/version.json"));
            QSslConfiguration sslConfig = request.sslConfiguration();
            sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
            request.setSslConfiguration(sslConfig);
            m_network.get(request);
    }
//    else
//        qDebug()<<__func__<<"not checking latest version because in " + currentPath;
}

void MainWindow::onUpgradeCheckFinished(QNetworkReply* reply)
{
//    qDebug()<<"MainWindow::onUpgradeCheckFinished";
    QString m_upgradeUrl = "https://mediasidekick.org/download/";

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
//            QNetworkRequest request(QUrl("http://www.mediasidekick.org/version.json"));
            m_network.get(request);
            return;
        }

        QJsonParseError *error = new QJsonParseError();
        QJsonDocument json = QJsonDocument::fromJson(response, error);
        QString current = qApp->applicationVersion();

        if (!json.isNull() && json.object().value("version_string").type() == QJsonValue::String)
        {
            QString latest = json.object().value("version_string").toString();
            latestVersionURL = json.object().value("url").toString();
            if (current != "adhoc" && QVersionNumber::fromString(current) < QVersionNumber::fromString(latest))
            {
                if (!json.object().value("url").isUndefined())
                    m_upgradeUrl = json.object().value("url").toString();

                QMessageBox::StandardButton reply;
                reply = QMessageBox::question(this, "Version check", tr("Media Sidekick version %1 is available! Click Yes to get it.").arg(latest), QMessageBox::Yes|QMessageBox::No);
                if (reply == QMessageBox::Yes)
                    QDesktopServices::openUrl(QUrl(m_upgradeUrl));

            } else {
                ui->statusBar->showMessage(tr("Version check: You are running the latest version of Media Sidekick."), 15000);
            }
            reply->deleteLater();
            return;
        }
//        else
//        {
//            ui->statusBar->showMessage("Version check: failed to parse version.json "+ error->errorString());
//        }
    } else {
//        ui->statusBar->showMessage("Network error " + reply->errorString());
        qDebug()<<"MainWindow::onUpgradeCheckFinished"<<"Network error " + reply->errorString();
    }
//    QMessageBox::StandardButton mreply;
//    mreply = QMessageBox::question(this, "Version check", tr("Failed to read version.json when checking. Click here to go to the Web site."), QMessageBox::Yes|QMessageBox::No);
//    if (mreply == QMessageBox::Yes)
//        QDesktopServices::openUrl(QUrl(m_upgradeUrl));
    reply->deleteLater();
}

void MainWindow::on_actionDonate_triggered()
{
    QDesktopServices::openUrl(QUrl("https://www.mediasidekick.org/support"));
}

void MainWindow::on_actionCheck_for_updates_triggered()
{
    showUpgradePrompt();
}

void MainWindow::on_actionWhatIsNew_triggered()
{
    QDesktopServices::openUrl(QUrl(latestVersionURL == ""?"https://mediasidekick.org":latestVersionURL));

//    QMessageBox::about(this, tr("About Media Sidekick"),
//            tr("<p><h1>Media Sidekick process flow</b></p>"
//               "<p>Version  %1</p>"
//               "<ul>"
//               "<li>Select a folder: %2</li>"
//               "<li>Create clips: %3 clips created</li>"
//               "</ul>"
//               ).arg(qApp->applicationVersion(), ui->folderLabel->text(), QString::number(ui->clipsTableView->clipsItemModel->rowCount())));

}

void MainWindow::on_actionGithub_MSK_Issues_triggered()
{
    QDesktopServices::openUrl(QUrl("https://github.com/ewoudwijma/MediaSidekick/issues"));
}

void MainWindow::on_actionMute_triggered()
{
//    ui->videoWidget->onMute();
    ui->graphicsView->processAction("actionMute");
}

void MainWindow::on_actionSpeed_Up_triggered()
{
    ui->graphicsView->processAction("actionSpeed_Up");
}

void MainWindow::on_actionSpeed_Down_triggered()
{
    ui->graphicsView->processAction("actionSpeed_Down");
}

void MainWindow::on_actionVolume_Up_triggered()
{
    ui->graphicsView->processAction("actionVolume_Up");
}

void MainWindow::on_actionVolume_Down_triggered()
{
    ui->graphicsView->processAction("actionVolume_Down");
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

void MainWindow::onShowInStatusBar(QString message, int timeout)
{
//    qDebug()<<__func__<<message<<timeout;
    ui->statusBar->showMessage(message, timeout);
}

int MainWindow::countFolders(QString folderName, int depth)
{
    int count = 0;

    QDir dir(folderName);
    dir.setFilter(QDir::AllDirs| QDir::NoDotAndDotDot | QDir::NoSymLinks);
//    qDebug()<<"countFolders"<<folderName<<depth<<dir.entryList().count();
    for (int i=0; i < dir.entryList().size(); i++)
    {
        if (dir.entryList().at(i) != "MSKRecycleBin")
        {
            count++;
//            qDebug()<<"countFolders"<<dir.absolutePath() + "/" + dir.entryList().at(i);
            count += countFolders(dir.absolutePath() + "/" + dir.entryList().at(i), depth + 1);
        }
    }
    return count;
}

void MainWindow::checkAndOpenFolder(QString selectedFolderName)
{
    int count = countFolders(selectedFolderName);
//    qDebug()<<"MainWindow::checkAndOpenFolder"<<selectedFolderName<<count;

    if (count > 10)
    {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Open Folder", tr("Folder %1 contains %2 sub folders, loading can take a while. Do you still want to continue?").arg(selectedFolderName, QString::number(count)), QMessageBox::Yes|QMessageBox::No);
        if (reply != QMessageBox::Yes)
        {
            selectedFolderName = "";
            QSettings().setValue("selectedFolderName", selectedFolderName);
            QSettings().sync();
        }
    }

    if (selectedFolderName != "")
    {
        //check unsaved changes

        on_actionReload_triggered();
    }
}

void MainWindow::on_actionOpen_Folder_triggered()
{
    QFileDialog dialog(this);

    dialog.setFileMode(QFileDialog::Directory);

    QString selectedFolderName = dialog.getExistingDirectory() + "/";
//    QString selectedFolderName = dialog.getExistingDirectory(this, "ewoud", "", QFileDialog::DontUseNativeDialog) + "/";

    if (selectedFolderName != "/")
    {
        QSettings().setValue("selectedFolderName", selectedFolderName);
        QSettings().sync();

        checkAndOpenFolder(selectedFolderName);
    }
}

void MainWindow::on_spotviewDownButton_clicked()
{
    ui->graphicsView->horizontalScrollBar()->setValue( 0 );
    ui->graphicsView->verticalScrollBar()->setValue( 0 );
    ui->graphicsView->scale(1,1);

    QSettings().setValue("viewMode", "SpotView");
    QSettings().setValue("viewDirection", "Down");
    QSettings().sync();

    ui->timelineViewButton->setEnabled(QSettings().value("viewMode") != "TimelineView");
    ui->spotviewDownButton->setEnabled((QSettings().value("viewMode") != "SpotView" || QSettings().value("viewDirection") != "Down"));
    ui->spotviewRightButton->setEnabled((QSettings().value("viewMode") != "SpotView" || QSettings().value("viewDirection") != "Right"));
    ui->spotviewReturnButton->setEnabled((QSettings().value("viewMode") != "SpotView" || QSettings().value("viewDirection") != "Return"));

    ui->graphicsView->onSetView();
}

void MainWindow::on_spotviewRightButton_clicked()
{
    ui->graphicsView->horizontalScrollBar()->setValue( 0 );
    ui->graphicsView->verticalScrollBar()->setValue( 0 );
    ui->graphicsView->scale(1,1);

    QSettings().setValue("viewMode", "SpotView");
    QSettings().setValue("viewDirection", "Right");
    QSettings().sync();

    ui->timelineViewButton->setEnabled(QSettings().value("viewMode") != "TimelineView");
    ui->spotviewDownButton->setEnabled((QSettings().value("viewMode") != "SpotView" || QSettings().value("viewDirection") != "Down"));
    ui->spotviewRightButton->setEnabled((QSettings().value("viewMode") != "SpotView" || QSettings().value("viewDirection") != "Right"));
    ui->spotviewReturnButton->setEnabled((QSettings().value("viewMode") != "SpotView" || QSettings().value("viewDirection") != "Return"));

    ui->graphicsView->onSetView();
}

void MainWindow::on_spotviewReturnButton_clicked()
{
    ui->graphicsView->horizontalScrollBar()->setValue( 0 );
    ui->graphicsView->verticalScrollBar()->setValue( 0 );
    ui->graphicsView->scale(1,1);

    QSettings().setValue("viewMode", "SpotView");
    QSettings().setValue("viewDirection", "Return");
    QSettings().sync();

    ui->timelineViewButton->setEnabled(QSettings().value("viewMode") != "TimelineView");
    ui->spotviewDownButton->setEnabled((QSettings().value("viewMode") != "SpotView" || QSettings().value("viewDirection") != "Down"));
    ui->spotviewRightButton->setEnabled((QSettings().value("viewMode") != "SpotView" || QSettings().value("viewDirection") != "Right"));
    ui->spotviewReturnButton->setEnabled((QSettings().value("viewMode") != "SpotView" || QSettings().value("viewDirection") != "Return"));

    ui->graphicsView->onSetView();
}

void MainWindow::on_timelineViewButton_clicked()
{
    ui->graphicsView->horizontalScrollBar()->setValue( 0 );
    ui->graphicsView->verticalScrollBar()->setValue( 0 );
    ui->graphicsView->scale(1,1);

    QSettings().setValue("viewMode", "TimelineView");
    QSettings().sync();

    ui->timelineViewButton->setEnabled(QSettings().value("viewMode") != "TimelineView");
    ui->spotviewDownButton->setEnabled((QSettings().value("viewMode") != "SpotView" || QSettings().value("viewDirection") != "Down"));
    ui->spotviewRightButton->setEnabled((QSettings().value("viewMode") != "SpotView" || QSettings().value("viewDirection") != "Right"));
    ui->spotviewReturnButton->setEnabled((QSettings().value("viewMode") != "SpotView" || QSettings().value("viewDirection") != "Return"));

    ui->graphicsView->onSetView();
}

void MainWindow::on_searchLineEdit_textChanged(const QString &arg1)
{
//    qDebug()<<"MainWindow::on_searchLineEdit_textChanged"<<arg1;
    ui->graphicsView->onSearchTextChanged(arg1);

    if (QSettings().value("searchLineEdit") != arg1)
    {
        QSettings().setValue("searchLineEdit", arg1);
        QSettings().sync();
    }
}

void MainWindow::on_actionReload_triggered()
{
    if (ui->actionReload->text() == "Cancel")
    {
        foreach (AGProcessAndThread *process, processes)
        {
            if ((process->process != nullptr && process->process->state() != QProcess::NotRunning) || (process->jobThread != nullptr && process->jobThread->isRunning()))
            {
//                qDebug()<<"MainWindow::on_actionReload_triggered Killing process"<<"Main"<<process->name<<process->process<<process->jobThread;
                process->kill();
            }
        }

        ui->graphicsView->isLoading = false;

        QPixmap base = QPixmap::fromImage(QImage(":/images/Folder.png")).scaledToWidth(56);
        QPixmap overlay = qApp->style()->standardIcon(QStyle::SP_BrowserReload).pixmap(32);
        QPixmap result(base.width(), base.height());
        result.fill(Qt::transparent); // force alpha channel
        QPainter painter(&result);
        painter.drawPixmap(0, 0, base);
        painter.drawPixmap((base.width() - overlay.width()) / 2, (base.height() - overlay.height()) / 2, overlay);
        ui->actionReload->setIcon(QIcon(result));

        ui->actionReload->setText("Reload");

        return;
    }

    //check unsaved changes
    if (ui->graphicsView->undoIndex != ui->graphicsView->undoSavePoint)
    {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Check changes", tr("There are %1 changes. Do you want to save these changes").arg(QString::number(qAbs(ui->graphicsView->undoIndex - ui->graphicsView->undoSavePoint))),
                                      QMessageBox::Yes|QMessageBox::No);

        if (reply == QMessageBox::Yes)
        {
            on_actionSave_triggered();
            //    wait otherwise crash (AGFileSystem does not find all the files for some reason
            QTime dieTime= QTime::currentTime().addSecs(1);
            while (QTime::currentTime() < dieTime)
                QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        }
    }

    ui->graphicsView->clearAll();

    if (agFileSystem->fileSystemWatcher->files().count() > 0)
        agFileSystem->fileSystemWatcher->removePaths(agFileSystem->fileSystemWatcher->files());
    if (agFileSystem->fileSystemWatcher->directories().count() > 0)
        agFileSystem->fileSystemWatcher->removePaths(agFileSystem->fileSystemWatcher->directories());

//    ui->timelineViewButton->setEnabled(false);
//    ui->spotviewDownButton->setEnabled(false);
//    ui->spotviewRightButton->setEnabled(false);

    AGProcessAndThread *process = new AGProcessAndThread(this);
    process->command("Load items", [=]()
    {
//        qDebug()<<"thread start"<<process->name<<qApp->thread()<<this->thread()<<QThread::currentThread();

//        qDebug()<<"MainWindow::on_actionReload_triggered"<<thread()<<process->thread();

        connect(process, &AGProcessAndThread::stopThreadProcess, agFileSystem, &AGFileSystem::onStopThreadProcess);

        agFileSystem->processStopped = false;

        agFileSystem->loadFilesAndFolders(QDir(QSettings().value("selectedFolderName").toString()), process);//2019-09-02 Fipre
    });
    processes<<process;
    connect(process, &AGProcessAndThread::processOutput, [=] (QTime time, QTime totalTime, QString event, QString outputString)
    {
//        qDebug()<<"currentThread during"<<qApp->thread()<<this->thread()<<QThread::currentThread()<<outputString;

        //this is executed in the created thread!

        if (event == "finished")
        {
//            qDebug()<<"LoadItems finished";

            if (process->errorMessage == "")
            {
            }
            else
                QMessageBox::information(this, "Error " + process->name, process->errorMessage);

            if (process->processStopped)
            {
                QString output = "on_actionReload_triggered finished and processStopped";
                process->addProcessLog("output", output);
//                qDebug()<<output<<process->processStopped;
                return;
            }

//            ui->graphicsView->arrangeItems(nullptr); //causes crash
            AGFolderRectItem *folderItem = (AGFolderRectItem *)ui->graphicsView->rootItem;
            folderItem->processes<<processes;

            QPixmap base = QPixmap::fromImage(QImage(":/images/Folder.png")).scaledToWidth(56);
            QPixmap overlay = qApp->style()->standardIcon(QStyle::SP_BrowserReload).pixmap(32);
            QPixmap result(base.width(), base.height());
            result.fill(Qt::transparent); // force alpha channel
            QPainter painter(&result);
            painter.drawPixmap(0, 0, base);
            painter.drawPixmap((base.width() - overlay.width()) / 2, (base.height() - overlay.height()) / 2, overlay);
            ui->actionReload->setIcon(QIcon(result));

            ui->actionReload->setText("Reload");
            ui->graphicsView->isLoading = false;

            QTimer::singleShot(0, this, [=]()->void
            {
//                qDebug()<<"on_actionReload_triggered"<<"Done";
                                   //tags and mainwindow done by clips
                ui->graphicsView->arrangeItems(nullptr, "Done");
            });
        }
    });

    process->start();

    ui->graphicsView->isLoading = true;
    ui->actionReload->setIcon(qApp->style()->standardIcon(QStyle::SP_BrowserStop));
    ui->actionReload->setText("Cancel");
}

void MainWindow::on_mediaFileScaleSlider_valueChanged(int value)
{
    ui->mediaFileScaleLabel->setText(QString::number(value) + " pix/min");
//    qDebug()<<"MainWindow::on_mediaFileScaleSlider_valueChanged"<<value;
    ui->graphicsView->setMediaScaleAndArrange(value); //0-1000 pixels / minute

    if (QSettings().value("mediaFileScaleSlider").toInt() != value)
    {
        QSettings().setValue("mediaFileScaleSlider", value);
        QSettings().sync();
    }
}

void MainWindow::on_playerInDialogcheckBox_clicked(bool checked)
{
    QSettings().setValue("playerInDialogcheckBox", checked);
    QSettings().sync();
    ui->graphicsView->setPlayInDialog(checked);
}

void MainWindow::on_clipScaleSlider_valueChanged(int value)
{
    ui->clipScaleLabel->setText(QString::number(value) + " pix/min");
//    qDebug()<<"MainWindow::on_clipScaleSlider_valueChanged"<<value;
    ui->graphicsView->setClipScaleAndArrange(value); //0-5000 pixels / minute

    if (QSettings().value("clipScaleSlider").toInt() != value)
    {
        QSettings().setValue("clipScaleSlider", value);
        QSettings().sync();
    }
}

void MainWindow::on_orderByDateButton_clicked()
{
    QString orderBy = "Date";
    if (QSettings().value("orderBy") != orderBy)
    {
        QSettings().setValue("orderBy", orderBy);
        QSettings().sync();
        ui->graphicsView->setOrderBy(orderBy);
        ui->orderByDateButton->setEnabled(QSettings().value("orderBy").toString() != "Date");
        ui->orderByNameButton->setEnabled(QSettings().value("orderBy").toString() != "Name");
    }
}

void MainWindow::on_orderByNameButton_clicked()
{
    QString orderBy = "Name";
    if (QSettings().value("orderBy") != orderBy)
    {
        QSettings().setValue("orderBy", orderBy);
        QSettings().sync();
        ui->graphicsView->setOrderBy(orderBy);
        ui->orderByDateButton->setEnabled(QSettings().value("orderBy").toString() != "Date");
        ui->orderByNameButton->setEnabled(QSettings().value("orderBy").toString() != "Name");
    }
}

void MainWindow::on_actionRefresh_triggered()
{
    ui->graphicsView->arrangeItems(nullptr, __func__);
}

void MainWindow::on_actionUndo_triggered()
{
    ui->graphicsView->undoOrRedo("Undo");
}

void MainWindow::on_actionRedo_triggered()
{
    ui->graphicsView->undoOrRedo("Redo");
}


void MainWindow::on_actionZoom_In_triggered()
{
    ui->graphicsView->processAction("actionZoom_In");
}

void MainWindow::on_actionZoom_Out_triggered()
{
    ui->graphicsView->processAction("actionZoom_Out");
}

void MainWindow::on_actionItem_Up_triggered()
{
    ui->graphicsView->processAction(__func__);
}

void MainWindow::on_actionItem_Down_triggered()
{
    ui->graphicsView->processAction(__func__);
}

void MainWindow::on_actionItem_Left_triggered()
{
    ui->graphicsView->processAction(__func__);
}

void MainWindow::on_actionItem_Right_triggered()
{
    ui->graphicsView->processAction(__func__);
}

void MainWindow::on_actionTop_Folder_triggered()
{
    ui->graphicsView->processAction(__func__);
}

QString addStep(int number, bool hyperlink, QString mainText, QString extraText, QString shortKey, QString imageURL, bool isPrimary)
{
    QString textStep = "";

    if (isPrimary)
        textStep += "<p style=\"color:red\">";
    else
        textStep += "<p>";

    if (number != -1)
        textStep += QString::number(number) + ") ";

    if (hyperlink)
    {
        if (isPrimary)
            textStep += "<a href=\"#" + mainText.toHtmlEscaped() + "\" style=\"color:red\">" + mainText + "</a>";
        else
            textStep += "<a href=\"#" + mainText.toHtmlEscaped() + "\" style=\"color:#33FFFF\">" + mainText + "</a>";
    }
    else
    {
        textStep += mainText;
    }

    if (extraText != "")
        textStep += " " + extraText;

    if (shortKey != "")
        textStep += " [" + shortKey + "]";

    if (imageURL != "")
            textStep += "<img src = \":" + imageURL + "\" width=20/>";

    textStep += "</p>";

    return textStep;
}

void MainWindow::on_actionHelp_triggered()
{
    QString commandControl = "Ctrl-";
#ifdef Q_OS_MAC
    commandControl = "⌘-";
#endif

    QDialog *about = new QDialog();

    about->setWindowTitle(tr("Media Sidekick Help"));
    QRect savedGeometry = QSettings().value("Geometry").toRect();
    savedGeometry.setX(savedGeometry.x() + savedGeometry.width() * .1);
    savedGeometry.setY(savedGeometry.y() + savedGeometry.height() * .1);
    savedGeometry.setWidth(savedGeometry.width() * .8);
    savedGeometry.setHeight(savedGeometry.height() * .8);
    about->setGeometry(savedGeometry);

    QVBoxLayout *mainLayout = new QVBoxLayout(about);


    QTextBrowser *textEdit = new QTextBrowser(this);
    mainLayout->addWidget(textEdit);
    textEdit->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse);

    QString text =
            "<h1 >Media Sidekick help</h1>\
            <p>Media Sidekick shows all media items of folder.</p>\
                <ul>"\
                    "<li>Media Items are Folders, Media files, clips and tags</li>"\
               "<li>Media Items are grouped per mediatype: Videos, Images, Audio, Export, Project, Parking</li>"\
                "</ul>"\
"<h1>Workflow</h1>"\
                                                                            "<p>The table shows the workflow of Media Sidekick</p>"\
           "<ul>"\
           "<li>The main workflow of Mediasidekick is split into View, Edit, Change and Export</li>"\
           "<li>The primary process are the steps in <span style=\"color:red\">red</span>. The other steps provide supporting functionality</li>"\
           "<li>If a step is <span style=\"color:#33FFFF\">highlighted</span>: Click on a step to get additional help (WIP)</li>"\
                                                                            "</ul>"\
                                                                            "<p></p>"\
             "<table border=1>\
               <tr>\
               <th></th>\
               <th>View</th>\
                 <th>Edit</th>\
               <th>Change</th>\
               <th>Export</th>\
               </tr>\
               <tr>\
                 <td><b>App</b><img src = \":/MediaSidekick.ico\" width=20/></td>\
                 <td><p>Spot view or timeline view</p>\
               <p>Sort by name or by date</p>\
               " + addStep(-1, false, "Properties", "", "Right-Click", "", false) + "\
               " + addStep(-1, false, "Open in explorer / finder", "", "Right-Click", "", false) + "</td>\
                 <td>" + addStep(-1, false, "Zoom in or out", "", "Ctrl-=/_", "/images/zoomin.png", false) + "\
               <p>Duration lines</p></td>\
<td></td>\
<td><p>Filter</p>\
               <p>Timeline view</p></td>\
               </tr>\
               <tr>\
                 <td><b>Folder</b><img src = \":/images/Folder.png\" width=20/></td>\
                 <td>" + addStep(1, false, "Open Folder", "", "Ctrl-O", "", true) + "\
            " + addStep(-1, true, "Show media items", "grouped per type", "", "", false) + "\
</td>\
                 <td>" + addStep(-1, false, "Download video or audio from streaming media", "(e.g. youtube)", "Right-Click", "", false) + "\
            " + addStep(-1, false, "Explorer/Finder", "Move media items into folder", "Right-Click", "", false) + "</td>\
            <td>" + addStep(-1, false, "Property Manager", "", "Right-Click", "", false) + "</td>\
            <td>" + addStep(8, false, "Export", "", "Ctrl-E", "", true) + "<p>Exported files in folder Export or Project</p></td>\
               </tr>\
               <tr>\
                 <td><b>Media file</b></td>\
                 <td>" + addStep(2, false, "Select item", "", "Arrows", "", true) + "\
            " + addStep(3, false, "Play video", "", "Space", "", true) + "\
            " + addStep(4, false, "Scrub video", "", "Shift-Mouse and hover over media file", "", true) +
                                                                               "<p><b>Duration line</b>: Red or green line above. Allows comparison of duration of media files and clips</p>"
                                                                               "<p><b>Progress slider</b>: Red slider below</p>"
                                                                 "</td>\
                <td></td>\
                 <td>" + addStep(-1, false, "Trim", "", "Right-Click", "", false) + "\
            " + addStep(-1, false, "Wideview", "", "Right-Click", "", false) + "\
            " + addStep(-1, false, "Archive to MSK recycle bin", "", "Right-Click", "", false) + "<p></p></td>\
            <td><p>Exported files</p></td>\
               </tr>\
               <tr>\
                 <td><b>Clip</b></td>\
                 <td>" + addStep(-1, false, "Scrub clip", "", "Shift-Mouse and hover over clip", "", false) + "</td>\
                 <td>" + addStep(5, false, "Add clip", "", "Ctrl-I", "", true) + "\
            " + addStep(6, false, "Change in and out", "", "Ctrl-I/O", "", true) + "\
            " + addStep(7, false, "Add tags, rating and alike", "", "a-Z, 0-9, Ctrl-1-5, Ctrl-L", "", true) + "\
            " + addStep(-1, false, "Delete clip or tags", "", "Right-Click", "", false) + "\
            " + addStep(-1, false, "Undo", "edits", "Ctrl-U", "/images/undo.png", false) + addStep(-1, false, "Redo", "edits", "Ctrl-R", "/images/redo.png", false) + addStep(-1, false, "Save", "edits", "Ctrl-S", "/images/save.png", false) +"\
               </tr>\
             </table>\
            <h2 id=\"Show media items\" style=\"color:#33FFFF\">Show Media Items</h2>\
               " +
                                         "<ul>"
                                         "<li>Only (sub) folders with mediafiles shown</li>"
                                         "<li><b>Scroll Wheel</b>: Scroll up and down</li>"
                                         "<li><b>" + commandControl + "Scroll Wheel</b>: Zoom-in and -out at current mouse position</li>"
                                         "<li><b>Left mouse click and drag</b>: Move around the scene</li>"
                                         "<li><b>Right mouse click on item</b>: Actions and properties of the item</li>"
                                         "</ul>"
            ; //.arg(qApp->applicationVersion())


            textEdit->setHtml(text);

//    about.setIconPixmap(QPixmap::fromImage(QImage(":/MediaSidekick.ico")));


    connect(textEdit, &QTextBrowser::anchorClicked, [=]()
    {
        qDebug()<<"anchorClicked";
    });

    about->show();
    about->exec();
}
