#include "agfilesystem.h"

#include "aglobal.h"
#include "astarrating.h"

#include <QDebug>
#include <QDateTime>
#include <QLabel>
#include <QFile>
#include <QSettings>
#include <QStandardItem>
#include <QApplication>

AGFileSystem::AGFileSystem(QObject *parent):QObject(parent)
{
//    qDebug()<<"AGFileSystem::AGFileSystem";

    fileSystemWatcher = new QFileSystemWatcher(this);

    connect(fileSystemWatcher, &QFileSystemWatcher::fileChanged, this, &AGFileSystem::onFileChanged);
    connect(fileSystemWatcher, &QFileSystemWatcher::directoryChanged, this, &AGFileSystem::onDirectoryChanged);
}

void AGFileSystem::onStopThreadProcess()
{
//    qDebug()<<"AGFileSystem::onStopThreadProcess"<<processes.count();

    processStopped = true;

    foreach (AGProcessAndThread *process, processes)
    {
        if ((process->process != nullptr && process->process->state() != QProcess::NotRunning) || (process->jobThread != nullptr && process->jobThread->isRunning()))
        {
            qDebug()<<"AGFileSystem::onStopThreadProcess Killing process"<<process->name<<process->process<<process->jobThread;
            process->kill();
        }
    }
}

void AGFileSystem::loadFilesAndFolders(QDir dir, AGProcessAndThread *process)
{
    if (processStopped || process->processStopped)
    {
        QString output = "loadFilesAndFolders processStopped for " + dir.absolutePath();
        process->onProcessOutput("output", output);
        qDebug()<<output<<dir.absolutePath()<<processStopped<<process->processStopped;
        return;
    }

    //thread
    QFileInfo fileInfo(dir.absolutePath());
    QString folderName = fileInfo.path();

    if (folderName.right(1) != "/")
        folderName = folderName + "/"; //on windows, in case of D:/

//    qDebug()<<"AGFileSystem::loadFilesAndFolders"<<(QThread::currentThread() == qApp->thread()?"Main":"Thread")<<folderName + fileInfo.fileName();

    process->onProcessOutput("output", "loadFilesAndFolders: " + folderName + fileInfo.fileName());

    bool result = fileSystemWatcher->addPath(folderName + fileInfo.fileName());
//    if (result)
//        qDebug()<<"fileSystemWatcher->addPath true"<<folderName + fileInfo.fileName();
//    else
//        qDebug()<<"fileSystemWatcher->addPath false"<<folderName + fileInfo.fileName();

    if (fileInfo.isDir())
    {
        emit addItem("Folder", "Folder", fileInfo);

        emit addItem("Folder", "FileGroup", QFileInfo(folderName + fileInfo.fileName() + "/", "Video"));
        emit addItem("Folder", "FileGroup", QFileInfo(folderName + fileInfo.fileName() + "/", "Image"));
        emit addItem("Folder", "FileGroup", QFileInfo(folderName + fileInfo.fileName() + "/", "Audio"));
        emit addItem("Folder", "FileGroup", QFileInfo(folderName + fileInfo.fileName() + "/", "Export"));
        emit addItem("Folder", "FileGroup", QFileInfo(folderName + fileInfo.fileName() + "/", "Project"));
        emit addItem("Folder", "FileGroup", QFileInfo(folderName + fileInfo.fileName() + "/", "Parking"));

        emit addItem("Video", "TimelineGroup", QFileInfo(folderName + fileInfo.fileName() + "/", "Timeline"));
        emit addItem("Image", "TimelineGroup", QFileInfo(folderName + fileInfo.fileName() + "/", "Timeline"));
        emit addItem("Audio", "TimelineGroup", QFileInfo(folderName + fileInfo.fileName() + "/", "Timeline"));
        emit addItem("Export", "TimelineGroup", QFileInfo(folderName + fileInfo.fileName() + "/", "Timeline"));
        emit addItem("Project", "TimelineGroup", QFileInfo(folderName + fileInfo.fileName() + "/", "Timeline"));
        emit addItem("Parking", "TimelineGroup", QFileInfo(folderName + fileInfo.fileName() + "/", "Timeline"));

    }
    else // file  if (parentItem->toolTip().contains("Video"))
    {
        loadItem(process, fileInfo, true); //new
    }

    //read subfolders
    dir.setFilter(QDir::Files | QDir::AllDirs | QDir::NoDotAndDotDot | QDir::NoSymLinks);
    dir.setSorting(QDir::Name | QDir::LocaleAware | QDir::IgnoreCase); //localeaware to get +99999ms sorted the right way

    QStringList nameFilters;
    foreach (QString extension, AGlobal().exportExtensions << AGlobal().audioExtensions << AGlobal().imageExtensions << AGlobal().projectExtensions)
        nameFilters << "*." + extension;

//    qDebug()<<"nameFilters"<<nameFilters;
    dir.setNameFilters(nameFilters);

    {
        for (int i=0; i < dir.entryList().size(); i++)
        {

            if (dir.entryList().at(i) != "MSKRecycleBin")
            {
                if (processStopped || process->processStopped)
                {
                    QString output = "LoadItems processStopped for " + dir.absolutePath() + "/" + dir.entryList().at(i);
                    process->onProcessOutput("output", output);
                    qDebug()<<output<<fileInfo.fileName()<<processStopped<<process->processStopped;
                    return;
                }

                loadFilesAndFolders(QDir(dir.absolutePath() + "/" + dir.entryList().at(i)), process);
            }
        }
    }
}

