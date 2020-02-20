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

    QAction *sepAction = new QAction(this);
    sepAction->setSeparator(true);
    fileContextMenu->addAction(sepAction);

    fileContextMenu->addAction(new QAction("Archive file",fileContextMenu));
    connect(fileContextMenu->actions().last(), &QAction::triggered, this, &AFilesTreeView::onArchiveFiles);

    fileContextMenu->addAction(new QAction("Archive clips",fileContextMenu));
    connect(fileContextMenu->actions().last(), &QAction::triggered, this, &AFilesTreeView::onArchiveClips);

    fileContextMenu->addAction(sepAction);

    fileContextMenu->addAction(new QAction("Remux to Mp4 / yuv420",fileContextMenu));
    connect(fileContextMenu->actions().last(), &QAction::triggered, this, &AFilesTreeView::onRemux);

    fileContextMenu->addAction(new QAction("Wideview by Derperview",fileContextMenu));
    connect(fileContextMenu->actions().last(), &QAction::triggered, this, &AFilesTreeView::onDerperview);

    fileContextMenu->addSeparator();

    fileContextMenu->addAction(new QAction("Open in explorer",fileContextMenu));
    connect(fileContextMenu->actions().last(), &QAction::triggered, this, &AFilesTreeView::onOpenInExplorer);

    fileContextMenu->addAction(new QAction("Open in default application",fileContextMenu));
    connect(fileContextMenu->actions().last(), &QAction::triggered, this, &AFilesTreeView::onOpenDefaultApplication);

    fileContextMenu->addSeparator();
}

void AFilesTreeView::setType(QString type)
{
    QString regExp = type;
    filesProxyModel->setFilterRegExp(QRegExp(regExp, Qt::CaseInsensitive,
                                                QRegExp::FixedString));
    filesProxyModel->setFilterKeyColumn(-1);

    QStringList exportMethods = QStringList() << "Lossless" << "Encode" << "shotcut" << "Premiere";
    QStringList videoExtensions = QStringList() << "*.MP4"<<"*.AVI"<<"*.WMV"<<"*.MTS";
    QStringList exportExtensions = QStringList() << "*.MP4"<<"*.JPG"<<"*.AVI"<<"*.WMV"<<"*.MTS" << "*.mlt" << "*.xml" << "*.mp3";
    QStringList audioExtensions = QStringList() << "*.mp3"<<"*.JPG";

    if (type == "Video") //exclude lossless done in sortfilterproxymodel
    {
        fileModel->setNameFilters(videoExtensions);
    }
    else if (type == "Audio")
    {
        fileModel->setNameFilters(audioExtensions);
    }
    else if (type == "Export")
    {
        QStringList filters;
        foreach (QString exportMethod, exportMethods)
            foreach (QString extension, exportExtensions)
                filters <<exportMethod + extension;
//        filters <<"Lossless*.*"<<"Encode*.*"<<"shotcut*.*"<<"Premiere*.*";
        fileModel->setNameFilters(filters);
    }
}

void AFilesTreeView::onIndexClicked(QModelIndex index)
{
//    QModelIndex fileModelIndex = fileModel->index(index.data(index.parent()).toString());
//    QModelIndex fileModelIndex = fileModel->index(index.parent().data().toString());

    QModelIndex fileModelIndex = filesProxyModel->mapToSource(index);

    QFileInfo fileInfo = fileModel->fileInfo(fileModelIndex);
    QString filePath = fileInfo.absolutePath() + "/";

//    qDebug()<<"AFilesTreeView::onIndexClicked"<<index.parent().data().toString()<<index.data().toString()<<fileInfo<<filePath;

    QSettings().setValue("LastFile", index.data().toString());
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
    setRootIndex(filesProxyModel->mapFromSource(fileModel->index(folderUrl.toString())));
}

void AFilesTreeView::this_customContextMenuRequested(const QPoint &point)
{
    QModelIndex index = indexAt(point);
    if (index.isValid() )
        fileContextMenu->exec(viewport()->mapToGlobal(point));
}

