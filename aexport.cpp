#include "aexport.h"

#include <QDebug>
#include <QSettings>
#include <QTime>

#include "aglobal.h"

#include <QMessageBox>

#include <QTimer>

#include <qmath.h>

#include <QApplication>

AExport::AExport(QWidget *parent) : QWidget(parent)
{
}

void AExport::s(QString inputString, QString arg1, QString arg2, QString arg3, QString arg4)
{
    if (arg4 != "")
        stream<<inputString.arg(arg1, arg2, arg3, arg4)<<endl;
    else if (arg3 != "")
        stream<<inputString.arg(arg1, arg2, arg3)<<endl;
    else if (arg2 != "")
        stream<<inputString.arg(arg1, arg2)<<endl;
    else if (arg1 != "")
        stream<<inputString.arg(arg1)<<endl;
    else
        stream<<inputString<<endl;
}

QStandardItem *AExport::losslessVideoAndAudio(QStandardItem *parentItem)
{
    //create audio and video stream
    QStringList mediaTypeList;

    mediaTypeList << "V";
    if (audioClipsMap.count() > 0)
        mediaTypeList << "A";

    QStandardItem *childItem = parentItem;

    foreach (QString mediaType, mediaTypeList) //prepare video and audio
    {
        QFile vidlistFile(selectedFolderName + fileNameWithoutExtension + mediaType + ".txt");
//        qDebug()<<"opening vidlistfile"<<vidlistFile;
        if ( vidlistFile.open(QIODevice::WriteOnly) )
        {
            QTextStream vidlistStream( &vidlistFile );

            bool clipsFound = false;

            int iterationCounter = 0;
            QMapIterator<int, int> clipsIterator(clipsMap);
            while (clipsIterator.hasNext()) //all files
            {
                clipsIterator.next();
                int row = clipsIterator.value();

                QTime inTime = QTime::fromString(timelineModel->index(row, inIndex).data().toString(),"HH:mm:ss.zzz");
                QTime outTime = QTime::fromString(timelineModel->index(row, outIndex).data().toString(),"HH:mm:ss.zzz");
                QString fileName = timelineModel->index(row, fileIndex).data().toString();

                if ((mediaType == "V" && !fileName.toLower().contains(".mp3")) || (mediaType == "A" && fileName.toLower().contains(".mp3"))) //if video then video files, if audio then audio files
                {
                    if (mediaType == "V" && videoFileExtension == "")
                    {
                        int lastIndex = fileName.lastIndexOf(".");
                        videoFileExtension = fileName.mid(lastIndex);
                    }
                    vidlistStream << "file '" << timelineModel->index(row, folderIndex).data().toString() + timelineModel->index(row, fileIndex).data().toString() << "'" << endl;
                    if (transitionTimeFrames > 0)
                    {
                        if (iterationCounter != 0)
                        {
                            inTime = inTime.addMSecs(AGlobal().frames_to_msec((transitionTimeFrames/2))); //subtract half of the transitionframes
                        }
                        if (iterationCounter != clipsMap.count()-1)
                        {
                            outTime = outTime.addMSecs(- AGlobal().frames_to_msec(qRound(transitionTimeFrames/2.0))); //subtract half of the transitionframes
                        }
                    }
//                    qDebug()<<"gen"<<mediaType<<row<<transitionTimeFrames/2<<qRound(transitionTimeFrames/2.0)<<AGlobal().frames_to_msec((transitionTimeFrames/2))<<inTime<<outTime<<fileExtension;

                    vidlistStream << "inpoint " <<  QString::number(inTime.msecsSinceStartOfDay() / 1000.0, 'g', 6) << endl;
                    vidlistStream << "outpoint " << QString::number((outTime.msecsSinceStartOfDay()) / 1000.0, 'g', 6) << endl;

                    clipsFound = true;

                }
                iterationCounter++;
            }

            vidlistFile.close();

            if (clipsFound) //at least one file found
            {
                QString fileNamePlusExtension;
                if (mediaType == "V")
                {
                    if (audioClipsMap.count() > 0)
                        fileNamePlusExtension = fileNameWithoutExtension + "V" + videoFileExtension;
                    else
                        fileNamePlusExtension = fileNameWithoutExtension + videoFileExtension;
                }
                else
                    fileNamePlusExtension = fileNameWithoutExtension + + "A.mp3";

                QString sourceFolderFileName = selectedFolderName + fileNameWithoutExtension + mediaType + ".txt";
                QString targetFolderFileName = selectedFolderName + fileNamePlusExtension;

#ifdef Q_OS_WIN
                sourceFolderFileName = sourceFolderFileName.replace("/", "\\");
                targetFolderFileName = targetFolderFileName.replace("/", "\\");
#endif
                QString command = "ffmpeg -f concat -safe 0 -i \"" + sourceFolderFileName + "\" -c copy -y \"" + targetFolderFileName + "\"";

                if (mediaType == "V" && exportVideoAudioValue == 0) //remove audio
                    command.replace("-c copy -y", " -an -c copy -y");

    //            if (frameRate != "")
    //                command.replace("-c copy -y", " -r " + frameRate + " -c copy -y");

                AJobParams jobParams;
//                jobParams.thisWidget = this;
                jobParams.parentItem = childItem;
                jobParams.folderName = selectedFolderName;
                jobParams.fileName = fileNamePlusExtension;
                jobParams.action = "FFMpeg lossless " + mediaType;
                jobParams.command = command;
                jobParams.parameters["exportFileName"] = fileNamePlusExtension;
                jobParams.parameters["totalDuration"] = QString::number(AGlobal().frames_to_msec(maxCombinedDurationInFrames));
                jobParams.parameters["durationMultiplier"] = QString::number(2);
                jobParams.parameters["startTime"] = QDateTime::currentDateTime().toString();

                childItem = jobTreeView->createJob(jobParams, nullptr, nullptr);
            } //if fileExtension

        } //if vidlist
        else
        {
            qDebug()<<"vidlistFile.error"<<vidlistFile.error()<< vidlistFile.errorString();
        }

    } //for each

    return childItem;

} //losslessVideoAndAudio

