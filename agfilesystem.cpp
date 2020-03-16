#include "agfilesystem.h"

#include "aglobal.h"
#include "astarrating.h"

#include <QDebug>
#include <QDateTime>
#include <QLabel>
#include <QGraphicsItem>

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

QGraphicsItem * AGFileSystem::loadFilesAndFolders(AGView *view, QGraphicsItem *parentItem , QDir dir)
{
    QFileInfo fileInfo(dir.absolutePath());
    fileSystemWatcher->addPath(fileInfo.path() + "/" + fileInfo.fileName());

    if (parentItem == nullptr)
        view->clearAll();

    QGraphicsItem *childItem = nullptr;

    if (fileInfo.isDir())
    {
        qDebug() <<"AGFileSystem::loadFilesAndFolders isDir"<< fileInfo.path() + "/" + fileInfo.fileName()<< fileInfo.metadataChangeTime().toString();

        childItem = view->addItem(parentItem, "Folder", fileInfo.path() + "/", fileInfo.fileName());

        videoParentItem = view->addItem(childItem, "FileGroup", fileInfo.path() + "/" + fileInfo.fileName(), "Video");
        audioParentItem = view->addItem(childItem, "FileGroup", fileInfo.path() + "/" + fileInfo.fileName(), "Audio");
        imageParentItem = view->addItem(childItem, "FileGroup", fileInfo.path() + "/" + fileInfo.fileName(), "Image");
        exportParentItem = view->addItem(childItem, "FileGroup", fileInfo.path() + "/" + fileInfo.fileName(), "Export");

        videoTimelineParentItem = view->addItem(videoParentItem, "TimelineGroup", fileInfo.path() + "/" + fileInfo.fileName(), "Timeline");
        audioTimelineParentItem = view->addItem(audioParentItem, "TimelineGroup", fileInfo.path() + "/" + fileInfo.fileName(), "Timeline");
        imageTimelineParentItem = view->addItem(imageParentItem, "TimelineGroup", fileInfo.path() + "/" + fileInfo.fileName(), "Timeline");
        exportTimelineParentItem = view->addItem(exportParentItem, "TimelineGroup", fileInfo.path() + "/" + fileInfo.fileName(), "Timeline");

    }
    else// if (parentItem->toolTip().contains("Video"))
    {
        QString srtFileName;
        int lastIndex = fileInfo.fileName().lastIndexOf(".");
        if (lastIndex > -1)
            srtFileName = fileInfo.fileName().left(lastIndex) + ".srt";

//        qDebug()<<"srtFileName"<<srtFileName;
        QFile file(fileInfo.path() + "/" + srtFileName);

//        qDebug()<< ""<< fileInfo.path()<< fileInfo.fileName()<<fileInfo.size()/1024<<fileInfo.metadataChangeTime().toString();

        QString mode = "fileView"; //default
//        QString mode = "timelineView"; //default

        if (!fileInfo.fileName().toLower().contains(".mp3"))
        {
//            qDebug()<<"loadMedia"<<fileInfo.path() + "/" << fileInfo.fileName();
            loadMedia(fileInfo.path() + "/", fileInfo.fileName());
        }

        if (fileInfo.fileName().toLower().contains("lossless") || fileInfo.fileName().toLower().contains("encode") || fileInfo.fileName().toLower().contains("shotcut") || fileInfo.fileName().toLower().contains("premiere"))
        {
            childItem = view->addItem(exportParentItem, "MediaFile", fileInfo.path() + "/", fileInfo.fileName());
            loadClips(mode=="fileView"?childItem:exportTimelineParentItem, file, view, fileInfo);
        }
        else if (fileInfo.fileName().toLower().contains(".mp4"))
        {
            childItem = view->addItem(videoParentItem, "MediaFile", fileInfo.path() + "/", fileInfo.fileName());
            loadClips(mode=="fileView"?childItem:videoTimelineParentItem, file, view, fileInfo);
        }
        else if (fileInfo.fileName().toLower().contains(".mp3"))
        {
            childItem = view->addItem(audioParentItem, "MediaFile", fileInfo.path() + "/", fileInfo.fileName());
            loadClips(mode=="fileView"?childItem:audioTimelineParentItem, file, view, fileInfo);
        }
        else
        {
            childItem = view->addItem(imageParentItem, "MediaFile", fileInfo.path() + "/", fileInfo.fileName());
            loadClips(mode=="fileView"?childItem:imageTimelineParentItem, file, view, fileInfo);
        }

    }
//    else
//        return nullptr;

    //read subfolders
    dir.setFilter(QDir::Files | QDir::AllDirs | QDir::NoDotAndDotDot | QDir::NoSymLinks);
    dir.setSorting(QDir::Name | QDir::LocaleAware); //localeaware to get +99999ms sorted the right way

    dir.setNameFilters(QStringList() << "*.MP4"<<"*.AVI"<<"*.WMV"<<"*.MTS"<< "*.mp3"<<"*.JPG");

    for (int i=0; i < dir.entryList().size(); ++i)
    {
        if (dir.entryList().at(i) != "ACVCRecycleBin")
        {
            loadFilesAndFolders(view, childItem, QDir(dir.absolutePath() + "/" + dir.entryList().at(i)));
        }
    }

    if (fileInfo.isDir())
    {
    }

    return childItem;
}