void AFilesTreeView::onTrim()
{
    QStringList filePathList;
    QModelIndexList indexList = selectionModel()->selectedIndexes();
    for (int i=0; i< indexList.count();i++)
    {
        if (indexList[i].column() == 0) //first column
        {
            QString fileName = indexList[i].data().toString();
            if (!fileName.toLower().contains(".jpg"))
                filePathList << fileModel->filePath( filesProxyModel->mapToSource(indexList[i]));
        }
    }

    if (filePathList.count() > 0)
    {
        QMessageBox::StandardButton reply;
         reply = QMessageBox::question(this, "Trim" + QString::number(filePathList.count()) + " File(s)", "Are you sure you want to trim " + filePathList.join(", ") + "?",
                                       QMessageBox::Yes|QMessageBox::No);

         if (reply == QMessageBox::Yes)
         {
             AJobParams jobParams;
             jobParams.action = "Trim";

             QStandardItem *parentItem = jobTreeView->createJob(jobParams, nullptr , nullptr);

             QStandardItem *currentItem = nullptr;

             bool trimDone = false;
             for (int i=0; i< filePathList.count();i++)
             {
                 int lastIndexOf = filePathList[i].lastIndexOf("/");
                 QString folderName = filePathList[i].left(lastIndexOf + 1);
                 QString fileName = filePathList[i].mid(lastIndexOf + 1);

                 if (!fileName.contains(".mlt") && !fileName.contains("*.xml"))
                 {
//                     QStandardItem *parentItem2 = nullptr;
                     emit trimF(parentItem, currentItem, folderName, fileName); //should return a parentItem
                     qDebug()<<"AFilesTreeView::onTrimF"<<fileName<<parentItem;
                     trimDone = true;
                 }
             }

             if (trimDone)
             {
                 emit loadClips(parentItem);

                 emit loadProperties(parentItem);
             }
         }
    }
    else
        QMessageBox::information(this, "Trim", "Nothing to trim");

     fileContextMenu->close();
}

void AFilesTreeView::onStopThreadProcess()
{
    qDebug()<<"AFilesTreeView::onStopThreadProcess"<<this;
    emit stopThreadProcess();
}