QStandardItem *AExport::encodeVideoClips(QStandardItem *parentItem)
{
    QStringList ffmpegFiles;
    QStringList ffmpegClips;
    QStringList ffmpegCombines;
    QStringList ffmpegMappings;

    //create audio and video stream
    QStringList mediaTypeList;
    mediaTypeList << "V";
    if (audioClipsMap.count() > 0)
        mediaTypeList << "A";

    bool useTrimFiles = true;

    int streamCounter = 0;
    bool differentFrameRateFound = false;

    QStandardItem *childItem = parentItem;

    QString lastV = "";
    QString lastA = "";
    QString audioStreams = "";

    QString recycleFolderName = selectedFolderName + "ACVCRecycleBin/";
    QDir recycleDir(recycleFolderName);
    if (!recycleDir.exists())
        recycleDir.mkpath(".");
    else
    {
//        if (QSettings().value("clipsDataChanged").toString() == "Yes")
        {
            recycleDir.setFilter( QDir::NoDotAndDotDot | QDir::Files );
            foreach( QString dirItem, recycleDir.entryList() )
            {
                if (dirItem.contains("Clip"))
                    recycleDir.remove( dirItem );
            }

//            QSettings().setValue("clipsDataChanged", "No");
        }
    }

    foreach (QString mediaType, mediaTypeList) //prepare video and audio
    {
        QMap<int,int> audioOrVideoClipsMap;

        if (mediaType == "A")
            audioOrVideoClipsMap = audioClipsMap;
        else
            audioOrVideoClipsMap = videoClipsMap;

        int iterationCounter = 0;
        double totalDurationInSeconds = 0;
        double totalDurationInFrames = 0;

        QMapIterator<int, int> clipsIterator(audioOrVideoClipsMap);
        while (clipsIterator.hasNext()) //all files
        {
            clipsIterator.next();
            int row = clipsIterator.value();

            QTime inTime = QTime::fromString(timelineModel->index(row, inIndex).data().toString(),"HH:mm:ss.zzz");
            QTime outTime = QTime::fromString(timelineModel->index(row, outIndex).data().toString(),"HH:mm:ss.zzz");
            QString folderName = timelineModel->index(row, folderIndex).data().toString();
            QString fileName = timelineModel->index(row, fileIndex).data().toString();
            QString fileFrameRate = timelineModel->index(row, fpsIndex).data().toString();
            QString imageWidth = timelineModel->index(row, imageWidthIndex).data().toString();
            QString imageHeight= timelineModel->index(row, imageHeightIndex).data().toString();
            QString tags = timelineModel->index(row, tagIndex).data().toString();

//            qDebug()<<"clipFrameRate"<<clipFrameRate;
            if (fileFrameRate != frameRate)
                    differentFrameRateFound = true;

            if (useTrimFiles && mediaType == "V")
            {
                QStandardItem *childItem = nullptr;
                QString recycleFileName = "Clip" + QString::number(ffmpegFiles.count()) + ".mp4";
                QFile file (recycleFolderName + recycleFileName);
                if (!file.exists())
                    emit trimC(parentItem, childItem, folderName, fileName, recycleFolderName, recycleFileName, inTime, outTime.addMSecs(AGlobal().frames_to_msec(1))); //one frame extra in case some more frames needed

                ffmpegFiles << "-i \"" + recycleFolderName + recycleFileName + "\"";
            }
            else
                ffmpegFiles << "-i \"" + folderName + fileName + "\"";

            int fileReference = ffmpegFiles.count() - 1;

            double inSeconds = inTime.msecsSinceStartOfDay() / 1000.0;
            double outSeconds = outTime.msecsSinceStartOfDay() / 1000.0 + AGlobal().frames_to_msec(1)/1000.0;
            double inFrames = AGlobal().msec_to_frames(inTime.msecsSinceStartOfDay());
            double outFrames = AGlobal().msec_to_frames(outTime.msecsSinceStartOfDay()) + 1;

            //if stream contains video
            if (mediaType == "V")
            {
                QString filterClip = tr("[%1:v]").arg(QString::number(fileReference));

                if (!useTrimFiles)
                    filterClip += tr("trim=%2:%3[vv%1];[vv%1]").arg(QString::number(fileReference), QString::number(inSeconds) , QString::number(outSeconds));

                filterClip += tr("setpts=PTS-STARTPTS+%1/TB").arg(QString::number(totalDurationInSeconds));

                if (transitionTimeFrames != 0)
                {
                    QString alphaIn = "0"; //transparent= firstIn
                    QString alphaOut = "0"; //black = transitions
                    if (iterationCounter != 0)
                        alphaIn = "1";
                    if (iterationCounter != audioOrVideoClipsMap.count()-1)
                        alphaOut = "1";

                    filterClip += tr(",fade=in:0:%1:alpha=%2").arg(QString::number(transitionTimeFrames), alphaIn);
                    filterClip += tr(",fade=out:%1:%2:alpha=%3").arg(QString::number(outFrames - inFrames - transitionTimeFrames), QString::number(transitionTimeFrames), alphaOut);
                }

                if (imageWidth != videoWidth || imageHeight != videoHeight)
                    filterClip += ",scale=" + videoWidth + "x" + videoHeight + ",fps=25";

                filterClip += tr("[v%1]").arg(QString::number(fileReference));

                if (tags.toLower().contains("backwards"))
                    filterClip.replace("setpts=PTS-STARTPTS", "setpts=PTS-STARTPTS, reverse");
                //https://stackoverflow.com/questions/42257354/concat-a-video-with-itself-but-in-reverse-using-ffmpeg
                if (tags.toLower().contains("slowmotion"))
                    filterClip.replace("setpts=PTS-STARTPTS", "setpts=2.0*PTS");
                if (tags.toLower().contains("fastmotion"))
                    filterClip.replace("setpts=PTS-STARTPTS", "setpts=0.5*PTS");

                ffmpegClips << filterClip;

                if (iterationCounter > 0)
                {
                    ffmpegCombines << tr("%1[v%2]overlay[vo%2]").arg(lastV, QString::number(fileReference));
                    lastV = tr("[vo%1]").arg(QString::number(fileReference));
                }
                else
                    lastV = tr("[v%1]").arg(QString::number(fileReference));
            }

            //if stream contains audio
            if ((mediaType == "V" && exportVideoAudioValue > 0) || mediaType == "A")
            {
                QString filterClip = tr("[%1:a]").arg(QString::number(fileReference));

                if (!useTrimFiles || mediaType == "A")
                    filterClip += tr("atrim=%2:%3[aa%1];[aa%1]").arg(QString::number(fileReference), QString::number(inSeconds) , QString::number(outSeconds));

                if (mediaType == "V")
                    filterClip += tr("volume=%1,").arg(QString::number(exportVideoAudioValue / 100.0));

                filterClip += tr("aformat=sample_fmts=fltp:sample_rates=44100:channel_layouts=stereo,asetpts=PTS-STARTPTS");

                if (iterationCounter == 0 && transitionTimeFrames != 0)
                    filterClip += tr(",afade=t=in:ss=0:d=%1").arg(QString::number(AGlobal().frames_to_msec(transitionTimeFrames)/1000.0));

                if (iterationCounter == audioOrVideoClipsMap.count()-1 && transitionTimeFrames != 0)
                    filterClip += tr(",afade=t=out:st=%1:d=%2").arg(QString::number(outSeconds - inSeconds - AGlobal().frames_to_msec(transitionTimeFrames)/1000.0), QString::number(AGlobal().frames_to_msec(transitionTimeFrames)/1000.0));

                filterClip += tr("[a%1]").arg(QString::number(fileReference));

                ffmpegClips << filterClip;

                if (iterationCounter > 0)
                {
                    if (transitionTimeFrames != 0)
                        ffmpegCombines << tr("%1[a%2]acrossfade=d=%3[ao%2]").arg( lastA, QString::number(fileReference), QString::number(AGlobal().frames_to_msec(transitionTimeFrames)/1000.0));
                    else
                        ffmpegCombines << tr("%1[a%2]concat=n=2:v=0:a=1[ao%2]").arg( lastA, QString::number(fileReference));
                    lastA = tr("[ao%1]").arg(QString::number(fileReference));
                }
                else
                    lastA = tr("[a%1]").arg(QString::number(fileReference));
            }

            totalDurationInSeconds += outSeconds - inSeconds - AGlobal().frames_to_msec(transitionTimeFrames) / 1000.0;
            totalDurationInFrames += outFrames - inFrames - transitionTimeFrames;

//                QString targetFolderFileName = fileNameWithoutExtension + "-" + QString::number(row) + ".mp4";
//                onTrim(timelineModel->index(i, folderIndex).data().toString(),  timelineModel->index(i, fileIndex).data().toString(), targetFolderFileName, inTime, outTime, 10*i/(timelineModel->rowCount())); //not -1 to avoid divby0
            iterationCounter++;

        } //while

        audioStreams += lastA;

        streamCounter++;

    } //foreach mediatype

    if (audioClipsMap.count() > 0 && exportVideoAudioValue > 0)
    {
        ffmpegCombines << audioStreams + "amerge=inputs=2[audio]";
        ffmpegMappings << "-map [audio]";
    }
    else
        ffmpegMappings << tr("-map %1").arg(lastA);

    if (watermarkFileName != "")
    {
        ffmpegFiles << "-i \"" + watermarkFileName + "\"";
        ffmpegClips << "[" + QString::number(ffmpegFiles.count() -1) + ":v]scale=" + QString::number(videoWidth.toInt()/10) + "x" + QString::number(videoHeight.toInt()/10) + "[wtm]";
        ffmpegCombines << lastV + "[wtm]overlay = main_w-overlay_w-10:main_h-overlay_h-10[video]";
        ffmpegMappings << "-map [video]";
    }
    else
        ffmpegMappings << tr("-map %1").arg(lastV);

//    qDebug()<<"frameRate"<<frameRate;
//    if (differentFrameRateFound)
//        ffmpegMappings << " -r " + frameRate; //tbd: consider -filter:v fps=24 (minterpolate instead of dropping or duplicating frames)

    videoFileExtension = ".mp4";

    QString targetFolderFileName = selectedFolderName + fileNameWithoutExtension + videoFileExtension;

#ifdef Q_OS_WIN
    targetFolderFileName = targetFolderFileName.replace("/", "\\");
#endif

    ffmpegMappings << "-y \"" + targetFolderFileName + "\"";

//    foreach (QString ffmpegFile, ffmpegFiles)
//        qDebug()<<"ffmpegFiles"<<ffmpegFile;
//    foreach (QString ffmpegClip, ffmpegClips)
//        qDebug()<<"ffmpegClip"<<ffmpegClip;
//    foreach (QString ffmpegCombine, ffmpegCombines)
//        qDebug()<<"ffmpegCombine"<<ffmpegCombine;
//    foreach (QString ffmpegMapping, ffmpegMappings)
//        qDebug()<<"ffmpegMapping"<<ffmpegMapping;

    QString command = "ffmpeg " + ffmpegFiles.join(" ") + " -filter_complex \"" + ffmpegClips.join(";") + ";" + ffmpegCombines.join(";") + "\" " + ffmpegMappings.join(" ");

//    qDebug()<<"command"<<command;

    //https://ffmpeg.org/ffmpeg-filters.html#trim

    AJobParams jobParams;
    jobParams.thisWidget = this;
    jobParams.parentItem = parentItem;
    jobParams.folderName = selectedFolderName;
    jobParams.fileName = fileNameWithoutExtension + videoFileExtension;
    jobParams.action = "FFMpeg Encode";
    jobParams.command = command;
    jobParams.parameters["fileNameWithoutExtension"] = fileNameWithoutExtension;
    jobParams.parameters["exportFileName"] = fileNameWithoutExtension + videoFileExtension;
    jobParams.parameters["totalDuration"] = QString::number(AGlobal().frames_to_msec(maxCombinedDurationInFrames));
    jobParams.parameters["durationMultiplier"] = QString::number(0.4); //overlay is 0.2
    jobParams.parameters["startTime"] = QDateTime::currentDateTime().toString();
    jobParams.parameters["ffmpegFiles"] = ffmpegFiles.join("\n");
    jobParams.parameters["ffmpegClips"] = ffmpegClips.join("\n");
    jobParams.parameters["ffmpegCombines"] = ffmpegCombines.join("\n");
    jobParams.parameters["ffmpegMappings"] = ffmpegMappings.join("\n");

    childItem = jobTreeView->createJob(jobParams, [] (AJobParams jobParams)
    {

            AExport *exportWidget = qobject_cast<AExport *>(jobParams.thisWidget);
            emit exportWidget->jobAddLog(jobParams, "");
            emit exportWidget->jobAddLog(jobParams, "Files:");
            emit exportWidget->jobAddLog(jobParams, jobParams.parameters["ffmpegFiles"]);
            emit exportWidget->jobAddLog(jobParams, "");
            emit exportWidget->jobAddLog(jobParams, "Clips");
            emit exportWidget->jobAddLog(jobParams, jobParams.parameters["ffmpegClips"]);
            emit exportWidget->jobAddLog(jobParams, "");
            emit exportWidget->jobAddLog(jobParams, "Combines");
            emit exportWidget->jobAddLog(jobParams, jobParams.parameters["ffmpegCombines"]);
            emit exportWidget->jobAddLog(jobParams, "");
            emit exportWidget->jobAddLog(jobParams, "Mappings");
            emit exportWidget->jobAddLog(jobParams, jobParams.parameters["ffmpegMappings"]);
            emit exportWidget->jobAddLog(jobParams, "");
            return QString();
    }, [] (AJobParams jobParams, QStringList result)
    {
        AExport *exportWidget = qobject_cast<AExport *>(jobParams.thisWidget);
    });

    return childItem;

} //encodeVideoClips

