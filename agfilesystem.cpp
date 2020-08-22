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

void AGFileSystem::onFileWatch(QString folderFileName, bool on)
{
    qDebug()<<"AGFileSystem::onFileWatch"<<folderFileName<<on;

    if (on)
        bool result = fileSystemWatcher->addPath(folderFileName);
    else
        bool result = fileSystemWatcher->removePath(folderFileName);

}

void AGFileSystem::loadFilesAndFolders(QDir dir, AGProcessAndThread *process)
{
    if (processStopped || process->processStopped)
    {
        QString output = "loadFilesAndFolders processStopped for " + dir.absolutePath();
        process->addProcessLog("output", output);
        qDebug()<<output<<dir.absolutePath()<<processStopped<<process->processStopped;
        return;
    }

    //thread
    QFileInfo fileInfo(dir.absolutePath());
    QString folderName = fileInfo.path();

    if (folderName.right(1) != "/")
        folderName = folderName + "/"; //on windows, in case of D:/

//    qDebug()<<"AGFileSystem::loadFilesAndFolders"<<(QThread::currentThread() == qApp->thread()?"Main":"Thread")<<folderName + fileInfo.fileName();

    process->addProcessLog("output", "loadFilesAndFolders: " + folderName + fileInfo.fileName());

    bool result = fileSystemWatcher->addPath(folderName + fileInfo.fileName());
//    if (result)
//        qDebug()<<"fileSystemWatcher->addPath true"<<folderName + fileInfo.fileName();
//    else
//        qDebug()<<"fileSystemWatcher->addPath false"<<folderName + fileInfo.fileName();

    if (fileInfo.isDir())
    {
        emit addItem(false, "Folder", "Folder", fileInfo);

        emit addItem(false, "Folder", "FileGroup", QFileInfo(folderName + fileInfo.fileName() + "/", "Video"));
        emit addItem(false, "Folder", "FileGroup", QFileInfo(folderName + fileInfo.fileName() + "/", "Image"));
        emit addItem(false, "Folder", "FileGroup", QFileInfo(folderName + fileInfo.fileName() + "/", "Audio"));
        emit addItem(false, "Folder", "FileGroup", QFileInfo(folderName + fileInfo.fileName() + "/", "Export"));
        emit addItem(false, "Folder", "FileGroup", QFileInfo(folderName + fileInfo.fileName() + "/", "Project"));
        emit addItem(false, "Folder", "FileGroup", QFileInfo(folderName + fileInfo.fileName() + "/", "Parking"));

        emit addItem(false, "Video", "TimelineGroup", QFileInfo(folderName + fileInfo.fileName() + "/", "Timeline")); //do not add filegroup in fileinfo as it is not a real folder
        emit addItem(false, "Image", "TimelineGroup", QFileInfo(folderName + fileInfo.fileName() + "/", "Timeline"));
        emit addItem(false, "Audio", "TimelineGroup", QFileInfo(folderName + fileInfo.fileName() + "/", "Timeline"));
        emit addItem(false, "Export", "TimelineGroup", QFileInfo(folderName + fileInfo.fileName() + "/", "Timeline"));
        emit addItem(false, "Project", "TimelineGroup", QFileInfo(folderName + fileInfo.fileName() + "/", "Timeline"));
        emit addItem(false, "Parking", "TimelineGroup", QFileInfo(folderName + fileInfo.fileName() + "/", "Timeline"));

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
                    process->addProcessLog("output", output);
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
        process->addProcessLog("output", output);
        qDebug()<<output<<fileInfo.fileName()<<processStopped<<process->processStopped;
        return;
    }
    //thread
//    qDebug()<<"AGFileSystem::loadItem"<<(QThread::currentThread() == qApp->thread()?"Main":"Thread")<<fileInfo.absoluteFilePath();

    process->addProcessLog("output", "loadItem: " + fileInfo.absoluteFilePath());

    bool exportFileFound = false;
    foreach (QString exportMethod, AGlobal().exportMethods)
        if (fileInfo.completeBaseName().toLower().contains(exportMethod))
            exportFileFound = true;

    if (isNewFile)
    {
        if (exportFileFound && AGlobal().exportExtensions.contains(fileInfo.suffix(), Qt::CaseInsensitive))
        {
            emit addItem(false, "Export", "MediaFile", fileInfo);
            loadClips(process, fileInfo);
        }
        else if (AGlobal().videoExtensions.contains(fileInfo.suffix(), Qt::CaseInsensitive))
        {
            emit addItem(false, "Video", "MediaFile", fileInfo);
            loadClips(process, fileInfo);
        }
        else if (AGlobal().audioExtensions.contains(fileInfo.suffix(), Qt::CaseInsensitive))
        {
            emit addItem(false, "Audio", "MediaFile", fileInfo);
            loadClips(process, fileInfo);
        }
        else if (AGlobal().imageExtensions.contains(fileInfo.suffix(), Qt::CaseInsensitive))
        {
            emit addItem(false, "Image", "MediaFile", fileInfo);
            loadClips(process, fileInfo);
        }
        else if (AGlobal().projectExtensions.contains(fileInfo.suffix(), Qt::CaseInsensitive))
        {
            emit addItem(false, "Project", "MediaFile", fileInfo);
//            loadClips(process, fileInfo); //no clips for project files
        }
        else if (fileInfo.suffix().toLower() == "srt")
        {
            loadClips(process, fileInfo); //if mediafile does not exists addClip of clip and tags will return (workaround)
        }

        //else  stuff which is outside Media Sidekick
    }
    else //not new
    {
    }
}

