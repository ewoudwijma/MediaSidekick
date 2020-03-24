#include "agfilesystem.h"

#include "aglobal.h"
#include "astarrating.h"

#include <QDebug>
#include <QDateTime>
#include <QLabel>

#include <QFile>

AGFileSystem::AGFileSystem(QObject *parent):QObject(parent)
{
    QString rootPath = "D:/ACVCVideo/";
//    fileSystemModel->setFilter(QDir::Dirs|QDir::Drives|QDir::NoDotAndDotDot|QDir::AllDirs|QDir::Files);
//    fileSystemModel->setFilter(QDir::AllEntries);

    fileSystemWatcher = new QFileSystemWatcher(this);
//    fileSystemWatcher->addPath(rootPath);
//    fileSystemWatcher->addPath(rootPath + "test.txt");

    connect(fileSystemWatcher, &QFileSystemWatcher::fileChanged, this, &AGFileSystem::onFileChanged);
    connect(fileSystemWatcher, &QFileSystemWatcher::directoryChanged, this, &AGFileSystem::onDirectoryChanged);
}

void AGFileSystem::loadFilesAndFolders(QDir dir)
{
    QFileInfo fileInfo(dir.absolutePath());
    QString folderName = fileInfo.path();

    if (folderName.right(1) != "/")
        folderName = folderName + "/"; //on windows, in case of D:/

    fileSystemWatcher->addPath(folderName + fileInfo.fileName());

//    qDebug() <<"AGFileSystem::loadFilesAndFolders"<<fileInfo.isDir()<<fileInfo.path()<< folderName <<fileInfo.fileName()<< fileInfo.metadataChangeTime().toString();
    if (fileInfo.isDir())
    {

        emit addItem("Folder", "Folder", folderName, fileInfo.fileName());
        //parentItem,

        emit addItem("Folder", "FileGroup", folderName + fileInfo.fileName() + "/", "Video");
        //childItem,
        emit addItem("Folder", "FileGroup", folderName + fileInfo.fileName() + "/", "Audio");
        emit addItem("Folder", "FileGroup", folderName + fileInfo.fileName() + "/", "Image");
        emit addItem("Folder", "FileGroup", folderName + fileInfo.fileName() + "/", "Export");
        emit addItem("Folder", "FileGroup", folderName + fileInfo.fileName() + "/", "Project");

        emit addItem("Video", "TimelineGroup", folderName + fileInfo.fileName() + "/", "Timeline");
        //videoParentItem,
        emit addItem("Audio", "TimelineGroup", folderName + fileInfo.fileName() + "/", "Timeline");
        emit addItem("Image", "TimelineGroup", folderName + fileInfo.fileName() + "/", "Timeline");
        emit addItem("Export", "TimelineGroup", folderName + fileInfo.fileName() + "/", "Timeline");
        emit addItem("Project", "TimelineGroup", folderName + fileInfo.fileName() + "/", "Timeline");

    }
    else // file  if (parentItem->toolTip().contains("Video"))
    {
        QString fileNameLow = fileInfo.fileName().toLower();
        int lastIndexOf = fileNameLow.lastIndexOf(".");
        QString extension = fileNameLow.toLower().mid(lastIndexOf + 1);

        bool exportFileFound = false;
        foreach (QString exportMethod, AGlobal().exportMethods)
            if (fileNameLow.contains(exportMethod))
                exportFileFound = true;

        if (exportFileFound && AGlobal().exportExtensions.contains(extension))
        {
            emit addItem("Export", "MediaFile", folderName, fileInfo.fileName());
//            if (!AGlobal().projectExtensions.contains(extension))
                loadClips(fileInfo);
        }
        else if (AGlobal().videoExtensions.contains(extension))
        {
            emit addItem("Video", "MediaFile", folderName, fileInfo.fileName());
            loadClips(fileInfo);
        }
        else if (AGlobal().audioExtensions.contains(extension))
        {
            emit addItem("Audio", "MediaFile", folderName, fileInfo.fileName());
            loadClips(fileInfo);
        }
        else if (AGlobal().imageExtensions.contains(extension))
        {
            emit addItem("Image", "MediaFile", folderName, fileInfo.fileName());
            loadClips(fileInfo);
        }
        else if (AGlobal().projectExtensions.contains(extension))
        {
            emit addItem("Project", "MediaFile", folderName, fileInfo.fileName());
            loadClips(fileInfo);
        }
        //else  stuff which is outside ACVC

        if (AGlobal().videoExtensions.contains(extension) || AGlobal().imageExtensions.contains(extension)) //load video and images
        {
            loadMedia(folderName, fileInfo.fileName());
        }

    }
//    else
//        return nullptr;

//    QString oldAbsolutePath = dir.absolutePath();
//    qDebug()<<"before dir filters1"<<dir.absolutePath()<<dir.entryList().size();

    //read subfolders
    dir.setFilter(QDir::Files | QDir::AllDirs | QDir::NoDotAndDotDot | QDir::NoSymLinks);
    dir.setSorting(QDir::Name | QDir::LocaleAware); //localeaware to get +99999ms sorted the right way

    QStringList nameFilters;
    foreach (QString extension, AGlobal().exportExtensions << AGlobal().audioExtensions << AGlobal().imageExtensions << AGlobal().projectExtensions)
        nameFilters << "*." + extension;

//    qDebug()<<"nameFilters"<<nameFilters;
    dir.setNameFilters(nameFilters);

    //calculate totals before
    for (int i=0; i < dir.entryList().size(); i++)
    {
        QString fileNameLow = (dir.absolutePath() + "/" + dir.entryList().at(i)).toLower();
        int lastIndexOf = fileNameLow.lastIndexOf(".");
        QString extension = fileNameLow.toLower().mid(lastIndexOf + 1);

        if (AGlobal().videoExtensions.contains(extension) || AGlobal().audioExtensions.contains(extension) || AGlobal().imageExtensions.contains(extension))
        {
//            qDebug()<<"loadMediaTotal"<<dir.absolutePath()<<dir.entryList().at(i);
            loadMediaTotal ++;
        }
    }

    for (int i=0; i < dir.entryList().size(); i++)
    {

        if (!dir.entryInfoList().at(i).isDir())
        {

        }
        if (dir.entryList().at(i) != "ACVCRecycleBin")
        {
//            qDebug()<<"before loadfaf"<<dir.absolutePath()<<dir.entryList().at(i);
            loadFilesAndFolders(QDir(dir.absolutePath() + "/" + dir.entryList().at(i)));
        }
    }
}