//void AExport::processOutput(QMap<QString, QString> parameters, QString result, int percentageStart, int percentageDelta)
//{
////    qDebug()<<"AExport::processOutput"<<result<<percentageStart<<percentageDelta;

//    if (!result.contains("Non-monotonous DTS in output stream")) //only warning, sort out later
//        emit addToJob(parameters["processId"], result);

//    foreach (QString resultLine, result.split("\n"))
//    {
//        if (resultLine.contains("Could not")  || resultLine.contains("Invalidxx") || resultLine.contains("No such file or directory"))
//            processError = resultLine;
//    }

//    int timeIndex = result.indexOf("time=");
//    if (timeIndex > 0)
//    {
//        QString timeString = result.mid(timeIndex + 5, 11) + "0";
//        QTime time = QTime::fromString(timeString,"HH:mm:ss.zzz");
//        if (parameters["totalDuration"].toInt() == 0)
//            qDebug()<<"PROGRAMMING ERROR totalDuration SHOULD NOT BE 0";
//        else
//            emit updateProgress(percentageStart + percentageDelta * time.msecsSinceStartOfDay() / parameters["totalDuration"].toInt());
//    }
//}

//void AExport::processFinished(QMap<QString, QString> parameters)
//{
//    qDebug()<<"AExport::processFinished"<<parameters["exportFileName"];
//    //check successful

//    if (parameters["exportFileName"] != "")
//    {
//        QString errorMessage = "OK";

//        QFile file(currentDirectory + parameters["exportFileName"]);
//        if (file.exists())
//        {
//            QDateTime changeTime = QFileInfo(currentDirectory + parameters["exportFileName"]).metadataChangeTime();
//            QDateTime currentTime = QDateTime::fromString(parameters["startTime"]);

//            qDebug()<<"  AExport::processFinished changetime"<<changeTime<<currentTime;

//            if (changeTime.secsTo(currentTime) > 0) // changetime always later then starttime
//                errorMessage = "File " + parameters["exportFileName"] + " created before this job";
//            else if (file.size() < 100)
//                errorMessage = "File possibly corrupted as size is smaller then 100 bytes: " + parameters["exportFileName"] + " " + QString::number(file.size()) + " bytes";
//        }
//        else
//            errorMessage = "File " + parameters["exportFileName"] + " not created";

//        if (errorMessage != "OK")
//        {
//            if (processError != "")
//            {
//                emit addToJob(parameters["processId"], errorMessage + "\n");
//                emit addToJob(parameters["processId"], processError);
//            }
//            else
//            {
//                processError = errorMessage;
//                emit addToJob(parameters["processId"], errorMessage);
//            }
//        }
//        else
//        {
//            emit addToJob(parameters["processId"], "Completed");
//            processError = "";
//        }

//        if (processError != "")
//            emit readyProgress(1, "Export error (go to Jobs for details): " + processError);
//        else
//            emit readyProgress(0, "");
//    }
//}

QStandardItem * AExport::muxVideoAndAudio(QStandardItem *parentItem)
{
    QString targetFolderFileName = selectedFolderName +  fileNameWithoutExtension + "V" + videoFileExtension;

#ifdef Q_OS_WIN
    targetFolderFileName = targetFolderFileName.replace("/", "\\");
#endif

    QStandardItem *childItem = parentItem;

    QString command = "ffmpeg -i \"" + targetFolderFileName + "\"";

    if (audioClipsMap.count() > 0)
    {
        QString targetFolderFileName = selectedFolderName +  fileNameWithoutExtension +  + "A.mp3";

    #ifdef Q_OS_WIN
        targetFolderFileName = targetFolderFileName.replace("/", "\\");
    #endif

        command += " -i \"" + targetFolderFileName + "\"";

        if (exportVideoAudioValue == 0)
            command += " -c copy -map 0:v -map 1:a";
        else if (exportVideoAudioValue == 100)
            command += " -filter_complex \"[0][1]amix=inputs=2[a]\" -map 0:v -map \"[a]\" -c:v copy";
        else
            command += " -filter_complex \"[0]volume=" + QString::number(exportVideoAudioValue / 100.0) + "[a0];[a0][1]amix=inputs=2[a]\" -map 0:v -map \"[a]\" -c:v copy";
    }
    else
        command += " -filter:a \"volume=" + QString::number(exportVideoAudioValue / 100.0) + "\"";

    targetFolderFileName = selectedFolderName +  fileNameWithoutExtension + videoFileExtension;

#ifdef Q_OS_WIN
    targetFolderFileName = targetFolderFileName.replace("/", "\\");
#endif

    command += " -y \"" + targetFolderFileName + "\"";

    AJobParams jobParams;
//    jobParams.thisWidget = this;
    jobParams.parentItem = parentItem;
    jobParams.folderName = selectedFolderName;
    jobParams.fileName = fileNameWithoutExtension + videoFileExtension;
    jobParams.action = "FFMpeg mux";
    jobParams.command = command;
    jobParams.parameters["exportFileName"] = fileNameWithoutExtension + videoFileExtension;
    jobParams.parameters["totalDuration"] = QString::number(AGlobal().frames_to_msec(maxCombinedDurationInFrames));
    jobParams.parameters["durationMultiplier"] = QString::number(50); //speed=50x
    jobParams.parameters["startTime"] = QDateTime::currentDateTime().toString();

    childItem = jobTreeView->createJob(jobParams, nullptr, nullptr);

    return childItem;

} //muxVideoAndAudio

//QStandardItem *AExport::removeTemporaryFiles(QStandardItem *parentItem)
//{
//    QStandardItem *childItem = parentItem;

//    AJobParams jobParams;
//    jobParams.thisWidget = this;
//    jobParams.parentItem = parentItem;
//    jobParams.folderName = currentDirectory;
//    jobParams.fileName = fileNameWithoutExtension;
//    jobParams.action = "removeTemporaryFiles";
//    jobParams.parameters["totalDuration"] = QString::number(1000);

//    childItem = jobTreeView->createJob(jobParams, [] (AJobParams jobParams)
//    {
//            AExport *exportWidget = qobject_cast<AExport *>(jobParams.thisWidget);

//            QDir dir(jobParams.folderName);
//            dir.setFilter( QDir::NoDotAndDotDot | QDir::Files );
//            foreach( QString dirItem, dir.entryList() )
//            {
//                if (dirItem.contains(jobParams.fileName + "V") || dirItem.contains(jobParams.fileName + "A"))
//                {
//                    dir.remove( dirItem );
//                }
//            }

//            return QString();
//    }, [] (AJobParams jobParams, QStringList result){});

//    return childItem;
//}

