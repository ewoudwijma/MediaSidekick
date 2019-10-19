#include "fvideowidget.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QStyleFactory>

#include <QFileDialog>
#include <QMessageBox>
#include <QShortcut>
#include <QTimer>
#include <QtDebug>

#include "fglobal.h"

#include <QtMath>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->graphicsView->addNode("folder", 0, 0);
    ui->graphicsView->addNode("prop", 5, 0);
    ui->graphicsView->addNode("main", 8, 0);
    ui->graphicsView->addNode("tf1", 13, 0);

    ui->graphicsView->addNode("time", 2, 3);
    ui->graphicsView->addNode("video", 5, 3);
    ui->graphicsView->addNode("star", 11, 3);
    ui->graphicsView->addNode("tf2", 13, 3);

    ui->graphicsView->addNode("files", 0, 6);
    ui->graphicsView->addNode("edit", 8, 6);
    ui->graphicsView->addNode("tags", 13, 6);

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
    ui->graphicsView->connectNodes("tf1", "main", "filter");

    tagFilter2Model = new QStandardItemModel(this);
    ui->tagFilter2ListView->setModel(tagFilter2Model);
    ui->tagFilter2ListView->setFlow(QListView::LeftToRight);
    ui->tagFilter2ListView->setWrapping(true);
    ui->tagFilter2ListView->setDragDropMode(QListView::DragDrop);
    ui->tagFilter2ListView->setDefaultDropAction(Qt::MoveAction);
    ui->tagFilter2ListView->setSpacing(4);
    connect(tagFilter2Model, &QStandardItemModel::dataChanged,  this, &MainWindow::onEditFilterChanged);
    connect(tagFilter2Model, &QStandardItemModel::rowsRemoved,  this, &MainWindow::onEditFilterChanged); //datachanged not signalled when removing
    ui->graphicsView->connectNodes("tf2", "main", "filter");

//    ui->newTagLineEdit->setDragEnabled(true);
//    setDefaultDropAction(Qt::MoveAction);


//    ui->verticalLayout_player->insertWidget(1,ui->videoWidget->scrubBar);

    QRect savedGeometry = QSettings().value("Geometry").toRect();
    if (savedGeometry != geometry())
        setGeometry(savedGeometry);

    connect(ui->folderTreeView, &FFolderTreeView::indexClicked, ui->propertyTreeView, &FPropertyTreeView::onFolderIndexClicked);
    ui->graphicsView->connectNodes("folder", "prop", "folder");
    connect(ui->folderTreeView, &FFolderTreeView::indexClicked, ui->filesTreeView, &FFilesTreeView::onFolderIndexClicked);
    ui->graphicsView->connectNodes("folder", "files", "folder");
    connect(ui->folderTreeView, &FFolderTreeView::indexClicked, ui->editTableView, &FEditTableView::onFolderIndexClicked);
    ui->graphicsView->connectNodes("folder", "edit", "folder");
    connect(ui->folderTreeView, &FFolderTreeView::indexClicked, ui->videoWidget, &FVideoWidget::onFolderIndexClicked);
    ui->graphicsView->connectNodes("folder", "video", "folder");

    connect(ui->propertyTreeView, &FPropertyTreeView::propertiesLoaded, ui->editTableView, &FEditTableView::onPropertiesLoaded);

    connect(ui->filesTreeView, &FFilesTreeView::indexClicked, ui->editTableView, &FEditTableView::onFileIndexClicked);
    ui->graphicsView->connectNodes("files", "edit", "file");
    connect(ui->filesTreeView, &FFilesTreeView::fileDelete, ui->videoWidget, &FVideoWidget::onFileDelete); //stop and release
    ui->graphicsView->connectNodes("files", "video", "delete");
