#include "ffilestreeview.h"

#include <QHeaderView>
#include <QMenu>
#include <QMessageBox>
#include <QSettings>

#include <QDebug>

#include "fglobal.h"

FFilesTreeView::FFilesTreeView(QWidget *parent) : QTreeView(parent)
{
    fileModel = new QFileSystemModel();

    QStringList filters;
    filters << "*.mp4"<<"*.jpg"<<"*.avi"<<"*.wmv"<<"*.mts";
    filters << "*.MP4"<<"*.JPG"<<"*.AVI"<<"*.WMV"<<"*.MTS";
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
    connect(fileContextMenu->actions().last(), &QAction::triggered, this, &FFilesTreeView::onFileRename);

    fileContextMenu->addAction(new QAction("Delete",fileContextMenu));
    connect(fileContextMenu->actions().last(), &QAction::triggered, this, &FFilesTreeView::onFileDelete);

    fileContextMenu->addSeparator();
}

void FFilesTreeView::onIndexClicked(QModelIndex index)
{
    QFileInfo fileInfo = fileModel->fileInfo(index);
    QString filePath = fileInfo.absolutePath() + "/";

    QSettings().setValue("LastFileFolder", filePath);
    QSettings().sync();

    QModelIndexList indexList = selectionModel()->selectedIndexes();

    qDebug()<<"FFilesTreeView::onIndexClicked"<<index.row()<<index.column()<<index.data()<<filePath<<indexList.count();
//    if (editItemModel->index(index.row(),fileIndex).data().toString()!=fileUrl.fileName())
//    {
//        qDebug()<<"tableClicked different!!!"<<index.data()<<editItemModel->index(index.row(),fileIndex).data()<<fileUrl.fileName();
//    }
    emit indexClicked(index, selectionModel()->selectedIndexes());
}

void FFilesTreeView::loadModel(QUrl folderUrl)
{
    fileModel->setRootPath(folderUrl.toString());
    int indexOf = folderUrl.toString().lastIndexOf("/");
    QString folderName = folderUrl.toString().left(indexOf);
//    indexOf = folderName.lastIndexOf("/");
//    folderName = folderName.left(indexOf);

    qDebug()<<"FFilesTreeView::loadModel"<<folderUrl<<indexOf<<folderName;
    setRootIndex(fileModel->index(folderName));
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
    QModelIndexList indexList = selectionModel()->selectedIndexes();
    for (int i=0; i< indexList.count();i++)
    {
        if (indexList[i].column() == 0) //first column
        {
            emit trim(indexList[i].data().toString());
//            qDebug()<<"indexList[i].data()"<<indexList[i].row()<<indexList[i].column()<<indexList[i].data();

        }
    }
}

void FFilesTreeView::onFileRename()
{
    QStringList fileNameList;
    QStringList newFileNameList;
    QModelIndexList indexList = selectionModel()->selectedIndexes();
    for (int i=0; i< indexList.count();i++)
    {
        if (indexList[i].column() == 0) //first column
        {
            fileNameList << indexList[i].data().toString();

            QString *generatedName = new QString();
            emit getPropertyValue(indexList[i].data().toString(), "GeneratedName", generatedName);

            newFileNameList << *generatedName;
        }
    }

    QString folderName = QSettings().value("LastFolder").toString();

    QMessageBox::StandardButton reply;
     reply = QMessageBox::question(this, "Rename " + QString::number(fileNameList.count()) + " File(s)", "Are you sure you want to rename " + fileNameList.join(", ") + " and its supporting files (srt and txt) to " + newFileNameList.join(", ") + ".* ?",
                                   QMessageBox::Yes|QMessageBox::No);

     if (reply == QMessageBox::Yes)
     {

         for (int i=0; i< fileNameList.count();i++)
         {
//             QString fileName = fileNameList[i];
             emit fileDelete(fileNameList[i]); //to stop the video
             QFile file(folderName + fileNameList[i]);
             QString extensionString = fileNameList[i].mid(fileNameList[i].lastIndexOf(".")); //.avi ..mp4 etc.
             qDebug()<<"Rename"<<fileNameList[i]<<newFileNameList[i] + extensionString;
             if (file.exists())
                file.rename(folderName + newFileNameList[i] + extensionString);

             int lastIndex = fileNameList[i].lastIndexOf(".");
             if (lastIndex > -1)
             {
                 QFile *file = new QFile(folderName + fileNameList[i].left(fileNameList[i].lastIndexOf(".")) + ".srt");
                 if (file->exists())
                    file->rename(folderName + newFileNameList[i] + ".srt");

                 file = new QFile(folderName + fileNameList[i].left(fileNameList[i].lastIndexOf(".")) + ".txt");
                 if (file->exists())
                    file->rename(folderName + newFileNameList[i] + ".txt");
             }
         }
     }

     fileContextMenu->close();
}

void FFilesTreeView::onFileDelete()
{
    QStringList fileNameList;
    QModelIndexList indexList = selectionModel()->selectedIndexes();
    for (int i=0; i< indexList.count();i++)
    {
        if (indexList[i].column() == 0) //first column
        {
            fileNameList << indexList[i].data().toString();
        }
    }

    QString folderName = QSettings().value("LastFolder").toString();

    QMessageBox::StandardButton reply;
     reply = QMessageBox::question(this, "Delete " + QString::number(fileNameList.count()) + " File(s)", "Are you sure you want to PERMANENTLY delete " + fileNameList.join(", ") + " and its supporting files (srt and txt)?",
                                   QMessageBox::Yes|QMessageBox::No);

     if (reply == QMessageBox::Yes)
     {

         for (int i=0; i< fileNameList.count();i++)
         {
             QString fileName = fileNameList[i];
             emit fileDelete(fileName);
             QFile file(folderName + fileName);
             if (file.exists())
                file.remove();

             int lastIndex = fileName.lastIndexOf(".");
             if (lastIndex > -1)
             {
                 QString srtFileName = fileName.left(lastIndex) + ".srt";
                 QFile *file = new QFile(folderName + srtFileName);
                 if (file->exists())
                    file->remove();
                 srtFileName = fileName.left(lastIndex) + ".txt";
                 file = new QFile(folderName + srtFileName);
                 if (file->exists())
                    file->remove();
             }
         }
     }

     fileContextMenu->close();
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
