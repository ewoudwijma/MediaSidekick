#include "fvideowidget.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QStyleFactory>

#include <QMessageBox>
#include <QShortcut>
#include <QtDebug>

#include "fglobal.h"

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

//    ui->verticalLayout_player->insertWidget(1,ui->videoWidget->scrubBar);

//    ui->videoWidget = new FVideoWidget(this);

    QRect savedGeometry = QSettings().value("Geometry").toRect();
//    qDebug()<<"Constructor"<<savedGeometry<<geometry();
    if (savedGeometry != geometry())
    {
        setGeometry(savedGeometry);
    }

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
    connect(ui->filesTreeView, &FFilesTreeView::fileDelete, ui->videoWidget, &FVideoWidget::onFileDelete);
    ui->graphicsView->connectNodes("files", "video", "delete");
    connect(ui->filesTreeView, &FFilesTreeView::fileDelete, ui->editTableView, &FEditTableView::onFileDelete);
    ui->graphicsView->connectNodes("files", "edit", "delete");
    connect(ui->filesTreeView, &FFilesTreeView::trim, ui->editTableView, &FEditTableView::onTrim);
    ui->graphicsView->connectNodes("files", "edit", "trim");

    connect(ui->editTableView, &FEditTableView::folderIndexClickedItemModel, ui->tagsListView, &FTagsListView::onFolderIndexClicked);
    ui->graphicsView->connectNodes("edit", "tags", "folder");
    connect(ui->editTableView, &FEditTableView::folderIndexClickedProxyModel, ui->timelineWidget, &FTimeline::onFolderIndexClicked);
    ui->graphicsView->connectNodes("edit", "time", "folder");
    connect(ui->editTableView, &FEditTableView::fileIndexClicked, ui->videoWidget, &FVideoWidget::onFileIndexClicked);
    ui->graphicsView->connectNodes("edit", "video", "file");
    connect(ui->editTableView, &FEditTableView::fileIndexClicked, ui->timelineWidget, &FTimeline::onFileIndexClicked);
    ui->graphicsView->connectNodes("edit", "time", "file");
    connect(ui->editTableView, &FEditTableView::indexClicked, ui->filesTreeView, &FFilesTreeView::onEditIndexClicked);
    ui->graphicsView->connectNodes("edit", "files", "edit");
    connect(ui->editTableView, &FEditTableView::indexClicked, ui->videoWidget, &FVideoWidget::onEditIndexClicked);
    ui->graphicsView->connectNodes("edit", "video", "edit");
    connect(ui->editTableView, &FEditTableView::editsChanged, ui->videoWidget, &FVideoWidget::onEditsChanged);
    ui->graphicsView->connectNodes("edit", "video", "editchange");
    connect(ui->editTableView, &FEditTableView::editsChanged, ui->timelineWidget,  &FTimeline::onEditsChanged);
    ui->graphicsView->connectNodes("edit", "time", "editchange");
    connect(ui->editTableView, &FEditTableView::editsChanged, this,  &MainWindow::onEditsChanged);
    ui->graphicsView->connectNodes("edit", "main", "editchange");
    connect(ui->editTableView, &FEditTableView::editsChangedFromVideo, ui->timelineWidget,  &FTimeline::onEditsChanged);
    ui->graphicsView->connectNodes("edit", "time", "editchangevideo");
    connect(ui->editTableView, &FEditTableView::getPropertyValue, ui->propertyTreeView, &FPropertyTreeView::onGetPropertyValue);
    ui->graphicsView->connectNodes("edit", "prop", "get");

    connect(ui->videoWidget, &FVideoWidget::videoPositionChanged, ui->editTableView, &FEditTableView::onVideoPositionChanged);
    ui->graphicsView->connectNodes("video", "edit", "pos");
    connect(ui->videoWidget, &FVideoWidget::videoPositionChanged, ui->timelineWidget, &FTimeline::onVideoPositionChanged);
    ui->graphicsView->connectNodes("video", "time", "pos");
    connect(ui->videoWidget, &FVideoWidget::inChanged, ui->editTableView, &FEditTableView::onInChanged);
    ui->graphicsView->connectNodes("video", "edit", "in");
    connect(ui->videoWidget, &FVideoWidget::outChanged, ui->editTableView, &FEditTableView::onOutChanged);
    ui->graphicsView->connectNodes("video", "edit", "out");
    connect(ui->videoWidget, &FVideoWidget::getPropertyValue, ui->propertyTreeView, &FPropertyTreeView::onGetPropertyValue);
    ui->graphicsView->connectNodes("video", "prop", "get");

    connect(ui->timelineWidget, &FTimeline::timelinePositionChanged, ui->videoWidget, &FVideoWidget::onTimelinePositionChanged);
    ui->graphicsView->connectNodes("time", "video", "pos");

    connect(ui->propertyTreeView, &FPropertyTreeView::addLogEntry, ui->logTableView, &FLogTableView::onAddEntry);
    ui->graphicsView->connectNodes("video", "prop", "get");
    connect(ui->propertyTreeView, &FPropertyTreeView::addLogToEntry, ui->logTableView, &FLogTableView::onAddLogToEntry);

    connect(this, &MainWindow::propertyFilterChanged, ui->propertyTreeView, &FPropertyTreeView::onPropertyFilterChanged);
    ui->graphicsView->connectNodes("main", "prop", "filter");
    connect(this, &MainWindow::editFilterChanged, ui->editTableView, &FEditTableView::onEditFilterChanged);
    ui->graphicsView->connectNodes("main", "edit", "filter");

    connect(this, &MainWindow::timelineWidgetsChanged, ui->timelineWidget, &FTimeline::onTimelineWidgetsChanged);


    connect(ui->starEditorFilterWidget, &FStarEditor::editingFinished, this, &MainWindow::onEditFilterChanged);
    ui->graphicsView->connectNodes("star", "main", "filter");

    int starInt = QSettings().value("starsFilter").toInt();
    QVariant starVar = QVariant::fromValue(FStarRating(starInt));
    FStarRating starRating = qvariant_cast<FStarRating>(starVar);
    ui->starEditorFilterWidget->setStarRating(starRating);

    ui->transitionTimeSpinBox->setValue(QSettings().value("transitionTime").toInt());
    bool checked = QSettings().value("transitionTimeCheckBox").toBool();
    Qt::CheckState checkState;
    if (checked)
        checkState = Qt::Checked;
    else
        checkState = Qt::Unchecked;
    ui->TransitionTimeCheckBox->setCheckState(checkState);
    ui->stretchedDurationSpinBox->setValue(QSettings().value("stretchedDuration").toInt());
    checked = QSettings().value("stretchedDurationCheckBox").toBool();
    if (checked)
        checkState = Qt::Checked;
    else
        checked = Qt::Unchecked;
    ui->stretchedDurationCheckBox->setCheckState(checkState);

    onEditFilterChanged(); //initial setup
    emit timelineWidgetsChanged(ui->transitionTimeSpinBox->value(), ui->TransitionTimeCheckBox->checkState(), ui->stretchedDurationSpinBox->value(), ui->stretchedDurationCheckBox->checkState(), ui->editTableView);

    on_actionBlack_theme_triggered();

    ui->folderTreeView->onIndexClicked(QModelIndex()); //initial load

