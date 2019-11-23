#include "ffilestreeview.h"

#include <QHeaderView>
#include <QMenu>
#include <QMessageBox>
#include <QSettings>

#include <QDebug>

#include "fglobal.h"

AFilesTreeView::AFilesTreeView(QWidget *parent) : QTreeView(parent)
{
    fileModel = new QFileSystemModel();

    QStringList filters;
    filters << "*.mp4"<<"*.jpg"<<"*.avi"<<"*.wmv"<<"*.mts"<<"shotcut*.mlt"<<"Premiere*.xml";
    filters << "*.MP4"<<"*.JPG"<<"*.AVI"<<"*.WMV"<<"*.MTS";
    fileModel->setNameFilters(filters);
    fileModel->setNameFilterDisables(false);

    setModel(fileModel);
    setColumnWidth(0,columnWidth(0) * 4);
    expandAll();
    show();
    header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    setSelectionMode(QAbstractItemView::ExtendedSelection);

    connect( this, &QTreeView::clicked, this, &AFilesTreeView::onIndexClicked);
    connect( this, &QTreeView::activated, this, &AFilesTreeView::onIndexActivated);

    connect(fileModel, &QFileSystemModel::directoryLoaded, this, &AFilesTreeView::onDirectoryLoaded);

//    QString lastFolder = QSettings().value("LastFolder").toString();
//    if (lastFolder != ""  && lastFolder.length()>3) //not the root folder
//    {
//        loadModel(QSettings().value("LastFolder").toString());
//    }

    //https://code-examples.net/en/q/152b89b
    fileContextMenu = new QMenu(this);
//    setfileContextMenuPolicy(Qt::ActionsfileContextMenu);
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &AFilesTreeView::customContextMenuRequested, this, &AFilesTreeView::this_customContextMenuRequested);

//    QColor darkColorAlt = QColor(45,90,45);
//    QPalette palette = fileContextMenu->palette();
//    palette.setColor(QPalette::Background, darkColorAlt);

    fileContextMenu->addAction(new QAction("Trim",fileContextMenu));
    fileContextMenu->actions().last()->setToolTip("tip");
    connect(fileContextMenu->actions().last(), &QAction::triggered, this, &AFilesTreeView::onTrim);

    fileContextMenu->addAction(new QAction("Rename",fileContextMenu));
    connect(fileContextMenu->actions().last(), &QAction::triggered, this, &AFilesTreeView::onFileRename);

    fileContextMenu->addAction(new QAction("Delete file(s)",fileContextMenu));
    connect(fileContextMenu->actions().last(), &QAction::triggered, this, &AFilesTreeView::onFileDelete);

    fileContextMenu->addAction(new QAction("Delete clips",fileContextMenu));
    connect(fileContextMenu->actions().last(), &QAction::triggered, this, &AFilesTreeView::onClipsDelete);

//    fileContextMenu->addAction(new QAction("Superview",fileContextMenu));
//    connect(fileContextMenu->actions().last(), &QAction::triggered, this, &AFilesTreeView::onSuperview);

    fileContextMenu->addSeparator();
}

void AFilesTreeView::onIndexClicked(QModelIndex index)
{
    QFileInfo fileInfo = fileModel->fileInfo(index);
    QString filePath = fileInfo.absolutePath() + "/";

    QSettings().setValue("LastFileFolder", filePath);
    QSettings().sync();

    QModelIndexList indexList = selectionModel()->selectedIndexes();

//    qDebug()<<"AFilesTreeView::onIndexClicked"<<index.row()<<index.column()<<index.data().toString()<<filePath<<indexList.count();
//    if (clipsItemModel->index(index.row(),fileIndex).data().toString()!=fileUrl.fileName())
//    {
//        qDebug()<<"tableClicked different!!!"<<index.data()<<clipsItemModel->index(index.row(),fileIndex).data()<<fileUrl.fileName();
//    }
    if (!index.data().toString().contains(".mlt") && !index.data().toString().contains(".xml"))
        emit indexClicked(index, selectionModel()->selectedIndexes());
}

void AFilesTreeView::onIndexActivated(QModelIndex index)
{
//    qDebug()<<"AFilesTreeView::onIndexActivated"<<index.row()<<index.column()<<index.data().toString();
    onIndexClicked(index);
}

void AFilesTreeView::loadModel(QUrl folderUrl)
{
    fileModel->setRootPath(folderUrl.toString());
    int indexOf = folderUrl.toString().lastIndexOf("/");
    QString folderName = folderUrl.toString().left(indexOf);
//    indexOf = folderName.lastIndexOf("/");
//    folderName = folderName.left(indexOf);

//    qDebug()<<"AFilesTreeView::loadModel"<<folderUrl<<indexOf<<folderName;
    setRootIndex(fileModel->index(folderName));
}

void AFilesTreeView::this_customContextMenuRequested(const QPoint &point)
{
    QModelIndex index = indexAt(point);
//    qDebug()<<"onFileRightClickMenu"<<point;
        if (index.isValid() ) {
            fileContextMenu->exec(viewport()->mapToGlobal(point));
        }
}