void AExport::addPremiereTrack(QString mediaType, QMap<int,int> clipsMap, QMap<QString, FileStruct> filesMap)
{
    for (int trackNr= 0; trackNr < 2; trackNr++) //even and odd tracks
    {
        int maxChannelNr;
        if (mediaType == "video")
            maxChannelNr = 1;
        else
            maxChannelNr = 1; //apparently no need to add 2 channels for stereo...

        for (int channelTrackNr = 1; channelTrackNr <= maxChannelNr; channelTrackNr++)
        {

            if (mediaType == "video")
                s("    <track>");
            else
                s("    <track currentExplodedTrackIndex=\"%1\" totalExplodedTrackCount=\"%2\" premiereTrackType=\"%3\">", QString::number(channelTrackNr - 1), QString::number(maxChannelNr), "Stereo");

            int totalFrames = 0;

            int iterationCounter = 0;

            QMapIterator<int, int> clipsIterator(clipsMap);
            while (clipsIterator.hasNext()) //all audio or video clips
            {
                clipsIterator.next();
                int row = clipsIterator.value();

                QString folderName = timelineModel->index(row, folderIndex).data().toString();
                QString fileName = timelineModel->index(row, fileIndex).data().toString();
                QString fileFrameRate = timelineModel->index(row, fpsIndex).data().toString();
                QString fileAudioChannels = timelineModel->index(row, channelsIndex).data().toString();
                QString imageWidth = timelineModel->index(row, imageWidthIndex).data().toString();
                QString imageHeight = timelineModel->index(row, imageHeightIndex).data().toString();
                QTime inTime = QTime::fromString(timelineModel->index(row, inIndex).data().toString(),"HH:mm:ss.zzz");
                QTime outTime = QTime::fromString(timelineModel->index(row, outIndex).data().toString(),"HH:mm:ss.zzz");

                int inFrames = qRound(inTime.msecsSinceStartOfDay() * frameRate.toInt() / 1000.0);
                int outFrames = qRound(outTime.msecsSinceStartOfDay() * frameRate.toInt() / 1000.0);

                QString clipAudioChannels = "";

                if (fileAudioChannels != "")
                    clipAudioChannels = fileAudioChannels;
                else
                {
                    if (clipsMap == audioClipsMap)
                        clipAudioChannels = "2";  //assume stereo for audiofiles
                    else
                        clipAudioChannels = "1";  //assume mono for videofiles
                }

                //                qDebug()<<"props"<<fileName<<mediaType<<clipFrameRate<<clipAudioChannels<<*audioChannelsPointer;

                if (mediaType == "audio" && clipsMap == audioClipsMap)
                {
                    if (iterationCounter%2 == trackNr) //even or odd tracks
                    {
                        if (audioClipsMap.first() == row) //fade in
                        {
                            addPremiereTransitionItem(0, fileFrameRate.toInt(), fileFrameRate, mediaType, "start");
                        }
                    }
                }

                if (trackNr == 1 && iterationCounter%2 == 1 && transitionTimeFrames > 0) //track 2 and not first clip (never is)
                {
                    addPremiereTransitionItem(totalFrames, totalFrames + transitionTimeFrames, fileFrameRate, mediaType, "start");
                }

                int deltaFrames = outFrames - inFrames + 1 - transitionTimeFrames;
                if (iterationCounter%2 == trackNr) //even or odd tracks
                {
                    addPremiereClipitem(QString::number(row), folderName, fileName, totalFrames, totalFrames + deltaFrames + transitionTimeFrames, inFrames, outFrames + 1, fileFrameRate, mediaType, &filesMap, channelTrackNr, clipAudioChannels, imageWidth, imageHeight);
                }

                totalFrames += deltaFrames;

                if (trackNr == 1 && iterationCounter%2 == 1 && iterationCounter != clipsMap.count()-1 && transitionTimeFrames > 0) //track 2 and not last clip
                {
                    addPremiereTransitionItem(totalFrames, totalFrames + transitionTimeFrames, fileFrameRate, mediaType, "end");
                }

                if (mediaType == "audio" && clipsMap == audioClipsMap)
                {
                    if (iterationCounter%2 == trackNr) //even or odd tracks
                    {
                        if (audioClipsMap.last() == row) //fade out
                        {
                            addPremiereTransitionItem(maxAudioDuration - fileFrameRate.toInt(), maxAudioDuration, fileFrameRate, mediaType, "end");
                        }
                    }
                }

    //            emit jobAddLog(jobParams, QString("  Producer%1 %2 %3"));

                iterationCounter++;
            } // clipsiterator

            if (mediaType == "audio")
            {
                s("     <outputchannelindex>%1</outputchannelindex>", QString::number(channelTrackNr));
            }

            s("    </track>");
        } //for (int channelTrackNr=0; channelTrackNr<maxChannelNr; channelTrackNr++) //even and odd tracks

    } //for (int trackNr=0; trackNr<2; trackNr++) //even and odd tracks

} //addPremiereTrack

void AExport::addPremiereTransitionItem(int startFrames, int endFrames, QString frameRate, QString mediaType, QString startOrEnd)
{
    s("     <transitionitem>");
    s("      <start>%1</start>", QString::number(startFrames));
    s("      <end>%1</end>", QString::number(endFrames));
    s("      <alignment>%1-black</alignment>", startOrEnd);
//                        s("      <cutPointTicks>0</cutPointTicks>");
    s("      <rate>");
    s("       <timebase>%1</timebase>", frameRate);
//                        s("       <ntsc>FALSE</ntsc>");
    s("      </rate>");

    if (mediaType == "video")
    {
        s("      <effect>");
        s("       <name>Cross Dissolve</name>");
        s("       <effectid>Cross Dissolve</effectid>");
        s("       <effectcategory>Dissolve</effectcategory>");
        s("       <effecttype>transition</effecttype>");
        s("       <mediatype>video</mediatype>");
        s("       <wipecode>0</wipecode>");
        s("       <wipeaccuracy>100</wipeaccuracy>");
        s("       <startratio>0</startratio>");
        s("       <endratio>1</endratio>");
        s("       <reverse>FALSE</reverse>");
        s("      </effect>");
    }
    else
    {
        s("      <effect>");
        s("       <name>Cross Fade (+3dB)</name>");
        s("       <effectid>KGAudioTransCrossFade3dB</effectid>");
        s("       <effecttype>transition</effecttype>");
        s("       <mediatype>audio</mediatype>");
        s("       <wipecode>0</wipecode>");
        s("       <wipeaccuracy>100</wipeaccuracy>");
        s("       <startratio>0</startratio>");
        s("       <endratio>1</endratio>");
        s("       <reverse>FALSE</reverse>");
        s("      </effect>");
    }
    s("     </transitionitem>");

} //addPremiereTransitionItem