//    connect(ui->filesTreeView, &FFilesTreeView::fileRename, ui->videoWidget, &FVideoWidget::onFileRename); //stop and release
//    ui->graphicsView->connectNodes("files", "video", "rename");
    connect(ui->filesTreeView, &FFilesTreeView::fileDelete, ui->editTableView, &FEditTableView::onFileDelete); //remove from edits
    ui->graphicsView->connectNodes("files", "edit", "delete");
    connect(ui->filesTreeView, &FFilesTreeView::fileRename, ui->editTableView, &FEditTableView::onFileRename); //reload
    ui->graphicsView->connectNodes("files", "edit", "rename");
    connect(ui->filesTreeView, &FFilesTreeView::fileDelete, ui->propertyTreeView, &FPropertyTreeView::onFileDelete); //remove from column
    ui->graphicsView->connectNodes("files", "prop", "delete");
    connect(ui->filesTreeView, &FFilesTreeView::fileRename, ui->propertyTreeView, &FPropertyTreeView::onFileRename); //reload
    ui->graphicsView->connectNodes("files", "prop", "rename");
    connect(ui->filesTreeView, &FFilesTreeView::trim, ui->editTableView, &FEditTableView::onTrim);
    ui->graphicsView->connectNodes("files", "edit", "trim");
    connect(ui->filesTreeView, &FFilesTreeView::getPropertyValue, ui->propertyTreeView, &FPropertyTreeView::onGetPropertyValue);

    connect(ui->editTableView, &FEditTableView::folderIndexClickedItemModel, ui->tagsListView, &FTagsListView::onFolderIndexClicked);
    ui->graphicsView->connectNodes("edit", "tags", "folder");
    connect(ui->editTableView, &FEditTableView::folderIndexClickedProxyModel, ui->timelineWidget, &FTimeline::onFolderIndexClicked);
    ui->graphicsView->connectNodes("edit", "time", "folder");
    connect(ui->editTableView, &FEditTableView::folderIndexClickedProxyModel, this,  &MainWindow::onFolderIndexClicked); //to set edit counter
    ui->graphicsView->connectNodes("edit", "main", "folder");
    connect(ui->editTableView, &FEditTableView::fileIndexClicked, ui->videoWidget, &FVideoWidget::onFileIndexClicked); //setmedia
    ui->graphicsView->connectNodes("edit", "video", "file");
    connect(ui->editTableView, &FEditTableView::fileIndexClicked, ui->timelineWidget, &FTimeline::onFileIndexClicked); //currently nothing happens
    ui->graphicsView->connectNodes("edit", "time", "file");
    connect(ui->editTableView, &FEditTableView::fileIndexClicked, ui->propertyTreeView, &FPropertyTreeView::onFileIndexClicked);
    ui->graphicsView->connectNodes("edit", "prop", "file");
    connect(ui->editTableView, &FEditTableView::fileIndexClicked, this, &MainWindow::onFileIndexClicked); //trigger oneditfilterchanged
    ui->graphicsView->connectNodes("edit", "prop", "file");
    connect(ui->editTableView, &FEditTableView::indexClicked, ui->filesTreeView, &FFilesTreeView::onEditIndexClicked);
    ui->graphicsView->connectNodes("edit", "files", "edit");
    connect(ui->editTableView, &FEditTableView::indexClicked, ui->videoWidget, &FVideoWidget::onEditIndexClicked);
    ui->graphicsView->connectNodes("edit", "video", "edit");
    connect(ui->editTableView, &FEditTableView::indexClicked, ui->propertyTreeView, &FPropertyTreeView::onEditIndexClicked);
    ui->graphicsView->connectNodes("edit", "video", "edit");
    connect(ui->editTableView, &FEditTableView::editsChangedToVideo, ui->videoWidget, &FVideoWidget::onEditsChangedToVideo);
    ui->graphicsView->connectNodes("edit", "video", "editchangevideo");
    connect(ui->editTableView, &FEditTableView::editsChangedToVideo, this,  &MainWindow::onEditsChangedToVideo);
    ui->graphicsView->connectNodes("edit", "main", "editchangevideo");
    connect(ui->editTableView, &FEditTableView::editsChangedToTimeline, ui->timelineWidget,  &FTimeline::onEditsChangedToTimeline);
    ui->graphicsView->connectNodes("edit", "time", "editchangetimeline");
    connect(ui->editTableView, &FEditTableView::getPropertyValue, ui->propertyTreeView, &FPropertyTreeView::onGetPropertyValue);