//    ui->tagsListView->loadModel(ui->editTableView->editItemModel);
    ui->defaultMinSpinBox->setValue(frameRate * 1);
    ui->defaultPlusSpinBox->setValue(frameRate * 2);

}

MainWindow::~MainWindow()
{
    qDebug()<<"Destructor"<<geometry();
//    QMessageLogger(QT_MESSAGELOG_FILE, QT_MESSAGELOG_LINE, QT_MESSAGELOG_FUNC).debug()<<"";
    QSettings().setValue("Geometry", geometry());
    QSettings().sync();

    delete ui;
}

void MainWindow::on_actionBlack_theme_triggered()
{
    qApp->setStyle(QStyleFactory::create("Fusion"));
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

}

void MainWindow::on_actionWhite_theme_triggered()
{
    qApp->setStyle(QStyleFactory::create("Fusion"));

    QColor mainColor = QColor(240,240,240);
    QColor disabledColor = QColor(127,127,127);
    QColor baseColor = QColor(255,255,255);
    QColor linkColor = QColor(0, 0, 255);
    QColor linkVisitedColor = QColor(255, 0, 255);
    QColor highlightColor = QColor(0,120,215);

    qDebug()<<""<<qApp->palette().color(QPalette::Window);
    qDebug()<<""<<qApp->palette().color(QPalette::Base);
    qDebug()<<""<<qApp->palette().color(QPalette::Disabled, QPalette::Text);
    qDebug()<<""<<qApp->palette().color(QPalette::Link);
    qDebug()<<""<<qApp->palette().color(QPalette::LinkVisited);
    qDebug()<<""<<qApp->palette().color(QPalette::Highlight);
    qDebug()<<""<<qApp->palette().color(QPalette::PlaceholderText);

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


}

void MainWindow::on_actionQuit_triggered()
{
    qApp->quit();
}