void AGFileSystem::loadItem(AGProcessAndThread *process, QFileInfo fileInfo, bool isNewFile)
{
    if (processStopped || process->processStopped)
    {
        QString output = "loadItem processStopped for " + fileInfo.absoluteFilePath();
        process->onProcessOutput("output", output);
        qDebug()<<output<<fileInfo.fileName()<<processStopped<<process->processStopped;
        return;
    }
    //thread
//    qDebug()<<"AGFileSystem::loadItem"<<(QThread::currentThread() == qApp->thread()?"Main":"Thread")<<fileInfo.absoluteFilePath();

    process->onProcessOutput("output", "loadItem: " + fileInfo.absoluteFilePath());

    bool exportFileFound = false;
    foreach (QString exportMethod, AGlobal().exportMethods)
        if (fileInfo.completeBaseName().toLower().contains(exportMethod))
            exportFileFound = true;

    if (isNewFile)
    {
        if (exportFileFound && AGlobal().exportExtensions.contains(fileInfo.suffix(), Qt::CaseInsensitive))
        {
            emit addItem("Export", "MediaFile", fileInfo);
            loadClips(process, "Export", fileInfo);
        }
        else if (AGlobal().videoExtensions.contains(fileInfo.suffix(), Qt::CaseInsensitive))
        {
            emit addItem("Video", "MediaFile", fileInfo);
            loadClips(process, "Video", fileInfo);
        }
        else if (AGlobal().audioExtensions.contains(fileInfo.suffix(), Qt::CaseInsensitive))
        {
            emit addItem("Audio", "MediaFile", fileInfo);
            loadClips(process, "Audio", fileInfo);
        }
        else if (AGlobal().imageExtensions.contains(fileInfo.suffix(), Qt::CaseInsensitive))
        {
            emit addItem("Image", "MediaFile", fileInfo);
            loadClips(process, "Image", fileInfo);
        }
        else if (AGlobal().projectExtensions.contains(fileInfo.suffix(), Qt::CaseInsensitive))
        {
            emit addItem("Project", "MediaFile", fileInfo);
//            loadClips(process, "Project", folderName, fileName); //no clips for project files
        }
        else if (fileInfo.suffix().toLower() == "srt")
        {
            loadClips(process, "Project", fileInfo); //if mediafile does not exists addClip of clip and tags will return (workaround)
        }

        //else  stuff which is outside Media Sidekick
    }
    else //not new
    {
    }
}

