#include "agfilesystem.h"

#include "aglobal.h"
#include "astarrating.h"

#include <QDebug>
#include <QDateTime>
#include <QLabel>
#include <QFile>
#include <QSettings>
#include <QTimer>

AGFileSystem::AGFileSystem(QObject *parent):QObject(parent)
{
//    qDebug()<<"AGFileSystem::AGFileSystem";

    fileSystemWatcher = new QFileSystemWatcher(this);

    connect(fileSystemWatcher, &QFileSystemWatcher::fileChanged, this, &AGFileSystem::onFileChanged);
    connect(fileSystemWatcher, &QFileSystemWatcher::directoryChanged, this, &AGFileSystem::onDirectoryChanged);
}

void AGFileSystem::loadFilesAndFolders(QDir dir, AJobParams jobParams)
{
    QFileInfo fileInfo(dir.absolutePath());
    QString folderName = fileInfo.path();

    if (folderName.right(1) != "/")
        folderName = folderName + "/"; //on windows, in case of D:/

//    qDebug()<<"AGFileSystem::loadFilesAndFolders"<<folderName + fileInfo.fileName();

    emit jobAddLog(jobParams, "loadFilesAndFolders: " + folderName + fileInfo.fileName());

    fileSystemWatcher->addPath(folderName + fileInfo.fileName());
//    if ()
//        qDebug()<<"fileSystemWatcher->addPath true"<<folderName + fileInfo.fileName();
//    else
//        qDebug()<<"fileSystemWatcher->addPath false"<<folderName + fileInfo.fileName();

    if (fileInfo.isDir())
    {
        emit addItem("Folder", "Folder", folderName, fileInfo.fileName());

        emit addItem("Folder", "FileGroup", folderName + fileInfo.fileName() + "/", "Video");
        emit addItem("Folder", "FileGroup", folderName + fileInfo.fileName() + "/", "Image");
        emit addItem("Folder", "FileGroup", folderName + fileInfo.fileName() + "/", "Audio");
        emit addItem("Folder", "FileGroup", folderName + fileInfo.fileName() + "/", "Export");
        emit addItem("Folder", "FileGroup", folderName + fileInfo.fileName() + "/", "Project");
        emit addItem("Folder", "FileGroup", folderName + fileInfo.fileName() + "/", "Parking");

        emit addItem("Video", "TimelineGroup", folderName + fileInfo.fileName() + "/", "Timeline");
        emit addItem("Image", "TimelineGroup", folderName + fileInfo.fileName() + "/", "Timeline");
        emit addItem("Audio", "TimelineGroup", folderName + fileInfo.fileName() + "/", "Timeline");
        emit addItem("Export", "TimelineGroup", folderName + fileInfo.fileName() + "/", "Timeline");
        emit addItem("Project", "TimelineGroup", folderName + fileInfo.fileName() + "/", "Timeline");
        emit addItem("Parking", "TimelineGroup", folderName + fileInfo.fileName() + "/", "Timeline");

    }
    else // file  if (parentItem->toolTip().contains("Video"))
    {
        loadOrModifyItem(jobParams, folderName, fileInfo.fileName(), true, false); //loadMedia not in seperate job because here we have already a job running
    }

    //read subfolders
    dir.setFilter(QDir::Files | QDir::AllDirs | QDir::NoDotAndDotDot | QDir::NoSymLinks);
    dir.setSorting(QDir::Name | QDir::LocaleAware | QDir::IgnoreCase); //localeaware to get +99999ms sorted the right way

    QStringList nameFilters;
    foreach (QString extension, AGlobal().exportExtensions << AGlobal().audioExtensions << AGlobal().imageExtensions << AGlobal().projectExtensions)
        nameFilters << "*." + extension;

//    qDebug()<<"nameFilters"<<nameFilters;
    dir.setNameFilters(nameFilters);

    //calculate totals before
    for (int i=0; i < dir.entryList().size(); i++)
    {
        if (dir.entryList().at(i) != "ACVCRecycleBin")
        {
            if (!dir.entryInfoList().at(i).isDir())
                    loadMediaTotal ++; //all selected files will cause medialoaded
        }
    }

    for (int i=0; i < dir.entryList().size(); i++)
    {

        if (dir.entryList().at(i) != "ACVCRecycleBin")
        {
//            qDebug()<<"before loadfaf"<<dir.absolutePath()<<dir.entryList().at(i);
            loadFilesAndFolders(QDir(dir.absolutePath() + "/" + dir.entryList().at(i)), jobParams);
        }
    }
//    if (fileInfo.isDir())
//    {
//        qDebug()<<"loadfaf arrangeItems isDir"<<dir;
//        emit arrangeItems();
//    }
}