void MainWindow::on_actionAbout_triggered()
{
    QMessageBox::about(this, tr("About Fipre"),
            tr("<p><b>Fipre</b> is a video pre-editor/<p>"
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
    ui->editTableView->addEdit(ui->defaultMinSpinBox->value()*1000 / frameRate,ui->defaultPlusSpinBox->value()*1000 / frameRate);
}

void MainWindow::on_propertyFilterLineEdit_textChanged(const QString &)//arg1
{
    emit propertyFilterChanged(ui->propertyFilterLineEdit, ui->propertyDiffCheckBox);
}

void MainWindow::on_propertyDiffCheckBox_stateChanged(int )//arg1
{
    emit propertyFilterChanged(ui->propertyFilterLineEdit, ui->propertyDiffCheckBox);
}

void MainWindow::onEditsChanged(FEditSortFilterProxyModel *editProxyModel)
{
    ui->editrowsCounterLabel->setText(QString::number(editProxyModel->rowCount()) + " / " + QString::number(ui->editTableView->editItemModel->rowCount()));
}

void MainWindow::onEditFilterChanged()
{
    qDebug()<<"MainWindow::onEditFilterChanged";
    ui->editrowsCounterLabel->setText(QString::number(ui->editTableView->editProxyModel->rowCount()) + " / " + QString::number(ui->editTableView->editItemModel->rowCount()));
    emit editFilterChanged(ui->starEditorFilterWidget, ui->tagFilter1ListView, ui->tagFilter2ListView);
    QSettings().setValue("starsFilter", ui->starEditorFilterWidget->starRating().starCount());
    QSettings().sync();
}


void MainWindow::on_allCheckBox_clicked(bool checked)
{
    qDebug()<<"MainWindow::on_allCheckBox_clicked"<<checked;
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

void MainWindow::on_actionRepeat_triggered()
{
    ui->editTableView->toggleRepeat();
}

void MainWindow::on_actionSave_triggered()
{
    ui->editTableView->saveModel();
}

void MainWindow::on_actionPlay_Pause_triggered()
{
    ui->videoWidget->togglePlayPaused();
}

void MainWindow::on_actionNew_triggered()
{
    ui->editTableView->addEdit(ui->defaultMinSpinBox->value()*1000 / frameRate, ui->defaultPlusSpinBox->value()*1000 / frameRate);
}

void MainWindow::on_tagFilter1ListView_indexesMoved(const QModelIndexList &indexes)
{
    qDebug()<<"MainWindow::on_tagFilter1ListView_indexesMoved"<<indexes.count();
}

void MainWindow::on_transitionTimeSpinBox_valueChanged(int arg1)
{
    qDebug()<<"MainWindow::on_transitionTimeSpinBox_valueChanged"<<arg1;
    QSettings().setValue("transitionTime", arg1);
    QSettings().sync();
    emit timelineWidgetsChanged(ui->transitionTimeSpinBox->value(), ui->TransitionTimeCheckBox->checkState(), ui->stretchedDurationSpinBox->value(), ui->stretchedDurationCheckBox->checkState(), ui->editTableView);
}

void MainWindow::on_TransitionTimeCheckBox_clicked(bool checked)
{
    qDebug()<<"MainWindow::on_TransitionTimeCheckBox_clicked"<<checked;
    QSettings().setValue("transitionTimeCheckBox", checked);
    QSettings().sync();
    emit timelineWidgetsChanged(ui->transitionTimeSpinBox->value(), ui->TransitionTimeCheckBox->checkState(), ui->stretchedDurationSpinBox->value(), ui->stretchedDurationCheckBox->checkState(), ui->editTableView);

}

void MainWindow::on_stretchedDurationCheckBox_clicked(bool checked)
{
    qDebug()<<"MainWindow::on_stretchedDurationCheckBox_clicked"<<checked;
    QSettings().setValue("stretchedDurationCheckBox", checked);
    QSettings().sync();
    emit timelineWidgetsChanged(ui->transitionTimeSpinBox->value(), ui->TransitionTimeCheckBox->checkState(), ui->stretchedDurationSpinBox->value(), ui->stretchedDurationCheckBox->checkState(), ui->editTableView);

}

void MainWindow::on_stretchedDurationSpinBox_valueChanged(int arg1)
{
    qDebug()<<"MainWindow::on_stretchedDurationSpinBox_valueChanged"<<arg1;
    QSettings().setValue("stretchedDuration", arg1);
    QSettings().sync();
    emit timelineWidgetsChanged(ui->transitionTimeSpinBox->value(), ui->TransitionTimeCheckBox->checkState(), ui->stretchedDurationSpinBox->value(), ui->stretchedDurationCheckBox->checkState(), ui->editTableView);
}