void AFilesTreeView::onDerperview()
{
    QStringList filePathList;
    QModelIndexList indexList = selectionModel()->selectedIndexes();
    for (int i=0; i< indexList.count();i++)
    {
        if (indexList[i].column() == 0) //first column
        {
            QString fileName = indexList[i].data().toString();
            if (!fileName.toLower().contains(".mp3") && !fileName.toLower().contains(".jpg"))
                filePathList << fileModel->filePath( filesProxyModel->mapToSource(indexList[i]));
        }
    }

    if (filePathList.count() > 0)
    {
        QMessageBox::StandardButton reply;
         reply = QMessageBox::question(this, "Wideview" + QString::number(filePathList.count()) + " File(s)", "Are you sure you want to wideview " + filePathList.join(", ") + "?",
                                       QMessageBox::Yes|QMessageBox::No);

         if (reply == QMessageBox::Yes)
         {
             AJobParams jobParams;
             jobParams.action = "Wideview";

             QStandardItem *parentItem = jobTreeView->createJob(jobParams, nullptr , nullptr);

             QStandardItem *currentItem = nullptr;

             for (int i=0; i< filePathList.count();i++)
             {
                 int lastIndexOf = filePathList[i].lastIndexOf("/");
                 QString folderName = filePathList[i].left(lastIndexOf + 1);
                 QString fileName = filePathList[i].mid(lastIndexOf + 1);

                 QVariant *durationPointer = new QVariant();
                 emit getPropertyValue(fileName, "Duration", durationPointer); //format <30s: [ss.mm s] >30s: [h.mm:ss]
                 *durationPointer = durationPointer->toString().replace(" (approx)", "");
                 QTime durationTime = QTime::fromString(durationPointer->toString(),"h:mm:ss");
                 if (durationTime == QTime())
                 {
                     QString durationString = durationPointer->toString();
                     durationString = durationString.left(durationString.length() - 2); //remove " -s"
                     durationTime = QTime::fromMSecsSinceStartOfDay(int(durationString.toDouble() * 1000.0));
                 }

                 if (durationTime.msecsSinceStartOfDay() == 0)
                     durationTime = QTime::fromMSecsSinceStartOfDay(24 * 60 * 60 * 1000 - 1);

                 derperView = new ADerperView();

                 connect(this, &AFilesTreeView::stopThreadProcess, derperView, &ADerperView::onStopThreadProcess);

                 AJobParams jobParams;
                 jobParams.thisWidget = this;
                 jobParams.parentItem = parentItem;
                 jobParams.folderName = folderName;
                 jobParams.fileName = fileName;
                 jobParams.action = "Wideview";
                 jobParams.parameters["totalDuration"] = QString::number(durationTime.msecsSinceStartOfDay());
                 jobParams.parameters["durationMultiplier"] = QString::number(0.6);

                 currentItem = jobTreeView->createJob(jobParams, [] (AJobParams jobParams)
                 {
                     AFilesTreeView *filesTreeView = qobject_cast<AFilesTreeView *>(jobParams.thisWidget);

                         connect(filesTreeView->derperView, &ADerperView::processOutput, [=](QString output)
                         {
        //                         qDebug() << "AFilesTreeView::processOutput" <<jobParams.parameters["totalDuration"] << output<<jobParams.currentIndex<<jobParams.currentIndex.data();
                             emit filesTreeView->jobAddLog(jobParams, output);
                         });

                         emit filesTreeView->jobAddLog(jobParams, "===================");
                         emit filesTreeView->jobAddLog(jobParams, "WideView by Derperview, Derperview by Banelle: https://github.com/banelle/derperview");
                         emit filesTreeView->jobAddLog(jobParams, "Perform non-linear stretch of 4:3 video to make it 16:9.");
                         emit filesTreeView->jobAddLog(jobParams, "See also Derperview - A Command Line Superview Alternative: https://intofpv.com/t-derperview-a-command-line-superview-alternative");
                         emit filesTreeView->jobAddLog(jobParams, "===================");
                         emit filesTreeView->jobAddLog(jobParams, "ACVC uses unmodified Derperview sourcecode and embedded it in the Qt and ACVC job handling structure.");
                         emit filesTreeView->jobAddLog(jobParams, "ACVC added 'Remux to MP4/Yuv420' to prepare videocontent for Wideview conversion");
                         emit filesTreeView->jobAddLog(jobParams, "===================");

                     return filesTreeView->derperView->Go((jobParams.folderName + jobParams.fileName).toUtf8().constData(), (jobParams.folderName + jobParams.fileName.left(jobParams.fileName.lastIndexOf(".")) + "WV.mp4").toUtf8().constData(), 1);

                 }, nullptr);

                 copyClips(currentItem, folderName, fileName, fileName.left(fileName.lastIndexOf(".")) + "WV.mp4");

                 emit propertyCopy(currentItem, folderName, fileName, fileName.left(fileName.lastIndexOf(".")) + "WV.mp4");

                 emit releaseMedia(fileName);
                 emit moveFilesToACVCRecycleBin(currentItem, folderName, fileName);
             } //for all files

             emit loadClips(parentItem);

             emit loadProperties(parentItem);

             emit derperviewCompleted("");

         }
    }
    else
        QMessageBox::information(this, "Wideview", "Nothing to derp");

     fileContextMenu->close();
}

void AFilesTreeView::onRemux()
{
    QStringList filePathList;
    QModelIndexList indexList = selectionModel()->selectedIndexes();
    for (int i=0; i< indexList.count();i++)
    {
        if (indexList[i].column() == 0) //first column
        {
            QString fileName = indexList[i].data().toString();
            if (!fileName.toLower().contains(".mp3") && !fileName.toLower().contains(".jpg"))
                filePathList << fileModel->filePath( filesProxyModel->mapToSource(indexList[i]));
        }
    }

    if (filePathList.count() > 0)
    {
        QMessageBox::StandardButton reply;
         reply = QMessageBox::question(this, "Remux to mp4 / yuv420" + QString::number(filePathList.count()) + " File(s)", "Are you sure you want to remux to mp4 / yuv420 " + filePathList.join(", ") + "?",
                                       QMessageBox::Yes|QMessageBox::No);

         if (reply == QMessageBox::Yes)
         {
             AJobParams jobParams;
             jobParams.thisWidget = this;
             jobParams.action = "Remux";

             QStandardItem *parentItem = jobTreeView->createJob(jobParams, nullptr , nullptr);

             QStandardItem *currentItem = nullptr;

             for (int i=0; i< filePathList.count();i++)
             {
                 int lastIndexOf = filePathList[i].lastIndexOf("/");
                 QString folderName = filePathList[i].left(lastIndexOf + 1);
                 QString fileName = filePathList[i].mid(lastIndexOf + 1);

                 currentItem = onRemux2(parentItem, folderName, fileName);

                 copyClips(currentItem, folderName, fileName, fileName.left(fileName.lastIndexOf(".")) + "RM.mp4");

                 emit propertyCopy(currentItem, folderName, fileName, fileName.left(fileName.lastIndexOf(".")) + "RM.mp4");

                 emit releaseMedia(fileName);
                 emit moveFilesToACVCRecycleBin(currentItem, folderName, fileName);
             }

             emit loadClips(parentItem);

             emit loadProperties(parentItem);

         }
    }
    else
        QMessageBox::information(this, "Remux to mp4 / yuv420", "Nothing to remux");

     fileContextMenu->close();
}