void AGFileSystem::loadOrModifyItem(AJobParams jobParams, QString folderName, QString fileName, bool isNewFile, bool loadMediaInJob)
{
//    qDebug()<<"AGFileSystem::loadOrModifyItem jobParams"<<jobParams.folderName<<jobParams.fileName;

    emit jobAddLog(jobParams, "loadOrModifyItem: " + folderName + fileName);

    QString fileNameLow = fileName.toLower();
    int lastIndexOf = fileNameLow.lastIndexOf(".");
    QString extension = fileNameLow.toLower().mid(lastIndexOf + 1);

    bool exportFileFound = false;
    foreach (QString exportMethod, AGlobal().exportMethods)
        if (fileNameLow.contains(exportMethod))
            exportFileFound = true;

    if (isNewFile)
    {
        if (exportFileFound && AGlobal().exportExtensions.contains(extension))
        {
            emit addItem("Export", "MediaFile", folderName, fileName);
            loadClips("Export", folderName, fileName);
            emit arrangeItems();
        }
        else if (AGlobal().videoExtensions.contains(extension))
        {
            emit addItem("Video", "MediaFile", folderName, fileName);
            loadClips("Video", folderName, fileName);
            emit arrangeItems();
        }
        else if (AGlobal().audioExtensions.contains(extension))
        {
            emit addItem("Audio", "MediaFile", folderName, fileName);
            loadClips("Audio", folderName, fileName);
            emit arrangeItems();
        }
        else if (AGlobal().imageExtensions.contains(extension))
        {
            emit addItem("Image", "MediaFile", folderName, fileName);
            loadClips("Image", folderName, fileName);
            emit arrangeItems();
        }
        else if (AGlobal().projectExtensions.contains(extension))
        {
            emit addItem("Project", "MediaFile", folderName, fileName);
//            loadClips("Project", folderName, fileName); //no clips for project files
            emit arrangeItems();
        }
        else if (extension == "srt")
        {
            loadClips("Project", folderName, fileName); //if mediafile does not exists addClip of clip and tags will return (workaround)
            emit arrangeItems();
        }

        //else  stuff which is outside ACVC
    }
    else
    {
        if (extension == "srt")
        {
            //remove existing clips
            emit deleteItem("Clip", folderName, fileName); //is .srt

            loadClips("Project", folderName, fileName); //if mediafile does not exists addClip of clip and tags will return (workaround)
            emit arrangeItems();
        }
    }

    if (AGlobal().videoExtensions.contains(extension) || AGlobal().audioExtensions.contains(extension) || AGlobal().imageExtensions.contains(extension)) //load video and images
    {
        if (!loadMediaInJob) //for loadFilesAndFolders
        {
            loadMedia(jobParams, folderName, fileName, isNewFile, loadMediaInJob);
        }
        else
        {
            //check if already in JobQueue because AGFileSystem::onFileChanged is triggered more then once during ffmpeg file creation

            bool inQueue = false;
            for (int i=0; i<jobTreeView->jobItemModel->rowCount();i++)
            {
                if (jobTreeView->jobItemModel->index(i,jobTreeView->headerlabels.indexOf("Job")).data().toString().contains(fileName)
                        && jobTreeView->jobItemModel->index(i,jobTreeView->headerlabels.indexOf("Command")).data().toString() == "Load media"
                        && jobTreeView->jobItemModel->index(i,jobTreeView->headerlabels.indexOf("Log")).data().toString() == "" //not started
                        )
                {
                   inQueue = true;
                }
            }

            if (!inQueue)
            {
                loadMediaTotal++;

//                qDebug()<<"AGFileSystem::loadOrModifyItem: loadMedia added to jobqueue"<<folderName<<fileName;

                //use jobqueue to make sure it will be done after encode or lossless process completes
                AJobParams jobParams;
                jobParams.thisObject = this;
                jobParams.parentItem = nullptr;
                jobParams.folderName = folderName;
                jobParams.fileName = fileName;
                jobParams.action = "Load media";
                jobParams.parameters["totalDuration"] = QString::number(1000);
                jobParams.parameters["isNewFile"] = isNewFile?"true":"false";

                jobTreeView->createJob(jobParams, [] (AJobParams jobParams)
                {
                    AGFileSystem *fileSystem = qobject_cast<AGFileSystem *>(jobParams.thisObject);

                    fileSystem->loadMedia(jobParams, jobParams.folderName, jobParams.fileName, jobParams.parameters["isNewFile"]=="true"?true:false, false); //

                    return QString();

                }, nullptr);

            }
        }
    }

}