//    ui->graphicsView->connectNodes("edit", "prop", "get");
    connect(ui->editTableView, &FEditTableView::addLogEntry, ui->logTableView, &FLogTableView::onAddEntry);
    ui->graphicsView->connectNodes("video", "prop", "get");
    connect(ui->editTableView, &FEditTableView::addLogToEntry, ui->logTableView, &FLogTableView::onAddLogToEntry);

    connect(ui->editTableView, &FEditTableView::trim, ui->propertyTreeView, &FPropertyTreeView::onTrim);

    connect(ui->editTableView, &FEditTableView::updateIn, ui->videoWidget, &FVideoWidget::onUpdateIn);
    connect(ui->editTableView, &FEditTableView::updateOut, ui->videoWidget, &FVideoWidget::onUpdateOut);

    connect(ui->editTableView, &FEditTableView::frameRateChanged, this, &MainWindow::onFrameRateChanged);

    connect(ui->videoWidget, &FVideoWidget::videoPositionChanged, ui->editTableView, &FEditTableView::onVideoPositionChanged);
    ui->graphicsView->connectNodes("video", "edit", "pos");
    connect(ui->videoWidget, &FVideoWidget::videoPositionChanged, ui->timelineWidget, &FTimeline::onVideoPositionChanged);
    ui->graphicsView->connectNodes("video", "time", "pos");
    connect(ui->videoWidget, &FVideoWidget::scrubberInChanged, ui->editTableView, &FEditTableView::onScrubberInChanged);
    ui->graphicsView->connectNodes("video", "edit", "in");
    connect(ui->videoWidget, &FVideoWidget::scrubberOutChanged, ui->editTableView, &FEditTableView::onScrubberOutChanged);
    ui->graphicsView->connectNodes("video", "edit", "out");
    connect(ui->videoWidget, &FVideoWidget::getPropertyValue, ui->propertyTreeView, &FPropertyTreeView::onGetPropertyValue);
//    ui->graphicsView->connectNodes("video", "prop", "get");

    connect(ui->timelineWidget, &FTimeline::timelinePositionChanged, ui->videoWidget, &FVideoWidget::onTimelinePositionChanged);
    ui->graphicsView->connectNodes("time", "video", "pos");
    connect(ui->timelineWidget, &FTimeline::getPropertyValue, ui->propertyTreeView, &FPropertyTreeView::onGetPropertyValue);
//    ui->graphicsView->connectNodes("time", "prop", "get");
    connect(ui->timelineWidget, &FTimeline::editsChangedToVideo, ui->videoWidget, &FVideoWidget::onEditsChangedToVideo);
    ui->graphicsView->connectNodes("time", "video", "editchangevideo");
    connect(ui->timelineWidget, &FTimeline::editsChangedToTimeline, this, &MainWindow::onEditsChangedToTimeline);
    ui->graphicsView->connectNodes("time", "main", "editchangetimeline");

    connect(ui->propertyTreeView, &FPropertyTreeView::addLogEntry, ui->logTableView, &FLogTableView::onAddEntry);
    ui->graphicsView->connectNodes("video", "prop", "get");
    connect(ui->propertyTreeView, &FPropertyTreeView::addLogToEntry, ui->logTableView, &FLogTableView::onAddLogToEntry);
    connect(ui->propertyTreeView, &FPropertyTreeView::fileDelete, ui->videoWidget, &FVideoWidget::onFileDelete); // on property change, stop video

    connect(this, &MainWindow::propertyFilterChanged, ui->propertyTreeView, &FPropertyTreeView::onPropertyFilterChanged);
    ui->graphicsView->connectNodes("main", "prop", "filter");
    connect(this, &MainWindow::editFilterChanged, ui->editTableView, &FEditTableView::onEditFilterChanged);
    ui->graphicsView->connectNodes("main", "edit", "filter");

    connect(this, &MainWindow::timelineWidgetsChanged, ui->timelineWidget, &FTimeline::onTimelineWidgetsChanged);

    connect(ui->generateWidget, &FGenerate::addLogEntry, ui->logTableView, &FLogTableView::onAddEntry);
    ui->graphicsView->connectNodes("video", "prop", "get");
    connect(ui->generateWidget, &FGenerate::addLogToEntry, ui->logTableView, &FLogTableView::onAddLogToEntry);
    connect(ui->generateWidget, &FGenerate::getPropertyValue, ui->propertyTreeView, &FPropertyTreeView::onGetPropertyValue);


    connect(ui->starEditorFilterWidget, &FStarEditor::editingFinished, this, &MainWindow::onEditFilterChanged);
    ui->graphicsView->connectNodes("star", "main", "filter");

    connect(ui->fileOnlyCheckBox, &QCheckBox::clicked, this, &MainWindow::onEditFilterChanged);
    connect(ui->alikeCheckBox, &QCheckBox::clicked, this, &MainWindow::onEditFilterChanged);

    //load settings

    int starInt = QSettings().value("starsFilter").toInt();
    QVariant starVar = QVariant::fromValue(FStarRating(starInt));
    FStarRating starRating = qvariant_cast<FStarRating>(starVar);
    ui->starEditorFilterWidget->setStarRating(starRating);

    ui->transitionTimeSpinBox->setValue(QSettings().value("transitionTime").toInt());
    ui->transitionComboBox->setCurrentText(QSettings().value("transitionType").toString());

