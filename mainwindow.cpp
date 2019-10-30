#include "fvideowidget.h"
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

#include "fglobal.h"

#include <QtMath>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->graphicsView1->addNode("folder", 0, 0);
    ui->graphicsView1->addNode("prop", 5, 0);
    ui->graphicsView1->addNode("main", 8, 0);
    ui->graphicsView1->addNode("tf1", 13, 0);

    ui->graphicsView1->addNode("time", 2, 3);
    ui->graphicsView1->addNode("video", 5, 3);
    ui->graphicsView1->addNode("star", 11, 3);
    ui->graphicsView1->addNode("tf2", 13, 3);

    ui->graphicsView1->addNode("files", 0, 6);
    ui->graphicsView1->addNode("edit", 8, 6);
    ui->graphicsView1->addNode("tags", 13, 6);

//    ui->graphicsView2->addNode("folder", 0, 0);
//    ui->graphicsView2->addNode("prop", 5, 0);
    ui->graphicsView2->addNode("main", 12, 3);

    ui->graphicsView2->addNode("time", 6, 6);
    ui->graphicsView2->addNode("video", 6, 0);

//    ui->graphicsView2->addNode("files", 0, 6);
    ui->graphicsView2->addNode("edit", 0, 3);

    //added designer settings
    ui->propertyDiffCheckBox->setCheckState(Qt::PartiallyChecked);
    ui->starEditorFilterWidget->setMaximumHeight(ui->newEditButton->height());
    ui->tagFilter1ListView->setMaximumHeight(ui->newEditButton->height());
    ui->tagFilter2ListView->setMaximumHeight(ui->newEditButton->height());
//    ui->newEditButton->setText(nullptr);
    ui->newEditButton->setIcon(style()->standardIcon(QStyle::SP_FileDialogStart));
//    ui->videoWidget->setMinimumHeight(500);

    //

    tagFilter1Model = new QStandardItemModel(this);
    ui->tagFilter1ListView->setModel(tagFilter1Model);
    ui->tagFilter1ListView->setFlow(QListView::LeftToRight);
    ui->tagFilter1ListView->setWrapping(true);
    ui->tagFilter1ListView->setDragDropMode(QListView::DragDrop);
    ui->tagFilter1ListView->setDefaultDropAction(Qt::MoveAction);
    ui->tagFilter1ListView->setSpacing(4);
    connect(tagFilter1Model, &QStandardItemModel::dataChanged,  this, &MainWindow::onEditFilterChanged);
    connect(tagFilter1Model, &QStandardItemModel::rowsRemoved,  this, &MainWindow::onEditFilterChanged); //datachanged not signalled when removing
    ui->graphicsView1->connectNodes("tf1", "main", "filter");

    tagFilter2Model = new QStandardItemModel(this);
    ui->tagFilter2ListView->setModel(tagFilter2Model);
    ui->tagFilter2ListView->setFlow(QListView::LeftToRight);
    ui->tagFilter2ListView->setWrapping(true);
    ui->tagFilter2ListView->setDragDropMode(QListView::DragDrop);
    ui->tagFilter2ListView->setDefaultDropAction(Qt::MoveAction);
    ui->tagFilter2ListView->setSpacing(4);
    connect(tagFilter2Model, &QStandardItemModel::dataChanged,  this, &MainWindow::onEditFilterChanged);
    connect(tagFilter2Model, &QStandardItemModel::rowsRemoved,  this, &MainWindow::onEditFilterChanged); //datachanged not signalled when removing
    ui->graphicsView1->connectNodes("tf2", "main", "filter");

//    ui->newTagLineEdit->setDragEnabled(true);
//    setDefaultDropAction(Qt::MoveAction);


//    ui->verticalLayout_player->insertWidget(1,ui->videoWidget->scrubBar);

    QRect savedGeometry = QSettings().value("Geometry").toRect();
    if (savedGeometry != geometry())
        setGeometry(savedGeometry);

    connect(ui->folderTreeView, &FFolderTreeView::indexClicked, ui->propertyTreeView, &FPropertyTreeView::onFolderIndexClicked);
    ui->graphicsView1->connectNodes("folder", "prop", "folder");
    connect(ui->folderTreeView, &FFolderTreeView::indexClicked, ui->filesTreeView, &FFilesTreeView::onFolderIndexClicked);
    ui->graphicsView1->connectNodes("folder", "files", "folder");
    connect(ui->folderTreeView, &FFolderTreeView::indexClicked, ui->editTableView, &FEditTableView::onFolderIndexClicked);
    ui->graphicsView1->connectNodes("folder", "edit", "folder");
    connect(ui->folderTreeView, &FFolderTreeView::indexClicked, ui->videoWidget, &FVideoWidget::onFolderIndexClicked);
    ui->graphicsView1->connectNodes("folder", "video", "folder");

    connect(ui->propertyTreeView, &FPropertyTreeView::propertiesLoaded, ui->editTableView, &FEditTableView::onPropertiesLoaded);

    connect(ui->filesTreeView, &FFilesTreeView::indexClicked, ui->editTableView, &FEditTableView::onFileIndexClicked);
    ui->graphicsView1->connectNodes("files", "edit", "file");
    connect(ui->filesTreeView, &FFilesTreeView::fileDelete, ui->videoWidget, &FVideoWidget::onFileDelete); //stop and release
    ui->graphicsView1->connectNodes("files", "video", "delete");