void AGFileSystem::loadClips(QFileInfo fileInfo)
{
    QString srtFileName;
    int lastIndex = fileInfo.fileName().lastIndexOf(".");
    if (lastIndex > -1)
        srtFileName = fileInfo.fileName().left(lastIndex) + ".srt";

//        qDebug()<<"srtFileName"<<srtFileName;
    QFile file(fileInfo.path() + "/" + srtFileName);

    QStringList list;
    int nrOfClips = 0;

    if(file.open(QIODevice::ReadOnly))
    {
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

            emit addItem("MediaFile", "Clip", fileInfo.path() + "/", fileInfo.fileName(), clipDuration, inTime.msecsSinceStartOfDay(), outTime.msecsSinceStartOfDay());
            //parentItem,

            AStarRating starRating = qvariant_cast<AStarRating>(starItem->data(Qt::EditRole));
            if (starRating.starCount() != 0)
            {
                QString stars = QString("*").repeated(starRating.starCount());
                emit addItem("Clip", "Tag", fileInfo.path() + "/", fileInfo.fileName(), clipDuration, inTime.msecsSinceStartOfDay(), outTime.msecsSinceStartOfDay(), stars);
//                clipItem,
            }

            if (alike == "true")
                emit addItem("Clip", "Tag", fileInfo.path() + "/", fileInfo.fileName(), clipDuration, inTime.msecsSinceStartOfDay(), outTime.msecsSinceStartOfDay(), "✔");

            QStringList tagsList = tags.split(";");
            foreach (QString tag, tagsList)
                if (tag != "")
                    emit addItem("Clip", "Tag", fileInfo.path() + "/", fileInfo.fileName(), clipDuration, inTime.msecsSinceStartOfDay(), outTime.msecsSinceStartOfDay(), tag);
        }
    }
}

void AGFileSystem::onFileChanged(const QString &path)
{
    if (!fileSystemWatcher->files().contains(path))
    {
        QFile file(path);
        if (file.exists())
        {
            fileSystemWatcher->addPath(path);
            qDebug()<<"AGFileSystem::onFileChanged - not in watch - file exists (new file!) - added in watch"<<path;
        }
        else
            qDebug()<<"AGFileSystem::onFileChanged - not in watch - file not exists (deleted!)"<<path;
    }
    else
        qDebug()<<"AGFileSystem::onFileChanged - in watch"<<path;

}

void AGFileSystem::onDirectoryChanged(const QString &path)
{
//    qDebug()<<"AGFileSystem::onDirectoryChanged"<<path;

    QDirIterator it(path, QDir::Dirs|QDir::Drives|QDir::NoDotAndDotDot|QDir::AllDirs|QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        it.next();
        QFileInfo fileInfo = it.fileInfo();

        QString filePath = fileInfo.path() + "/" + fileInfo.fileName();

        if (!fileSystemWatcher->files().contains(filePath))
        {
            fileSystemWatcher->addPath(filePath);

//            qDebug()<<"  AGFileSystem::onDirectoryChanged - not in watch - file or filder exists (new!) - added in watch"<<filePath;
        }
        // else nothing special
    }
}