void AFilesTreeView::copyClips(QStandardItem *parentItem, QString folderName, QString fileName, QString targetFileName)
{
    AJobParams jobParams;
    jobParams.parentItem = parentItem;
    jobParams.folderName = folderName;
    jobParams.fileName = fileName;
    jobParams.action = "Copy clips";
    jobParams.parameters["targetFileName"] = targetFileName;

    jobTreeView->createJob(jobParams,  [] (AJobParams jobParams)
    {
//        qDebug()<<"AFilesTreeView::copyClips thread"<<jobParams.folderName + jobParams.fileName.left(jobParams.fileName.lastIndexOf(".")) + ".srt"<<jobParams.folderName + jobParams.parameters["targetFileName"].left(jobParams.parameters["targetFileName"].lastIndexOf(".")) + ".srt";
        QFile file(jobParams.folderName + jobParams.fileName.left(jobParams.fileName.lastIndexOf(".")) + ".srt");
        if (file.exists())
           file.copy(jobParams.folderName + jobParams.parameters["targetFileName"].left(jobParams.parameters["targetFileName"].lastIndexOf(".")) + ".srt");

        return QString();
    }
        , nullptr);
}

QStandardItem *AFilesTreeView::onRemux2(QStandardItem *parentItem, QString folderName, QString fileName)
{
    QString sourceFolderFileName = folderName + fileName;

    QString targetFolderFileName;
    int lastIndex = fileName.lastIndexOf(".");
    if (lastIndex > -1)
        targetFolderFileName = folderName + fileName.left(lastIndex) + "RM.mp4";

#ifdef Q_OS_WIN
    sourceFolderFileName = sourceFolderFileName.replace("/", "\\");
    targetFolderFileName = targetFolderFileName.replace("/", "\\");
#endif

    QVariant *durationPointer = new QVariant();
    emit getPropertyValue(fileName, "Duration", durationPointer); //format <30s: [ss.mm s] >30s: [h.mm:ss]
    *durationPointer = durationPointer->toString().replace(" (approx)", "");
    QTime durationTime = QTime::fromString(durationPointer->toString(),"h:mm:ss");
    if (durationTime == QTime())
    {
        QString durationString = durationPointer->toString();
        durationString = durationString.left(durationString.length() - 2); //remove " -s"
        durationTime = QTime::fromMSecsSinceStartOfDay(int(durationString.toDouble() * 1000.0));
    }

    if (durationTime.msecsSinceStartOfDay() == 0)
        durationTime = QTime::fromMSecsSinceStartOfDay(24 * 60 * 60 * 1000 - 1);

    qDebug()<<"AFilesTreeView::onRemux"<<folderName<<fileName;

    AJobParams jobParams;
    jobParams.parentItem = parentItem;
    jobParams.folderName = folderName;
    jobParams.fileName = fileName;
    jobParams.action = "Remux";
    jobParams.command = "ffmpeg -y -i \"" + sourceFolderFileName + "\" -pix_fmt yuv420p -y \"" + targetFolderFileName + "\""; //-map_metadata 0  -loglevel +verbose
    jobParams.parameters["exportFolderFileName"] = targetFolderFileName;
    jobParams.parameters["totalDuration"] = QString::number(durationTime.msecsSinceStartOfDay());
    jobParams.parameters["durationMultiplier"] = QString::number(2);

    return jobTreeView->createJob(jobParams, nullptr, nullptr);
}