//    connect(ui->filesTreeView, &FFilesTreeView::fileRename, ui->videoWidget, &FVideoWidget::onFileRename); //stop and release
//    ui->graphicsView1->connectNodes("files", "video", "rename");
    connect(ui->filesTreeView, &FFilesTreeView::fileDelete, ui->editTableView, &FEditTableView::onFileDelete); //remove from edits
    ui->graphicsView1->connectNodes("files", "edit", "delete");
    connect(ui->filesTreeView, &FFilesTreeView::fileRename, ui->editTableView, &FEditTableView::onFileRename); //reload
    ui->graphicsView1->connectNodes("files", "edit", "rename");
    connect(ui->filesTreeView, &FFilesTreeView::fileDelete, ui->propertyTreeView, &FPropertyTreeView::onFileDelete); //remove from column
    ui->graphicsView1->connectNodes("files", "prop", "delete");
    connect(ui->filesTreeView, &FFilesTreeView::fileRename, ui->propertyTreeView, &FPropertyTreeView::onFileRename); //reload
    ui->graphicsView1->connectNodes("files", "prop", "rename");
    connect(ui->filesTreeView, &FFilesTreeView::trim, ui->editTableView, &FEditTableView::onTrim);
    ui->graphicsView1->connectNodes("files", "edit", "trim");
    connect(ui->filesTreeView, &FFilesTreeView::getPropertyValue, ui->propertyTreeView, &FPropertyTreeView::onGetPropertyValue);

    connect(ui->editTableView, &FEditTableView::folderIndexClickedItemModel, ui->tagsListView, &FTagsListView::onFolderIndexClicked);
    ui->graphicsView1->connectNodes("edit", "tags", "folder");
    connect(ui->editTableView, &FEditTableView::folderIndexClickedProxyModel, ui->timelineWidget, &FTimeline::onFolderIndexClicked);
    ui->graphicsView2->connectNodes("edit", "time", "folder");
    connect(ui->editTableView, &FEditTableView::folderIndexClickedProxyModel, this,  &MainWindow::onFolderIndexClicked); //to set edit counter
    ui->graphicsView1->connectNodes("edit", "main", "folder");
    connect(ui->editTableView, &FEditTableView::fileIndexClicked, ui->videoWidget, &FVideoWidget::onFileIndexClicked); //setmedia
    ui->graphicsView1->connectNodes("edit", "video", "file");
    connect(ui->editTableView, &FEditTableView::fileIndexClicked, ui->timelineWidget, &FTimeline::onFileIndexClicked); //currently nothing happens
    ui->graphicsView2->connectNodes("edit", "time", "file");
    connect(ui->editTableView, &FEditTableView::fileIndexClicked, ui->propertyTreeView, &FPropertyTreeView::onFileIndexClicked);
    ui->graphicsView1->connectNodes("edit", "prop", "file");
    connect(ui->editTableView, &FEditTableView::fileIndexClicked, this, &MainWindow::onFileIndexClicked); //trigger oneditfilterchanged
    ui->graphicsView1->connectNodes("edit", "prop", "file");
    connect(ui->editTableView, &FEditTableView::indexClicked, ui->filesTreeView, &FFilesTreeView::onEditIndexClicked);
    ui->graphicsView1->connectNodes("edit", "files", "edit");
    connect(ui->editTableView, &FEditTableView::indexClicked, ui->videoWidget, &FVideoWidget::onEditIndexClicked);
    ui->graphicsView1->connectNodes("edit", "video", "edit");
    connect(ui->editTableView, &FEditTableView::indexClicked, ui->propertyTreeView, &FPropertyTreeView::onEditIndexClicked);
    ui->graphicsView1->connectNodes("edit", "video", "edit");
    connect(ui->editTableView, &FEditTableView::editsChangedToVideo, ui->videoWidget, &FVideoWidget::onEditsChangedToVideo);
    ui->graphicsView2->connectNodes("edit", "video", "editchangevideo");
    connect(ui->editTableView, &FEditTableView::editsChangedToVideo, this,  &MainWindow::onEditsChangedToVideo);
    ui->graphicsView2->connectNodes("edit", "main", "editchangevideo");
    connect(ui->editTableView, &FEditTableView::editsChangedToTimeline, ui->timelineWidget,  &FTimeline::onEditsChangedToTimeline);
    ui->graphicsView2->connectNodes("edit", "time", "editchangetimeline");
    connect(ui->editTableView, &FEditTableView::getPropertyValue, ui->propertyTreeView, &FPropertyTreeView::onGetPropertyValue);
//    ui->graphicsView1->connectNodes("edit", "prop", "get");
    connect(ui->editTableView, &FEditTableView::addLogEntry, ui->logTableView, &FLogTableView::onAddEntry);
    ui->graphicsView1->connectNodes("video", "prop", "get");
    connect(ui->editTableView, &FEditTableView::addLogToEntry, ui->logTableView, &FLogTableView::onAddLogToEntry);

    connect(ui->editTableView, &FEditTableView::trim, ui->propertyTreeView, &FPropertyTreeView::onTrim);

    connect(ui->editTableView, &FEditTableView::updateIn, ui->videoWidget, &FVideoWidget::onUpdateIn);
    connect(ui->editTableView, &FEditTableView::updateOut, ui->videoWidget, &FVideoWidget::onUpdateOut);

    connect(ui->editTableView, &FEditTableView::frameRateChanged, this, &MainWindow::onFrameRateChanged);

    connect(ui->videoWidget, &FVideoWidget::videoPositionChanged, ui->editTableView, &FEditTableView::onVideoPositionChanged);
    ui->graphicsView1->connectNodes("video", "edit", "pos");
    connect(ui->videoWidget, &FVideoWidget::videoPositionChanged, ui->timelineWidget, &FTimeline::onVideoPositionChanged);
    ui->graphicsView2->connectNodes("video", "time", "pos");
    connect(ui->videoWidget, &FVideoWidget::scrubberInChanged, ui->editTableView, &FEditTableView::onScrubberInChanged);
    ui->graphicsView1->connectNodes("video", "edit", "in");
    connect(ui->videoWidget, &FVideoWidget::scrubberOutChanged, ui->editTableView, &FEditTableView::onScrubberOutChanged);
    ui->graphicsView1->connectNodes("video", "edit", "out");
    connect(ui->videoWidget, &FVideoWidget::getPropertyValue, ui->propertyTreeView, &FPropertyTreeView::onGetPropertyValue);
//    ui->graphicsView1->connectNodes("video", "prop", "get");

    connect(ui->timelineWidget, &FTimeline::timelinePositionChanged, ui->videoWidget, &FVideoWidget::onTimelinePositionChanged);
    ui->graphicsView2->connectNodes("time", "video", "pos");