void AExport::addPremiereClipitem(QString clipId, QString folderName, QString fileName, int startFrames, int endFrames, int inFrames, int outFrames, QString frameRate, QString mediaType, QMap<QString, FileStruct> *filesMap, int channelTrackNr, QString clipAudioChannels, QString imageWidth, QString imageHeight)
{
    if (mediaType == "video")
        s("     <clipitem>");
    else
    {
        if (clipAudioChannels == "2")
            s("     <clipitem premiereChannelType=\"%1\">", "stereo");
        else
            s("     <clipitem premiereChannelType=\"%1\">", "mono");
    }

    s("      <masterclipid>masterclip-%1</masterclipid>", clipId);
    s("      <name>%1</name>", fileName);
//            s("      <enabled>TRUE</enabled>");
//                        s("      <duration>%1</duration>", QString::number( outFrames - inFrames + 1));
    s("      <start>%1</start>", QString::number(startFrames)); //will be reassigned in premiere to -1

    s("      <end>%1</end>", QString::number(endFrames)); //will be reassigned in premiere to -1

    if (inFrames != -1)
        s("      <in>%1</in>", QString::number(inFrames));
    if (outFrames != -1)
        s("      <out>%1</out>", QString::number(outFrames));

    QString AVType;
    if (fileName.toLower().contains(".mp3"))
        AVType = "A";
    else if (clipId == "WM") //watermark
        AVType = "W";
    else
        AVType = "V";

    if (!(*filesMap)[folderName + fileName].definitionGenerated)
    {
        s("      <file id=\"file-%1%2\">", AVType, QString::number((*filesMap)[folderName + fileName].counter)); //define file
        s("       <name>%1</name>", fileName);
        s("       <pathurl>file://localhost/%1</pathurl>", folderName + fileName);
        s("       <rate>");
        s("        <timebase>%1</timebase>", frameRate);
//                            s("        <ntsc>FALSE</ntsc>");
        s("       </rate>");
//                            s("       <duration>%1</duration>", QString::number( outFrames - inFrames));

        if (AVType == "A")
        {
            s("       <timecode>");
            s("           <rate>");
            s("               <timebase>%1</timebase>", frameRate);
//            s("               <ntsc>TRUE</ntsc>");
            s("           </rate>");
            s("           <string>00;00;00;00</string>");
            s("           <frame>0</frame>");
            s("           <displayformat>DF</displayformat>");
            s("       </timecode>");
        }

        s("       <media>");

        if (AVType == "V"|| AVType == "W")
        {
            s("        <video>");
    //                            s("         <samplecharacteristics>");
    //                            s("          <width>%1</width>", *widthPointer);
    //                            s("          <height>%1</height>", *heightPointer);
    //                            s("         </samplecharacteristics>");
            s("        </video>");
        }

        if ((AVType == "A" || exportVideoAudioValue > 0) && AVType != "W")
        {
            s("        <audio>");
//                                s("         <samplecharacteristics>");
//                                s("          <depth>16</depth>");
////                                s("          <samplerate>44100</samplerate>");
//                                s("         </samplecharacteristics>");
            s("         <channelcount>2</channelcount>");
            s("        </audio>");
        }
        s("       </media>");
        s("      </file>");

        (*filesMap)[folderName + fileName].definitionGenerated = true;
    }
    else //already defined
    {
        s("      <file id=\"file-%1%2\"/>", AVType, QString::number((*filesMap)[folderName + fileName].counter));  //refer to file if already defined
    }

    if (mediaType == "video")
    {
        if (AVType == "V")
        {
            if (imageHeight != videoHeight || imageWidth != videoWidth)
            {
                double heightRatio = 100.0 * videoHeight.toInt() / imageHeight.toInt();
                double widthRatio = 100.0 * videoWidth.toInt() / imageWidth.toInt();
                s("      <filter>");
                s("       <effect>");
                s("        <name>Basic Motion</name>");
                s("        <effectid>basic</effectid>");
                s("        <effectcategory>motion</effectcategory>");
                s("        <effecttype>motion</effecttype>");
                s("        <mediatype>video</mediatype>");
                s("        <pproBypass>false</pproBypass>");
                s("        <parameter authoringApp=\"PremierePro\">");
                s("         <parameterid>scale</parameterid>");
                s("         <name>Scale</name>");
                s("         <valuemin>0</valuemin>");
                s("         <valuemax>1000</valuemax>");
                s("         <value>%1</value>", QString::number(qMin(heightRatio, widthRatio), 'g', 6));
                s("        </parameter>");
                s("       </effect>");
                s("      </filter>");
            }
        }
        else if (AVType == "W")
        {
            QImage myImage;
            myImage.load(watermarkFileName);
//            qDebug()<<"Basic Motion"<<myImage.height()<<myImage.width()<<videoHeight<<videoWidth<<QString::number(videoHeight.toDouble() / myImage.height() * 10)<<QString::number(videoWidth.toDouble() / myImage.width())<<QString::number(videoHeight.toDouble() / myImage.height());
//            qDebug()<<"Basic Motion"<<myImage.width()<<videoWidth<<QString::number(videoHeight.toDouble() / myImage.height() * 10)<<QString::number(videoWidth.toDouble() / myImage.width());

            s("      <filter>");
            s("          <effect>");
            s("              <name>Basic Motion</name>");
            s("              <effectid>basic</effectid>");
            s("              <effectcategory>motion</effectcategory>");
            s("              <effecttype>motion</effecttype>");
            s("              <mediatype>video</mediatype>");
            s("              <pproBypass>false</pproBypass>");
            s("              <parameter authoringApp=\"PremierePro\">");
            s("                  <parameterid>scale</parameterid>");
            s("                  <name>Scale</name>");
            s("                  <valuemin>0</valuemin>");
            s("                  <valuemax>1000</valuemax>");
            s("                  <value>%1</value>", QString::number(videoHeight.toDouble() / myImage.height() * 10));
            s("              </parameter>");
            s("              <parameter authoringApp=\"PremierePro\">");
            s("                  <parameterid>center</parameterid>");
            s("                  <name>Center</name>");
            s("                  <value>");
            s("                      <horiz>%1</horiz>", QString::number(videoWidth.toDouble() / myImage.width() * 1.7)); //don't know why 1.7 and 1.4 but looks ok...
            s("                      <vert>%1</vert>", QString::number(videoHeight.toDouble() / myImage.height() * 1.4));
            s("                  </value>");
            s("              </parameter>");
            s("              <parameter authoringApp=\"PremierePro\">");
            s("                  <parameterid>centerOffset</parameterid>");
            s("                  <name>Anchor Point</name>");
            s("                  <value>");
            s("                      <horiz>%1</horiz>", QString::number(0));//0.9
            s("                      <vert>%1</vert>", QString::number(0));//0.5
            s("                  </value>");
            s("              </parameter>");

//                s("              <parameter authoringApp=\"PremierePro\">");
//                s("                  <parameterid>antiflicker</parameterid>");
//                s("                  <name>Anti-flicker Filter</name>");
//                s("                  <valuemin>0.0</valuemin>");
//                s("                  <valuemax>1.0</valuemax>");
//                s("                  <value>0</value>");
//                s("              </parameter>");
            s("          </effect>");
            s("      </filter>");
        }
    }
    else //audio
    {
        s("      <sourcetrack>");
        s("       <mediatype>audio</mediatype>");
        s("       <trackindex>%1</trackindex>", QString::number(channelTrackNr));
        s("      </sourcetrack>");

        if (AVType == "V"  && exportVideoAudioValue > 0 && exportVideoAudioValue < 100)
        {
            s("      <filter>");
            s("       <effect>");
            s("        <name>Audio Levels</name>");
            s("        <effectid>audiolevels</effectid>");
            s("        <effectcategory>audiolevels</effectcategory>");
            s("        <effecttype>audiolevels</effecttype>");
            s("        <mediatype>audio</mediatype>");
            s("        <pproBypass>false</pproBypass>");
            s("        <parameter authoringApp=\"PremierePro\">");
            s("         <parameterid>level</parameterid>");
            s("         <name>Level</name>");
            s("         <valuemin>0</valuemin>");
            s("         <valuemax>3.98109</valuemax>");
            s("         <value>%1</value>", QString::number(exportVideoAudioValue / 100.0));
            s("        </parameter>");
            s("       </effect>");
            s("      </filter>");
        }
    }
    s("     </clipitem>");
} //addPremiereClipItem