void AFilesTreeView::onTrim()
{
    QModelIndexList indexList = selectionModel()->selectedIndexes();
    bool somethingTrimmed = false;
    for (int i=0; i< indexList.count();i++)
    {
        if (indexList[i].column() == 0) //first column
        {
            QString fileName = indexList[i].data().toString();
            if (!fileName.contains(".mlt") && !fileName.contains("*.xml"))
            {
                emit trim(fileName);
                somethingTrimmed = true;
            }
//            qDebug()<<"indexList[i].data()"<<indexList[i].row()<<indexList[i].column()<<indexList[i].data();
        }
    }

    if (!somethingTrimmed)
            QMessageBox::information(this, "Trim", "Nothing to do");

}

void AFilesTreeView::onFileRename()
{
    QStringList fileNameList;
    QStringList newFileNameList;
    QStringList noSuggestedList;
    QModelIndexList indexList = selectionModel()->selectedIndexes();

    for (int i=0; i< indexList.count();i++)
    {
        if (indexList[i].column() == 0) //first column
        {
            QString fileName = indexList[i].data().toString();
            if (!fileName.contains(".mlt") && !fileName.contains("*.xml"))
            {
                QString *suggestedName = new QString();
                emit getPropertyValue(fileName, "SuggestedName", suggestedName);

                if (*suggestedName != "")
                {
                    fileNameList << fileName;
                    newFileNameList << *suggestedName;
                }
                else
                    noSuggestedList << fileName;
            }
        }
    }

    if (noSuggestedList.count() == 0 && fileNameList.count() > 0)
    {
        QString folderName = QSettings().value("LastFolder").toString();

        QMessageBox::StandardButton reply;
         reply = QMessageBox::question(this, "Rename " + QString::number(fileNameList.count()) + " File(s)", "Are you sure you want to rename " + fileNameList.join(", ") + " and its supporting files (srt and txt) to " + newFileNameList.join(", ") + ".* ?",
                                       QMessageBox::Yes|QMessageBox::No);

         if (reply == QMessageBox::Yes)
         {

             for (int i=0; i< fileNameList.count();i++)
             {
    //             QString fileName = fileNameList[i];
                 emit fileDelete(fileNameList[i]); //to stop the video (tbd:but not to remove the clips!!!)
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
             emit fileRename();
         }
    }
    else
    {
        if (noSuggestedList.count() > 0)
            QMessageBox::information(this, "Rename", "No valid suggested name for the following files (see properties tab): " + noSuggestedList.join(", "));
        else
            QMessageBox::information(this, "Rename", "Nothing to do");
    }

    fileContextMenu->close();
}

void AFilesTreeView::onFileDelete()
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

    if (fileNameList.count()>0)
    {
        QString folderName = QSettings().value("LastFolder").toString();

        QMessageBox::StandardButton reply;
         reply = QMessageBox::question(this, "Delete file(s)" + QString::number(fileNameList.count()) + " File(s)", "Are you sure you want to PERMANENTLY delete " + fileNameList.join(", ") + " and its supporting files (srt and txt)?",
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
    }

    fileContextMenu->close();
}

void AFilesTreeView::onClipsDelete()
{
    QStringList fileNameList;
    QModelIndexList indexList = selectionModel()->selectedIndexes();
    for (int i=0; i< indexList.count();i++)
    {
        if (indexList[i].column() == 0) //first column
        {
            QString fileName = indexList[i].data().toString();
            if (!fileName.contains(".mlt") && !fileName.contains("*.xml"))
                fileNameList << fileName;
        }
    }

    if (fileNameList.count()>0)
    {
        QString folderName = QSettings().value("LastFolder").toString();

        QMessageBox::StandardButton reply;
         reply = QMessageBox::question(this, "Delete clips" + QString::number(fileNameList.count()) + " File(s)", "Are you sure you want to PERMANENTLY delete  supporting files (srt and txt) of " + fileNameList.join(", ") + "?",
                                       QMessageBox::Yes|QMessageBox::No);

         if (reply == QMessageBox::Yes)
         {

             for (int i=0; i< fileNameList.count();i++)
             {
                 QString fileName = fileNameList[i];
                 emit clipsDelete(fileName);

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
    }
    else
        QMessageBox::information(this, "Delete clips", "Nothing to do");

     fileContextMenu->close();
}

double derp_it(int tx, int target_width, int src_width)
{
    double x = (static_cast<double>(tx) / target_width - 0.5) * 2; //  - 1 -> 1
    double sx = tx - (target_width - src_width) / 2;
    double offset = x * x * (x < 0 ? -1 : 1) * ((target_width - src_width) / 2);
    return sx - offset;
}

void BuildLookup(int width)
{
    // Generate lookup table
    int targetWidth = width * 4 / 3;
//    LOOKUP[width] = vector<double>(targetWidth);
    for (int tx = 0; tx < targetWidth; tx++)
    {
        double x = (static_cast<double>(tx) / targetWidth - 0.5) * 2; //  - 1 -> 1
        double sx = tx - (targetWidth - width) / 2;
        double offset = x * x * (x < 0 ? -1 : 1) * ((targetWidth - width) / 2);
//        LOOKUP[width][tx] = derp_it(tx, targetWidth, width)
    }
}

void AFilesTreeView::onSuperview()
{
       int target_width = 3500;//int(sys.argv[1])
       int height = 1520;//int(sys.argv[2])
       int src_width = 2704;//int(sys.argv[3])

       QFile srtOutputFile(QSettings().value("LastFileFolder").toString() + "xmap.pgm");
       if (srtOutputFile.open(QIODevice::WriteOnly) )
       {
           QTextStream srtStream( &srtOutputFile );

           srtStream << "P2 " + QString::number(target_width) + " " + QString::number(height) + " 65535" << endl;

           for (int y = 0; y < height; y++)
           {
               for (int x = 0; x < target_width; x++)
               {
                   srtStream << QString::number(derp_it(x, target_width, src_width)) + " ";
               }
               srtStream << endl;
           }
       }
       srtOutputFile.close();

       QFile srtOutputFile2(QSettings().value("LastFileFolder").toString() + "ymap.pgm");
       if (srtOutputFile2.open(QIODevice::WriteOnly) )
       {
           QTextStream srtStream( &srtOutputFile2 );

           srtStream << "P2 " + QString::number(target_width) + " " + QString::number(height) + " 65535" << endl;

           for (int y = 0; y < height; y++)
           {
               for (int x = 0; x < target_width; x++)
               {
                   srtStream << QString::number(y) + " ";
               }
               srtStream << endl;
           }
       }
       srtOutputFile2.close();

}

void AFilesTreeView::onFolderIndexClicked(QModelIndex )//index
{
    QString lastFolder = QSettings().value("LastFolder").toString();
//    qDebug()<<"AFilesTreeView::onFolderIndexClicked"<<index.data().toString()<<lastFolder;
    loadModel(lastFolder);
}

void AFilesTreeView::onClipIndexClicked(QModelIndex index)
{
    qDebug()<<"AFilesTreeView::onClipIndexClicked"<<index;
    QString folderName = index.model()->index(index.row(),folderIndex).data().toString();
    QString fileName = index.model()->index(index.row(),fileIndex).data().toString();
    QModelIndex modelIndex = fileModel->index(folderName + fileName, 0);
    qDebug()<<"AFilesTreeView::onClipIndexClicked"<<index.data().toString()<<fileName<<modelIndex.data().toString();
    setCurrentIndex(modelIndex); //does also the scrollTo
}

QModelIndex recursiveFirstFile(QFileSystemModel *fileModel, QModelIndex parentIndex)
{
    QModelIndex fileIndex = QModelIndex();
    for (int childRow=0;childRow<fileModel->rowCount(parentIndex);childRow++)
    {
        QModelIndex childIndex = fileModel->index(childRow, 0, parentIndex);
        if (childIndex.data().toString().toLower().contains(".mp4") || childIndex.data().toString().toLower().contains(".avi") || childIndex.data().toString().toLower().contains(".wmv") || childIndex.data().toString().toLower().contains(".mts")) // && fileIndex == QModelIndex()
        {
//            qDebug()<<"recursiveFiles"<<childIndex.data().toString();
            return childIndex;
        }
        fileIndex = recursiveFirstFile(fileModel, childIndex);
    }
    return fileIndex;
}

void AFilesTreeView::onDirectoryLoaded(const QString &path)
{
//    qDebug()<<"AFilesTreeView::onDirectoryLoaded"<<path<<fileModel->rowCount()<<model()->rowCount();
    QModelIndex fileIndex = QModelIndex();
//    for (int row=0; row<fileModel->rowCount();row++)
//    {
        QModelIndex parentIndex = rootIndex();

//        qDebug()<<"AFilesTreeView::onDirectoryLoaded"<<parentIndex.data().toString()<<fileModel->index(row, 1).data().toString()<<fileModel->rowCount(parentIndex);
        fileIndex = recursiveFirstFile(fileModel, parentIndex);

//        if (fileModel->index(row, 0).data().toString().toLower().contains(".mp4") && fileIndex == QModelIndex())
//            fileIndex = fileModel->index(row, 0);
//        for (int childRow=0;childRow<fileModel->rowCount(parentIndex);childRow++)
//        {
//            QModelIndex childIndex = fileModel->index(childRow, 0, parentIndex);
//            if (childIndex.data().toString().toLower().contains(".mp4") && fileIndex == QModelIndex())
//                fileIndex = childIndex;
//        }
//    }

    if (fileIndex != QModelIndex())
    {
        onIndexClicked(fileIndex);
        setCurrentIndex(fileIndex);
    }
}