//    connect(ui->timelineWidget, &FTimeline::getPropertyValue, ui->propertyTreeView, &FPropertyTreeView::onGetPropertyValue);
//    ui->graphicsView1->connectNodes("time", "prop", "get");
    connect(ui->timelineWidget, &FTimeline::editsChangedToVideo, ui->videoWidget, &FVideoWidget::onEditsChangedToVideo);
    ui->graphicsView2->connectNodes("time", "video", "editchangevideo");
    connect(ui->timelineWidget, &FTimeline::editsChangedToTimeline, this, &MainWindow::onEditsChangedToTimeline);
    ui->graphicsView2->connectNodes("time", "main", "editchangetimeline");

    connect(ui->timelineWidget, &FTimeline::adjustTransitionAndStretchTime, this, &MainWindow::onAdjustTransitionAndStretchTime);

    connect(ui->propertyTreeView, &FPropertyTreeView::addLogEntry, ui->logTableView, &FLogTableView::onAddEntry);
    ui->graphicsView1->connectNodes("video", "prop", "get");
    connect(ui->propertyTreeView, &FPropertyTreeView::addLogToEntry, ui->logTableView, &FLogTableView::onAddLogToEntry);
    connect(ui->propertyTreeView, &FPropertyTreeView::fileDelete, ui->videoWidget, &FVideoWidget::onFileDelete); // on property change, stop video

    connect(this, &MainWindow::propertyFilterChanged, ui->propertyTreeView, &FPropertyTreeView::onPropertyFilterChanged);
    ui->graphicsView1->connectNodes("main", "prop", "filter");
    connect(this, &MainWindow::editFilterChanged, ui->editTableView, &FEditTableView::onEditFilterChanged);
    ui->graphicsView1->connectNodes("main", "edit", "filter");

    connect(this, &MainWindow::timelineWidgetsChanged, ui->timelineWidget, &FTimeline::onTimelineWidgetsChanged);
    ui->graphicsView2->connectNodes("main", "video", "widgetchanged");

    connect(ui->generateWidget, &FGenerate::addLogEntry, ui->logTableView, &FLogTableView::onAddEntry);
    ui->graphicsView1->connectNodes("video", "prop", "get");
    connect(ui->generateWidget, &FGenerate::addLogToEntry, ui->logTableView, &FLogTableView::onAddLogToEntry);
    connect(ui->generateWidget, &FGenerate::getPropertyValue, ui->propertyTreeView, &FPropertyTreeView::onGetPropertyValue);

    connect(ui->starEditorFilterWidget, &FStarEditor::editingFinished, this, &MainWindow::onEditFilterChanged);
    ui->graphicsView1->connectNodes("star", "main", "filter");

    connect(ui->fileOnlyCheckBox, &QCheckBox::clicked, this, &MainWindow::onEditFilterChanged);
    connect(ui->alikeCheckBox, &QCheckBox::clicked, this, &MainWindow::onEditFilterChanged);

    //load settings

    ui->defaultMinSpinBox->setValue(QSettings().value("defaultMinDuration").toInt());
    ui->defaultPlusSpinBox->setValue(QSettings().value("defaultPlusDuration").toInt());

