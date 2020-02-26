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


    QString selectedFolderName= QSettings().value("selectedFolderName").toString();
    if (selectedFolderName != ""  && selectedFolderName.length() > 4) //not the root folder
        setCurrentIndex(directoryModel->index(selectedFolderName)); //does also the scrollTo
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
    QSettings().setValue("selectedFolderName", lastFolder);
    QSettings().sync();

    if (lastFolder != ""  && lastFolder.length()>4) //not the root folder
    {
        emit indexClicked(selectedIndex);
    }
}

void AFolderTreeView::recursiveFileRenameCopyIfExists(QString folderName, QString fileName)
{
    bool success;
    QFile file(folderName + fileName);
    if (file.exists())
    {
        QString fileNameWithoutExtension = fileName.left(fileName.lastIndexOf("."));
        QString fileExtension = fileName.mid(fileName.lastIndexOf("."));

//        qDebug()<<"AFolderTreeView::recursiveFileRenameCopyIfExists"<<fileNameWithoutExtension<<fileExtension<<fileNameWithoutExtension + "BU" + fileExtension;

        recursiveFileRenameCopyIfExists(folderName, fileNameWithoutExtension + "BU" + fileExtension);

        success = file.rename(folderName + fileNameWithoutExtension + "BU" + fileExtension);
    }
}

void AFolderTreeView::onMoveFilesToACVCRecycleBin(QStandardItem *parentItem, QString folderName, QString fileName, bool supportingFilesOnly)
{
//    qDebug()<<"AFolderTreeView::onMoveFilesToACVCRecycleBin"<<folderName<<fileName<<supportingFilesOnly;

    AJobParams jobParams;
    jobParams.thisWidget = this;
    jobParams.parentItem = parentItem;
    jobParams.folderName = folderName;
    jobParams.fileName = fileName;
    jobParams.action = "ToACVCRecycleBin";
    jobParams.parameters["totalDuration"] = QString::number(1000);
    jobParams.parameters["supportingFilesOnly"] = QString::number(supportingFilesOnly);

    parentItem = jobTreeView->createJob(jobParams, [] (AJobParams jobParams)
    {
            AFolderTreeView *folderTreeView = qobject_cast<AFolderTreeView *>(jobParams.thisWidget);

            QString folderName = jobParams.folderName;
            QString fileName = jobParams.fileName;

            QString recycleFolder = folderName + "ACVCRecycleBin/";

//            qDebug()<<"AFolderTreeView::onMoveFilesToACVCRecycleBin"<<folderName<<fileName<<recycleFolder<<jobParams.parameters["supportingFilesOnly"];

            bool success = true;;

            QDir dir(recycleFolder);
            if (!dir.exists())
            {
                success = dir.mkpath(".");
                if (success)
                    emit folderTreeView->jobAddLog(jobParams, tr("%1 created").arg(recycleFolder));
            }

            if (success)
            {
                if (jobParams.parameters["supportingFilesOnly"] != "1")
                {
                    QFile file(folderName + fileName);

                    if (file.exists())
                    {
                        folderTreeView->recursiveFileRenameCopyIfExists(recycleFolder, fileName);
                        success = file.rename(recycleFolder + fileName);
                        if (success)
                            emit folderTreeView->jobAddLog(jobParams, tr("%1 moved to recycle folder").arg(fileName));
                    }
                }

                if (success)
                {
                    int lastIndex = fileName.lastIndexOf(".");
                    if (lastIndex > -1)
                    {
                        QString srtFileName = fileName.left(lastIndex) + ".srt";
                        QFile *file = new QFile(folderName + srtFileName);
                        if (file->exists())
                        {
                            folderTreeView->recursiveFileRenameCopyIfExists(recycleFolder, srtFileName);
                            success = file->rename(recycleFolder + srtFileName);
                            if (success)
                                emit folderTreeView->jobAddLog(jobParams, tr("%1 moved to recycle folder").arg(srtFileName));
                        }
                        if (success)
                        {
                            srtFileName = fileName.left(lastIndex) + ".txt";
                            file = new QFile(folderName + srtFileName);
                            if (file->exists())
                            {
                                folderTreeView->recursiveFileRenameCopyIfExists(recycleFolder, srtFileName);
                                success = file->rename(recycleFolder + srtFileName);
                                if (success)
                                    emit folderTreeView->jobAddLog(jobParams, tr("%1 moved to recycle folder").arg(srtFileName));
                            }
                        }
                        else
                             return QString("-3, could not rename to " + recycleFolder + srtFileName);
                    }
               }
                else
                     return QString("-2, could not rename to " + recycleFolder + fileName);

           }
           else
            return QString("-1, could not create folder " + recycleFolder);


           if (success)
            return QString("");
           else
            return QString("-1, something wrong");
    }, nullptr);
}
