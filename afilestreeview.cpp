#include "afilestreeview.h"

#include <QDesktopServices>
#include <QHeaderView>
#include <QMenu>
#include <QMessageBox>
#include <QProcess>
#include <QSettings>
#include <QTime>

#include <QDebug>

#include "aglobal.h"

#include <QApplication>

AFilesTreeView::AFilesTreeView(QWidget *parent) : QTreeView(parent)
{
    fileModel = new QFileSystemModel();

    fileModel->setNameFilterDisables(false);

    filesProxyModel = new AFilesSortFilterProxyModel(this);
    filesProxyModel->setSourceModel(fileModel);
    setModel(filesProxyModel);

//    setModel(fileModel);
    setColumnWidth(0,columnWidth(0) * 4);
    expandAll();
    show();
    header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    setSelectionMode(QAbstractItemView::ExtendedSelection);

    connect(this, &QTreeView::clicked, this, &AFilesTreeView::onIndexClicked);
    connect(this, &QTreeView::activated, this, &AFilesTreeView::onIndexActivated);

//    connect(fileModel, &QFileSystemModel::directoryLoaded, this, &AFilesTreeView::onModelLoaded);

    //https://code-examples.net/en/q/152b89b
}

void AFilesTreeView::setType(QString type)
{
    QString regExp = type;
    filesProxyModel->setFilterRegExp(QRegExp(regExp, Qt::CaseInsensitive,
                                                QRegExp::FixedString));
    filesProxyModel->setFilterKeyColumn(-1);

    if (type == "Video") //exclude lossless done in sortfilterproxymodel
    {
        QStringList videoFilters;
        foreach (QString extension, AGlobal().videoExtensions)
            videoFilters << "*." + extension;

        fileModel->setNameFilters(videoFilters);
    }
    else if (type == "Audio")
    {
        QStringList audioFilters;
        foreach (QString extension, AGlobal().audioExtensions)
            audioFilters << "*." + extension;
        foreach (QString extension, AGlobal().imageExtensions)
            audioFilters << "*." + extension;

        fileModel->setNameFilters(audioFilters);
    }
    else if (type == "Export")
    {
        QStringList exportFilters;
        foreach (QString extension, AGlobal().exportExtensions)
            exportFilters << "*." + extension;
        foreach (QString extension, AGlobal().projectExtensions)
            exportFilters << "*." + extension;

        QStringList exportMethodAndExtensionFilter;
        foreach (QString exportMethod, AGlobal().exportMethods)
            foreach (QString exportFilter, exportFilters)
                exportMethodAndExtensionFilter <<exportMethod + exportFilter;
//        filters <<"Lossless*.*"<<"Encode*.*"<<"shotcut*.*"<<"Premiere*.*";
        fileModel->setNameFilters(exportMethodAndExtensionFilter);
    }
}

void AFilesTreeView::onIndexClicked(QModelIndex index)
{
    QModelIndex fileIndex = index.model()->index(index.row(), 0, index.parent());

    QModelIndex fileModelIndex = filesProxyModel->mapToSource(fileIndex);

    QFileInfo fileInfo = fileModel->fileInfo(fileModelIndex);
    QString filePath = fileInfo.absolutePath() + "/";

//    qDebug()<<"AFilesTreeView::onIndexClicked"<<index.parent().data().toString()<<index.data().toString()<<fileInfo<<filePath;

    QStringList filePathList;
    QModelIndexList indexList = selectionModel()->selectedIndexes();
    for (int i=0; i< indexList.count();i++)
    {
        if (indexList[i].column() == 0) //first column => name!!!
        {
            QString fileName = indexList[i].data().toString();
            filePathList << fileModel->filePath( filesProxyModel->mapToSource(indexList[i]));
        }
    }

    QString folderFileNameLow = fileIndex.data().toString().toLower();
    int lastIndexOf = folderFileNameLow.lastIndexOf(".");
    QString extension = folderFileNameLow.mid(lastIndexOf + 1);

    if (!AGlobal().projectExtensions.contains(extension, Qt::CaseInsensitive))
        emit fileIndexClicked(fileIndex, filePathList);
}

void AFilesTreeView::onIndexActivated(QModelIndex index)
{
//    qDebug()<<"AFilesTreeView::onIndexActivated"<<index.row()<<index.column()<<index.data().toString();
    onIndexClicked(index);
}

void AFilesTreeView::loadModel(QUrl folderUrl)
{
//    qDebug()<<"AFilesTreeView::loadModel"<<folderUrl.toString();

    fileModel->setRootPath(folderUrl.toString());
    setRootIndex(filesProxyModel->mapFromSource(fileModel->index(folderUrl.toString())));
}

void AFilesTreeView::onFolderSelected(QString folderName)
{
//    qDebug()<<"AFilesTreeView::onFolderSelected"<<folderName;
    setCurrentIndex(QModelIndex());
    loadModel(folderName);
}

void AFilesTreeView::onClipIndexClicked(QModelIndex index)
{
//    qDebug()<<"AFilesTreeView::onClipIndexClicked"<<index;
    QString folderName = index.model()->index(index.row(),folderIndex).data().toString();
    QString fileName = index.model()->index(index.row(),fileIndex).data().toString();
    QModelIndex modelIndex = fileModel->index(folderName + fileName, 0);
//    qDebug()<<"AFilesTreeView::onClipIndexClicked"<<index.data().toString()<<fileName<<modelIndex.data().toString();
    setCurrentIndex(filesProxyModel->mapFromSource(modelIndex)); //does also the scrollTo
}

QModelIndex recursiveFirstFile(QFileSystemModel *fileModel, QModelIndex parentIndex)
{
    QModelIndex fileIndex = QModelIndex();
    for (int childRow=0;childRow<fileModel->rowCount(parentIndex);childRow++)
    {
        QModelIndex childIndex = fileModel->index(childRow, 0, parentIndex);

        QString fileNameLow = childIndex.data().toString().toLower();
        int lastIndexOf = fileNameLow.lastIndexOf(".");
        QString extension = fileNameLow.mid(lastIndexOf + 1);

        if (AGlobal().videoExtensions.contains(extension, Qt::CaseInsensitive))
            return childIndex;

        fileIndex = recursiveFirstFile(fileModel, childIndex);
    }
    return fileIndex;
}

//void AFilesTreeView::onModelLoaded(const QString &)//path
//{
//    if (filesProxyModel->filterRegExp().pattern() == "Video")
//        qDebug()<<"AFilesTreeView::onModelLoaded"<<filesProxyModel->filterRegExp().pattern();

//    setCurrentIndex(QModelIndex());

//    return; //tbd: find out why not enabled

//    QModelIndex fileIndex = QModelIndex();

//    QString lastFile = QSettings().value("LastFile").toString();
//    if (lastFile != "" ) //not the root folder
//    {
//        QString lastFileFolder = QSettings().value("LastFileFolder").toString();
//        fileIndex = fileModel->index(lastFileFolder + lastFile, 0);
//    }
//    else
//    {
//        QModelIndex parentIndex = rootIndex();
//        fileIndex = recursiveFirstFile(fileModel, parentIndex);
//    }

//    if (fileIndex != QModelIndex())
//    {
//        onIndexClicked(filesProxyModel->mapFromSource(fileIndex));
//        setCurrentIndex(filesProxyModel->mapFromSource(fileIndex));
//    }
//}