//    qDebug()<<"tagFilter1"<<QSettings().value("tagFilter1").toString();
//    qDebug()<<"tagFilter2"<<QSettings().value("tagFilter2").toString();
    QStringList tagList1 = QSettings().value("tagFilter1").toString().split(";");
    if (tagList1.count()==1 && tagList1[0] == "")
        tagList1.clear();

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
    QStringList tagList2 = QSettings().value("tagFilter2").toString().split(";");
    if (tagList2.count()==1 && tagList2[0] == "")
        tagList2.clear();

    for (int j=0; j < tagList2.count(); j++)
    {
        qDebug()<<"tagList2[j].toLower()"<<j<<tagList2[j].toLower();
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

    //
//    onEditFilterChanged(); //initial setup, before MainWindow::onFolderIndexClicked because this is done after edits are loaded
    emit editFilterChanged(ui->starEditorFilterWidget, ui->alikeCheckBox, ui->tagFilter1ListView, ui->tagFilter2ListView, ui->fileOnlyCheckBox);

//    emit timelineWidgetsChanged(ui->transitionTimeSpinBox->value(), ui->transitionComboBox->currentText(), ui->stretchedDurationSpinBox->value(), false, ui->editTableView);

    ui->folderTreeView->onIndexClicked(QModelIndex()); //initial load

//    ui->progressBar->setRange(0, 100);
    ui->progressBar->setValue(0);


    //    QPixmap pixmap(":/fiprelogo.ico");
//        QLabel *mylabel = new QLabel (this);
//        ui->logoLabel->setPixmap( QPixmap (":/fiprelogo.ico"));
//        mylabel->show();
//        mylabel->update();
    //    m_durationLabel->setPixmap(pixmap);
    //    mylabel->show();
    //    toolbar->addWidget(mylabel);


    //tooltips

    //tt edit filters
    ui->alikeCheckBox->setToolTip(tr("<p><b>Alike</b></p>"
                                     "<p><i>Check if other edits are 'alike' this edit. This is typically the case with Action-cam footage: multiple shots which are similar.</i></p>"
                                     "<p><i>This is used to exclude similar edits from the generated video.</i></p>"
                                     "<ul>"
                                     "<li>Alike filter checkbox: Show only alike edits</li>"
                                     "<li>Alike column: Set if this edit is like another edit <CTRL-A></li>"
                                     "<li>Exclude: Give the best of the alikes a higher rating than the others</li>"
                                     "<li>Hint: Give alike edits the same tags to filter on them later</li>"
                                     "</ul>"
                                  ));

    ui->starEditorFilterWidget->setToolTip(tr("<p><b>Rates</b></p>"
                                              "<p><i>Description</i></p>"
                                              "<ul>"
                                              "<li>Feature 1</li>"
                                              "</ul>"));

    ui->tagFilter1ListView->setToolTip(tr("<p><b>Tag filter...</b></p>"
                                          "<p><i>Description</i></p>"
                                          "<ul>"
                                          "<li>Feature 1</li>"
                                          "</ul>"));

    ui->tagFilter2ListView->setToolTip(ui->tagFilter1ListView->toolTip());

    ui->fileOnlyCheckBox->setToolTip(tr("<p><b>File only</b></p>"
                                        "<p><i>Description</i></p>"
                                        "<ul>"
                                        "<li>Feature 1</li>"
                                        "</ul>"));
    ui->stretchedCheckBox->setToolTip(tr("<p><b>Stretched edits</b></p>"
                                         "<p><i>Description</i></p>"
                                         "<ul>"
                                         "<li>Feature 1</li>"
                                         "</ul>"));

    //tt edit table
    ui->editTableView->setToolTip(tr("<p><b>Edit list</b></p>"
                                     "<p><i>Show the edits for the files in the selected folder</i></p>"
                                     "<ul>"
                                     "<li>Only edits applying to the filters above this table are shown</li>"
                                     "<li>Move over the column headers to see column tooltips</li>"
                                     "<li>Edits belonging to the selected file are highlighted gray</li>"
                                     "<li>The edit currently shown in the video window is highlighted blue</li>"
                                     "<li>To delete an edit: right mouse click</li>"
                                     "<li>To change the order of edits: drag the number on the left of this table up or down</li>"
                                     "</ul>"
                                     ));

    ui->editTableView->editItemModel->horizontalHeaderItem(inIndex)->setToolTip(tr("<p><b>Title</b></p>"
                                                                                   "<p><i>Description</i></p>"
                                                                                   "<ul>"
                                                                                   "<li>Feature 1</li>"
                                                                                   "</ul>"));
    ui->editTableView->editItemModel->horizontalHeaderItem(outIndex)->setToolTip( ui->editTableView->editItemModel->horizontalHeaderItem(inIndex)->toolTip());
    ui->editTableView->editItemModel->horizontalHeaderItem(durationIndex)->setToolTip( ui->editTableView->editItemModel->horizontalHeaderItem(inIndex)->toolTip());
    ui->editTableView->editItemModel->horizontalHeaderItem(ratingIndex)->setToolTip(ui->starEditorFilterWidget->toolTip());
    ui->editTableView->editItemModel->horizontalHeaderItem(alikeIndex)->setToolTip(ui->alikeCheckBox->toolTip());
    ui->editTableView->editItemModel->horizontalHeaderItem(tagIndex)->setToolTip(tr("<p><b>Tags per edit</b></p>"
                                                                                    "<p><i>Show the tags per edit</i></p>"
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
                                      "<li>Delete tags from filter, edits and taglist: drag tag and drop in the tag field</li>"
                                      "</ul>"
                                      ));
    ui->tagsListView->setToolTip(tr("<p><b>Tag list</b></p>"
                                    "<p><i>List of tags used in edits or filter</i></p>"
                                    "<ul>"
                                    "<li>Add tag to edit: Drag and drop to tag column of edits</li>"
                                    "<li>Add tag to filter: Drag and drop to filters</li>"
                                    "</ul>"
                                    ));

    ui->stretchDial->setToolTip(tr("<p><b>Dial your stress away</b></p>"
                                   "<p><i>Description</i></p>"
                                   "<ul>"
                                   "<li>Feature 1</li>"
                                   "</ul>"));
    //end tooltips

    //do not show the graph tab (only in debug mode)
    graphWidget1 = ui->editTabWidget->widget(2);
    graphWidget2 = ui->editTabWidget->widget(3);

    on_actionDebug_mode_triggered(false); //no debug mode

    ui->stretchDial->setNotchesVisible(true);
    ui->transitionDial->setNotchesVisible(true);
//    ui->transitionDial->setWrapping(true);

    m_upgradeUrl = "https://www.acioncamvideocompanion.com/download/";
    connect(&m_network, SIGNAL(finished(QNetworkReply*)), SLOT(onUpgradeCheckFinished(QNetworkReply*)));

    QTimer::singleShot(0, this, [this]()->void
    {
                           QString theme = QSettings().value("theme").toString();
                           if (theme == "White")
                               on_actionWhite_theme_triggered();
                           else
                               on_actionBlack_theme_triggered();
                           showUpgradePrompt();
                       });
}

MainWindow::~MainWindow()
{
    if (ui->editTableView->checkSaveIfEditsChanged())
        on_actionSave_triggered();

//    qDebug()<<"Destructor"<<geometry();

    QSettings().setValue("Geometry", geometry());
    QSettings().sync();

    delete ui;
}

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

    qApp->setStyleSheet("QToolTip { color: #ffffff; background-color: #2a82da; border: 1px solid white; }");

    QSettings().setValue("theme", "Black");
    QSettings().sync();
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

    QSettings().setValue("theme", "White");
    QSettings().sync();
}

void MainWindow::on_actionQuit_triggered()
{
    qApp->quit();
}

void MainWindow::on_actionAbout_triggered()
{
    QMessageBox::about(this, tr("About ACVC"),
            tr("<p><b>ACVC</b> is an Action Cam Video Companion</p>"
               "<p>Version  %1</p>"
               "<p>Copyright (c)2019 <a href=\"https://actioncamvideocompanion.com\">ACVC</a></p>"
               "<p>This program proudly uses the following projects:</p>"
               "<ul>"
               "<li><a href=\"https://www.qt.io/\">Qt</a> application and UI framework</li>"
               "<li><a href=\"https://www.shotcut.org/\">Shotcut</a> Open source video editor (timeline and version check)</li>"
               "<li><a href=\"https://www.ffmpeg.org/\">FFmpeg</a> multimedia format and codec libraries (generate previews)</li>"
               "<li><a href=\"https://www.sno.phy.queensu.ca/~phil/exiftool/\">Exiftool</a> Read, Write and Edit Meta Information! (Metadata)</li>"
               "</ul>"
               ).arg(qApp->applicationVersion()));
}

void MainWindow::on_actionAbout_Qt_triggered()
{
    qApp->aboutQt();
}

void MainWindow::on_newEditButton_clicked()
{
    ui->editTableView->addEdit(ui->defaultMinSpinBox->value()*FGlobal().frames_to_msec(1),ui->defaultPlusSpinBox->value()*FGlobal().frames_to_msec(1));
}

void MainWindow::on_propertyFilterLineEdit_textChanged(const QString &)//arg1
{
    emit propertyFilterChanged(ui->propertyFilterLineEdit, ui->propertyDiffCheckBox, ui->locationCheckBox, ui->cameraCheckBox);
}