void AGFileSystem::loadClips(AGProcessAndThread *process, QString parentName, QFileInfo fileInfo)
{
    if (processStopped || process->processStopped)
    {
        QString output = "loadClips processStopped for " + fileInfo.absoluteFilePath();
        process->onProcessOutput("output", output);
        qDebug()<<output<<fileInfo.fileName()<<processStopped<<process->processStopped;
        return;
    }

    QString srtFileName = fileInfo.completeBaseName() + ".srt";

//    qDebug()<<"AGFileSystem::loadClips"<<(QThread::currentThread() == qApp->thread()?"Main":"Thread")<<fileInfo.absolutePath()<<srtFileName;

    QFile file(fileInfo.absolutePath() + "/" + srtFileName);

    QStringList list;
    int nrOfClips = 0;

    if(file.open(QIODevice::ReadOnly))
    {
        bool result = fileSystemWatcher->addPath(fileInfo.absolutePath() + "/" + srtFileName);
//        if (result)
//            qDebug()<<"fileSystemWatcher->addPath true "<<fileInfo.absolutePath() + "/" + srtFileName;
//        else
//            qDebug()<<"fileSystemWatcher->addPath false "<<fileInfo.absolutePath() + "/" + srtFileName;

        QTextStream in(&file);
        while (!in.atEnd())
           {
              QString line = in.readLine();
              list << line;
              if (line.indexOf("-->") >= 0)
                  nrOfClips++;
           }

        file.close();
    }

//    bool tagsContainSemiColon = false;
    int nrOfProcessedClips = 0;
    for (int i = 0; i< list.count(); i++)
    {
        QString line = list[i];
        int indexOf = line.indexOf("-->");
        if (indexOf >= 0)
        {
            nrOfProcessedClips ++;
            line.replace(",",".");

            QTime inTime = QTime::fromString(line.left(12),"HH:mm:ss.zzz");
            QTime outTime = QTime::fromString(line.mid(17,12),"HH:mm:ss.zzz");

//            STimeSpinBox *inSpin = new STimeSpinBox();
//            inSpin->setValue(inTime.msecsSinceStartOfDay());
//            QStandardItem *inSpinItem = new QStandardItem;
//            inSpinItem->setData(QVariant::fromValue(inSpin));

            QString srtContentString = list[i+1];
            int start;
            int end;
            QString value;

            start = srtContentString.indexOf("<o>");
            end = srtContentString.indexOf("</o>");
            if (start >= 0 && end >= 0)
                value = srtContentString.mid(start+3, end - start - 3);
            else
                value = "";
            QString order = value;

            start = srtContentString.indexOf("<r>");
            end = srtContentString.indexOf("</r>");
            if (start >= 0 && end >= 0)
                value = srtContentString.mid(start+3, end - start - 3);
            else
                value = "";
            QStandardItem *starItem = new QStandardItem;
            starItem->setData(QVariant::fromValue(AStarRating(value.toInt())), Qt::EditRole);

            start = srtContentString.indexOf("<a>");
            end = srtContentString.indexOf("</a>");
            if (start >= 0 && end >= 0)
                value = srtContentString.mid(start+3, end - start - 3);
            else
                value = "";
            QString alike = value;

            start = srtContentString.indexOf("<h>");
            end = srtContentString.indexOf("</h>");
            if (start >= 0 && end >= 0)
                value = srtContentString.mid(start+3, end - start - 3);
            else
                value = "";
            QString hint = value;

            start = srtContentString.indexOf("<t>");
            end = srtContentString.indexOf("</t>");
            if (start >= 0 && end >= 0)
            {
                value = srtContentString.mid(start+3, end - start - 3);
//                tagsContainSemiColon = tagsContainSemiColon || value.contains(";");
//                if (!tagsContainSemiColon)
//                    value = value.replace(" ",";");
            }
            else
                value = "";

            QString tags = value;

            if (start == -1 || end == -1) //backwards compatibility
            {
                tags = srtContentString;

//                order = QString::number(clipCounter*10);

                QVariant starVar = QVariant::fromValue(AStarRating(0));
                if (tags.indexOf("r9") >= 0)
                    starVar = QVariant::fromValue(AStarRating(5));
                else if (tags.indexOf("r8") >= 0)
                    starVar = QVariant::fromValue(AStarRating(4));
                else if (tags.indexOf("r7") >= 0)
                    starVar = QVariant::fromValue(AStarRating(3));
                else if (tags.indexOf("r6") >= 0)
                    starVar = QVariant::fromValue(AStarRating(2));
                else if (tags.indexOf("r5") >= 0)
                    starVar = QVariant::fromValue(AStarRating(1));
                starItem->setData(starVar, Qt::EditRole);
                tags.replace(" r5", "").replace("r5","");
                tags.replace(" r6", "").replace("r6","");
                tags.replace(" r7", "").replace("r7","");
                tags.replace(" r8", "").replace("r8","");
                tags.replace(" r9", "").replace("r9","");
                tags.replace(" ", ";");
            }

            QStandardItem *alikeItem = new QStandardItem(alike);
            alikeItem->setTextAlignment(Qt::AlignCenter); //tbd: not working ...

            int clipDuration = AGlobal().frames_to_msec(AGlobal().msec_to_frames(outTime.msecsSinceStartOfDay()) - AGlobal().msec_to_frames(inTime.msecsSinceStartOfDay()) + 1);

            emit addItem(parentName, "Clip", fileInfo, clipDuration, inTime.msecsSinceStartOfDay(), outTime.msecsSinceStartOfDay());

            if (true)
            {
                AStarRating starRating = qvariant_cast<AStarRating>(starItem->data(Qt::EditRole));
                if (starRating.starCount() != 0)
                {
                    QString stars = QString("*").repeated(starRating.starCount());
                    emit addItem("Clip", "Tag", fileInfo, clipDuration, inTime.msecsSinceStartOfDay(), outTime.msecsSinceStartOfDay(), stars);
                }

                if (alike == "true")
                    emit addItem("Clip", "Tag", fileInfo, clipDuration, inTime.msecsSinceStartOfDay(), outTime.msecsSinceStartOfDay(), "✔");

                QStringList tagsList = tags.split(";");
                foreach (QString tag, tagsList)
                    if (tag != "")
                        emit addItem("Clip", "Tag", fileInfo, clipDuration, inTime.msecsSinceStartOfDay(), outTime.msecsSinceStartOfDay(), tag);
            }
        }
    }
}