void AExport::exportShotcut(AJobParams jobParams)
{
    int transitionTimeMSecs = transitionTimeFrames * AGlobal().frames_to_msec(1);
    QTime transitionQTime = QTime::fromMSecsSinceStartOfDay(transitionTimeMSecs);

    QString fileName = jobParams.fileName;
    QFile fileWrite(jobParams.folderName + fileName);
    fileWrite.open(QIODevice::WriteOnly);

    stream.setDevice(&fileWrite);

    emit jobAddLog(jobParams, QString("Generating %1").arg(fileName));

    s("<?xml version=\"1.0\" encoding=\"utf-8\"?>");
    s("<mlt LC_NUMERIC=\"C\" version=\"6.17.0\" title=\"Shotcut v19.10.20 by ACVC v%1\" producer=\"main_bin\">", qApp->applicationVersion());
    s("  <profile description=\"automatic\" width=\"%1\" height=\"%2\" progressive=\"1\" sample_aspect_num=\"1\" sample_aspect_den=\"1\" display_aspect_num=\"%1\" display_aspect_den=\"%2\" frame_rate_num=\"%3\" frame_rate_den=\"1\"/>", videoWidth, videoHeight, frameRate);

    emit jobAddLog(jobParams, "Producers");

    QMapIterator<int, int> clipsIterator(clipsMap);
    while (clipsIterator.hasNext()) //all files
    {
        clipsIterator.next();
        int row = clipsIterator.value();

        QString folderName = timelineModel->index(row, folderIndex).data().toString();
        QString fileName = timelineModel->index(row, fileIndex).data().toString();
        QTime inTime = QTime::fromString(timelineModel->index(row, inIndex).data().toString(),"HH:mm:ss.zzz");
        QTime outTime = QTime::fromString(timelineModel->index(row, outIndex).data().toString(),"HH:mm:ss.zzz");
        QTime fileDuration = QTime::fromString(timelineModel->index(row, fileDurationIndex).data().toString(),"HH:mm:ss.zzz");
        int clipDuration = AGlobal().frames_to_msec(AGlobal().msec_to_frames(outTime.msecsSinceStartOfDay()) - AGlobal().msec_to_frames(inTime.msecsSinceStartOfDay()) + 1);

        s("  <producer id=\"producer%1\" title=\"Anonymous Submission\" in=\"00:00:00.000\" out=\"%2\">", QString::number(row), fileDuration.toString("hh:mm:ss.zzz"));
        s("    <property name=\"length\">%1</property>", fileDuration.toString("hh:mm:ss.zzz"));
        s("    <property name=\"resource\">%1</property>", folderName + fileName);

        if (fileName.toLower().contains(".mp3"))
        {
            if (audioClipsMap.first() == row || audioClipsMap.last() == row)
            {
                s("    <filter id=\"filter%1\" in=\"%2\" out=\"%3\">", QString::number(row), inTime.toString("HH:mm:ss.zzz"), outTime.toString("HH:mm:ss.zzz"));
                s("          <property name=\"window\">75</property>");
                s("          <property name=\"max_gain\">20dB</property>");
                s("          <property name=\"mlt_service\">volume</property>");
                if (audioClipsMap.first() == row)
                {
                    s("          <property name=\"level\">00:00:00.000=-60;00:00:00.960=0</property>");
                    s("          <property name=\"shotcut:filter\">fadeInVolume</property>");
                    s("          <property name=\"shotcut:animIn\">00:00:01.000</property>");
                }
                else //last
                {
                    s("          <property name=\"level\">%1=0;%2=-60</property>", QTime::fromMSecsSinceStartOfDay(clipDuration - 1000).toString("hh:mm:ss.zzz"), QTime::fromMSecsSinceStartOfDay(clipDuration).toString("hh:mm:ss.zzz"));
                    s("          <property name=\"shotcut:filter\">fadeOutVolume</property>");
                    s("          <property name=\"shotcut:animOut\">00:00:01.000</property>");
                }
                s("        </filter>");
            }
        }
        else //video
        {
            if (exportVideoAudioValue > 0)
            {
//                    qDebug()<<"qLn"<<qLn(10)<<exportVideoAudioSlider->value()<<10 * qLn(exportVideoAudioValue / 100.0);
                s("          <filter id=\"filter%1\" in=\"%2\" out=\"%3\">", QString::number(row), inTime.toString("HH:mm:ss.zzz"), outTime.toString("HH:mm:ss.zzz"));
                s("             <property name=\"window\">75</property>");
                s("             <property name=\"max_gain\">20dB</property>");
                s("             <property name=\"level\">%1</property>", QString::number(10 * qLn(exportVideoAudioValue / 100.0))); //percentage to dB
                s("             <property name=\"mlt_service\">volume</property>");
                s("           </filter>");
            }
        }
        s("  </producer>");
    }

    emit jobAddLog(jobParams, "");

    emit jobAddLog(jobParams, "Playlist");
    s("  <playlist id=\"main_bin\" title=\"Main playlist\">");
    s("    <property name=\"xml_retain\">1</property>");

    clipsIterator.toFront();
    while (clipsIterator.hasNext()) //all files
    {
        clipsIterator.next();
        int row = clipsIterator.value();

        QTime inTime = QTime::fromString(timelineModel->index(row, inIndex).data().toString(),"HH:mm:ss.zzz");
        QTime outTime = QTime::fromString(timelineModel->index(row, outIndex).data().toString(),"HH:mm:ss.zzz");

        s("    <entry producer=\"producer%1\" in=\"%2\" out=\"%3\"/>"
          , QString::number(row)
          , inTime.toString("HH:mm:ss.zzz"), outTime.toString("HH:mm:ss.zzz"));
        emit jobAddLog(jobParams, QString("  Producer%1 %2 %3").arg( QString::number(row), inTime.toString("HH:mm:ss.zzz"), outTime.toString("HH:mm:ss.zzz")));
    }
    s("  </playlist>"); //playlist main bin
    emit jobAddLog(jobParams, "");

    //create audio and video stream
    QStringList mediaTypeList;

    mediaTypeList << "V";
    if (audioClipsMap.count() > 0)
        mediaTypeList << "A";

    foreach (QString mediaType, mediaTypeList) //prepare video and audio
    {
        QMap<int,int> audioOrVideoClipsMap;

        if (mediaType == "A")
            audioOrVideoClipsMap = audioClipsMap;
        else
            audioOrVideoClipsMap = videoClipsMap;

        emit jobAddLog(jobParams, "Transitions");

        //transitions
        if (transitionTimeMSecs > 0)
        {
            QTime previousInTime = QTime();
            QTime previousOutTime = QTime();
            QString previousProducerNr = "";
            int iterationCounter = 0;

            QMapIterator<int, int> clipsIterator(audioOrVideoClipsMap);
            while (clipsIterator.hasNext()) //all files
            {
                clipsIterator.next();
                int row = clipsIterator.value();

                QTime inTime = QTime::fromString(timelineModel->index(row, inIndex).data().toString(),"HH:mm:ss.zzz");
                QTime outTime = QTime::fromString(timelineModel->index(row, outIndex).data().toString(),"HH:mm:ss.zzz");
                QString producerNr = QString::number(row);

                if (iterationCounter > 0 && iterationCounter < audioOrVideoClipsMap.count()) //not first and not last
                {
                    s("<tractor id=\"tractor%1\" title=\"%2\" global_feed=\"1\" in=\"00:00:00.000\" out=\"%3\">", previousProducerNr + "-" + producerNr, "Transition " + previousProducerNr + "-" + producerNr, transitionQTime.toString("HH:mm:ss.zzz"));
                    s("   <property name=\"shotcut:transition\">lumaMix</property>");
                    s("   <track producer=\"producer%1\" in=\"%2\" out=\"%3\"/>", previousProducerNr, previousOutTime.addMSecs(-transitionTimeMSecs + AGlobal().frames_to_msec(1)).toString("HH:mm:ss.zzz"), previousOutTime.toString("HH:mm:ss.zzz"));
                    s("   <track producer=\"producer%1\" in=\"%2\" out=\"%3\"/>", producerNr, inTime.toString("HH:mm:ss.zzz"), inTime.addMSecs(transitionTimeMSecs - AGlobal().frames_to_msec(1)).toString("HH:mm:ss.zzz"));
                    s("   <transition id=\"transition0\" out=\"%1\">", transitionQTime.toString("HH:mm:ss.zzz"));
                    s("     <property name=\"a_track\">0</property>");
                    s("     <property name=\"b_track\">1</property>");
                    s("     <property name=\"factory\">loader</property>");
                    s("     <property name=\"mlt_service\">luma</property>");
                    s("   </transition>");
                    s("   <transition id=\"transition1\" out=\"%1\">", transitionQTime.toString("HH:mm:ss.zzz"));
                    s("     <property name=\"a_track\">0</property>");
                    s("     <property name=\"b_track\">1</property>");
                    s("     <property name=\"start\">-1</property>");
                    s("     <property name=\"accepts_blanks\">1</property>");
                    s("     <property name=\"mlt_service\">mix</property>");
                    s("   </transition>");
                    s(" </tractor>");

                    emit jobAddLog(jobParams, QString("  Transition%1 %2 %3").arg( previousProducerNr + "-" + producerNr, "Transition " + previousProducerNr + "-" + producerNr, transitionQTime.toString("HH:mm:ss.zzz")));

                }

                previousInTime = inTime;
                previousOutTime = outTime;
                previousProducerNr = producerNr;
                iterationCounter++;
            }
        }
        emit jobAddLog(jobParams, "");
    }

    foreach (QString mediaType, mediaTypeList) //prepare video and audio
    {
        s("  <playlist id=\"playlist%1\">", mediaType);

        QMap<int,int> audioOrVideoClipsMap;

        if (mediaType == "A")
        {
            audioOrVideoClipsMap = audioClipsMap;
            s("    <property name=\"shotcut:audio\">1</property>");
            s("    <property name=\"shotcut:name\">A1</property>");
        }
        else
        {
            audioOrVideoClipsMap = videoClipsMap;
            s("    <property name=\"shotcut:video\">1</property>");
            s("    <property name=\"shotcut:name\">V1</property>");
        }

        emit jobAddLog(jobParams, "Timeline");

        QString previousProducerNr = "";
        int iterationCounter = 0;
        QMapIterator<int, int> clipsIterator(audioOrVideoClipsMap);
        while (clipsIterator.hasNext()) //all videos or all audios
        {
            clipsIterator.next();
            int row = clipsIterator.value();

            QTime inTime = QTime::fromString(timelineModel->index(row, inIndex).data().toString(),"HH:mm:ss.zzz");
            QTime outTime = QTime::fromString(timelineModel->index(row, outIndex).data().toString(),"HH:mm:ss.zzz");

            QString producerNr = QString::number(row);

            if (transitionTimeFrames > 0)
            {
                if (iterationCounter != 0)
                {
                    s("    <entry producer=\"tractor%1\" in=\"00:00:00.000\" out=\"%2\"/>", previousProducerNr + "-" + producerNr, transitionQTime.toString("HH:mm:ss.zzz"));
                    emit jobAddLog(jobParams, QString("  Transition%1 %2").arg( QString::number(iterationCounter - 1), transitionQTime.toString("HH:mm:ss.zzz")));
                    inTime = inTime.addMSecs(transitionTimeMSecs); //subtract half of the transitionframes
                }
                if (iterationCounter != audioOrVideoClipsMap.count()-1)
                {
                    outTime = outTime.addMSecs(- transitionTimeMSecs); //subtract half of the transitionframes
                }
            }

            s("    <entry producer=\"producer%1\" in=\"%2\" out=\"%3\"/>", producerNr, inTime.toString("HH:mm:ss.zzz"), outTime.toString("HH:mm:ss.zzz") );
            emit jobAddLog(jobParams, QString("  Producer%1 %2 %3").arg( producerNr, inTime.toString("HH:mm:ss.zzz"), outTime.toString("HH:mm:ss.zzz") ));

            previousProducerNr = producerNr;

            iterationCounter++;
        }

        s("  </playlist>"); //playlist0..x

    } //for audio and video

    if (watermarkFileName != "")
    {
//            QImage myImage;
//            myImage.load(watermarkFileName);

//            myImage.width();

        s("  <producer id=\"producer%1\" title=\"Anonymous Submission\" in=\"00:00:00.000\" out=\"%2\">", "WM", "03:59:59.960");
        s("    <property name=\"length\">%1</property>", "04:00:00.000");
        s("    <property name=\"resource\">%1</property>", watermarkFileName);

        s("     <filter id=\"filter%1\" out=\"%2\">", "WM", QTime::fromMSecsSinceStartOfDay(AGlobal().frames_to_msec(maxCombinedDurationInFrames)).toString("hh:mm:ss.zzz"));
        s("       <property name=\"background\">colour:0</property>");
        s("       <property name=\"mlt_service\">affine</property>");
        s("       <property name=\"shotcut:filter\">affineSizePosition</property>");
        s("       <property name=\"transition.fill\">1</property>");
        s("       <property name=\"transition.distort\">0</property>");
        s("       <property name=\"transition.rect\">%1 %2 %3 %4 1</property>", QString::number(videoWidth.toDouble() * 0.85),  QString::number(videoHeight.toDouble() * 0.85), QString::number(videoWidth.toDouble() * 0.15),  QString::number(videoHeight.toDouble() * 0.15));
        s("       <property name=\"transition.valign\">bottom</property>");
        s("       <property name=\"transition.halign\">right</property>");
        s("       <property name=\"shotcut:animIn\">00:00:00.000</property>");
        s("       <property name=\"shotcut:animOut\">00:00:00.000</property>");
        s("       <property name=\"transition.threads\">0</property>");
        s("     </filter>");


        s("  </producer>");

        s("  <playlist id=\"playlist%1\">", "WM");
        s("      <property name=\"shotcut:video\">1</property>");
        s("      <property name=\"shotcut:name\">V2</property>");
        s("      <entry producer=\"producer%1\" in=\"00:00:00.000\" out=\"%2\"/>", "WM", QTime::fromMSecsSinceStartOfDay(AGlobal().frames_to_msec(maxCombinedDurationInFrames)).toString("hh:mm:ss.zzz"));
        s("  </playlist>");
    }

    s("  <tractor id=\"tractor%1\" title=\"Shotcut v19.10.20 by ACVC v%2\">", "Main", qApp->applicationVersion());
    s("    <property name=\"shotcut\">1</property>");
    s("    <track producer=\"\"/>");

    if (exportVideoAudioValue > 0)
        s("    <track producer=\"playlist%1\"/>", "V");
    else
        s("    <track producer=\"playlist%1\"  hide=\"audio\"/>", "V");

    s("    <track producer=\"playlist%1\"  hide=\"video\"/>", "A");

    if (watermarkFileName != "")
    {
        s("    <track producer=\"playlist%1\"/>", "WM");
        s("    <transition id=\"transition%1\">", "Mix"); //mix V1 and V2?
        s("      <property name=\"a_track\">0</property>");
        s("      <property name=\"b_track\">1</property>");
        s("      <property name=\"mlt_service\">mix</property>");
        s("      <property name=\"always_active\">1</property>");
        s("      <property name=\"sum\">1</property>");
        s("    </transition>");
        s("    <transition id=\"transition%1\">", "Mix"); //mix audio from V1 and A1?
        s("      <property name=\"a_track\">0</property>");
        s("      <property name=\"b_track\">2</property>");
        s("      <property name=\"mlt_service\">mix</property>");
        s("      <property name=\"always_active\">1</property>");
        s("      <property name=\"sum\">1</property>");
        s("    </transition>");
        s("    <transition id=\"transition%1\">", "Blend"); //blend A1 and V2?
        s("      <property name=\"a_track\">1</property>");
        s("      <property name=\"b_track\">3</property>");
        s("      <property name=\"version\">0.9</property>");
        s("      <property name=\"mlt_service\">frei0r.cairoblend</property>");
        s("      <property name=\"disable\">0</property>");
        s("    </transition>");
    }

    s("  </tractor>");

    s("</mlt>");

    fileWrite.close();

} //exportShotcut

