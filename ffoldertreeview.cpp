#include "ffoldertreeview.h"

#include <QHeaderView>
#include <QSettings>

#include <QDebug>

FFolderTreeView::FFolderTreeView(QWidget *parent) : QTreeView(parent)
{
    directoryModel = new QFileSystemModel();
    directoryModel->setRootPath("");
    directoryModel->setFilter(QDir::Dirs|QDir::Drives|QDir::NoDotAndDotDot|QDir::AllDirs);

    setModel(directoryModel);
    setColumnWidth(0,columnWidth(0) * 4);
    this->header()->setSectionResizeMode(QHeaderView::ResizeToContents);

    QString lastFolder = QSettings().value("LastFolder").toString();
    if (lastFolder != ""  && lastFolder.length()>3) //not the root folder
    {
        QModelIndex modelIndex;
        modelIndex = directoryModel->index(QSettings().value("LastFolder").toString());
//        qDebug()<<"modelIndex"<<QSettings().value("LastFolder").toString()<<modelIndex;
    //    ui->folderTreeView->expand(modelIndex);
    //    ui->folderTreeView->scrollTo(modelIndex);
        setCurrentIndex(modelIndex); //does also the scrollTo
//        on_folderTreeView_clicked(modelIndex);
    }

    connect( this, &QTreeView::clicked, this, &FFolderTreeView::onIndexClicked);

}

void FFolderTreeView::onIndexClicked(const QModelIndex &index)
{
    QModelIndex selectedIndex = index;
    if (selectedIndex == QModelIndex())
        selectedIndex = currentIndex();

    if (isExpanded(selectedIndex))
        collapse(selectedIndex);
    else
        expand(selectedIndex);

    QString lastFolder = directoryModel->fileInfo(selectedIndex).absoluteFilePath() + "/";
    qDebug()<<"FFolderTreeView::onIndexClicked"<<selectedIndex.data().toString()<<lastFolder;
    QSettings().setValue("LastFolder", lastFolder);
    QSettings().sync();

    if (lastFolder != ""  && lastFolder.length()>3) //not the root folder
    {
        qDebug()<<"emitttt";
        emit indexClicked(selectedIndex);
    }
}
