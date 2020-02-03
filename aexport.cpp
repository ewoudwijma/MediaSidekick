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
    processManager = new AProcessManager(this);
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

void AExport::onPropertyUpdate(QString folderName, QString fileNameSource, QString fileNameTarget)
{
    QString *processId = new QString();
    QMap<QString, QString> parameters;

    emit addJob(folderName, fileNameTarget, "Property update", processId);
    QString attributeString = "";
    QStringList texts;
    texts << "CreateDate" << "GPSLongitude" << "GPSLatitude" << "GPSAltitude" << "GPSAltitudeRef" << "Make" << "Model" << "Director" << "Producer"  << "Publisher";
    for (int iText = 0; iText < texts.count(); iText++)
    {
        QVariant *propertyName = new QVariant();
        emit getPropertyValue(fileNameSource, texts[iText], propertyName);
        attributeString += " -" + texts[iText] + "=\"" + propertyName->toString() + "\"";
    }

    QString targetFileName = folderName + fileNameTarget;
#ifdef Q_OS_WIN
    targetFileName = targetFileName.replace("/", "\\");
#endif

    QString command = "exiftool" + attributeString + " -overwrite_original \"" + targetFileName + "\"";
    emit addToJob(*processId, command + "\n");

#ifdef Q_OS_WIN
    command = qApp->applicationDirPath() + "/" + command;
#else
    command = qApp->applicationDirPath() + "/../PlugIns/exiftool/" + command;
#endif

    parameters["processId"] = *processId;
    processManager->startProcess(command, parameters, [](QWidget *parent, QMap<QString, QString> parameters, QString result)
    {
        AExport *exportWidget = qobject_cast<AExport *>(parent);
        emit exportWidget->addToJob(parameters["processId"], result);

//        exportWidget->progressBar->setValue(95);
    }, [] (QWidget *parent, QString, QMap<QString, QString> parameters, QStringList )//result
    {
        AExport *exportWidget = qobject_cast<AExport *>(parent);
        emit exportWidget->addToJob(parameters["processId"], "Completed");
    });
}

void AExport::onTrimC(QString folderName, QString fileNameSource, QString fileNameTarget, QTime inTime, QTime outTime, int progressPercentage)
{
//    qDebug()<<"AExport::onTrimC"<<folderName<<fileNameSource<<fileNameTarget<<inTime<<outTime<<progressPercentage;
    int duration = AGlobal().frames_to_msec(AGlobal().msec_to_frames(outTime.msecsSinceStartOfDay()) - AGlobal().msec_to_frames(inTime.msecsSinceStartOfDay()) + 1);

    QMap<QString, QString> parameters;

    QString *processId = new QString();
    emit addJob(folderName, fileNameTarget, "Trim", processId);
    parameters["processId"] = *processId;

    QString sourceFileName = folderName + fileNameSource;
    QString targetFileName = folderName + fileNameTarget;

#ifdef Q_OS_WIN
    sourceFileName = sourceFileName.replace("/", "\\");
    targetFileName = targetFileName.replace("/", "\\");
#endif

//    QString command = tool + " -y -i \"" + QString(folderName + "//" + fileNameSource).replace("/", "//") + "\" -ss " + inTime.toString("HH:mm:ss.zzz") + " -t " + QTime::fromMSecsSinceStartOfDay(duration).toString("hh:mm:ss.zzz") + " -map_metadata 0 -vcodec copy -acodec copy \"" + QString(folderName + fileNameTarget).replace("/", "//") + "\"";
    QString command = "ffmpeg -y -i \"" + sourceFileName + "\" -ss " + inTime.toString("HH:mm:ss.zzz") + " -t " + QTime::fromMSecsSinceStartOfDay(duration).toString("hh:mm:ss.zzz") + " -map_metadata 0 -vcodec copy -acodec copy \"" + targetFileName + "\"";

    emit addToJob(parameters["processId"], command + "\n");

#ifdef Q_OS_WIN
    command = qApp->applicationDirPath() + "/" + command;
#else
    command = qApp->applicationDirPath() + "/../PlugIns/ffmpeg/" + command;
#endif

    parameters["percentage"] = QString::number(progressPercentage);

    processManager->startProcess(command, parameters, nullptr,  [] (QWidget *parent, QString , QMap<QString, QString> parameters, QStringList )
    {
        AExport *exportWidget = qobject_cast<AExport *>(parent);
        emit exportWidget->addToJob(parameters["processId"], "Completed");
//        qDebug()<<"AExport::onTrim"<<exportWidget->progressBar<<(exportWidget->progressBar != nullptr);
//        if (exportWidget->progressBar != nullptr)
//            exportWidget->progressBar->setValue(parameters["percentage"].toInt());
    });

}

void AExport::onReloadAll(bool includingSRT)
{
//    qDebug()<<"AExport::onReloadAll"<<includingSRT;
    QMap<QString, QString> parameters;
    parameters["includingSRT"] = QString::number(includingSRT);
    processManager->startProcess(parameters, [] (QWidget *parent, QString , QMap<QString, QString> parameters, QStringList ) //command, result
    {
        AExport *exportWidget = qobject_cast<AExport *>(parent);

        if (parameters["includingSRT"].toInt())
            emit exportWidget->reloadClips();
//        qDebug()<<"AExport::onReloadAll processdone";
        emit exportWidget->reloadProperties();
    });
}