void MainWindow::on_propertyDiffCheckBox_stateChanged(int )//arg1
{
    emit propertyFilterChanged(ui->propertyFilterLineEdit, ui->propertyDiffCheckBox, ui->locationCheckBox, ui->cameraCheckBox);
}

void MainWindow::onEditsChangedToVideo(QAbstractItemModel *itemModel)
{
    ui->editrowsCounterLabel->setText(QString::number(itemModel->rowCount()) + " / " + QString::number(ui->editTableView->editItemModel->rowCount()));
}

void MainWindow::onFolderIndexClicked(QAbstractItemModel *itemModel)
{
    qDebug()<<"MainWindow::onFolderIndexClicked"<<itemModel->rowCount()<<ui->stretchedDurationSpinBox->value()<<ui->timelineWidget->transitiontimeDuration;

    ui->editrowsCounterLabel->setText(QString::number(itemModel->rowCount()) + " / " + QString::number(ui->editTableView->editItemModel->rowCount()));

    int starInt = ui->folderTreeView->folderSettings->value("starsFilter").toInt();
    QVariant starVar = QVariant::fromValue(FStarRating(starInt));
    FStarRating starRating = qvariant_cast<FStarRating>(starVar);
    ui->starEditorFilterWidget->setStarRating(starRating);

    ui->transitionTimeSpinBox->setValue(ui->folderTreeView->folderSettings->value("transitionTime").toInt());
    ui->transitionComboBox->setCurrentText(ui->folderTreeView->folderSettings->value("transitionType").toString());

    //    ui->stretchedDurationSpinBox->blockSignals(true); // do not save immediately in on_stretchedDurationSpinBox_valueChanged
    ui->stretchedDurationSpinBox->setValue(ui->folderTreeView->folderSettings->value("stretchedDuration").toInt());
    //    ui->stretchedDurationSpinBox->blockSignals(false);

    ui->generateTargetComboBox->setCurrentText(ui->folderTreeView->folderSettings->value("generateTarget").toString());
    ui->generateSizeComboBox->setCurrentText(ui->folderTreeView->folderSettings->value("generateSize").toString());
    int lframeRate = ui->folderTreeView->folderSettings->value("frameRate").toInt();
    if (lframeRate < 1)
            lframeRate = 25;
    ui->frameRateSpinBox->setValue(lframeRate);

//    ui->alikeCheckBox->setCheckState(Qt::Unchecked);
//    on_alikeCheckBox_clicked(false); //save in QSettings

//    tagFilter1Model->clear();
//    tagFilter2Model->clear();
//    onTagFiltersChanged();

//    ui->fileOnlyCheckBox->setCheckState(Qt::Unchecked);
//    on_fileOnlyCheckBox_clicked(false); //save in QSettings

//    onEditFilterChanged(); //already done in constructor

    emit editFilterChanged(ui->starEditorFilterWidget, ui->alikeCheckBox, ui->tagFilter1ListView, ui->tagFilter2ListView, ui->fileOnlyCheckBox);
    emit timelineWidgetsChanged(ui->transitionTimeSpinBox->value(), ui->transitionComboBox->currentText(), ui->stretchedDurationSpinBox->value(), ui->editTableView);
}

void MainWindow::onFileIndexClicked(QModelIndex index)
{
    qDebug()<<"MainWindow::onFileIndexClicked"<<index.data().toString();
//    onEditFilterChanged();
    emit editFilterChanged(ui->starEditorFilterWidget, ui->alikeCheckBox, ui->tagFilter1ListView, ui->tagFilter2ListView, ui->fileOnlyCheckBox);
}

void MainWindow::onEditFilterChanged()
{
    ui->editrowsCounterLabel->setText(QString::number(ui->editTableView->model()->rowCount()) + " / " + QString::number(ui->editTableView->editItemModel->rowCount()));

    qDebug()<<"MainWindow::onEditFilterChanged"<<ui->folderTreeView->folderSettings->value("starsFilter")<<ui->starEditorFilterWidget->starRating().starCount();
    onTagFiltersChanged(); //save tagfilters

    if (ui->folderTreeView->folderSettings->value("starsFilter") != ui->starEditorFilterWidget->starRating().starCount())
    {
        ui->folderTreeView->folderSettings->setValue("starsFilter", ui->starEditorFilterWidget->starRating().starCount());
        ui->folderTreeView->folderSettings->sync();
    }

    emit editFilterChanged(ui->starEditorFilterWidget, ui->alikeCheckBox, ui->tagFilter1ListView, ui->tagFilter2ListView, ui->fileOnlyCheckBox);
//    qDebug()<<"MainWindow::onEditFilterChanged"<<ui->folderTreeView->folderSettings->value("starsFilter")<<ui->starEditorFilterWidget->starRating().starCount();
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
}

void MainWindow::on_action5_stars_triggered()
{
    ui->editTableView->giveStars(5);
}

void MainWindow::on_action4_stars_triggered()
{
    ui->editTableView->giveStars(4);
}

void MainWindow::on_action1_star_triggered()
{
    ui->editTableView->giveStars(1);
}

void MainWindow::on_action2_stars_triggered()
{
    ui->editTableView->giveStars(2);
}

void MainWindow::on_action3_stars_triggered()
{
    ui->editTableView->giveStars(3);
}

void MainWindow::on_action0_stars_triggered()
{
    ui->editTableView->giveStars(0);

}

void MainWindow::on_actionAlike_triggered()
{
    ui->editTableView->toggleAlike();
}

void MainWindow::on_actionSave_triggered()
{
    int changeCount = 0;
    for (int row=0; row<ui->editTableView->editItemModel->rowCount();row++)
    {
        if (ui->editTableView->editItemModel->index(row, changedIndex).data().toString() == "yes")
            changeCount++;
    }
    if (changeCount>0)
    {
        ui->editTableView->onSectionMoved(-1,-1,-1); //to reorder the items

        for (int row =0; row < ui->editTableView->srtFileItemModel->rowCount();row++)
        {
    //        qDebug()<<"MainWindow::on_actionSave_triggered"<<ui->editTableView->srtFileItemModel->rowCount()<<row<<ui->editTableView->srtFileItemModel->index(row, 0).data().toString()<<ui->editTableView->srtFileItemModel->index(row, 1).data().toString();
            ui->editTableView->saveModel(ui->editTableView->srtFileItemModel->index(row, 0).data().toString(), ui->editTableView->srtFileItemModel->index(row, 1).data().toString());
        }
    }
}