void AGFileSystem::onFileChanged(const QString &path)
{
    QFileInfo fileInfo(path);

    if (!fileSystemWatcher->files().contains(path))
    {
        QFile file(path);
        if (file.exists())
        {
            bool result = fileSystemWatcher->addPath(path);
//            if (result)
//                qDebug()<<"AGFileSystem::onFileChanged true addpath - not in watch - file exists (new file!) - added in watch"<<path;
//            else
//                qDebug()<<"AGFileSystem::onFileChanged false addpath - not in watch - file exists (new file!) - added in watch"<<path;
        }
        else
        {
//            qDebug()<<"AGFileSystem::onFileChanged - not in watch - file not exists (deleted!)"<<path;
            QFileInfo fileInfo(path);

            if (fileInfo.suffix().toLower() == "srt") //clips
                emit deleteItem("Clip", fileInfo);
            else
                emit deleteItem("MediaFile", fileInfo);
        }
    }
    else //file changed
    {
//        qDebug()<<"AGFileSystem::onFileChanged - in watch"<<path;

        //add a small delay to give OS the chance to release lock on file (avoid Permission denied erro)

        if (fileInfo.suffix() == "srt")
        {
            //remove and create clips
            emit deleteItem("Clip", fileInfo); //is .srt

            if (!processStopped)
            {
                AGProcessAndThread *process = new AGProcessAndThread(this);
                processes<<process;
                process->command("Load clips", [=]()
                {
                    loadClips(process, "Project", fileInfo); //if mediafile does not exists addClip of clip and tags will return (workaround)
                });
                process->start();
            }
        }
        else
            emit fileChanged(fileInfo);
    }
}

void AGFileSystem::onDirectoryChanged(const QString &path)
{
//    qDebug()<<"AGFileSystem::onDirectoryChanged"<<path;

    QDir dir(path);
    dir.setFilter(QDir::Files | QDir::AllDirs | QDir::NoDotAndDotDot | QDir::NoSymLinks);
//    dir.setSorting(QDir::Name | QDir::LocaleAware); //localeaware to get +99999ms sorted the right way

    QStringList nameFilters;
    foreach (QString extension, AGlobal().exportExtensions << AGlobal().audioExtensions << AGlobal().imageExtensions << AGlobal().projectExtensions)
        nameFilters << "*." + extension;

    nameFilters << "*.srt";

    dir.setNameFilters(nameFilters);

    //check all files in folder
    for (int i=0; i < dir.entryList().size(); i++)
    {
//        QString fileNameLow = (dir.absolutePath() + "/" + dir.entryList().at(i)).toLower();
        QFileInfo fileInfo = dir.entryInfoList().at(i);

        QString filePath = fileInfo.path() + "/" + fileInfo.fileName();

        if (!fileSystemWatcher->files().contains(filePath) && !fileSystemWatcher->directories().contains(filePath) && !filePath.contains("MSKRecycleBin"))
        {
            bool result = fileSystemWatcher->addPath(filePath);
//            if (result)
//                qDebug()<<"AGFileSystem::onDirectoryChanged true addpath - not in watch - file or folder exists (new!) - added in watch"<<filePath;
//            else
//                qDebug()<<"AGFileSystem::onDirectoryChanged false addpath - not in watch - file or folder exists (new!) - added in watch"<<filePath;

            QString folderName = fileInfo.path() + "/";

            if (!processStopped)
            {
                AGProcessAndThread *process = new AGProcessAndThread(this);
                processes<<process;
                process->command("Load item", [=]()
                {
                    loadItem(process, fileInfo, true); //new
                });
                process->start();
            }
        }
//        else //nothing special
//            qDebug()<<"  AGFileSystem::onDirectoryChanged already in watch - file or folder exists ()"<<filePath;
    }
}