void AExport::losslessVideoAndAudio()
{
    //create audio and video stream
    QStringList mediaTypeList;

    mediaTypeList << "V";
    if (audioClipsMap.count() > 0)
        mediaTypeList << "A";

    foreach (QString mediaType, mediaTypeList) //prepare video and audio
    {
        QFile vidlistFile(currentDirectory + fileNameWithoutExtension + mediaType + ".txt");
        qDebug()<<"opening vidlistfile"<<vidlistFile;
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

            progressBar->setRange(0, 100);

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

                QString *processId = new QString();
                emit addJob(currentDirectory, fileNamePlusExtension, "FFMpeg lossless " + mediaType, processId);

                QString sourceFileName = currentDirectory + fileNameWithoutExtension + mediaType + ".txt";
                QString targetFileName = currentDirectory + fileNamePlusExtension;

#ifdef Q_OS_WIN
                sourceFileName = sourceFileName.replace("/", "\\");
                targetFileName = targetFileName.replace("/", "\\");
#endif
                QString command = "ffmpeg -f concat -safe 0 -i \"" + sourceFileName + "\" -c copy -y \"" + targetFileName + "\"";

                if (mediaType == "V" && exportVideoAudioValue == 0) //remove audio
                    command.replace("-c copy -y", " -an -c copy -y");

    //            if (frameRate != "")
    //                command.replace("-c copy -y", " -r " + frameRate + " -c copy -y");

                emit addToJob(*processId, command + "\n");

#ifdef Q_OS_WIN
    command = qApp->applicationDirPath() + "/" + command;
#else
    command = qApp->applicationDirPath() + "/../PlugIns/ffmpeg/" + command;
#endif

                QMap<QString, QString> parameters;
                parameters["processId"] = *processId;
                parameters["exportFileName"] = fileNamePlusExtension;

                processManager->startProcess(command, parameters, [] (QWidget *parent, QMap<QString, QString> parameters, QString result)
                {
                    AExport *exportWidget = qobject_cast<AExport *>(parent);

                    exportWidget->processOutput(parameters, result, 10, 60);

                },  [] (QWidget *parent, QString , QMap<QString, QString> parameters, QStringList )//command, result
                {
                    AExport *exportWidget = qobject_cast<AExport *>(parent);

                    exportWidget->processFinished(parameters);


                });
            } //if fileExtension

        } //if vidlist
        else
        {
            qDebug()<<"vidlistFile.error"<<vidlistFile.error()<< vidlistFile.errorString();
        }

    } //for each

} //losslessVideoAndAudio

