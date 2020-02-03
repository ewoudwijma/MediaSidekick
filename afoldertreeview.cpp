#include "afoldertreeview.h"

#include <QHeaderView>
#include <QSettings>

#include <QDebug>

AFolderTreeView::AFolderTreeView(QWidget *parent) : QTreeView(parent)
{
    directoryModel = new QFileSystemModel();
//    qDebug()<<"AFolderTreeView::AFolderTreeView"<<directoryModel->myComputer();
    directoryModel->setFilter(QDir::Dirs|QDir::Drives|QDir::NoDotAndDotDot|QDir::AllDirs);
//    directoryModel->setFilter(QDir::AllEntries);

#ifdef Q_OS_MAC
    directoryModel->setRootPath(QDir::home().homePath());
    setModel(directoryModel);
    setRootIndex(directoryModel->index(QDir::home().homePath()));
#else
    directoryModel->setRootPath("");
    setModel(directoryModel);
#endif
    setColumnWidth(0,columnWidth(0) * 4);
    this->header()->setSectionResizeMode(QHeaderView::ResizeToContents);

    connect( this, &QTreeView::clicked, this, &AFolderTreeView::onIndexClicked);


    QString lastFolder = QSettings().value("LastFolder").toString();
    if (lastFolder != ""  && lastFolder.length() > 4) //not the root folder
        setCurrentIndex(directoryModel->index(lastFolder)); //does also the scrollTo
}

void AFolderTreeView::onIndexClicked(const QModelIndex &index)
{
    QModelIndex selectedIndex = index;
    if (selectedIndex == QModelIndex())
        selectedIndex = currentIndex();

    if (isExpanded(selectedIndex))
        collapse(selectedIndex);
    else
        expand(selectedIndex);

    QString lastFolder = directoryModel->fileInfo(selectedIndex).absoluteFilePath() + "/";
//    qDebug()<<"AFolderTreeView::onIndexClicked"<<selectedIndex.data().toString()<<lastFolder;
    QSettings().setValue("LastFolder", lastFolder);
    QSettings().sync();

    if (lastFolder != ""  && lastFolder.length()>4) //not the root folder
    {
        emit indexClicked(selectedIndex);
    }
}