void AFilesTreeView::onArchiveFiles()
{
    QStringList filePathList;
    QModelIndexList indexList = selectionModel()->selectedIndexes();
    for (int i=0; i< indexList.count();i++)
    {
        if (indexList[i].column() == 0) //first column
        {
            filePathList << fileModel->filePath( filesProxyModel->mapToSource(indexList[i]));
        }
    }

    if (filePathList.count()>0)
    {
        QMessageBox::StandardButton reply;
         reply = QMessageBox::question(this, "Archive file(s)" + QString::number(filePathList.count()) + " File(s)", "Are you sure you want to move " + filePathList.join(", ") + " and its supporting files (srt and txt) to the ACVC recycle bin folder?",
                                       QMessageBox::Yes|QMessageBox::No);

         if (reply == QMessageBox::Yes)
         {
             AJobParams jobParams;
             jobParams.thisWidget = this;
             jobParams.action = "Archive Files";

             QStandardItem *parentItem = jobTreeView->createJob(jobParams, nullptr , nullptr);

             for (int i=0; i< filePathList.count();i++)
             {

                 int lastIndexOf = filePathList[i].lastIndexOf("/");
                 QString folderName = filePathList[i].left(lastIndexOf + 1);
                 QString fileName = filePathList[i].mid(lastIndexOf + 1);

                 emit releaseMedia(fileName);
                 emit moveFilesToACVCRecycleBin(parentItem, folderName, fileName);

             }
             emit loadClips(parentItem);
             emit loadProperties(parentItem);
         }
    }

    fileContextMenu->close();
}

void AFilesTreeView::onArchiveClips()
{
    QStringList filePathList;
    QModelIndexList indexList = selectionModel()->selectedIndexes();
    for (int i=0; i< indexList.count();i++)
    {
        if (indexList[i].column() == 0) //first column
        {
            QString fileName = indexList[i].data().toString();
            if (!fileName.contains(".mlt") && !fileName.contains("*.xml"))
                filePathList << fileModel->filePath( filesProxyModel->mapToSource(indexList[i]));
        }
    }

    if (filePathList.count()>0)
    {
        QMessageBox::StandardButton reply;
         reply = QMessageBox::question(this, "Archive clips" + QString::number(filePathList.count()) + " File(s)", "Are you sure you want to move supporting files (srt and txt) of " + filePathList.join(", ") + " to the ACVC recycle bin folder?",
                                       QMessageBox::Yes|QMessageBox::No);

         if (reply == QMessageBox::Yes)
         {

             AJobParams jobParams;
             jobParams.action = "Archive Clips";

             QStandardItem *parentItem = jobTreeView->createJob(jobParams, nullptr , nullptr);

             for (int i=0; i< filePathList.count();i++)
             {
                 int lastIndexOf = filePathList[i].lastIndexOf("/");
                 QString folderName = filePathList[i].left(lastIndexOf + 1);
                 QString fileName = filePathList[i].mid(lastIndexOf + 1);

                 emit releaseMedia(fileName);
                 emit moveFilesToACVCRecycleBin(parentItem, folderName, fileName, true); //supporting files only
             }
             emit loadClips(parentItem);
             emit loadProperties(parentItem);
         }
    }
    else
        QMessageBox::information(this, "Archive clips", "Nothing to do");

     fileContextMenu->close();
}

void AFilesTreeView::onOpenInExplorer()
{
    QStringList filePathList;
    QModelIndexList indexList = selectionModel()->selectedIndexes();
    for (int i=0; i< indexList.count();i++)
    {
        if (indexList[i].column() == 0) //first column
        {
            filePathList << fileModel->filePath( filesProxyModel->mapToSource(indexList[i]));
        }
    }

    if (filePathList.count() > 0)
    {
        for (int i=0; i< filePathList.count();i++)
        {
            int lastIndexOf = filePathList[i].lastIndexOf("/");
            QString folderName = filePathList[i].left(lastIndexOf + 1);
            QString fileName = filePathList[i].mid(lastIndexOf + 1);

            //http://lynxline.com/show-in-finder-show-in-explorer/
            //https://stackoverflow.com/questions/3490336/how-to-reveal-in-finder-or-show-in-explorer-with-qt

            #ifdef Q_OS_MAC
                QStringList args;
                args << "-e";
                args << "tell application \"Finder\"";
                args << "-e";
                args << "activate";
                args << "-e";
                args << "select POSIX file \""+folderName + fileName+"\"";
                args << "-e";
                args << "end tell";
                QProcess::startDetached("osascript", args);
            #endif

            #ifdef Q_OS_WIN
                QStringList args;
                args << "/select," << QDir::toNativeSeparators(folderName + fileName);
                QProcess::startDetached("explorer", args);
            #endif
        }
    }
}