void AExport::encodeVideoClips()
{
    QString mediaClipString = "";
    QString videoConcatString = "";
    QString audioConcatString = "";
    QString sep = "";
//    int resultDurationMSec = 0;

    progressBar->setRange(0, 100);

    //create audio and video stream
    QStringList mediaTypeList;
    mediaTypeList << "V";
    if (audioClipsMap.count() > 0)
        mediaTypeList << "A";

    int streamCounter = 0;

    foreach (QString mediaType, mediaTypeList) //prepare video and audio
    {
        QMap<int,int> audioOrVideoClipsMap;

        if (mediaType == "A")
            audioOrVideoClipsMap = audioClipsMap;
        else
            audioOrVideoClipsMap = videoClipsMap;

        bool videoClipAdded = false;
        bool audioClipAdded = false;

        int iterationCounter = 0;
        QMapIterator<int, int> clipsIterator(audioOrVideoClipsMap);
        while (clipsIterator.hasNext()) //all files
        {
            clipsIterator.next();
            int row = clipsIterator.value();

            QTime inTime = QTime::fromString(timelineModel->index(row, inIndex).data().toString(),"HH:mm:ss.zzz");
            QTime outTime = QTime::fromString(timelineModel->index(row, outIndex).data().toString(),"HH:mm:ss.zzz");
            QString folderName = timelineModel->index(row, folderIndex).data().toString();
            QString fileName = timelineModel->index(row, fileIndex).data().toString();
            QString tags = timelineModel->index(row, tagIndex).data().toString();

            if (transitionTimeFrames > 0)
            {
                if (iterationCounter != 0)
                {
                    inTime = inTime.addMSecs(AGlobal().frames_to_msec((transitionTimeFrames/2))); //subtract half of the transitionframes
                }
                if (iterationCounter != audioOrVideoClipsMap.count()-1)
                {
                    outTime = outTime.addMSecs(- AGlobal().frames_to_msec(qRound(transitionTimeFrames/2.0))); //subtract half of the transitionframes
                }
            }

//            int duration = AGlobal().frames_to_msec(AGlobal().msec_to_frames(outTime.msecsSinceStartOfDay()) - AGlobal().msec_to_frames(inTime.msecsSinceStartOfDay()) + 1);
//            resultDurationMSec += duration;

//            qDebug()<<row<<timelineModel->index(row, inIndex).data().toString()<<timelineModel->index(row, outIndex).data().toString();

            double inSeconds = inTime.msecsSinceStartOfDay() / 1000.0;
            double outSeconds = outTime.msecsSinceStartOfDay() / 1000.0;
            int fileCounter = filesMap[folderName + fileName].counter;

            if (mediaType == "V")
            {
                QString newfcs = sep + "[" + QString::number(fileCounter) + ":v]trim=" + QString::number(inSeconds, 'g', 6) + ":" + QString::number(outSeconds, 'g', 6) + ",setpts=PTS-STARTPTS,scale=" + videoWidth + "x" + videoHeight + "[v" + QString::number(row) + "]";

                if (tags.toLower().contains("backwards"))
                    newfcs.replace("setpts=PTS-STARTPTS", "setpts=PTS-STARTPTS, reverse");
                //https://stackoverflow.com/questions/42257354/concat-a-video-with-itself-but-in-reverse-using-ffmpeg
                if (tags.toLower().contains("slowmotion"))
                    newfcs.replace("setpts=PTS-STARTPTS", "setpts=2.0*PTS");
                if (tags.toLower().contains("fastmotion"))
                    newfcs.replace("setpts=PTS-STARTPTS", "setpts=0.5*PTS");

                mediaClipString += newfcs;

                sep = ";";

                if (!videoClipAdded)
                    videoConcatString += sep;

                videoConcatString += "[v" + QString::number(row) + "]";

                videoClipAdded = true;

                if (exportVideoAudioValue > 0)
                {
                    mediaClipString += sep + "[" + QString::number(fileCounter) + ":a]";

                    if (exportVideoAudioValue < 100)
                        mediaClipString += "volume=" + QString::number(exportVideoAudioValue / 100.0) + ",";

                    mediaClipString += "aformat=sample_fmts=fltp:sample_rates=44100:channel_layouts=stereo,atrim=" + QString::number(inSeconds, 'g', 6) + ":" + QString::number(outSeconds, 'g', 6) + ",asetpts=PTS-STARTPTS[a" + QString::number(row) + "]";

                    if (!audioClipAdded)
                        audioConcatString += sep;

                    audioConcatString += "[a" + QString::number(row) + "]";

                    audioClipAdded = true;
                }

            }
            else
            {
                mediaClipString += sep + "[" + QString::number(fileCounter) + ":a]";

                mediaClipString += "atrim=" + QString::number(inSeconds, 'g', 6) + ":" + QString::number(outSeconds, 'g', 6) + ",asetpts=PTS-STARTPTS[a" + QString::number(row) + "]";

                sep = ";";

                if (!audioClipAdded)
                    audioConcatString += sep;

                audioConcatString += "[a" + QString::number(row) + "]";

                audioClipAdded = true;

            }

//                QString targetFileName = fileNameWithoutExtension + "-" + QString::number(row) + ".mp4";
//                onTrim(timelineModel->index(i, folderIndex).data().toString(),  timelineModel->index(i, fileIndex).data().toString(), targetFileName, inTime, outTime, 10*i/(timelineModel->rowCount())); //not -1 to avoid divby0
            iterationCounter++;
        } //while

        if (videoClipAdded)
            videoConcatString += " concat=n=" + QString::number(iterationCounter) + ":v=1:a=0[video" + QString::number(streamCounter) + "]";
        if (audioClipAdded)
            audioConcatString += " concat=n=" + QString::number(iterationCounter) + ":v=0:a=1[audio" + QString::number(streamCounter) + "]";

        streamCounter++;

    } //foreach mediatype

    QString filesString = "";

    for (int i = 0; i < filesMap.count(); i++)
    {
        QMapIterator<QString, FileStruct> videoFilesIterator(filesMap);
        while (videoFilesIterator.hasNext()) //all files
        {
            videoFilesIterator.next();

            if (videoFilesIterator.value().counter == i)
                filesString = filesString + " -i \"" + videoFilesIterator.value().folderName + videoFilesIterator.value().fileName + "\"";
        }
    }

    QString command = "ffmpeg " + filesString;

    //tbd: consider Simple fade-in and fade-out for transitions: ffmpeg -i <input> -filter:v "fade=t=in:st=0:d=5,fade=t=out:st=30:d=5" <output>

    if (watermarkFileName != "")
    {
        command += " -i \"" + watermarkFileName + "\"";
        mediaClipString += sep + "[" + QString::number(filesMap.count()) + ":v]scale=" + QString::number(videoWidth.toInt()/10) + "x" + QString::number(videoHeight.toInt()/10) + "[wtm]";
    }

    command += " -filter_complex \"" + mediaClipString + " " + videoConcatString + audioConcatString;

    //.\ffmpeg  -i "D:/Video/2019-09-02 Fipre/2000-01-19 00-00-00 +53632ms.MP4" -i "D:/Video/2019-09-02 Fipre/Blindfold.mp3" -i "D:/Video/2019-09-02 Fipre/Child in Time (2016 Remaster).mp3" -i "C:/Users/ewoud/OneDrive/Documents/ACVC project/ACVC/acvclogo.png" -filter_complex "[0:v]trim=1.56:4.36,setpts=PTS-STARTPTS,scale=640x480[v0][0:a]volume=0.18,atrim=1.56:4.36,asetpts=PTS-STARTPTS[a0];[0:v]trim=6.8:9.44,setpts=PTS-STARTPTS,scale=640x480[v1][0:a]volume=0.18,atrim=6.8:9.44,asetpts=PTS-STARTPTS[a1];[0:v]trim=14.44:15.64,setpts=PTS-STARTPTS,scale=640x480[v2][0:a]volume=0.18,atrim=14.44:15.64,asetpts=PTS-STARTPTS[a2];[0:v]trim=19.84:22.48,setpts=PTS-STARTPTS,scale=640x480[v3][0:a]volume=0.18,atrim=19.84:22.48,asetpts=PTS-STARTPTS[a3];[0:v]trim=25.72:28.52,setpts=PTS-STARTPTS,scale=640x480[v4][0:a]volume=0.18,atrim=25.72:28.52,asetpts=PTS-STARTPTS[a4];[1:a]atrim=85.92:87.2,asetpts=PTS-STARTPTS[a5];[2:a]atrim=120.24:122.88,asetpts=PTS-STARTPTS[a6];[2:a]atrim=236.24:238.88,asetpts=PTS-STARTPTS[a7];[2:a]atrim=401.92:404.56,asetpts=PTS-STARTPTS[a8];[2:a]atrim=467.76:470.56,asetpts=PTS-STARTPTS[a9];[3:v]scale=64x48[wtm] ;[v0][v1][v2][v3][v4] concat=n=5:v=1:a=0[video0];[a0][a1][a2][a3][a4] concat=n=5:v=0:a=1[audio0];[a5][a6][a7][a8][a9] concat=n=5:v=0:a=1[audio1];[audio0][audio1]amerge=inputs=2[audio];[video0][wtm]overlay = main_w-overlay_w-10:main_h-overlay_h-10[video]" -map "[video]" -map "[audio]" -r 25  -y "D:\Video\2019-09-02 Fipre\\Encode480p@25.mp4"
    //.\ffmpeg  -i "D:/Video/Bras_DVR/04-Devilette.mp3" -i "D:/Video/Bras_DVR/PICT0203.AVI" -i "D:/Video/Bras_DVR/PICT0206.AVI" -i "D:/Video/Bras_DVR/PICT0208.AVI" -i "D:/Video/Bras_DVR/PICT0209.AVI" -i "D:/Video/Bras_DVR/PICT0216.AVI" -i "C:/Users/ewoud/OneDrive/Documents/ACVC project/ACVC/acvclogo.png" -filter_complex "[1:v]trim=48.84:65.747,setpts=PTS-STARTPTS,scale=640x480[v1];[1:a]volume=0.18,atrim=48.84:65.747,asetpts=PTS-STARTPTS[a1];[1:v]trim=128.293:147.147,setpts=PTS-STARTPTS,scale=640x480[v2];[1:a]volume=0.18,atrim=128.293:147.147,asetpts=PTS-STARTPTS[a2];[2:v]trim=145.213:169.267,setpts=PTS-STARTPTS,scale=640x480[v3];[2:a]volume=0.18,atrim=145.213:169.267,asetpts=PTS-STARTPTS[a3];[3:v]trim=13.133:27.347,setpts=PTS-STARTPTS,scale=640x480[v4];[3:a]volume=0.18,atrim=13.133:27.347,asetpts=PTS-STARTPTS[a4];[3:v]trim=139.373:175.907,setpts=PTS-STARTPTS,scale=640x480[v5];[3:a]volume=0.18,atrim=139.373:175.907,asetpts=PTS-STARTPTS[a5];[4:v]trim=5.413:25.347,setpts=PTS-STARTPTS,scale=640x480[v6];[4:a]volume=0.18,atrim=5.413:25.347,asetpts=PTS-STARTPTS[a6];[5:v]trim=60.433:121.467,setpts=PTS-STARTPTS,scale=640x480[v7];[5:a]volume=0.18,atrim=60.433:121.467,asetpts=PTS-STARTPTS[a7];[0:a]atrim=100.7:252.533,asetpts=PTS-STARTPTS[a0];[6:v]scale=64x48[wtm] ;[v1][v2][v3][v4][v5][v6][v7] concat=n=7:v=1:a=0[video0];[a1][a2][a3][a4][a5][a6][a7] concat=n=7:v=0:a=1[audio0];[a0] concat=n=1:v=0:a=1[audio1];[audio0][audio1]amerge=inputs=2[audio];[video0][wtm]overlay = main_w-overlay_w-10:main_h-overlay_h-10[video]" -map "[video]" -map "[audio]" -r 30  -y "D:\Video\Bras_DVR\\Encode480@30.mp4"
    //.\ffmpeg  -i "D:/Video/Bras_DVR/04-Devilette.mp3" -i "D:/Video/Bras_DVR/PICT0203.AVI" -i "D:/Video/Bras_DVR/PICT0206.AVI" -i "D:/Video/Bras_DVR/PICT0208.AVI" -i "D:/Video/Bras_DVR/PICT0209.AVI" -i "D:/Video/Bras_DVR/PICT0216.AVI" -i "C:/Users/ewoud/OneDrive/Documents/ACVC project/ACVC/acvclogo.png"
    // -filter_complex "[1:v]trim=48.84:65.747,setpts=PTS-STARTPTS,scale=640x480[v2];[1:a]volume=0.18,atrim=48.84:65.747,asetpts=PTS-STARTPTS[a2]
    // ;[1:v]trim=128.293:147.147,setpts=PTS-STARTPTS,scale=640x480[v3];[1:a]volume=0.18,atrim=128.293:147.147,asetpts=PTS-STARTPTS[a3]
    // ;[2:v]trim=145.213:169.267,setpts=PTS-STARTPTS,scale=640x480[v4];[2:a]volume=0.18,atrim=145.213:169.267,asetpts=PTS-STARTPTS[a4]
    // ;[3:v]trim=13.133:27.347,setpts=PTS-STARTPTS,scale=640x480[v5];[3:a]volume=0.18,atrim=13.133:27.347,asetpts=PTS-STARTPTS[a5]
    // ;[3:v]trim=139.373:175.907,setpts=PTS-STARTPTS,scale=640x480[v6];[3:a]volume=0.18,atrim=139.373:175.907,asetpts=PTS-STARTPTS[a6]
    // ;[4:v]trim=5.413:25.347,setpts=PTS-STARTPTS,scale=640x480[v7];[4:a]volume=0.18,atrim=5.413:25.347,asetpts=PTS-STARTPTS[a7]
    // ;[5:v]trim=60.433:121.467,setpts=PTS-STARTPTS,scale=640x480[v8];[5:a]volume=0.18,atrim=60.433:121.467,asetpts=PTS-STARTPTS[a8]
    // ;[0:a]atrim=100.7:186.267,asetpts=PTS-STARTPTS[a0]
    // ;[0:a]atrim=259.166:327.5,asetpts=PTS-STARTPTS[a1]
    // ;[6:v]scale=64x48[wtm]
    // ;[v2][v3][v4][v5][v6][v7][v8] concat=n=7:v=1:a=0[video0]
    // ;[a2][a3][a4][a5][a6][a7][a8] concat=n=7:v=0:a=1[audio0];[a0][a1] concat=n=2:v=0:a=1[audio1]
    // ;[audio0][audio1]amerge=inputs=2[audio]
    // ;[video0][wtm]overlay = main_w-overlay_w-10:main_h-overlay_h-10[video]" -map "[video]" -map "[audio]" -r 30  -y "D:\Video\Bras_DVR\\Encode480@30.mp4"



    if (audioClipsMap.count() > 0 && exportVideoAudioValue > 0)
    {
        command += sep + "[audio0][audio1]amerge=inputs=2[audio]";
    }

    if (watermarkFileName != "")
        command += sep + "[video0][wtm]overlay = main_w-overlay_w-10:main_h-overlay_h-10[video]\" -map \"[video]\"";
    else
        command += "\" -map \"[video0]\"";

    if (audioClipsMap.count() > 0 && exportVideoAudioValue > 0)
        command += " -map \"[audio]\"";
    else if (audioClipsMap.count() == 0 && exportVideoAudioValue > 0)
        command += " -map \"[audio0]\"";
    else if (audioClipsMap.count() > 0 && exportVideoAudioValue == 0)
        command += " -map \"[audio1]\"";

    if (frameRate != "")
        command += " -r " + frameRate; //tbd: consider -filter:v fps=24 (minterpolate instead of dropping or duplicating frames)

    videoFileExtension = ".mp4";

    QString targetFileName = currentDirectory +  fileNameWithoutExtension + videoFileExtension;

#ifdef Q_OS_WIN
    targetFileName = targetFileName.replace("/", "\\");
#endif

    command +=  "  -y \"" + targetFileName + "\"";

//        qDebug()<<"filter_complex"<<command;

    //        -filter_complex \"[0:v]scale=640x640 [0:a] [1:v]scale=640x640 [1:a] concat=n=2:v=1:a=1 [v] [a]\" -map \"[v]\" -map \"[a]\" .\\outputreencode.mp4";

    //https://ffmpeg.org/ffmpeg-filters.html#trim

    QString *processId = new QString();

    emit addJob(currentDirectory, fileNameWithoutExtension + videoFileExtension, "FFMPeg Encode", processId);
    QMap<QString, QString> parameters;
    parameters["processId"] = *processId;
    emit addToJob(parameters["processId"], command + "\n");

#ifdef Q_OS_WIN
    command = qApp->applicationDirPath() + "/" + command;
#else
    command = qApp->applicationDirPath() + "/../PlugIns/ffmpeg/" + command;
#endif


    parameters["fileNameWithoutExtension"] = fileNameWithoutExtension;
    parameters["exportFileName"] = fileNameWithoutExtension + videoFileExtension;

    processManager->startProcess(command, parameters, [] (QWidget *parent, QMap<QString, QString> parameters, QString result)
    {
        AExport *exportWidget = qobject_cast<AExport *>(parent);
        exportWidget->processOutput(parameters, result, 10, 80);

    },  [] (QWidget *parent, QString , QMap<QString, QString> parameters, QStringList )
    {
        AExport *exportWidget = qobject_cast<AExport *>(parent);

        exportWidget->processFinished(parameters);

        QDir dir(exportWidget->currentDirectory);
        dir.setFilter( QDir::NoDotAndDotDot | QDir::Files );
        foreach( QString dirItem, dir.entryList() )
        {
            if (dirItem.contains(parameters["fileNameWithoutExtension"] + "-"))
                dir.remove( dirItem );
        }

    });

} //encodeVideoClips