void AGFileSystem::loadClips(AGProcessAndThread *process, QFileInfo fileInfo)
{
    if (processStopped || process->processStopped)
    {
        QString output = "loadClips processStopped for " + fileInfo.absoluteFilePath();
        process->addProcessLog("output", output);
        qDebug()<<output<<fileInfo.fileName()<<processStopped<<process->processStopped;
        return;
    }

    QString srtFileName = fileInfo.completeBaseName() + ".srt";

//    qDebug()<<"AGFileSystem::loadClips"<<(QThread::currentThread() == qApp->thread()?"Main":"Thread")<<fileInfo.absolutePath()<<srtFileName;

    QFile file(fileInfo.absolutePath() + "/" + srtFileName);

    QStringList list;
    int nrOfClips = 0;

    if (file.open(QIODevice::ReadOnly))
    {
        if (!fileSystemWatcher->files().contains(fileInfo.absolutePath() + "/" + srtFileName))
        {
            bool result = fileSystemWatcher->addPath(fileInfo.absolutePath() + "/" + srtFileName);
//            if (result)
//                qDebug()<<"fileSystemWatcher->addPath true "<<fileInfo.absolutePath() + "/" + srtFileName;
//            else
//                qDebug()<<"fileSystemWatcher->addPath false "<<fileInfo.absolutePath() + "/" + srtFileName;
        }

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
            {
                value = srtContentString.mid(start+3, end - start - 3);
                srtContentString.replace(srtContentString.mid(start, end - start + 4), ""); //remove value from srtContentString
            }
            else
                value = "";
            QString order = value;

            start = srtContentString.indexOf("<r>");
            end = srtContentString.indexOf("</r>");
            if (start >= 0 && end >= 0)
            {
                value = srtContentString.mid(start+3, end - start - 3);
                srtContentString.replace(srtContentString.mid(start, end - start + 4), ""); //remove value from srtContentString
            }
            else
                value = "";
            QString stars = QString("*").repeated(value.toInt());
//            QStandardItem *starItem = new QStandardItem;
//            starItem->setData(QVariant::fromValue(AStarRating(value.toInt())), Qt::EditRole);

            start = srtContentString.indexOf("<a>");
            end = srtContentString.indexOf("</a>");
            if (start >= 0 && end >= 0)
            {
                value = srtContentString.mid(start+3, end - start - 3);
                srtContentString.replace(srtContentString.mid(start, end - start + 4), ""); //remove value from srtContentString
            }
            else
                value = "";
            QString alike = value;

            start = srtContentString.indexOf("<h>");
            end = srtContentString.indexOf("</h>");
            if (start >= 0 && end >= 0)
            {
                value = srtContentString.mid(start+3, end - start - 3);
                srtContentString.replace(srtContentString.mid(start, end - start + 4), ""); //remove value from srtContentString
            }
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
                srtContentString.replace(srtContentString.mid(start, end - start + 4), ""); //remove value from srtContentString
            }
            else
                value = "";

            QString tags = value;

            if (start == -1 || end == -1) //backwards compatibility
            {
                tags = srtContentString;

//                order = QString::number(clipCounter*10);

                if (tags.indexOf("r9") >= 0)
                    stars = "*****";
                else if (tags.indexOf("r8") >= 0)
                    stars = "****";
                else if (tags.indexOf("r7") >= 0)
                    stars = "***";
                else if (tags.indexOf("r6") >= 0)
                    stars = "**";
                else if (tags.indexOf("r5") >= 0)
                    stars = "*";
                else
                    stars = "";
                tags.replace(" r5", "").replace("r5","");
                tags.replace(" r6", "").replace("r6","");
                tags.replace(" r7", "").replace("r7","");
                tags.replace(" r8", "").replace("r8","");
                tags.replace(" r9", "").replace("r9","");
                tags.replace(" ", ";");
            }

            QStandardItem *alikeItem = new QStandardItem(alike);
            alikeItem->setTextAlignment(Qt::AlignCenter); //tbd: not working ...

            int clipDuration = outTime.msecsSinceStartOfDay() - inTime.msecsSinceStartOfDay();

//            qDebug()<<"Clip file changed - add item"<<fileInfo.fileName()<<inTime.msecsSinceStartOfDay();
            emit addItem(false, "Timeline", "Clip", fileInfo, clipDuration, inTime.msecsSinceStartOfDay(), outTime.msecsSinceStartOfDay());

            if (true)
            {
//                AStarRating starRating = qvariant_cast<AStarRating>(starItem->data(Qt::EditRole));
                if (stars.length() > 0)
                {
                    emit addItem(false, "Clip", "Tag", fileInfo, clipDuration, inTime.msecsSinceStartOfDay(), outTime.msecsSinceStartOfDay(), stars);
                }

                if (alike == "true")
                    emit addItem(false, "Clip", "Tag", fileInfo, clipDuration, inTime.msecsSinceStartOfDay(), outTime.msecsSinceStartOfDay(), "✔");

                QStringList tagsList = tags.split(";");
                foreach (QString tag, tagsList)
                    if (tag != "")
                        emit addItem(false, "Clip", "Tag", fileInfo, clipDuration, inTime.msecsSinceStartOfDay(), outTime.msecsSinceStartOfDay(), tag);
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
        if (file.exists()) //new file
        {
            bool result = fileSystemWatcher->addPath(path);
            if (result)
                qDebug()<<"AGFileSystem::onFileChanged true addpath - not in watch - file exists (new file!) - added in watch"<<path;
            else
                qDebug()<<"AGFileSystem::onFileChanged false addpath - not in watch - file exists (new file!) - added in watch"<<path;
        }
        else // file does not exist, so is deleted, delete from view.
        {
            qDebug()<<"AGFileSystem::onFileChanged - not in watch - file not exists (deleted!)"<<path;
            QFileInfo fileInfo(path);

            if (fileInfo.suffix().toLower() == "srt") //clips
                emit deleteItem(false, "Clip", fileInfo);
            else
                emit deleteItem(false, "MediaFile", fileInfo);
        }
    }
    else //file changed
    {
        qDebug()<<"AGFileSystem::onFileChanged - in watch"<<path;

        //add a small delay to give OS the chance to release lock on file (avoid Permission denied erro)

        if (fileInfo.suffix() == "srt") //not for the moment
        {

//            qDebug()<<"Clip file changed - delete items"<<fileInfo.fileName();
            emit deleteItem(false, "Clip", fileInfo); //is .srt

            if (!processStopped)
            {
                AGProcessAndThread *process = new AGProcessAndThread(this);
                processes<<process;
                process->command("Load clips", [=]()
                {
                    //remove and create clips
//                    qDebug()<<"Clip file changed - load clips"<<fileInfo.fileName();
                    loadClips(process, fileInfo); //if mediafile does not exists addClip of clip and tags will return (workaround)
//                    qDebug()<<"Clip file changed - done"<<fileInfo.fileName();
                });
                process->start();
            }
        }
        else
            emit fileChanged(fileInfo); //send to view to call mediaItem loadmedia
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