//    ui->stretchedDurationSpinBox->blockSignals(true); // do not save immediately in on_stretchedDurationSpinBox_valueChanged
    ui->stretchedDurationSpinBox->setValue(QSettings().value("stretchedDuration").toInt());
//    ui->stretchedDurationSpinBox->blockSignals(false);

    ui->defaultMinSpinBox->setValue(QSettings().value("defaultMinDuration").toInt());
    ui->defaultPlusSpinBox->setValue(QSettings().value("defaultPlusDuration").toInt());

    ui->generateTargetComboBox->setCurrentText(QSettings().value("generateTarget").toString());
    ui->generateSizeComboBox->setCurrentText(QSettings().value("generateSize").toString());

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

    int lframeRate = QSettings().value("frameRate").toInt();
    if (lframeRate < 1)
            lframeRate = 25;
    ui->frameRateSpinBox->setValue(lframeRate);

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
    onEditFilterChanged(); //initial setup
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

    //tt edit
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
    ui->editTableView->setToolTip(tr("<p><b>Edit list</b></p>"
                                     "<p><i>Show the edits which apply to the filters set</i></p>"
                                     ));
    ui->editTableView->editItemModel->horizontalHeaderItem(alikeIndex)->setToolTip(ui->alikeCheckBox->toolTip());
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
    //end tooltips

    //do not show the graph tab (only in debug mode)
    graphWidget = ui->editTabWidget->widget(2);

    on_actionDebug_mode_triggered(false); //no debug mode

    ui->stretchDial->setNotchesVisible(true);

    QTimer::singleShot(0, this, [this]()->void
    {
                           QString theme = QSettings().value("theme").toString();
                           if (theme == "White")
                               on_actionWhite_theme_triggered();
                           else
                               on_actionBlack_theme_triggered();
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
    qDebug()<<"on_actionBlack_theme_triggered 1"<<qApp->style()<<QStyleFactory::keys();

    qApp->setStyle(QStyleFactory::create("Fusion"));
//    qApp->setPalette(QApplication::style()->standardPalette());
    qDebug()<<"on_actionBlack_theme_triggered 1.1";
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
    qDebug()<<"on_actionWhite_theme_triggered 1";
    qApp->setStyle(QStyleFactory::create("Fusion"));
    qDebug()<<"on_actionWhite_theme_triggered 1.1";

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
    QMessageBox::about(this, tr("About Fipre"),
            tr("<p><b>Fipre</b> is a video pre-editor</p>"
               "<p>Copyright (c)2019 Fipre.org</p>"
               "<p>This program proudly uses the following projects:</p>"
               "<ul>"
               "<li>Qt: application and UI framework</li>"
               "<li>Shotcut: Open source video editor</li>"
               "<li>FFMpeg</li>"
               "<li>Exiftool</li>"
               "</ul>"
               ));
}

void MainWindow::on_actionAbout_Qt_triggered()
{
    qApp->aboutQt();
}

void MainWindow::on_newEditButton_clicked()
{
    ui->editTableView->addEdit(ui->defaultMinSpinBox->value()*1000 / QSettings().value("frameRate").toInt(),ui->defaultPlusSpinBox->value()*1000 / QSettings().value("frameRate").toInt());
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
//    qDebug()<<"MainWindow::onFolderIndexClicked"<<itemModel->rowCount();

    ui->editrowsCounterLabel->setText(QString::number(itemModel->rowCount()) + " / " + QString::number(ui->editTableView->editItemModel->rowCount()));

//    ui->alikeCheckBox->setCheckState(Qt::Unchecked);
//    on_alikeCheckBox_clicked(false); //save in QSettings

//    tagFilter1Model->clear();
//    tagFilter2Model->clear();
//    onTagFiltersChanged();

//    ui->fileOnlyCheckBox->setCheckState(Qt::Unchecked);
//    on_fileOnlyCheckBox_clicked(false); //save in QSettings

//    onEditFilterChanged();
}

void MainWindow::onFileIndexClicked(QModelIndex index)
{
    onEditFilterChanged();
}

void MainWindow::onEditFilterChanged()
{
    ui->editrowsCounterLabel->setText(QString::number(ui->editTableView->model()->rowCount()) + " / " + QString::number(ui->editTableView->editItemModel->rowCount()));

    QSettings().setValue("starsFilter", ui->starEditorFilterWidget->starRating().starCount());

    onTagFiltersChanged(); //save tagfilters

    emit editFilterChanged(ui->starEditorFilterWidget, ui->alikeCheckBox, ui->tagFilter1ListView, ui->tagFilter2ListView, ui->fileOnlyCheckBox);
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

    QSettings().setValue("tagFilter1", string1);
    QSettings().setValue("tagFilter2", string2);
    QSettings().sync();
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
    ui->editTableView->addEdit(ui->defaultMinSpinBox->value()*1000 / QSettings().value("frameRate").toInt(), ui->defaultPlusSpinBox->value()*1000 / QSettings().value("frameRate").toInt());
}

void MainWindow::on_transitionTimeSpinBox_valueChanged(int arg1)
{
    if (QSettings().value("transitionTime") != arg1)
    {
        qDebug()<<"MainWindow::on_transitionTimeSpinBox_valueChanged"<<arg1;
        QSettings().setValue("transitionTime", arg1);
        QSettings().sync();
        emit timelineWidgetsChanged(ui->transitionTimeSpinBox->value(), ui->transitionComboBox->currentText(), ui->stretchedDurationSpinBox->value(), ui->stretchedDurationSpinBox->value() != ui->editTableView->originalDuration, ui->editTableView);
    }
}

void MainWindow::on_stretchedDurationSpinBox_valueChanged(int arg1)
{
    if (QSettings().value("stretchedDuration") != arg1)
    {
        qDebug()<<"MainWindow::on_stretchedDurationSpinBox_valueChanged"<<arg1<<ui->editTableView->originalDuration;
        QSettings().setValue("stretchedDuration", arg1);
        QSettings().sync();
        emit timelineWidgetsChanged(ui->transitionTimeSpinBox->value(), ui->transitionComboBox->currentText(), ui->stretchedDurationSpinBox->value(), ui->stretchedDurationSpinBox->value() != ui->editTableView->originalDuration, ui->editTableView);
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
    if (QSettings().value("generateTarget") != arg1)
    {
        QSettings().setValue("generateTarget", arg1);
        QSettings().sync();
    }
}

void MainWindow::on_generateSizeComboBox_currentTextChanged(const QString &arg1)
{
    if (QSettings().value("generateSize") != arg1)
    {
        QSettings().setValue("generateSize", arg1);
        QSettings().sync();
    }
}

void MainWindow::on_frameRateSpinBox_valueChanged(int arg1)
{
    if (QSettings().value("frameRate") != arg1)
    {
        QSettings().setValue("frameRate", arg1);
        QSettings().sync();
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
//    qDebug()<<"on_actionDebug_mode_triggered"<<checked;

    if (checked)
    {
        ui->editTabWidget->insertTab(2, graphWidget, "Graph");
    }
    else
    {
        ui->editTabWidget->removeTab(2);
    }

    ui->editTableView->setColumnHidden(orderBeforeLoadIndex, !checked);
    ui->editTableView->setColumnHidden(orderAtLoadIndex, !checked);
    ui->editTableView->setColumnHidden(orderAfterMovingIndex, !checked);
    ui->editTableView->setColumnHidden(fpsIndex, !checked);
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
    qDebug()<<"MainWindow::on_stretchedCheckBox_clicked"<<ui->editTableView->editTriggers();
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
    qDebug()<<"MainWindow::on_stretchedCheckBox_clicked"<<ui->editTableView->editTriggers();
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
    if (QSettings().value("transitionType") != arg1)
    {
        qDebug()<<"MainWindow::on_transitionComboBox_currentTextChanged"<<arg1;
        QSettings().setValue("transitionType", arg1);
        QSettings().sync();
        emit timelineWidgetsChanged(ui->transitionTimeSpinBox->value(), ui->transitionComboBox->currentText(), ui->stretchedDurationSpinBox->value(), ui->stretchedDurationSpinBox->value() != ui->editTableView->originalDuration, ui->editTableView);
    }
}

void MainWindow::on_stretch100Button_clicked()
{
//    ui->stretchedDurationSpinBox->setValue(ui->editTableView->originalDuration);
    ui->stretchDial->setValue(50);
    emit timelineWidgetsChanged(ui->transitionTimeSpinBox->value(), ui->transitionComboBox->currentText(), ui->stretchedDurationSpinBox->value(), ui->stretchedDurationSpinBox->value() != ui->editTableView->originalDuration, ui->editTableView);
}

void MainWindow::on_stretchDial_valueChanged(int value)
{
    double result;
    if (value < 50)
        result = qSin(M_PI * value / 100);
    else
        result = 1 / qSin(M_PI * value / 100);

//    result = value / 50.0;
    qDebug()<<"MainWindow::on_stretchDial_valueChanged"<<value<<ui->editTableView->originalDuration<<result<<stretchValueChangedBy;

    if (stretchValueChangedBy != "SpinBox")
    {
        stretchValueChangedBy = "Dial";
        ui->stretchedDurationSpinBox->setValue(ui->editTableView->originalDuration * result);
        stretchValueChangedBy = "";
    }
    else
        emit timelineWidgetsChanged(ui->transitionTimeSpinBox->value(), ui->transitionComboBox->currentText(), ui->stretchedDurationSpinBox->value(), ui->stretchedDurationSpinBox->value() != ui->editTableView->originalDuration, ui->editTableView);
    ui->stretch100Button->setText(QString::number(ui->stretchedDurationSpinBox->value() * 100 / ui->editTableView->originalDuration, 'g', 3) + "%");
}

void MainWindow::onEditsChangedToTimeline(QAbstractItemModel *itemModel)
{
    if (ui->editTableView->originalDuration != 0 && stretchValueChangedBy != "Dial")
    {
        stretchValueChangedBy = "SpinBox";
        qDebug()<<"MainWindow::onEditsChangedToTimeline"<<ui->stretchedDurationSpinBox->value() << ui->editTableView->originalDuration<<ui->stretchedDurationSpinBox->value() * 100 / ui->editTableView->originalDuration;

        double result = ui->stretchedDurationSpinBox->value() * 50.0/ ui->editTableView->originalDuration;

        if (ui->stretchedDurationSpinBox->value() / double(ui->editTableView->originalDuration) < 1)
            result = qAsin(ui->stretchedDurationSpinBox->value() / double(ui->editTableView->originalDuration)) * 100 / M_PI;
        else
            result = 100 - qAsin(double(ui->editTableView->originalDuration) / double( ui->stretchedDurationSpinBox->value())) * 100 / M_PI;
//        qDebug()<<"MainWindow::onEditsChangedToTimeline"<<ui->stretchedDurationSpinBox->value() << ui->stretchedDurationSpinBox->value() * 50.0 << result<<ui->stretchedDurationSpinBox->value() /  double(ui->editTableView->originalDuration)<<double(ui->editTableView->originalDuration) / double( ui->stretchedDurationSpinBox->value());

        ui->stretchDial->setValue(result );
        ui->stretch100Button->setText(QString::number(ui->stretchedDurationSpinBox->value() * 100 / ui->editTableView->originalDuration, 'g', 3) + "%");
        stretchValueChangedBy = "";
    }
}

void MainWindow::on_stretchDial_sliderMoved(int position)
{
    int value = position;

}