void AExport::processOutput(QMap<QString, QString> parameters, QString result, int percentageStart, int percentageDelta)
{
    if (!result.contains("Non-monotonous DTS in output stream")) //only warning, sort out later
        emit addToJob(parameters["processId"], result);

    foreach (QString resultLine, result.split("\n"))
    {
        if (resultLine.contains("Could not")  || resultLine.contains("Invalid"))
            processError = resultLine;
    }

    int timeIndex = result.indexOf("time=");
    if (timeIndex > 0)
    {
        QString timeString = result.mid(timeIndex + 5, 11) + "0";
        QTime time = QTime::fromString(timeString,"HH:mm:ss.zzz");
        progressBar->setValue(percentageStart + percentageDelta * time.msecsSinceStartOfDay() / AGlobal().frames_to_msec(maxCombinedDuration));
    }
}

void AExport::processFinished(QMap<QString, QString> parameters)
{
    //check successful
    QString errorMessage = "";

    QFile file(currentDirectory + parameters["exportFileName"]);
    if (file.exists())
    {
        if (file.size() < 100)
            errorMessage = "File " + parameters["exportFileName"] + " " + QString::number(file.size()) + " bytes";
        else
            errorMessage = "OK";
    }
    else
        errorMessage = "File " + parameters["exportFileName"] + " not exported";

    if (errorMessage != "OK")
    {
        if (processError != "")
        {
            emit addToJob(parameters["processId"], errorMessage + "\n");
            emit addToJob(parameters["processId"], processError);
        }
        else
            emit addToJob(parameters["processId"], errorMessage);
    }
    else
    {
        emit addToJob(parameters["processId"], "Completed");
        processError = "";
    }
}