void MainWindow::on_actionPlay_Pause_triggered()
{
    ui->videoWidget->togglePlayPaused();
}

void MainWindow::on_actionNew_triggered()
{
    ui->editTableView->addEdit(ui->defaultMinSpinBox->value()*FGlobal().frames_to_msec(1), ui->defaultPlusSpinBox->value()*FGlobal().frames_to_msec(1));
}

void MainWindow::on_stretchedDurationSpinBox_valueChanged(int arg1)
{
    if (ui->folderTreeView->folderSettings->value("stretchedDuration") != arg1)
    {
//        qDebug()<<"MainWindow::on_stretchedDurationSpinBox_valueChanged"<<arg1<<ui->timelineWidget->transitiontimeDuration;
        ui->folderTreeView->folderSettings->setValue("stretchedDuration", arg1);
        ui->folderTreeView->folderSettings->sync();
        emit timelineWidgetsChanged(ui->transitionTimeSpinBox->value(), ui->transitionComboBox->currentText(), ui->stretchedDurationSpinBox->value(), ui->editTableView);
    }
}

void MainWindow::on_actionIn_triggered()
{
    qDebug()<<"MainWindow::on_updateInButton_clicked"<<ui->editTableView->highLightedRow<<ui->videoWidget->m_position;
    ui->videoWidget->onUpdateIn();
}