void AGFileSystem::loadClips(QGraphicsItem *parentItem, QFile &file, AGView *view, QFileInfo fileInfo)
{
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

            QGraphicsItem *clipItem = view->addItem(parentItem, "Clip", fileInfo.path() + "/", fileInfo.fileName(), clipDuration, inTime.msecsSinceStartOfDay(), outTime.msecsSinceStartOfDay());

            AStarRating starRating = qvariant_cast<AStarRating>(starItem->data(Qt::EditRole));
            if (starRating.starCount() != 0)
            {
                QString stars = QString("*").repeated(starRating.starCount());
                view->addItem(clipItem, "Tag", fileInfo.path() + "/", fileInfo.fileName(), clipDuration, inTime.msecsSinceStartOfDay(), outTime.msecsSinceStartOfDay(), stars);
            }

            if (alike == "true")
                view->addItem(clipItem, "Tag", fileInfo.path() + "/", fileInfo.fileName(), clipDuration, inTime.msecsSinceStartOfDay(), outTime.msecsSinceStartOfDay(), "✔");

            QStringList tagsList = tags.split(";");
            foreach (QString tag, tagsList)
                if (tag != "")
                    view->addItem(clipItem, "Tag", fileInfo.path() + "/", fileInfo.fileName(), clipDuration, inTime.msecsSinceStartOfDay(), outTime.msecsSinceStartOfDay(), tag);
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
    qDebug()<<"AGFileSystem::onDirectoryChanged"<<path;

    QDirIterator it(path, QDir::Dirs|QDir::Drives|QDir::NoDotAndDotDot|QDir::AllDirs|QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        it.next();
        QFileInfo fileInfo = it.fileInfo();

        QString filePath = fileInfo.path() + "/" + fileInfo.fileName();

        if (!fileSystemWatcher->files().contains(filePath))
        {
            fileSystemWatcher->addPath(filePath);

            qDebug()<<"  AGFileSystem::onDirectoryChanged - not in watch - file or filder exists (new!) - added in watch"<<filePath;
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
        AJobParams jobParams;
        jobParams.thisWidget = this;
        jobParams.parentItem = nullptr;
        jobParams.folderName = folderName;
        jobParams.fileName = fileName;
        jobParams.action = "Load first frame";
        jobParams.parameters["totalDuration"] = QString::number(1000);

        jobTreeView->createJob(jobParams, [] (AJobParams jobParams)
        {
            AGFileSystem *fileSystem = qobject_cast<AGFileSystem *>(jobParams.thisWidget);

            QString folderName = jobParams.folderName;
            QString fileName = jobParams.fileName;

            DerperView::VideoInfo videoInfo;

            QImage myImage = QImage();
            if (fileName.toLower().contains(".mp4"))
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

//                qDebug()<<"AGFileSystem::loadMedia frame"<<folderName<<fileName<<frameCounter<<frame->format<<frame->width<<frame->height<<frame->sample_aspect_ratio.num<<frame->sample_rate<<frame->key_frame<<frame->pict_type;

//                qDebug()<<"AGFileSystem::loadMedia myImage"<<myImage<<myImage.size();
    //            myImage.save(folderName + fileName + ".png");
            }

           else if (fileName.toLower().contains(".jpg"))
            {
                myImage.load(folderName + fileName);

            }
//            else if (fileName.toLower().contains(".mp3"))
//            {
//                myImage.load(":/acvc.ico");
//            }
//            else
//                myImage.load(":/movie.png");

            if (myImage != QImage())
            {
                int duration;

                if (videoInfo.frameRate.num != 0)
                    duration = 1000 * videoInfo.totalFrames / videoInfo.frameRate.num * videoInfo.frameRate.den;
                else
                    duration = 0; //for images

                emit fileSystem->mediaLoaded(folderName, fileName, myImage, duration , QSize(videoInfo.width,videoInfo.height));
            }
//            qDebug()<<"AGFileSystem::loadMedia videoinfo"<<videoInfo.totalFrames << videoInfo.frameRate.num<<videoInfo.frameRate.den;

            return QString();
        }, nullptr);
    }
}