void AFilesTreeView::onOpenDefaultApplication()
{
    QStringList filePathList;
    QModelIndexList indexList = selectionModel()->selectedIndexes();
    for (int i=0; i< indexList.count();i++)
    {
        if (indexList[i].column() == 0) //first column
        {
            filePathList << fileModel->filePath( filesProxyModel->mapToSource(indexList[i]));
        }
    }

    if (filePathList.count()>0)
    {
        for (int i=0; i< filePathList.count();i++)
        {
            int lastIndexOf = filePathList[i].lastIndexOf("/");
            QString folderName = filePathList[i].left(lastIndexOf + 1);
            QString fileName = filePathList[i].mid(lastIndexOf + 1);

            QDesktopServices::openUrl( QUrl::fromLocalFile( folderName + fileName) );
        }
    }
}

void AFilesTreeView::onFolderIndexClicked(QModelIndex )//index
{
    QString lastFolder = QSettings().value("LastFolder").toString();
//    qDebug()<<"AFilesTreeView::onFolderIndexClicked"<<index.data().toString()<<lastFolder;
    setCurrentIndex(QModelIndex());
    loadModel(lastFolder);
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

//QModelIndex AFilesTreeView::recursiveFiles(QAbstractItemModel *fileModel, QModelIndex parentIndex, QMap<QString, QString> parameters, void (*processOutput)(QWidget *, QMap<QString, QString> , QModelIndex))
//{
//    QModelIndex fileIndex = QModelIndex();
//    for (int childRow=0;childRow<fileModel->rowCount(parentIndex);childRow++)
//    {
//        QModelIndex childIndex = fileModel->index(childRow, 0, parentIndex);

//        processOutput(this, parameters, childIndex);

//        fileIndex = recursiveFiles(fileModel, childIndex, parameters, processOutput);
//    }
//    return fileIndex;
//}


QModelIndex recursiveFirstFile(QFileSystemModel *fileModel, QModelIndex parentIndex)
{
    QModelIndex fileIndex = QModelIndex();
    for (int childRow=0;childRow<fileModel->rowCount(parentIndex);childRow++)
    {
        QModelIndex childIndex = fileModel->index(childRow, 0, parentIndex);
        if (childIndex.data().toString().toLower().contains(".mp4") || childIndex.data().toString().toLower().contains(".avi") || childIndex.data().toString().toLower().contains(".wmv") || childIndex.data().toString().toLower().contains(".mts")) // && fileIndex == QModelIndex()
        {
//            qDebug()<<"recursiveFirstFile"<<childIndex.data().toString();
            return childIndex;
        }
        fileIndex = recursiveFirstFile(fileModel, childIndex);
    }
    return fileIndex;
}

void AFilesTreeView::onModelLoaded(const QString &)//path
{
    if (filesProxyModel->filterRegExp().pattern() == "Video")
        qDebug()<<"AFilesTreeView::onModelLoaded"<<filesProxyModel->filterRegExp().pattern();

    setCurrentIndex(QModelIndex());

    return; //tbd: find out why not enabled

    QModelIndex fileIndex = QModelIndex();

    QString lastFile = QSettings().value("LastFile").toString();
    if (lastFile != "" ) //not the root folder
    {
        QString lastFileFolder = QSettings().value("LastFileFolder").toString();
        fileIndex = fileModel->index(lastFileFolder + lastFile, 0);
    }
    else
    {
        QModelIndex parentIndex = rootIndex();
        fileIndex = recursiveFirstFile(fileModel, parentIndex);
    }

    if (fileIndex != QModelIndex())
    {
        onIndexClicked(filesProxyModel->mapFromSource(fileIndex));
        setCurrentIndex(filesProxyModel->mapFromSource(fileIndex));
    }
}
