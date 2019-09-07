#include "ffilestreeview.h"

#include <QHeaderView>
#include <QMenu>
#include <QSettings>

#include <QDebug>

static const int orderBeforeLoadIndex = 0;
static const int orderAtLoadIndex = 1;
static const int orderAfterMovingIndex = 2;
static const int folderIndex = 3;
static const int fileIndex = 4;
static const int inIndex = 5;
static const int outIndex = 6;
static const int durationIndex = 7;
static const int ratingIndex = 8;
static const int repeatIndex = 9;
static const int hintIndex = 10;
static const int tagIndex = 11;

FFilesTreeView::FFilesTreeView(QWidget *parent) : QTreeView(parent)
{
    fileModel = new QFileSystemModel();

    QStringList filters;
    filters << "*.mp4"<<"*.jpg"<<"*.avi"<<"*.wmv";
    filters << "*.MP4"<<"*.JPG"<<"*.AVI"<<"*.WMV";
    fileModel->setNameFilters(filters);
    fileModel->setNameFilterDisables(false);

    setModel(fileModel);
    setColumnWidth(0,columnWidth(0) * 4);
    expandAll();
    show();
    header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    setSelectionMode(QAbstractItemView::ContiguousSelection);

    connect( this, &QTreeView::clicked, this, &FFilesTreeView::onIndexClicked);

//    QString lastFolder = QSettings().value("LastFolder").toString();
//    if (lastFolder != ""  && lastFolder.length()>3) //not the root folder
//    {
//        loadModel(QSettings().value("LastFolder").toString());
//    }

    //https://code-examples.net/en/q/152b89b
    fileContextMenu = new QMenu(this);
//    setfileContextMenuPolicy(Qt::ActionsfileContextMenu);
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &FFilesTreeView::customContextMenuRequested, this, &FFilesTreeView::this_customContextMenuRequested);

//    QColor darkColorAlt = QColor(45,90,45);
//    QPalette palette = fileContextMenu->palette();
//    palette.setColor(QPalette::Background, darkColorAlt);

    fileContextMenu->addAction(new QAction("Trim",fileContextMenu));
    fileContextMenu->actions().last()->setToolTip("tip");
    connect(fileContextMenu->actions().last(), &QAction::triggered, this, &FFilesTreeView::onTrim);

    fileContextMenu->addAction(new QAction("Rename",fileContextMenu));
//    connect(fileContextMenu->actions().last(), SIGNAL(triggered()), this, SLOT(onFileDelete()));

    fileContextMenu->addAction(new QAction("Delete",fileContextMenu));
    connect(fileContextMenu->actions().last(), &QAction::triggered, this, &FFilesTreeView::onFileDelete);

    fileContextMenu->addSeparator();

    fileContextMenu->addAction(new QAction("Properties",fileContextMenu));
    connect(fileContextMenu->actions().last(), &QAction::triggered, this, &FFilesTreeView::onProperties);


}

void FFilesTreeView::onIndexClicked(QModelIndex index)
{
    qDebug()<<"FFilesTreeView::onIndexClicked"<<index.data()<<fileModel->index(index.row(),0).data();
//    if (editItemModel->index(index.row(),fileIndex).data().toString()!=fileUrl.fileName())
//    {
//        qDebug()<<"tableClicked different!!!"<<index.data()<<editItemModel->index(index.row(),fileIndex).data()<<fileUrl.fileName();
//    }
    emit indexClicked(index);
}

void FFilesTreeView::loadModel(QUrl folderUrl)
{
    fileModel->setRootPath(folderUrl.toString());
    setRootIndex(fileModel->index(folderUrl.toString()));

}

void FFilesTreeView::this_customContextMenuRequested(const QPoint &point)
{

    QModelIndex index = indexAt(point);
//    qDebug()<<"onFileRightClickMenu"<<point;
        if (index.isValid() ) {
            fileContextMenu->exec(viewport()->mapToGlobal(point));
        }
}

void FFilesTreeView::onTrim()
{

}

void FFilesTreeView::onFileDelete()
{

}

void FFilesTreeView::onProperties()
{

}

void FFilesTreeView::onFolderIndexClicked(QModelIndex index)
{
    QString lastFolder = QSettings().value("LastFolder").toString();
    qDebug()<<"FFilesTreeView::onFolderIndexClicked"<<index.data().toString()<<lastFolder;
    loadModel(lastFolder);
}


void FFilesTreeView::onEditIndexClicked(QModelIndex index)
{
//    qDebug()<<"FFilesTreeView::onEditIndexClicked"<<index;
    QString folderName = index.model()->index(index.row(),folderIndex).data().toString();
    QString fileName = index.model()->index(index.row(),fileIndex).data().toString();
    QModelIndex modelIndex = fileModel->index(folderName + fileName, 0);
    qDebug()<<"FFilesTreeView::onEditIndexClicked"<<index.data().toString()<<fileName<<modelIndex.data().toString();
    setCurrentIndex(modelIndex); //does also the scrollTo
}