void AExport::muxVideoAndAudio()
{
    QString *processId = new QString();

    emit addJob(currentDirectory, fileNameWithoutExtension + videoFileExtension, "FFMpeg mux", processId);

    QString targetFileName = currentDirectory +  fileNameWithoutExtension + "V" + videoFileExtension;

#ifdef Q_OS_WIN
    targetFileName = targetFileName.replace("/", "\\");
#endif

    QString command = "ffmpeg -i \"" + targetFileName + "\"";

    if (audioClipsMap.count() > 0)
    {
        QString targetFileName = currentDirectory +  fileNameWithoutExtension +  + "A.mp3";

    #ifdef Q_OS_WIN
        targetFileName = targetFileName.replace("/", "\\");
    #endif

        command += " -i \"" + targetFileName + "\"";

        if (exportVideoAudioValue == 0)
            command += " -c copy -map 0:v -map 1:a";
        else if (exportVideoAudioValue == 100)
            command += " -filter_complex \"[0][1]amix=inputs=2[a]\" -map 0:v -map \"[a]\" -c:v copy";
        else
            command += " -filter_complex \"[0]volume=" + QString::number(exportVideoAudioValue / 100.0) + "[a0];[a0][1]amix=inputs=2[a]\" -map 0:v -map \"[a]\" -c:v copy";
    }
    else
        command += " -filter:a \"volume=" + QString::number(exportVideoAudioValue / 100.0) + "\"";

    targetFileName = currentDirectory +  fileNameWithoutExtension + videoFileExtension;

#ifdef Q_OS_WIN
    targetFileName = targetFileName.replace("/", "\\");
#endif

    command += " -y \"" + targetFileName + "\"";

    emit addToJob(*processId, command + "\n");

#ifdef Q_OS_WIN
    command = qApp->applicationDirPath() + "/" + command;
#else
    command = qApp->applicationDirPath() + "/../PlugIns/ffmpeg/" + command;
#endif

    QMap<QString, QString> parameters;
    parameters["processId"] = *processId;
    parameters["exportFileName"] = fileNameWithoutExtension + videoFileExtension;

    processManager->startProcess(command, parameters, [] (QWidget *parent, QMap<QString, QString> parameters, QString result)
    {
        AExport *exportWidget = qobject_cast<AExport *>(parent);
        exportWidget->processOutput(parameters, result, 70, 20);

    },  [] (QWidget *parent, QString , QMap<QString, QString> parameters, QStringList )//command, result
    {
        AExport *exportWidget = qobject_cast<AExport *>(parent);
        exportWidget->processFinished(parameters);
    });

} //muxVideoAndAudio

