#include "fvideowidget.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QStyleFactory>

#include <QMessageBox>
#include <QShortcut>
#include <QtDebug>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

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

    tagFilter2Model = new QStandardItemModel(this);
    ui->tagFilter2ListView->setModel(tagFilter2Model);
    ui->tagFilter2ListView->setFlow(QListView::LeftToRight);
    ui->tagFilter2ListView->setWrapping(true);
    ui->tagFilter2ListView->setDragDropMode(QListView::DragDrop);
    ui->tagFilter2ListView->setDefaultDropAction(Qt::MoveAction);
    ui->tagFilter2ListView->setSpacing(4);
    connect(tagFilter2Model, &QStandardItemModel::dataChanged,  this, &MainWindow::onEditFilterChanged);

//    ui->verticalLayout_player->insertWidget(1,ui->videoWidget->scrubBar);

//    ui->videoWidget = new FVideoWidget(this);

    QRect savedGeometry = QSettings().value("Geometry").toRect();
//    qDebug()<<"Constructor"<<savedGeometry<<geometry();
    if (savedGeometry != geometry())
    {
        setGeometry(savedGeometry);
    }

    connect(ui->folderTreeView, &FFolderTreeView::indexClicked, ui->filesTreeView, &FFilesTreeView::onFolderIndexClicked);
    connect(ui->folderTreeView, &FFolderTreeView::indexClicked, ui->editTableView, &FEditTableView::onFolderIndexClicked);
    connect(ui->folderTreeView, &FFolderTreeView::indexClicked, ui->propertyTreeView, &FPropertyTreeView::onFolderIndexClicked);
    connect(ui->folderTreeView, &FFolderTreeView::indexClicked, ui->videoWidget, &FVideoWidget::onFolderIndexClicked);

    connect(ui->filesTreeView, &FFilesTreeView::indexClicked, ui->editTableView, &FEditTableView::onFileIndexClicked);

    connect(ui->editTableView, &FEditTableView::folderIndexClickedItemModel, ui->tagsListView, &FTagsListView::onFolderIndexClicked);
    connect(ui->editTableView, &FEditTableView::folderIndexClickedProxyModel, ui->timelineWidget, &FTimeline::onFolderIndexClicked);
    connect(ui->editTableView, &FEditTableView::fileIndexClicked, ui->videoWidget, &FVideoWidget::onFileIndexClicked);
    connect(ui->editTableView, &FEditTableView::fileIndexClicked, ui->timelineWidget, &FTimeline::onFileIndexClicked);
    connect(ui->editTableView, &FEditTableView::indexClicked, ui->filesTreeView, &FFilesTreeView::onEditIndexClicked);
    connect(ui->editTableView, &FEditTableView::indexClicked, ui->videoWidget, &FVideoWidget::onEditIndexClicked);
    connect(ui->editTableView, &FEditTableView::editsChanged, ui->videoWidget, &FVideoWidget::onEditsChanged);
    connect(ui->editTableView, &FEditTableView::editsChanged, ui->timelineWidget,  &FTimeline::onEditsChanged);
    connect(ui->editTableView, &FEditTableView::editsChangedFromVideo, ui->timelineWidget,  &FTimeline::onEditsChanged);

    connect(ui->videoWidget, &FVideoWidget::positionChanged, ui->editTableView, &FEditTableView::onPositionChanged);
    connect(ui->videoWidget, &FVideoWidget::positionChanged, ui->timelineWidget, &FTimeline::onPositionChanged);
    connect(ui->videoWidget, &FVideoWidget::inChanged, ui->editTableView, &FEditTableView::onInChanged);
    connect(ui->videoWidget, &FVideoWidget::outChanged, ui->editTableView, &FEditTableView::onOutChanged);

    connect(this, &MainWindow::propertyFilterChanged, ui->propertyTreeView, &FPropertyTreeView::onPropertyFilterChanged);
    connect(this, &MainWindow::editFilterChanged, ui->editTableView, &FEditTableView::onEditFilterChanged);


    connect(ui->starEditorFilterWidget, &FStarEditor::editingFinished, this, &MainWindow::onEditFilterChanged);

    int starInt = QSettings().value("starsFilter").toInt();
    QVariant starVar = QVariant::fromValue(FStarRating(starInt));
    FStarRating starRating = qvariant_cast<FStarRating>(starVar);

    ui->starEditorFilterWidget->setStarRating(starRating);

    on_actionBlack_theme_triggered();

    ui->folderTreeView->onIndexClicked(QModelIndex()); //initial load

//    ui->tagsListView->loadModel(ui->editTableView->editItemModel);

}

MainWindow::~MainWindow()
{
    qDebug()<<"Destructor"<<geometry();
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
    ui->editTableView->addEdit(400,800);
}

void MainWindow::on_propertyFilterLineEdit_textChanged(const QString &)//arg1
{
    emit propertyFilterChanged(ui->propertyFilterLineEdit, ui->propertyDiffCheckBox);
}

void MainWindow::on_propertyDiffCheckBox_stateChanged(int )//arg1
{
    emit propertyFilterChanged(ui->propertyFilterLineEdit, ui->propertyDiffCheckBox);
}

void MainWindow::onEditFilterChanged()
{
    qDebug()<<"MainWindow::onEditFilterChanged";
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