void AGFileSystem::loadMedia(QString folderName, QString fileName)
{
//    return;
//    if (!fileName.toLower().contains("bras"))
//        return;
    {
//        AJobParams jobParams;
//        jobParams.thisObject = this;
//        jobParams.parentItem = nullptr;
//        jobParams.folderName = folderName;
//        jobParams.fileName = fileName;
//        jobParams.action = "Load first frame";
//        jobParams.parameters["totalDuration"] = QString::number(1000);

//        jobTreeView->createJob(jobParams, [] (AJobParams jobParams)
//        {
            AGFileSystem *fileSystem = this;//qobject_cast<AGFileSystem *>(jobParams.thisObject);

//            QString folderName = jobParams.folderName;
//            QString fileName = jobParams.fileName;

            DerperView::VideoInfo videoInfo;

            QImage myImage = QImage();

            QString fileNameLow = fileName.toLower();
            int lastIndexOf = fileNameLow.lastIndexOf(".");
            QString extension = fileNameLow.toLower().mid(lastIndexOf + 1);

            if (AGlobal().videoExtensions.contains(extension))
            {
//                qDebug()<<"AGFileSystem::loadMedia frame"<<folderName<<fileName;
                DerperView::InputVideoFile input((folderName + fileName).toUtf8().constData());

                videoInfo = input.GetVideoInfo();

                if (input.GetLastError() != 0)
                    qDebug()<<"myImage error"<<QString::number(input.GetLastError());
                input.Dump();

                AVFrame *frame = input.GetNextFrame();

                int frameCounter = 0;
                while (frame->width == 0 && frame != nullptr)
                {
                    frame = input.GetNextFrame();
                    frameCounter++;
                }

//                AVPixelFormat pixelFormat;

                if (frame->format == 0 || frame->format == 12 || frame->format == 13 || true)
                {
                    //AV_PIX_FMT_YUVJ420P = 12 e.g. gopro
                    //AV_PIX_FMT_YUV420P = 0 e.g. ffmpeg lossless and encode
                    //AV_PIX_FMT_YUVJ422P = 13 (e.g. fatshark avi)

                    myImage = QImage(frame->width,frame->height,QImage::Format_RGB888);

                    //https://forum.qt.io/topic/22137/convert-yuv420-to-rgb-and-show-on-qt-pixmap/2
                    for (int y = 0; y < frame->height; y++)
                    {
                        for (int x = 0; x < frame->width; x++)
                        {
                            const int xx = x >> 1;
                            const int yy = y >> 1;
                            const int Y = frame->data[0][y * frame->linesize[0] + x] - 16;
                            const int U = frame->data[1][yy * frame->linesize[1] + xx] - 128;
                            const int V = frame->data[2][yy * frame->linesize[2] + xx] - 128;
                            const int r = qBound(0, (298 * Y + 409 * V + 128) >> 8, 255);
                            const int g = qBound(0, (298 * Y - 100 * U - 208 * V + 128) >> 8, 255);
                            const int b = qBound(0, (298 * Y + 516 * U + 128) >> 8, 255);

                            myImage.setPixel(x, y, qRgb(r, g, b));
                        }
                    }
                }
                else
                    myImage = QImage((uchar*)frame->data[0], frame->width, frame->height, frame->linesize[0], QImage::Format_RGB888);

            }
            else if (AGlobal().imageExtensions.contains(extension))
            {
                myImage.load(folderName + fileName);
            }

            if (myImage != QImage())
            {
                int duration;

                QStringList ffMpegMetaString;
                if (videoInfo.avg_frame_rate.num != 0)
                {
                    duration = 1000.0 * videoInfo.totalFrames / videoInfo.avg_frame_rate.num * videoInfo.avg_frame_rate.den;

                    ffMpegMetaString << "Width = " + QString::number(videoInfo.width);
                    ffMpegMetaString << "Height = " + QString::number(videoInfo.height);
                    ffMpegMetaString << "Total frames = " + QString::number(videoInfo.totalFrames);
                    if (videoInfo.frameRate.den != 0)
                        ffMpegMetaString << "Framerate = " + QString::number(videoInfo.frameRate.num / videoInfo.frameRate.den);
                    ffMpegMetaString << "Average framerate = " + QString::number(videoInfo.avg_frame_rate.num / videoInfo.avg_frame_rate.den);
                }
                else
                    duration = 0; //for images

//                qDebug()<<"AGFileSystem::loadMedia videoinfo"<<duration<<videoInfo.totalFrames << videoInfo.avg_frame_rate.num<<videoInfo.avg_frame_rate.den;

                emit fileSystem->mediaLoaded(folderName, fileName, myImage, duration , QSize(videoInfo.width,videoInfo.height), ffMpegMetaString.join(";"));
            }
            else
                emit fileSystem->mediaLoaded(folderName, fileName, myImage, -1 , QSize(), ""); //as not always image loaded (e.g. png currently)

//            return QString();
//        }, nullptr);
    }
}