void AExport::removeTemporaryFiles()
{
    //remove temp files
    QMap<QString, QString> parameters;
    processManager->startProcess(parameters, [] (QWidget *parent, QString , QMap<QString, QString> , QStringList ) //, command, parameters, result
    {
        AExport *exportWidget = qobject_cast<AExport *>(parent);

        QDir dir(exportWidget->currentDirectory);
        dir.setFilter( QDir::NoDotAndDotDot | QDir::Files );
        foreach( QString dirItem, dir.entryList() )
        {
            if (dirItem.contains(exportWidget->fileNameWithoutExtension + "V") || dirItem.contains(exportWidget->fileNameWithoutExtension + "A"))
                dir.remove( dirItem );
        }
    });

}

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
                QTime inTime = QTime::fromString(timelineModel->index(row, inIndex).data().toString(),"HH:mm:ss.zzz");
                QTime outTime = QTime::fromString(timelineModel->index(row, outIndex).data().toString(),"HH:mm:ss.zzz");

                int inFrames = qRound(inTime.msecsSinceStartOfDay() * frameRate.toInt() / 1000.0);
                int outFrames = qRound(outTime.msecsSinceStartOfDay() * frameRate.toInt() / 1000.0);

                QString clipFrameRate = "";
                QString clipAudioChannels = "";

                QVariant *frameRatePointer = new QVariant();
                emit getPropertyValue(fileName, "VideoFrameRate", frameRatePointer);
                if (frameRatePointer->toString() != "")
                    clipFrameRate = frameRatePointer->toString();
                else
                    clipFrameRate = frameRate;

                QVariant *audioChannelsPointer = new QVariant();
                emit getPropertyValue(fileName, "AudioChannels", audioChannelsPointer);
                if (audioChannelsPointer->toString() != "")
                    clipAudioChannels = audioChannelsPointer->toString();
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
                            addPremiereTransitionItem(0, clipFrameRate.toInt(), clipFrameRate, mediaType, "start");
                        }
                    }
                }

                if (trackNr == 1 && iterationCounter%2 == 1 && transitionTimeFrames > 0) //track 2 and not first clip (never is)
                {
                    addPremiereTransitionItem(totalFrames, totalFrames + transitionTimeFrames, clipFrameRate, mediaType, "start");
                }

                int deltaFrames = outFrames - inFrames + 1 - transitionTimeFrames;
                if (iterationCounter%2 == trackNr) //even or odd tracks
                {
                    addPremiereClipitem(QString::number(row), folderName, fileName, totalFrames, totalFrames + deltaFrames + transitionTimeFrames, inFrames, outFrames + 1, clipFrameRate, mediaType, &filesMap, channelTrackNr, clipAudioChannels);
                }

                totalFrames += deltaFrames;

                if (trackNr == 1 && iterationCounter%2 == 1 && iterationCounter != clipsMap.count()-1 && transitionTimeFrames > 0) //track 2 and not last clip
                {
                    addPremiereTransitionItem(totalFrames, totalFrames + transitionTimeFrames, clipFrameRate, mediaType, "end");
                }

                if (mediaType == "audio" && clipsMap == audioClipsMap)
                {
                    if (iterationCounter%2 == trackNr) //even or odd tracks
                    {
                        if (audioClipsMap.last() == row) //fade out
                        {
                            addPremiereTransitionItem(maxAudioDuration - clipFrameRate.toInt(), maxAudioDuration, clipFrameRate, mediaType, "end");
                        }
                    }
                }

    //            emit addToJob(*processId, QString("  Producer%1 %2 %3\n"));

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