void AGFileSystem::loadClips(QString parentName, QString folderName, QString fileName)
{
    int lastIndex = fileName.lastIndexOf(".");
    QString srtFileName = fileName.left(lastIndex) + ".srt";

//    qDebug()<<"AGFileSystem::loadClips"<<folderName<<srtFileName;

    QFile file(folderName + srtFileName);

    QStringList list;
    int nrOfClips = 0;

    if(file.open(QIODevice::ReadOnly))
    {
        fileSystemWatcher->addPath(folderName + srtFileName);
//        if ()
//            qDebug()<<"fileSystemWatcher->addPath true "<<folderName + srtFileName;
//        else
//            qDebug()<<"fileSystemWatcher->addPath false "<<folderName + srtFileName;

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

            emit addItem(parentName, "Clip", folderName, fileName, clipDuration, inTime.msecsSinceStartOfDay(), outTime.msecsSinceStartOfDay());

            if (true)
            {
                AStarRating starRating = qvariant_cast<AStarRating>(starItem->data(Qt::EditRole));
                if (starRating.starCount() != 0)
                {
                    QString stars = QString("*").repeated(starRating.starCount());
                    emit addItem("Clip", "Tag", folderName, fileName, clipDuration, inTime.msecsSinceStartOfDay(), outTime.msecsSinceStartOfDay(), stars);
    //                clipItem,
                }

                if (alike == "true")
                    emit addItem("Clip", "Tag", folderName, fileName, clipDuration, inTime.msecsSinceStartOfDay(), outTime.msecsSinceStartOfDay(), "✔");

                QStringList tagsList = tags.split(";");
                foreach (QString tag, tagsList)
                    if (tag != "")
                        emit addItem("Clip", "Tag", folderName, fileName, clipDuration, inTime.msecsSinceStartOfDay(), outTime.msecsSinceStartOfDay(), tag);
            }
        }
    }
}

void AGFileSystem::onFileChanged(const QString &path)
{
    QFileInfo fileInfo(path);

    QString fileNameLow = fileInfo.fileName().toLower();
    int lastIndexOf = fileNameLow.lastIndexOf(".");
    QString extension = fileNameLow.toLower().mid(lastIndexOf + 1);

    if (!fileSystemWatcher->files().contains(path))
    {
        QFile file(path);
        if (file.exists())
        {
            fileSystemWatcher->addPath(path);
//            if ()
//                qDebug()<<"AGFileSystem::onFileChanged true addpath - not in watch - file exists (new file!) - added in watch"<<path;
//            else
//                qDebug()<<"AGFileSystem::onFileChanged false addpath - not in watch - file exists (new file!) - added in watch"<<path;
        }
        else
        {
//            qDebug()<<"AGFileSystem::onFileChanged - not in watch - file not exists (deleted!)"<<path;
            QFileInfo fileInfo(path);

            if (extension == "srt") //clips
                emit deleteItem("Clip", fileInfo.path() + "/", fileInfo.fileName());
            else
                emit deleteItem("MediaFile", fileInfo.path() + "/", fileInfo.fileName());
        }
    }
    else //file changed
    {
//        qDebug()<<"AGFileSystem::onFileChanged - in watch"<<path;

        //add a small delay to give OS the chance to release lock on file (avoid Permission denied erro)
        QTimer::singleShot(500, this, [this, fileInfo]()->void
        {
            loadOrModifyItem(AJobParams(), fileInfo.path() + "/", fileInfo.fileName(), false, true);
        });
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

        if (!fileSystemWatcher->files().contains(filePath) && !fileSystemWatcher->directories().contains(filePath) && !filePath.contains("ACVCRecycleBin"))
        {
            bool result = fileSystemWatcher->addPath(filePath);
//            if (result)
//                qDebug()<<"AGFileSystem::onDirectoryChanged true addpath - not in watch - file or folder exists (new!) - added in watch"<<filePath;
//            else
//                qDebug()<<"AGFileSystem::onDirectoryChanged false addpath - not in watch - file or folder exists (new!) - added in watch"<<filePath;

            QString folderName = fileInfo.path() + "/";

            loadOrModifyItem(AJobParams(), folderName, fileInfo.fileName(), true, true);

        }
//        else //nothing special
//            qDebug()<<"  AGFileSystem::onDirectoryChanged already in watch - file or folder exists ()"<<filePath;
    }
}