void AExport::exportPremiere(AJobParams jobParams)
{
    QString fileName = fileNameWithoutExtension + ".xml";
    QFile fileWrite(jobParams.folderName + fileName);
    fileWrite.open(QIODevice::WriteOnly);

    stream.setDevice(&fileWrite);

    emit jobAddLog(jobParams, QString("Generating %1").arg(fileName));

    s("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
    s("<!DOCTYPE xmeml>");
    s("<xmeml version=\"4\">");
//        s(" <project>");
//        s("  <name>%1</name>", fileNameWithoutExtension);
//        s("  <children>");
    s(" <sequence>");
//        s("  <duration>114</duration>");
    s("  <rate>");
    s("   <timebase>%1</timebase>", frameRate);
//        s("   <ntsc>FALSE</ntsc>");
    s("  </rate>");
    s("  <name>%1</name>", fileNameWithoutExtension);
    s("  <media>");

    QStringList mediaTypeList;
    mediaTypeList << "video";
    if (exportVideoAudioValue > 0 || audioClipsMap.count() > 0)
        mediaTypeList << "audio";

    foreach (QString mediaType, mediaTypeList) //for video and audio of video stream
    {
//            QMap<int,int> audioOrVideoClipsMap;

//            if (mediaType == "audio")
//                audioOrVideoClipsMap = audioClipsMap;
//            else
//                audioOrVideoClipsMap = videoClipsMap;


        s("   <%1>", mediaType);

        if (mediaType == "audio")
            s("    <numOutputChannels>%1</numOutputChannels>", "1");

        s("    <format>");
        s("     <samplecharacteristics>");
        if (mediaType == "video")
        {
            s("      <width>%1</width>", videoWidth);
            s("      <height>%1</height>", videoHeight);
        }
        else //audio
        {
//                s("      <depth>16</depth>");
//                s("      <samplerate>44100</samplerate>");
        }
        s("     </samplecharacteristics>");
        s("    </format>");

        emit jobAddLog(jobParams, "Timeline");

        if (mediaType == "video" || (mediaType == "audio" && exportVideoAudioValue > 0))
        {
            addPremiereTrack(mediaType, videoClipsMap, videoFilesMap);
        }

        if (mediaType == "audio" && audioClipsMap.count() > 0) //add audio track if exists
        {
            addPremiereTrack(mediaType, audioClipsMap, audioFilesMap);
        }

        if (mediaType == "video" && watermarkFileName != "") //add audio track if exists
        {
            s("    <track>");

            int indexOf = watermarkFileName.lastIndexOf("/");
            QString WMFolderName = watermarkFileName.left(indexOf + 1);
            QString WMFileName = watermarkFileName.mid(indexOf + 1);

            addPremiereClipitem("WM", WMFolderName, WMFileName, 0, maxCombinedDurationInFrames, -1, -1, frameRate, mediaType, &videoFilesMap, 1, "", "", "");

            s("    </track>");
        }

        s("   </%1>", mediaType);

    } //foreach mediatype (source video)

    s("  </media>");

    s(" </sequence>");
//        s("  </children>");
//        s(" </project>");
    s("</xmeml>");

    fileWrite.close();
} //exportPremiere

void AExport::exportClips(QAbstractItemModel *ptimelineModel, QString ptarget, QString ptargetSize, QString pframeRate, int ptransitionTimeFrames, QSlider *exportVideoAudioSlider, QString pwatermarkFileName, QComboBox *clipsFramerateComboBox, QComboBox *clipsSizeComboBox)
{
    if (ptimelineModel->rowCount()==0)
    {
        QMessageBox::information(this, "Export", "No clips");
        return;
    }

    timelineModel = ptimelineModel;

    clipsMap.clear();
    videoClipsMap.clear();
    audioClipsMap.clear();

    int videoOriginalDuration = 0;
    int audioOriginalDuration = 0;
    int videoCountNrOfClips = 0;
    int audioCountNrOfClips = 0;

    for (int row = 0; row < timelineModel->rowCount();row++)
    {
        clipsMap[timelineModel->index(row, orderAfterMovingIndex).data().toInt()] = row;

        QString fileName = timelineModel->index(row, fileIndex).data().toString();
        QTime inTime = QTime::fromString(timelineModel->index(row,inIndex).data().toString(),"HH:mm:ss.zzz");
        QTime outTime = QTime::fromString(timelineModel->index(row,outIndex).data().toString(),"HH:mm:ss.zzz");

        int frameDuration = AGlobal().msec_to_frames(outTime.msecsSinceStartOfDay()) - AGlobal().msec_to_frames(inTime.msecsSinceStartOfDay()) + 1;

        if (fileName.toLower().contains(".mp3"))
        {
            audioClipsMap[timelineModel->index(row, orderAfterMovingIndex).data().toInt()] = row;
            audioOriginalDuration += frameDuration;
            audioCountNrOfClips++;
        }
        else
        {
            videoClipsMap[timelineModel->index(row, orderAfterMovingIndex).data().toInt()] = row;
            videoOriginalDuration += frameDuration;
            videoCountNrOfClips++;
        }
    }

    maxAudioDuration = audioOriginalDuration - ptransitionTimeFrames * (audioCountNrOfClips-1);
    maxVideoDuration = videoOriginalDuration - ptransitionTimeFrames * (videoCountNrOfClips-1);
    maxCombinedDurationInFrames = qMax(maxVideoDuration, maxAudioDuration);

    if (maxVideoDuration == 0)
    {
        QMessageBox::information(this, "Export", "No video clips");
        return;
    }

    selectedFolderName = QSettings().value("selectedFolderName").toString();

    transitionTimeFrames = ptransitionTimeFrames;

    watermarkFileName = pwatermarkFileName;

//    if (ptarget == "Lossless" && exportVideoAudioSlider->value() < 100)
//        exportVideoAudioValue = 0;
//    else
        exportVideoAudioValue = exportVideoAudioSlider->value();

    if (pframeRate == "Source")
        frameRate = clipsFramerateComboBox->currentText();
    else
        frameRate = pframeRate;

    target = ptarget;

    int indexOf = clipsSizeComboBox->currentText().indexOf(" x ");
    QString sourceWidth = clipsSizeComboBox->currentText().mid(0,indexOf);
    QString sourceHeight = clipsSizeComboBox->currentText().mid(indexOf + 3);


    QString targetSize;
    if (ptargetSize.contains("240p")) //3:4
    {
        targetSize = "240p";
//        videoWidth = "320"; //352 of 427?
        videoHeight = "240";
    }
    else if (ptargetSize.contains("360p")) //3:4
    {
        targetSize = "360p";
//        videoWidth = "480"; //640
        videoHeight = "360";
    }
    else if (ptargetSize.contains("480p"))
    {
        targetSize = "480p";
//        videoWidth = "640"; //853 or 858
        videoHeight = "480";
    }
    else if (ptargetSize.contains("720p"))//9:16
    {
        targetSize = "720p";
//        videoWidth = "1280";
        videoHeight = "720";
    }
    else if (ptargetSize.contains("Source")) //9:16
    {
//        videoWidth = sourceWidth;
        videoHeight = sourceHeight;
        targetSize = videoHeight;
    }
    else if (ptargetSize.contains("1K")) //9:16
    {
        targetSize = "1K";
//        videoWidth = "1920";
        videoHeight = "1080";
    }
    else if (ptargetSize.contains("2.7K"))//9:16
    {
        targetSize = "2.7K";
        videoHeight = "1620";
//        videoWidth = QString::number(videoHeight.toDouble() * sourceWidth.toDouble() / sourceHeight.toDouble());
    }
    else if (ptargetSize.contains("4K"))//9:16
    {
        targetSize = "4K";
//        videoWidth = "3840";
        videoHeight = "2160";
    }
    else if (ptargetSize.contains("8K"))//9:16
    {
        targetSize = "8K";
//        videoWidth = "7680";
        videoHeight = "4320";
    }

    if (sourceHeight.toDouble() != 0)
        videoWidth = QString::number(int(videoHeight.toDouble() * sourceWidth.toDouble() / sourceHeight.toDouble())); //maintain aspect ratio
    else
        videoWidth = sourceWidth.toDouble();

//    qDebug()<<"targetSize and framerate"<<ptargetSize<<targetSize<<videoWidth<<videoHeight<<pframeRate<<frameRate;

    if (videoWidth == "" || videoHeight == "" || frameRate == "")
    {
        QMessageBox::information(this, "Export", QString("One of the following values is not known: ImageWidth %1 ImageHeight %2 VideoFrameRate %3. Check the properties tab for their values").arg(videoWidth, videoHeight, frameRate));
        return;
    }

    fileNameWithoutExtension = target;
    if (target != "Lossless")
    {
        fileNameWithoutExtension += targetSize;
        fileNameWithoutExtension += "@" + frameRate;
    }
    videoFileExtension = ""; //assigned later

    bool includingSRT = false;
    if (includingSRT)
    {
        QFile srtOutputFile(selectedFolderName + fileNameWithoutExtension + ".srt");
        if (srtOutputFile.open(QIODevice::WriteOnly) )
        {
            QTextStream srtStream( &srtOutputFile );
            int totalDuration = 0;
            QMapIterator<int, int> clipsIterator(clipsMap);
            while (clipsIterator.hasNext()) //all files
            {
                clipsIterator.next();
                int row = clipsIterator.value();

                QString srtContentString = "";

                QTime inTime = QTime::fromString(timelineModel->index(row, inIndex).data().toString(),"HH:mm:ss.zzz");
                QTime outTime = QTime::fromString(timelineModel->index(row, outIndex).data().toString(),"HH:mm:ss.zzz");
                AStarRating starRating = qvariant_cast<AStarRating>(timelineModel->index(row, ratingIndex).data());

                srtContentString += "<o>" + QString::number(timelineModel->index(row, orderAfterMovingIndex).data().toInt()+2) + "</o>"; //+1 for file trim, +2 for export
                srtContentString += "<r>" + QString::number(starRating.starCount()) + "</r>";
                srtContentString += "<a>" + timelineModel->index(row, alikeIndex).data().toString() + "</a>";
                srtContentString += "<h>" + timelineModel->index(row, hintIndex).data().toString() + "</h>";
                srtContentString += "<t>" + timelineModel->index(row, tagIndex).data().toString() + "</t>";

                int duration = AGlobal().frames_to_msec(AGlobal().msec_to_frames(outTime.msecsSinceStartOfDay()) - AGlobal().msec_to_frames(inTime.msecsSinceStartOfDay()) + 1);

                srtStream << row+1 << endl;
                srtStream << QTime::fromMSecsSinceStartOfDay(totalDuration).toString("HH:mm:ss.zzz") << " --> " << QTime::fromMSecsSinceStartOfDay(totalDuration + duration - AGlobal().frames_to_msec(1)).toString("HH:mm:ss.zzz") << endl;
                srtStream << srtContentString << endl;//timelineModel->index(i, tagIndex).data().toString()
                srtStream << endl;

                totalDuration += duration;
            }
            srtOutputFile.close();
        }
    }

    filesMap.clear();
    audioFilesMap.clear();
    videoFilesMap.clear();
    for (int row = 0; row < timelineModel->rowCount(); row++)
    {
        QString folderName = timelineModel->index(row,folderIndex).data().toString();
        QString fileName = timelineModel->index(row,fileIndex).data().toString();
        filesMap[folderName + fileName].folderName = folderName;
        filesMap[folderName + fileName].fileName = fileName;
        filesMap[folderName + fileName].counter = filesMap.count() - 1;
        filesMap[folderName + fileName].definitionGenerated = false;

        if (fileName.toLower().contains(".mp3"))
        {
            audioFilesMap[folderName + fileName].folderName = folderName;
            audioFilesMap[folderName + fileName].fileName = fileName;
            audioFilesMap[folderName + fileName].counter = audioFilesMap.count() - 1;
            audioFilesMap[folderName + fileName].definitionGenerated = false;
        }
        else
        {
            videoFilesMap[folderName + fileName].folderName = folderName;
            videoFilesMap[folderName + fileName].fileName = fileName;
            videoFilesMap[folderName + fileName].counter = videoFilesMap.count() - 1;
            videoFilesMap[folderName + fileName].definitionGenerated = false;
        }
    }

    //delete old files
//    QDir dir(currentDirectory);
//    dir.setFilter( QDir::NoDotAndDotDot | QDir::Files );
//    foreach( QString dirItem, dir.entryList() )
//    {
//        if (dirItem.contains(fileNameWithoutExtension))
//            dir.remove( dirItem );
//    }

    AJobParams jobParams;
//    jobParams.folderName = currentDirectory;
    jobParams.fileName = "Timeline";
    jobParams.action = "Export " + target;

    QStandardItem *parentItem = jobTreeView->createJob(jobParams, nullptr , nullptr);

    QStandardItem *childItem;

    if (target == "Lossless") //Lossless FFMpeg
    {
        childItem = losslessVideoAndAudio(parentItem); //assigning videoFileExtension

        emit releaseMedia(fileNameWithoutExtension + videoFileExtension);

        if (audioClipsMap.count() > 0)
            childItem = muxVideoAndAudio(childItem);

        emit propertyCopy(childItem, selectedFolderName, videoFilesMap.first().fileName, selectedFolderName, fileNameWithoutExtension + videoFileExtension);

        emit moveFilesToACVCRecycleBin(childItem, selectedFolderName, fileNameWithoutExtension + "V" + videoFileExtension);
        if (audioClipsMap.count() > 0)
            emit moveFilesToACVCRecycleBin(childItem, selectedFolderName, fileNameWithoutExtension + "A.mp3");

        if (includingSRT)
            emit loadClips(parentItem);

        emit loadProperties(parentItem);
    }
    else if (target == "Encode") //FFMpeg encode
    {
        childItem = encodeVideoClips(parentItem); //assigning videoFileExtension

        emit releaseMedia(fileNameWithoutExtension + videoFileExtension);

        emit propertyCopy(childItem, selectedFolderName, videoFilesMap.first().fileName, selectedFolderName, fileNameWithoutExtension + videoFileExtension);

        if (includingSRT)
            emit loadClips(parentItem);

        emit loadProperties(parentItem);
    }
    else if (target == "Shotcut")
    {
        AJobParams jobParams;
        jobParams.thisWidget = this;
        jobParams.parentItem = parentItem;
        jobParams.folderName = selectedFolderName;
        jobParams.fileName = fileNameWithoutExtension + ".mlt";
        jobParams.action = "Export Shotcut";
        jobParams.parameters["totalDuration"] = QString::number(1000);

        jobTreeView->createJob(jobParams, [] (AJobParams jobParams)
        {
            AExport *exportWidget = qobject_cast<AExport *>(jobParams.thisWidget);

            exportWidget->exportShotcut(jobParams);

            return QString();
        }, nullptr);

    }
    else if (target == "Premiere")
    {
        AJobParams jobParams;
        jobParams.thisWidget = this;
        jobParams.parentItem = parentItem;
        jobParams.folderName = selectedFolderName;
        jobParams.fileName = fileNameWithoutExtension + ".xml";
        jobParams.action = "Export Premiere";
        jobParams.parameters["totalDuration"] = QString::number(1000);

        jobTreeView->createJob(jobParams, [] (AJobParams jobParams)
        {
            AExport *exportWidget = qobject_cast<AExport *>(jobParams.thisWidget);

            exportWidget->exportPremiere(jobParams);

            return QString();
        }, nullptr);
    }

    {
    AJobParams jobParams;
    jobParams.thisWidget = this;
    jobParams.parentItem = parentItem;
    jobParams.folderName = selectedFolderName;
    jobParams.fileName = fileNameWithoutExtension;
    jobParams.action = "Export ready";
    jobParams.parameters["totalDuration"] = QString::number(1000);

    jobTreeView->createJob(jobParams, [] (AJobParams jobParams)
    {
        AExport *exportWidget = qobject_cast<AExport *>(jobParams.thisWidget);

        emit exportWidget->exportCompleted("");

        return QString();
    }, nullptr);
    }

} //exportClips