void MainWindow::on_actionOut_triggered()
{
    qDebug()<<"MainWindow::on_updateOutButton_clicked"<<ui->editTableView->highLightedRow<<ui->videoWidget->m_position;
    ui->videoWidget->onUpdateOut();
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

void MainWindow::on_defaultMinSpinBox_valueChanged(int arg1)
{
    if (QSettings().value("defaultMinDuration") != arg1)
    {
        qDebug()<<"MainWindow::on_defaultMinSpinBox_valueChanged"<<arg1;
        QSettings().setValue("defaultMinDuration", arg1);
        QSettings().sync();
    }
}

void MainWindow::on_defaultPlusSpinBox_valueChanged(int arg1)
{
    if (QSettings().value("defaultPlusDuration") != arg1)
    {
        qDebug()<<"MainWindow::on_defaultPlusSpinBox_valueChanged"<<arg1;
        QSettings().setValue("defaultPlusDuration", arg1);
        QSettings().sync();
    }
}

void MainWindow::on_actionAdd_tag_triggered()
{
    QList<QStandardItem *> items;
    QStandardItem *item = new QStandardItem("ewoud");
    item->setBackground(QBrush(Qt::red));

    items.append(item);
    items.append(new QStandardItem("I"));
    ui->tagsListView->tagsItemModel->appendRow(items);
}

void MainWindow::on_newTagLineEdit_returnPressed()
{
    if (ui->tagsListView->addTag(ui->newTagLineEdit->text()))
        ui->newTagLineEdit->clear();
    else
        QMessageBox::information(this, "Add tag", "Tag " + ui->newTagLineEdit->text() + " already in list");
}

void MainWindow::on_generateButton_clicked()
{
    int transitionTime = 0;
    if (ui->transitionComboBox->currentText() != "No transition")
        transitionTime = ui->transitionTimeSpinBox->value();
    ui->generateWidget->generate(ui->timelineWidget->timelineModel, ui->generateTargetComboBox->currentText(), ui->generateSizeComboBox->currentText(), ui->frameRateSpinBox->value(), transitionTime, ui->progressBar);
}

void MainWindow::on_generateTargetComboBox_currentTextChanged(const QString &arg1)
{
    if (ui->folderTreeView->folderSettings->value("generateTarget") != arg1)
    {
        ui->folderTreeView->folderSettings->setValue("generateTarget", arg1);
        ui->folderTreeView->folderSettings->sync();
    }
}

void MainWindow::on_generateSizeComboBox_currentTextChanged(const QString &arg1)
{
    if (ui->folderTreeView->folderSettings->value("generateSize") != arg1)
    {
        ui->folderTreeView->folderSettings->setValue("generateSize", arg1);
        ui->folderTreeView->folderSettings->sync();
    }
}

void MainWindow::on_frameRateSpinBox_valueChanged(int arg1)
{
    ui->transitionDial->setRange(0, ui->frameRateSpinBox->value() * 4);
    QSettings().setValue("frameRate", arg1); //as used globally
    if (ui->folderTreeView->folderSettings->value("frameRate") != arg1)
    {
        ui->folderTreeView->folderSettings->setValue("frameRate", arg1);
        ui->folderTreeView->folderSettings->sync();
    }
}

void MainWindow::onFrameRateChanged(int frameRate)
{
    ui->frameRateSpinBox->setValue(frameRate);
}

void MainWindow::on_actionGenerate_triggered()
{
    ui->generateButton->click();
}

void MainWindow::on_alikeCheckBox_clicked(bool checked)
{
    if (QSettings().value("alikeCheckBox").toBool() != checked)
    {
        QSettings().setValue("alikeCheckBox", checked);
        QSettings().sync();
    }
}

void MainWindow::on_fileOnlyCheckBox_clicked(bool checked)
{
    if (QSettings().value("fileOnlyCheckBox").toBool() != checked)
    {
        QSettings().setValue("fileOnlyCheckBox", checked);
        QSettings().sync();
    }
}

void MainWindow::on_actionDebug_mode_triggered(bool checked)
{
    qDebug()<<"on_actionDebug_mode_triggered"<<checked;

    if (checked)
    {
        ui->editTabWidget->insertTab(2, graphWidget1, "Graph1");
        ui->editTabWidget->insertTab(3, graphWidget2, "Graph2");
    }
    else
    {
        ui->editTabWidget->removeTab(3);
        ui->editTabWidget->removeTab(2);
    }

    ui->editTableView->setColumnHidden(orderBeforeLoadIndex, !checked);
    ui->editTableView->setColumnHidden(orderAtLoadIndex, !checked);
    ui->editTableView->setColumnHidden(orderAfterMovingIndex, !checked);
    ui->editTableView->setColumnHidden(fpsIndex, !checked);
    ui->editTableView->setColumnHidden(fileDurationIndex, !checked);
    ui->editTableView->setColumnHidden(changedIndex, !checked);
}

void MainWindow::on_resetSortButton_clicked()
{
    for (int row=0;row<ui->editTableView->model()->rowCount();row++)
    {
        ui->editTableView->verticalHeader()->moveSection(ui->editTableView->model()->index(row, orderBeforeLoadIndex).data().toInt() - 1, row);
//        ui->editTableView->model()->setData(ui->editTableView->model()->index(row, orderAtLoadIndex), (row + 1) * 10);
//        ui->editTableView->model()->setData(ui->editTableView->model()->index(row, orderAfterMovingIndex), (row + 1) * 10);
//        ui->editTableView->model()->setData(ui->editTableView->model()->index(row, changedIndex), "yes");
    }
}

void MainWindow::on_stretchedCheckBox_clicked(bool checked)
{
//    qDebug()<<"MainWindow::on_stretchedCheckBox_clicked"<<ui->editTableView->editTriggers();
    if (checked) //no edit
    {
        ui->editTableView->setModel(ui->timelineWidget->timelineModel);
        ui->editTableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
//        ui->editTableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    }
    else //edit allowed
    {
        ui->editTableView->setModel(ui->editTableView->editProxyModel);
        ui->editTableView->setEditTriggers(QAbstractItemView::DoubleClicked|QAbstractItemView::EditKeyPressed|QAbstractItemView::AnyKeyPressed);
//        ui->editTableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    }
//    qDebug()<<"MainWindow::on_stretchedCheckBox_clicked"<<ui->editTableView->editTriggers();
    onEditFilterChanged();
}

void MainWindow::on_locationCheckBox_clicked(bool checked)
{
    if (QSettings().value("locationCheckBox").toBool() != checked)
    {
        QSettings().setValue("locationCheckBox", checked);
        QSettings().sync();
        emit propertyFilterChanged(ui->propertyFilterLineEdit, ui->propertyDiffCheckBox, ui->locationCheckBox, ui->cameraCheckBox);
    }
}

void MainWindow::on_cameraCheckBox_clicked(bool checked)
{
    if (QSettings().value("cameraCheckBox").toBool() != checked)
    {
        QSettings().setValue("cameraCheckBox", checked);
        QSettings().sync();
        emit propertyFilterChanged(ui->propertyFilterLineEdit, ui->propertyDiffCheckBox, ui->locationCheckBox, ui->cameraCheckBox);

    //    QString fileName = QFileDialog::getOpenFileName(this,
    //          tr("Open Image"), "/home/", tr("MediaFiles (*.mp4)"));

        QFileDialog dialog(this);
        dialog.setFileMode(QFileDialog::AnyFile);
        dialog.setNameFilter(tr("MediaFiles (*.mp4)"));
        dialog.setViewMode(QFileDialog::List);
        QStringList fileNames;
         if (dialog.exec())
             fileNames = dialog.selectedFiles();
    }
}

void MainWindow::on_transitionComboBox_currentTextChanged(const QString &arg1)
{
    if (ui->folderTreeView->folderSettings->value("transitionType") != arg1)
    {
        qDebug()<<"MainWindow::on_transitionComboBox_currentTextChanged"<<arg1;
        ui->folderTreeView->folderSettings->setValue("transitionType", arg1);
        ui->folderTreeView->folderSettings->sync();
        emit timelineWidgetsChanged(ui->transitionTimeSpinBox->value(), ui->transitionComboBox->currentText(), ui->stretchedDurationSpinBox->value(), ui->editTableView);
    }
}

void MainWindow::on_stretch100Button_clicked()
{
//    ui->stretchedDurationSpinBox->setValue(ui->timelineWidget->transitiontimeDuration);
    ui->stretchDial->setValue(50);
//    emit timelineWidgetsChanged(ui->transitionTimeSpinBox->value(), ui->transitionComboBox->currentText(), ui->stretchedDurationSpinBox->value(), ui->stretchedDurationSpinBox->value() != ui->timelineWidget->transitiontimeDuration, ui->editTableView);
}

void MainWindow::on_stretchDial_valueChanged(int value)
{
    if (stretchValueChangedBy != "SpinBox") //do not change the spinbox if the spinbox triggers the change
    {
        double result;
        if (value < 50)
            result = qSin(M_PI * value / 100);
        else
            result = 1 / qSin(M_PI * value / 100);
        //    result = value / 50.0;

//        qDebug()<<"MainWindow::on_stretchDial_valueChanged"<<value<<ui->timelineWidget->transitiontimeDuration<<result<<stretchValueChangedBy;

        stretchValueChangedBy = "Dial";
        ui->stretchedDurationSpinBox->setValue(ui->timelineWidget->transitiontimeDuration * result);
        stretchValueChangedBy = "";
        ui->stretch100Button->setText(QString::number(ui->stretchedDurationSpinBox->value() * 100 / ui->timelineWidget->transitiontimeDuration, 'g', 3) + "%");
    }
}

void MainWindow::onEditsChangedToTimeline(QAbstractItemModel *itemModel)
{
    if (ui->timelineWidget->transitiontimeDuration != 0 && stretchValueChangedBy != "Dial")//do not change the dial if the dial triggers the change
    {
        double result;
        if (ui->stretchedDurationSpinBox->value() / double(ui->timelineWidget->transitiontimeDuration) < 1)
            result = qAsin(ui->stretchedDurationSpinBox->value() / double(ui->timelineWidget->transitiontimeDuration)) * 100 / M_PI;
        else
            result = 100 - qAsin(double(ui->timelineWidget->transitiontimeDuration) / double( ui->stretchedDurationSpinBox->value())) * 100 / M_PI;
//        result = ui->stretchedDurationSpinBox->value() * 50.0/ ui->timelineWidget->transitiontimeDuration;

//        qDebug()<<"MainWindow::onEditsChangedToTimeline"<<ui->stretchedDurationSpinBox->value() << ui->timelineWidget->transitiontimeDuration<<ui->stretchedDurationSpinBox->value() * 100 / ui->timelineWidget->transitiontimeDuration;

        stretchValueChangedBy = "SpinBox";
        ui->stretchDial->setValue( int(result ));
        stretchValueChangedBy = "";
        ui->stretch100Button->setText(QString::number(ui->stretchedDurationSpinBox->value() * 100 / ui->timelineWidget->transitiontimeDuration, 'g', 3) + "%");
    }

    if (transitionValueChangedBy != "Dial")//do not change the dial if the dial triggers the change
    {
        double result;
        if (ui->stretchedDurationSpinBox->value() / double(ui->timelineWidget->transitiontimeDuration) < 1)
            result = qAsin(ui->stretchedDurationSpinBox->value() / double(ui->timelineWidget->transitiontimeDuration)) * 100 / M_PI;
        else
            result = 100 - qAsin(double(ui->timelineWidget->transitiontimeDuration) / double( ui->stretchedDurationSpinBox->value())) * 100 / M_PI;
        result = (ui->transitionTimeSpinBox->value());

//        qDebug()<<"MainWindow::onEditsChangedToTimeline"<<ui->stretchedDurationSpinBox->value() << ui->timelineWidget->transitiontimeDuration<<ui->stretchedDurationSpinBox->value() * 100 / ui->timelineWidget->transitiontimeDuration;

        transitionValueChangedBy = "SpinBox";
        ui->transitionDial->setValue( int(result ));
        transitionValueChangedBy = "";
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

        qDebug()<<"MainWindow::on_transitionDial_valueChanged"<<value<<ui->timelineWidget->transitiontimeDuration<<result<<stretchValueChangedBy;

        transitionValueChangedBy = "Dial";
        ui->transitionTimeSpinBox->setValue(result);
        transitionValueChangedBy = "";
    }
}

void MainWindow::on_transitionTimeSpinBox_valueChanged(int arg1)
{
    if (ui->folderTreeView->folderSettings->value("transitionTime") != arg1)
    {
        qDebug()<<"MainWindow::on_transitionTimeSpinBox_valueChanged"<<arg1;
        ui->folderTreeView->folderSettings->setValue("transitionTime", arg1);
        ui->folderTreeView->folderSettings->sync();

        emit timelineWidgetsChanged(ui->transitionTimeSpinBox->value(), ui->transitionComboBox->currentText(), ui->stretchedDurationSpinBox->value(), ui->editTableView);
    }
}

void MainWindow::onAdjustTransitionAndStretchTime(int transitionTime, int stretchTime)
{
    transitionValueChangedBy = "SpinBox";
    ui->transitionTimeSpinBox->setValue(transitionTime);
    transitionValueChangedBy = "";

    stretchValueChangedBy = "SpinBox";
    ui->stretchedDurationSpinBox->setValue(stretchTime);
    stretchValueChangedBy = "";
}


void MainWindow::on_positionDial_valueChanged(int value)
{
    if (positionValueChangedBy != "SpinBox") //do not change the spinbox if the spinbox triggers the change
    {
        double result;
        result = ( value) * QSettings().value("frameRate").toInt() / 19;

        qDebug()<<"MainWindow::on_positionDial_valueChanged"<<value<<result<<positionValueChangedBy;

        positionValueChangedBy = "Dial";
        ui->videoWidget->m_positionSpinner->setValue(result);
        positionValueChangedBy = "";
    }
}

void MainWindow::showUpgradePrompt()
{
//    QSettings().setValue("checkUpgradeAutomatic", false);
    if (QSettings().value("checkUpgradeAutomatic").toBool())
    {
        ui->statusBar->showMessage("Checking for upgrade...", 15000);
        QNetworkRequest request(QUrl("http://www.actioncamvideocompanion.com/version.json"));
        QSslConfiguration sslConfig = request.sslConfiguration();
        sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
        request.setSslConfiguration(sslConfig);
        m_network.get(request);
    } else {
        m_network.setStrictTransportSecurityEnabled(false);
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Upgrade", tr("Click here to check for a new version of ACVC."), QMessageBox::Yes|QMessageBox::No);
        if (reply == QMessageBox::Yes)
            on_actionUpgrade_triggered();
    }
}

void MainWindow::on_actionUpgrade_triggered()
{
    if (QSettings().value("askUpgradeAutomatic", true).toBool())
    {
        QMessageBox dialog(QMessageBox::Question, qApp->applicationName(), tr("Do you want to automatically check for updates in the future?"), QMessageBox::No | QMessageBox::Yes, this);
        dialog.setWindowModality(Qt::ApplicationModal);//QmlApplication::dialogModality()
        dialog.setDefaultButton(QMessageBox::Yes);
        dialog.setEscapeButton(QMessageBox::No);
        dialog.setCheckBox(new QCheckBox(tr("Do not show this anymore.", "Automatic upgrade check dialog")));
        QSettings().setValue("checkUpgradeAutomatic", dialog.exec() == QMessageBox::Yes);
        if (dialog.checkBox()->isChecked())
            QSettings().setValue("askUpgradeAutomatic", false);
    }
    ui->statusBar->showMessage("Checking for upgrade...", 15000);
    m_network.get(QNetworkRequest(QUrl("http://actioncamvideocompanion.com/version.json")));
}

void MainWindow::onUpgradeCheckFinished(QNetworkReply* reply)
{
    if (!reply->error()) {
        QByteArray response = reply->readAll();
//        qDebug() << "response: " << response;
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
                reply = QMessageBox::question(this, "Upgrade", tr("ACVC version %1 is available! Click here to get it.").arg(latest), QMessageBox::Yes|QMessageBox::No);
                if (reply == QMessageBox::Yes)
                    onUpgradeTriggered();

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
        onUpgradeTriggered();
    reply->deleteLater();
}

void MainWindow::onUpgradeTriggered()
{
    QDesktopServices::openUrl(QUrl(m_upgradeUrl));
}