void AGFileSystem::loadMedia(AJobParams jobParams, QString folderName, QString fileName, bool isNewFile, bool loadMediaInJob)
{
//    qDebug()<<"AGFileSystem::loadMedia"<<folderName<<fileName;

    emit jobAddLog(jobParams, "LoadMedia: " + folderName + fileName);

    AGFileSystem *fileSystem = this;

    DerperView::VideoInfo videoInfo;

    QImage myImage = QImage();

    QString fileNameLow = fileName.toLower();
    int lastIndexOf = fileNameLow.lastIndexOf(".");
    QString extension = fileNameLow.toLower().mid(lastIndexOf + 1);

    if (AGlobal().videoExtensions.contains(extension))
    {
        DerperView::InputVideoFile input((folderName + fileName).toUtf8().constData());

        if (input.GetLastError() != 0)
        {
            QString errorMessage = "";
            if (input.GetLastError() == -13)
                errorMessage = tr("Load Media. Error on opening file %1. Permission denied").arg(fileName);

            if (jobParams.fileName != "")
                emit jobAddLog(jobParams, "Error: " + errorMessage + "(" + QString::number(input.GetLastError()) + ")");

            loadMediaTotal--;

//            QTimer::singleShot(500, this, [this, folderName, fileName, isNewFile, loadMediaInJob]()->void
//            {
//                loadOrModifyItem(AJobParams(), folderName, fileName, isNewFile, loadMediaInJob); //false: no new file
//            });
        }
        else
        {
            videoInfo = input.GetVideoInfo();

            AVFrame *frame = input.GetNextFrame();

            int frameCounter = 0;
            while (frame->width == 0 && frame != nullptr) //audioframes
            {
                frame = input.GetNextFrame();
                frameCounter++;
            }

//            for (int i = 0; i<QSettings().value("frameRate").toInt();) //skip first second
//            {
////                qDebug()<<"next frame";
//                frame = input.GetNextFrame();
//                if (frame->width != 0 || frame == nullptr)
//                    i++;
//            }


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

//            while (frame != nullptr)
//            {
//                qDebug()<<frameCounter;
//                frame = input.GetNextFrame();
//                frameCounter++;
//            }

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

//            qDebug()<<"AGFileSystem::loadMedia videoinfo"<<duration<<videoInfo.totalFrames << videoInfo.avg_frame_rate.num<<videoInfo.avg_frame_rate.den;

            emit jobAddLog(jobParams, "LoadMedia->mediaLoaded: " + folderName + fileName + " d: " + QString::number(duration) + " s: " + QString::number(videoInfo.width) + "x" + QString::number(videoInfo.height) + " m: " + ffMpegMetaString.join(";"));

            emit fileSystem->mediaLoaded(folderName, fileName, myImage, duration , QSize(videoInfo.width,videoInfo.height), ffMpegMetaString.join(";"));

    //                qDebug()<<"AGFileSystem::loadMedia frame"<<folderName<<fileName<<videoInfo.audioSampleRate<<videoInfo.totalFrames<<videoInfo.audioBitRate<<videoInfo.audioChannelLayout<<videoInfo.audioTimeBase.num << videoInfo.audioTimeBase.den;
            input.Dump();
        }
    } //if video
    else if (AGlobal().imageExtensions.contains(extension))
    {
        myImage.load(folderName + fileName);

        emit jobAddLog(jobParams, "LoadMedia->mediaLoaded: " + folderName + fileName + " s: " + QString::number(myImage.width()) + "x" + QString::number(myImage.height()));

        emit fileSystem->mediaLoaded(folderName, fileName, myImage, 0 , QSize(myImage.width(), myImage.height())); //as not always image loaded (e.g. png currently)
    }
    else if (AGlobal().audioExtensions.contains(extension))
    {
        DerperView::InputVideoFile input((folderName + fileName).toUtf8().constData());

        if (input.GetLastError() != 0)
        {
            QString errorMessage = "";
            if (input.GetLastError() == -13)
                errorMessage = tr("Load Media. Error on opening file %1: Permission denied").arg(fileName);

            if (jobParams.fileName != "")
                emit jobAddLog(jobParams, "Error: " + errorMessage + "(" + QString::number(input.GetLastError()) + ")");

            loadMediaTotal--;
        }
        else
        {
            videoInfo = input.GetVideoInfo();

            AVFrame *frame = input.GetNextFrame();

            QList<int> samples;

            int counter = 0;
            int progressInMSeconds = 0;
            int durationInMSeconds = 1000 * videoInfo.audioDuration / videoInfo.audioTimeBase.den * videoInfo.audioTimeBase.num;
            while (frame != nullptr)
            {

                //https://stackoverflow.com/questions/37151584/de-quantising-audio-with-ffmpeg
                //https://steemit.com/programming/@targodan/decoding-audio-files-with-ffmpeg
                float sample_total = 0;
                float sample_min = 1e99;
                float sample_max = -1e99;

                //                    if (frame->width == 0) // Audio - stream it through
                if (frame->format == AV_SAMPLE_FMT_FLTP )
                {
                    for (int c = 0; c < 1; c++)//frame->channels
                    {
                        unsigned char *samples = frame->extended_data[c];
                        for (int i = 0; i < frame->nb_samples; i++)
                        {
                            float sample = samples[i];
                            sample_total += sample;

                            if (sample_min == 1e99)
                                sample_min = sample;
                            else
                                sample_min = qMin(sample_min, sample);

                            if (sample_max == -1e99)
                                sample_max = sample;
                             else
                                sample_max = qMax(sample_max, sample);
                            // now this sample is accessible, it's in the range [-1.0, 1.0]
                        }
                    }
                }

                progressInMSeconds = 1000 * frame->pkt_dts / videoInfo.audioTimeBase.den * videoInfo.audioTimeBase.num;
//                    qDebug()<<"frame"<<counter<<frame->pts<<frame->pts/368640.0<<frame->pkt_dts<<frame->best_effort_timestamp<<frame->format<<frame->channels<<frame->nb_samples<<sample_total<<sample_min<<sample_max<<sample_total / frame->nb_samples / frame->channels<<frame->extended_data[0][frame->nb_samples / 2]<<progressInMSeconds;

                //frame->sample_rate=44.100

                if (counter%20 == 0)
                {
//                    qDebug()<<counter<<progressInMSeconds / 100<<durationInMSeconds<<frame->extended_data[0][frame->nb_samples / 2]/2.56;
//                    painterPath.lineTo((1.0 * progressInMSeconds / durationInMSeconds) * durationInMSeconds / 500.0, sample_total / frame->nb_samples / frame->channels);
                    samples << frame->extended_data[0][frame->nb_samples / 2]/2.56;
                }

                counter++;
                frame = input.GetNextFrame();
            }

//                qDebug()<<"frames"<<counter;


//                qDebug()<<"AGFileSystem::loadMedia frame"<<folderName<<fileName<<videoInfo.audioSampleRate<<videoInfo.audioBitRate<<videoInfo.audioChannelLayout<<videoInfo.audioTimeBase.num << videoInfo.audioTimeBase.den<<videoInfo.duration<<durationInMSeconds;
            //audioTimeBase is like fps

            emit jobAddLog(jobParams, "LoadMedia->mediaLoaded: " + folderName + fileName + " d: " + QString::number(durationInMSeconds) + " s: " + QString::number(samples.count()));

            emit fileSystem->mediaLoaded(folderName, fileName, QImage(), durationInMSeconds , QSize(), "", samples); //as not always image loaded (e.g. png currently)

            input.Dump();
        }

    }  //if audio
}