void AExport::addPremiereClipitem(QString clipId, QString folderName, QString fileName, int startFrames, int endFrames, int inFrames, int outFrames, QString frameRate, QString mediaType, QMap<QString, FileStruct> *filesMap, int channelTrackNr, QString clipAudioChannels)
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
            QVariant *widthPointer = new QVariant();
            emit getPropertyValue(fileName, "ImageWidth", widthPointer);
            QVariant *heightPointer = new QVariant();
            emit getPropertyValue(fileName, "ImageHeight", heightPointer);

            if (heightPointer->toString() != videoHeight || widthPointer->toString() != videoWidth)
            {
                double heightRatio = 100.0 * videoHeight.toInt() / (heightPointer)->toInt();
                double widthRatio = 100.0 * videoWidth.toInt() / (widthPointer)->toInt();
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

void AExport::exportClips(QAbstractItemModel *ptimelineModel, QString ptarget, QString ptargetSize, QString pframeRate, int ptransitionTimeFrames, QProgressBar *p_progressBar, QSlider *exportVideoAudioSlider, ASpinnerLabel *pSpinnerLabel, QString pwatermarkFileName, QPushButton *pExportButton, QComboBox *clipsFramerateComboBox, QComboBox *clipsSizeComboBox, QStatusBar *pstatusBar)
{
    if (ptimelineModel->rowCount()==0)
    {
        QMessageBox::information(this, "Export", "No clips");
        return;
    }

    processError = "";

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
    maxCombinedDuration = qMax(maxVideoDuration, maxAudioDuration);

    if (maxVideoDuration == 0)
    {
        QMessageBox::information(this, "Export", "No video clips");
        return;
    }

    currentDirectory = QSettings().value("LastFolder").toString();

    transitionTimeFrames = ptransitionTimeFrames;
    int transitionTimeMSecs = transitionTimeFrames * AGlobal().frames_to_msec(1);
    QTime transitionQTime = QTime::fromMSecsSinceStartOfDay(transitionTimeMSecs);

    progressBar = p_progressBar;
    progressBar->setRange(0, 100);
    progressBar->setStyleSheet("QProgressBar::chunk {background: " + palette().highlight().color().name() + "}");

    statusBar = pstatusBar;

    exportButton = pExportButton;
    exportButton->setEnabled(false);

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

    videoWidth = QString::number(int(videoHeight.toDouble() * sourceWidth.toDouble() / sourceHeight.toDouble())); //maintain aspect ratio

//    qDebug()<<"targetSize and framerate"<<ptargetSize<<targetSize<<videoWidth<<videoHeight<<pframeRate<<frameRate;

    if (videoWidth == "" || videoHeight == "" || frameRate == "")
    {
        QMessageBox::information(this, "Export", QString("One of the following values is not known. Videowidth %1 VideoHeight %2 Framerate %3").arg(videoWidth, videoHeight, frameRate));
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
        QFile srtOutputFile(currentDirectory + fileNameWithoutExtension + ".srt");
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

    spinnerLabel = pSpinnerLabel;
    spinnerLabel->start();

    //wideview
    //https://intofpv.com/t-using-free-command-line-sorcery-to-fake-superview
    //https://github.com/Niek/superview

    QDir dir(currentDirectory);
    dir.setFilter( QDir::NoDotAndDotDot | QDir::Files );
    foreach( QString dirItem, dir.entryList() )
    {
        if (dirItem.contains(fileNameWithoutExtension))
            dir.remove( dirItem );
    }

    if (target == "Lossless") //Lossless FFMpeg
    {
        losslessVideoAndAudio();

        if (audioClipsMap.count() > 0)
            muxVideoAndAudio();

        onPropertyUpdate(currentDirectory, videoFilesMap.first().fileName, fileNameWithoutExtension + videoFileExtension);

        removeTemporaryFiles();

        onReloadAll(includingSRT);

    }
    else if (target == "Encode") //FFMpeg encode
    {
        encodeVideoClips();

        onPropertyUpdate(currentDirectory, videoFilesMap.first().fileName, fileNameWithoutExtension + videoFileExtension);

//        removeTemporaryFiles();

        onReloadAll(includingSRT);
    }
    else if (target == "Shotcut")
    {
        QString *processId = new QString();
        emit addJob(currentDirectory, fileNameWithoutExtension, "file export", processId);

        QString fileName = fileNameWithoutExtension + ".mlt";
        QFile fileWrite(currentDirectory + fileName);
        fileWrite.open(QIODevice::WriteOnly);

//        QTextStream stream(&fileWrite);

        stream.setDevice(&fileWrite);

        emit addToJob(*processId, QString("Generating %1\n\n").arg(fileName));

        s("<?xml version=\"1.0\" encoding=\"utf-8\"?>");
        s("<mlt LC_NUMERIC=\"C\" version=\"6.17.0\" title=\"Shotcut v19.10.20 by ACVC v%1\" producer=\"main_bin\">", qApp->applicationVersion());
        s("  <profile description=\"automatic\" width=\"%1\" height=\"%2\" progressive=\"1\" sample_aspect_num=\"1\" sample_aspect_den=\"1\" display_aspect_num=\"%1\" display_aspect_den=\"%2\" frame_rate_num=\"%3\" frame_rate_den=\"1\"/>", videoWidth, videoHeight, frameRate);

        emit addToJob(*processId, "Producers\n");

        QMapIterator<int, int> clipsIterator(clipsMap);
        while (clipsIterator.hasNext()) //all files
        {
            clipsIterator.next();
            int row = clipsIterator.value();

            QString folderName = timelineModel->index(row, folderIndex).data().toString();
            QString fileName = timelineModel->index(row, fileIndex).data().toString();
            QTime inTime = QTime::fromString(timelineModel->index(row, inIndex).data().toString(),"HH:mm:ss.zzz");
            QTime outTime = QTime::fromString(timelineModel->index(row, outIndex).data().toString(),"HH:mm:ss.zzz");
            int clipDuration = AGlobal().frames_to_msec(AGlobal().msec_to_frames(outTime.msecsSinceStartOfDay()) - AGlobal().msec_to_frames(inTime.msecsSinceStartOfDay()) + 1);

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

            s("  <producer id=\"producer%1\" title=\"Anonymous Submission\" in=\"00:00:00.000\" out=\"%2\">", QString::number(row), durationTime.toString("hh:mm:ss.zzz"));
            s("    <property name=\"length\">%1</property>", durationTime.toString("hh:mm:ss.zzz"));
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

        emit addToJob(*processId, "\n");

        emit addToJob(*processId, "Playlist\n");
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
            emit addToJob(*processId, QString("  Producer%1 %2 %3\n").arg( QString::number(row), inTime.toString("HH:mm:ss.zzz"), outTime.toString("HH:mm:ss.zzz")));
        }
        s("  </playlist>"); //playlist main bin
        emit addToJob(*processId, "\n");

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

            emit addToJob(*processId, "Transitions\n");

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

                        emit addToJob(*processId, QString("  Transition%1 %2 %3\n").arg( previousProducerNr + "-" + producerNr, "Transition " + previousProducerNr + "-" + producerNr, transitionQTime.toString("HH:mm:ss.zzz")));

                    }

                    previousInTime = inTime;
                    previousOutTime = outTime;
                    previousProducerNr = producerNr;
                    iterationCounter++;
                }
            }
            emit addToJob(*processId, "\n");
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

            emit addToJob(*processId, "Timeline\n");

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
                        emit addToJob(*processId, QString("  Transition%1 %2\n").arg( QString::number(iterationCounter - 1), transitionQTime.toString("HH:mm:ss.zzz")));
                        inTime = inTime.addMSecs(transitionTimeMSecs); //subtract half of the transitionframes
                    }
                    if (iterationCounter != audioOrVideoClipsMap.count()-1)
                    {
                        outTime = outTime.addMSecs(- transitionTimeMSecs); //subtract half of the transitionframes
                    }
                }

                s("    <entry producer=\"producer%1\" in=\"%2\" out=\"%3\"/>", producerNr, inTime.toString("HH:mm:ss.zzz"), outTime.toString("HH:mm:ss.zzz") );
                emit addToJob(*processId, QString("  Producer%1 %2 %3\n").arg( producerNr, inTime.toString("HH:mm:ss.zzz"), outTime.toString("HH:mm:ss.zzz") ));

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

            s("     <filter id=\"filter%1\" out=\"%2\">", "WM", QTime::fromMSecsSinceStartOfDay(AGlobal().frames_to_msec(maxCombinedDuration)).toString("hh:mm:ss.zzz"));
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
            s("      <entry producer=\"producer%1\" in=\"00:00:00.000\" out=\"%2\"/>", "WM", QTime::fromMSecsSinceStartOfDay(AGlobal().frames_to_msec(maxCombinedDuration)).toString("hh:mm:ss.zzz"));
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

        emit addToJob(*processId, "\nSuccesfully completed");
    }
    else if (target == "Premiere")
    {
        QString *processId = new QString();
        emit addJob(currentDirectory, fileNameWithoutExtension, "file export", processId);

        QString fileName = fileNameWithoutExtension + ".xml";
        QFile fileWrite(currentDirectory + fileName);
        fileWrite.open(QIODevice::WriteOnly);

        stream.setDevice(&fileWrite);

        emit addToJob(*processId, QString("Generating %1\n\n").arg(fileName));

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

            emit addToJob(*processId, "Timeline\n");

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

                addPremiereClipitem("WM", WMFolderName, WMFileName, 0, maxCombinedDuration, -1, -1, frameRate, mediaType, &videoFilesMap, 1, "");

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

        emit addToJob(*processId, "\nSuccesfully completed");
    }

    QMap<QString, QString> parameters;
    processManager->startProcess(parameters, [] (QWidget *parent, QString , QMap<QString, QString> , QStringList ) //command, parameters, result
    {
        AExport *exportWidget = qobject_cast<AExport *>(parent);
        exportWidget->progressBar->setValue(exportWidget->progressBar->maximum());

        if (exportWidget->processError != "")
        {
            exportWidget->progressBar->setStyleSheet("QProgressBar::chunk {background: red}");
            exportWidget->statusBar->showMessage("Export error (go to Jobs for details): " + exportWidget->processError, 10000);
        }
        else
            exportWidget->progressBar->setStyleSheet("QProgressBar::chunk {background: green}");

        exportWidget->spinnerLabel->stop();
        exportWidget->exportButton->setEnabled(true);

        emit exportWidget->exportCompleted(exportWidget->processError);

    });
}

void AExport::stopAllProcesses()
{
    processManager->stopAll();
}
