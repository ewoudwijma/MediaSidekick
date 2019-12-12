#include "aexport.h"

#include <QDebug>
#include <QSettings>
#include <QTime>

#include "aglobal.h"

#include <QMessageBox>

#include <QMovie>

#include <QTimer>

#include <qmath.h>

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

    emit addJobsEntry(folderName, fileNameTarget, "Property update", processId);
    QString attributeString = "";
    QStringList texts;
    texts << "CreateDate" << "GPSLongitude" << "GPSLatitude" << "GPSAltitude" << "GPSAltitudeRef" << "Make" << "Model" << "Director" << "Producer"  << "Publisher";
    for (int iText = 0; iText < texts.count(); iText++)
    {
        QString *value = new QString();
        emit getPropertyValue(fileNameSource, texts[iText], value);
        attributeString += " -" + texts[iText] + "=\"" + *value + "\"";
    }

    QString command = "exiftool" + attributeString + " -overwrite_original \"" + QString(folderName + fileNameTarget).replace("/", "//") + "\"";
    emit addToJob(*processId, command + "\n");
    parameters["processId"] = *processId;
    processManager->startProcess(command, parameters, [](QWidget *parent, QMap<QString, QString> parameters, QString result)
    {
        AExport *exportWidget = qobject_cast<AExport *>(parent);
        emit exportWidget->addToJob(parameters["processId"], result);
    }, [] (QWidget *parent, QString, QMap<QString, QString> parameters, QStringList )//result
    {
        AExport *exportWidget = qobject_cast<AExport *>(parent);
        emit exportWidget->addToJob(parameters["processId"], "Completed");
    });
}

void AExport::onTrimC(QString folderName, QString fileNameSource, QString fileNameTarget, QTime inTime, QTime outTime, int progressPercentage)
{
    qDebug()<<"AExport::onTrimC"<<folderName<<fileNameSource<<fileNameTarget<<inTime<<outTime<<progressPercentage<<progressBar;
    int duration = AGlobal().frames_to_msec(AGlobal().msec_to_frames(outTime.msecsSinceStartOfDay()) - AGlobal().msec_to_frames(inTime.msecsSinceStartOfDay()) + 1);

    QMap<QString, QString> parameters;

    QString *processId = new QString();
    emit addJobsEntry(folderName, fileNameTarget, "Trim", processId);
    parameters["processId"] = *processId;

    QString code = "ffmpeg -y -i \"" + QString(folderName + "//" + fileNameSource).replace("/", "//") + "\" -ss " + inTime.toString("HH:mm:ss.zzz") + " -t " + QTime::fromMSecsSinceStartOfDay(duration).toString("hh:mm:ss.zzz") + " -map_metadata 0 -vcodec copy -acodec copy \"" + QString(folderName + fileNameTarget).replace("/", "//") + "\"";

    emit addToJob(parameters["processId"], code + "\n");

    parameters["percentage"] = QString::number(progressPercentage);

    processManager->startProcess(code, parameters, nullptr,  [] (QWidget *parent, QString , QMap<QString, QString> parameters, QStringList )
    {
        AExport *exportWidget = qobject_cast<AExport *>(parent);
        emit exportWidget->addToJob(parameters["processId"], "Completed");
//        qDebug()<<"AExport::onTrim"<<exportWidget->progressBar<<(exportWidget->progressBar != nullptr);
        if (exportWidget->progressBar != nullptr)
            exportWidget->progressBar->setValue(parameters["percentage"].toInt());
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
        emit exportWidget->reloadProperties();
    });
}

void AExport::losslessVideoAndAudio(bool includingVideo)
{
    //create audio and video stream
    QStringList mediaTypeList;

    if (audioClipsMap.count() > 0)
        mediaTypeList << "A";
    if (includingVideo)
        mediaTypeList << "V";

    foreach (QString mediaType, mediaTypeList) //prepare video and audio
    {
        QString fileExtension = "";

        QFile vidlistFile(currentDirectory + "\\" + fileNameWithoutExtension + mediaType + ".txt");
        if ( vidlistFile.open(QIODevice::WriteOnly) )
        {
            QTextStream vidlistStream( &vidlistFile );

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
                    if (fileExtension == "")
                    {
                        int lastIndex = fileName.lastIndexOf(".");
                        fileExtension = fileName.mid(lastIndex);
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

                }
                iterationCounter++;
            }

            vidlistFile.close();

            progressBar->setRange(0, 100);

            if (fileExtension != "") //at least one file found
            {
                QString *processId = new QString();
                emit addJobsEntry(currentDirectory, fileNameWithoutExtension + mediaType + fileExtension, "FFMpeg lossless " + mediaType, processId);

                QString code = "ffmpeg -f concat -safe 0 -i \"" + QString(currentDirectory).replace("/", "\\") + "\\" + fileNameWithoutExtension + mediaType + ".txt\" -c copy -y \"" + QString(currentDirectory).replace("/", "\\") + "\\" + fileNameWithoutExtension + mediaType + fileExtension + "\"";

                if (mediaType == "V" && exportVideoAudioValue == 0)
                    code.replace("-c copy -y", " -an -c copy -y");

    //            if (frameRate != "")
    //                code.replace("-c copy -y", " -r " + frameRate + " -c copy -y");

                emit addToJob(*processId, code + "\n");

                QMap<QString, QString> parameters;
                parameters["processId"] = *processId;
                processManager->startProcess(code, parameters, [] (QWidget *parent, QMap<QString, QString> parameters, QString result)
                {
                    AExport *exportWidget = qobject_cast<AExport *>(parent);
                    emit exportWidget->addToJob(parameters["processId"], result);

                    if (result.contains("Could not"))
                    {
                        foreach (QString resultLine, result.split("\n"))
                        {
                            if (resultLine.contains("Could not") && !resultLine.contains("Could not find codec parameters for stream")) //apparently not an issue
                                exportWidget->processError = resultLine;
                        }
                    }

                },  [] (QWidget *parent, QString , QMap<QString, QString> parameters, QStringList )//command, result
                {
                    AExport *exportWidget = qobject_cast<AExport *>(parent);

                    exportWidget->progressBar->setValue(exportWidget->progressBar->maximum());

                    if (exportWidget->processError != "")
                        emit exportWidget->addToJob(parameters["processId"], exportWidget->processError);
                    else
                        emit exportWidget->addToJob(parameters["processId"], "Completed");

                });
            } //if fileExtension

        } //if vidlist

    } //for each

} //losslessVideoAndAudio

void AExport::encodeVideoClips()
{
    QString filterComplexString = "";
    QString filterComplexString2 = "";
    int resultDurationMSec = 0;

    progressBar->setRange(0, 100);

    int iterationCounter = 0;
    QMapIterator<int, int> clipsIterator(videoClipsMap);
    while (clipsIterator.hasNext()) //all files
    {
        clipsIterator.next();
        int row = clipsIterator.value();

        QTime inTime = QTime::fromString(timelineModel->index(row, inIndex).data().toString(),"HH:mm:ss.zzz");
        QTime outTime = QTime::fromString(timelineModel->index(row, outIndex).data().toString(),"HH:mm:ss.zzz");
        QString folderName = timelineModel->index(row, folderIndex).data().toString();
        QString fileName = timelineModel->index(row, fileIndex).data().toString();
        QString tags = timelineModel->index(row, tagIndex).data().toString();

        if (!fileName.toLower().contains(".mp3"))
        {
            if (transitionTimeFrames > 0)
            {
                if (iterationCounter != 0)
                {
                    inTime = inTime.addMSecs(AGlobal().frames_to_msec((transitionTimeFrames/2))); //subtract half of the transitionframes
                }
                if (iterationCounter != videoClipsMap.count()-1)
                {
                    outTime = outTime.addMSecs(- AGlobal().frames_to_msec(qRound(transitionTimeFrames/2.0))); //subtract half of the transitionframes
                }
            }

            int duration = AGlobal().frames_to_msec(AGlobal().msec_to_frames(outTime.msecsSinceStartOfDay()) - AGlobal().msec_to_frames(inTime.msecsSinceStartOfDay()) + 1);
            resultDurationMSec += duration;

//            qDebug()<<row<<timelineModel->index(row, inIndex).data().toString()<<timelineModel->index(row, outIndex).data().toString();

            double inSeconds = inTime.msecsSinceStartOfDay() / 1000.0;
            double outSeconds = outTime.msecsSinceStartOfDay() / 1000.0;
            int videoFileCounter = videoFilesMap[folderName + fileName].counter;

            QString newfcs = "[" + QString::number(videoFileCounter) + ":v]trim=" + QString::number(inSeconds, 'g', 6) + ":" + QString::number(outSeconds, 'g', 6) + ",setpts=PTS-STARTPTS,scale=" + videoWidth + "x" + videoHeight + ",setdar=16/9[v" + QString::number(row) + "];";

            if (tags.toLower().contains("backwards"))
                newfcs.replace("setdar=16/9", "setdar=16/9, reverse");
            //https://stackoverflow.com/questions/42257354/concat-a-video-with-itself-but-in-reverse-using-ffmpeg
            if (tags.toLower().contains("slowmotion"))
                newfcs.replace("setpts=PTS-STARTPTS", "setpts=2.0*PTS");
            if (tags.toLower().contains("fastmotion"))
                newfcs.replace("setpts=PTS-STARTPTS", "setpts=0.5*PTS");

            filterComplexString += newfcs;

            filterComplexString2 += "[v" + QString::number(row) + "]";

            if (exportVideoAudioValue > 0)
            {
                filterComplexString += "[" + QString::number(videoFileCounter) + ":a]atrim=" + QString::number(inSeconds, 'g', 6) + ":" + QString::number(outSeconds, 'g', 6) + ",asetpts=PTS-STARTPTS[a" + QString::number(row) + "];";
                filterComplexString2 += "[a" + QString::number(row) + "]";
            }

//                QString targetFileName = fileNameWithoutExtension + "-" + QString::number(row) + ".mp4";
//                onTrim(timelineModel->index(i, folderIndex).data().toString(),  timelineModel->index(i, fileIndex).data().toString(), targetFileName, inTime, outTime, 10*i/(timelineModel->rowCount())); //not -1 to avoid divby0
        }
        iterationCounter++;
    }

    QString videoFilesString = "";
    QMapIterator<QString, FileStruct> videoFilesIterator(videoFilesMap);
    while (videoFilesIterator.hasNext()) //all files
    {
        videoFilesIterator.next();
        videoFilesString = videoFilesString + " -i \"" + videoFilesIterator.value().folderName + videoFilesIterator.value().fileName + "\"";
    }

    QString code = "ffmpeg " + videoFilesString;

    if (watermarkFileName != "")
    {
        code += " -i \"" + watermarkFileName + "\"";
        filterComplexString += "[" + QString::number(videoFilesMap.count()) + ":v]scale=" + QString::number(videoWidth.toInt()/10) + "x" + QString::number(videoHeight.toInt()/10) + ",setdar=16/9[wtm];";
    }

    code += " -filter_complex \"" + filterComplexString + " " + filterComplexString2 + " concat=n=" + QString::number(videoClipsMap.count()) + ":v=1";

    if (exportVideoAudioValue > 0)
        code += ":a=1";

    code += "[out]";

    if (watermarkFileName != "")
        code += ";[out][wtm]overlay = main_w-overlay_w-10:main_h-overlay_h-10[out2]\" -map \"[out2]\"";
    else
        code += "\" -map \"[out]\"";

    if (frameRate != "")
        code += " -r " + frameRate;

    code +=  "  -y \"" + QString(currentDirectory).replace("/", "\\") + "\\" + fileNameWithoutExtension + "V.mp4\"";

//        qDebug()<<"filter_complex"<<code;

    //        -filter_complex \"[0:v]scale=640x640 [0:a] [1:v]scale=640x640 [1:a] concat=n=2:v=1:a=1 [v] [a]\" -map \"[v]\" -map \"[a]\" .\\outputreencode.mp4";

    //https://ffmpeg.org/ffmpeg-filters.html#trim

    QString *processId = new QString();

    emit addJobsEntry(currentDirectory, fileNameWithoutExtension + ".mp4", "FFMPeg Encode V", processId);
    QMap<QString, QString> parameters;
    parameters["processId"] = *processId;
    emit addToJob(parameters["processId"], code + "\n");

    parameters["currentDirectory"] = currentDirectory;
    parameters["resultDurationMSec"] = QString::number(resultDurationMSec);
    parameters["fileNameWithoutExtension"] = fileNameWithoutExtension;
    processManager->startProcess(code, parameters, [] (QWidget *parent, QMap<QString, QString> parameters, QString result)
    {
        AExport *exportWidget = qobject_cast<AExport *>(parent);

        emit exportWidget->addToJob(parameters["processId"], result);

        if (result.contains("Cannot")) //find a matching stream
        {
            foreach (QString resultLine, result.split("\n"))
            {
                if (resultLine.contains("Cannot"))
                    exportWidget->processError = resultLine;
            }
        }

        int timeIndex = result.indexOf("time=");
        if (timeIndex > 0)
        {
            QString timeString = result.mid(timeIndex + 5, 11) + "0";
            QTime time = QTime::fromString(timeString,"HH:mm:ss.zzz");
            exportWidget->progressBar->setValue(10 + 90*time.msecsSinceStartOfDay()/parameters["resultDurationMSec"].toInt());
        }

        emit exportWidget->addToJob(parameters["processId"], result);
    },  [] (QWidget *parent, QString , QMap<QString, QString> parameters, QStringList )
    {
        AExport *exportWidget = qobject_cast<AExport *>(parent);

        exportWidget->progressBar->setValue(exportWidget->progressBar->maximum());
        exportWidget->progressBar->setStyleSheet("QProgressBar::chunk {background: green}");

        QDir dir(parameters["currentDirectory"]);
        dir.setFilter( QDir::NoDotAndDotDot | QDir::Files );
        foreach( QString dirItem, dir.entryList() )
        {
            if (dirItem.contains(parameters["fileNameWithoutExtension"] + "-"))
                dir.remove( dirItem );
        }

        emit exportWidget->addToJob(parameters["processId"], "completed");
    });

} //encodeVideoClips

void AExport::muxVideoAndAudio()
{
    QString *processId = new QString();

    emit addJobsEntry(currentDirectory, fileNameWithoutExtension + ".mp4", "FFMpeg mux", processId);

    QString code = "ffmpeg -i \"" + QString(currentDirectory).replace("/", "\\") + "\\" + fileNameWithoutExtension + "V.mp4\"";

    if (audioClipsMap.count() > 0)
    {
        code += " -i \"" + QString(currentDirectory).replace("/", "\\") + "\\" + fileNameWithoutExtension + "A.mp3\"";

        if (exportVideoAudioValue == 0)
            code += " -c copy -map 0:v -map 1:a";
        else if (exportVideoAudioValue == 100)
            code += " -filter_complex \"[0][1]amix=inputs=2[a]\" -map 0:v -map \"[a]\" -c:v copy";
        else
            code += " -filter_complex \"[0]volume=" + QString::number(exportVideoAudioValue / 100.0) + "[a0];[a0][1]amix=inputs=2[a]\" -map 0:v -map \"[a]\" -c:v copy";
    }
    else
        code += " -filter:a \"volume=" + QString::number(exportVideoAudioValue / 100.0) + "\"";

    code += " -y \"" + QString(currentDirectory).replace("/", "\\") + "\\" + fileNameWithoutExtension + ".mp4\"";

    emit addToJob(*processId, code + "\n");

    QMap<QString, QString> parameters;
    parameters["processId"] = *processId;

    processManager->startProcess(code, parameters, [] (QWidget *parent, QMap<QString, QString> parameters, QString result)
    {
        AExport *exportWidget = qobject_cast<AExport *>(parent);
        emit exportWidget->addToJob(parameters["processId"], result);

        if (result.contains("Could not"))
        {
            foreach (QString resultLine, result.split("\n"))
            {
                if (resultLine.contains("Could not")  && !resultLine.contains("Could not find codec parameters for stream")) //apparently not an issue
                    exportWidget->processError = resultLine;
            }
        }

    },  [] (QWidget *parent, QString , QMap<QString, QString> parameters, QStringList )//command, result
    {
        AExport *exportWidget = qobject_cast<AExport *>(parent);

        exportWidget->progressBar->setValue(exportWidget->progressBar->maximum());
        exportWidget->progressBar->setStyleSheet("QProgressBar::chunk {background: green}");

        if (exportWidget->processError != "")
            emit exportWidget->addToJob(parameters["processId"], exportWidget->processError);
        else
            emit exportWidget->addToJob(parameters["processId"], "Completed");

    });

} //muxVideoAndAudio

void AExport::removeTemporaryFiles()
{
    //remove temp files
    QMap<QString, QString> parameters;
    parameters["currentDirectory"] = currentDirectory;
    processManager->startProcess(parameters, [] (QWidget *parent, QString , QMap<QString, QString> parameters, QStringList ) //, command, result
    {
        AExport *exportWidget = qobject_cast<AExport *>(parent);

        QDir dir(parameters["currentDirectory"]);
        dir.setFilter( QDir::NoDotAndDotDot | QDir::Files );
        foreach( QString dirItem, dir.entryList() )
        {
            if (dirItem.contains(exportWidget->fileNameWithoutExtension + "V") || dirItem.contains(exportWidget->fileNameWithoutExtension + "A"))
                dir.remove( dirItem );
        }
    });

}

void AExport::addPremiereTransitionItem(int startFrames, int endFrames, QString frameRate, QString mediaType)
{
    s("     <transitionitem>");
    s("      <start>%1</start>", QString::number(startFrames));
    s("      <end>%1</end>", QString::number(endFrames));
    s("      <alignment>start-black</alignment>");
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

}

void AExport::addPremiereClipitem(QString clipId, QString folderName, QString fileName, int startFrames, int endFrames, int inFrames, int outFrames, QString frameRate, QString mediaType, QMap<QString, FileStruct> *filesMap)
{
    s("     <clipitem>");
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
            s("               <ntsc>TRUE</ntsc>");
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
//                                s("         <channelcount>2</channelcount>");
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
            QString *widthPointer = new QString();
            emit getPropertyValue(fileName, "ImageWidth", widthPointer);
            QString *heightPointer = new QString();
            emit getPropertyValue(fileName, "ImageHeight", heightPointer);

            if (*heightPointer != videoHeight || *widthPointer != videoWidth)
            {
                double heightRatio = 100.0 * videoHeight.toInt() / (*heightPointer).toInt();
                double widthRatio = 100.0 * videoWidth.toInt() / (*widthPointer).toInt();
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
            qDebug()<<""<<myImage.height()<<videoHeight<<QString::number(videoHeight.toDouble() / myImage.height() * 10);

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
            s("                      <horiz>1</horiz>");
            s("                      <vert>1</vert>");
            s("                  </value>");
            s("              </parameter>");
            s("              <parameter authoringApp=\"PremierePro\">");
            s("                  <parameterid>centerOffset</parameterid>");
            s("                  <name>Anchor Point</name>");
            s("                  <value>");
            s("                      <horiz>0.9</horiz>");
            s("                      <vert>0.5</vert>");
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
//                            s("       <trackindex>%1</trackindex>", QString::number(trackNr));
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
}

void AExport::exportClips(QAbstractItemModel *ptimelineModel, QString ptarget, QString ptargetSize, QString pframeRate, int ptransitionTimeFrames, QProgressBar *p_progressBar, QSlider *exportVideoAudioSlider, QLabel *pSpinnerLabel, QString pwatermarkFileName, QPushButton *pExportButton, QComboBox *clipsFramerateComboBox, QComboBox *clipsSizeComboBox, QStatusBar *pstatusBar)
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

//    int maxOriginalDuration = qMax(videoOriginalDuration, audioOriginalDuration);
    int maxCombinedDuration = qMax(videoOriginalDuration - ptransitionTimeFrames * (videoCountNrOfClips-1), audioOriginalDuration - ptransitionTimeFrames * (audioCountNrOfClips-1));

    currentDirectory = QSettings().value("LastFolder").toString();

    transitionTimeFrames = ptransitionTimeFrames;
    int transitionTimeMSecs = transitionTimeFrames * AGlobal().frames_to_msec(1);
    QTime transitionQTime = QTime::fromMSecsSinceStartOfDay(transitionTimeMSecs);

    progressBar = p_progressBar;
    progressBar->setRange(0, timelineModel->rowCount());
    progressBar->setStyleSheet("QProgressBar::chunk {background: " + palette().highlight().color().name() + "}");

    statusBar = pstatusBar;

    exportButton = pExportButton;
    exportButton->setEnabled(false);

    watermarkFileName = pwatermarkFileName;
    exportVideoAudioValue = exportVideoAudioSlider->value();

    if (pframeRate == "Source")
        frameRate = clipsFramerateComboBox->currentText();
    else
        frameRate = pframeRate;

    target = ptarget;

    QString targetSize;
    if (ptargetSize.contains("240p")) //3:4
    {
        targetSize = "240p";
        videoWidth = "320"; //352 of 427?
        videoHeight = "240";
    }
    else if (ptargetSize.contains("360p")) //3:4
    {
        targetSize = "360p";
        videoWidth = "480"; //640
        videoHeight = "360";
    }
    else if (ptargetSize.contains("480p"))
    {
        targetSize = "480p";
        videoWidth = "640"; //853 or 858
        videoHeight = "480";
    }
    else if (ptargetSize.contains("720p"))//9:16
    {
        targetSize = "720p";
        videoWidth = "1280";
        videoHeight = "720";
    }
    else if (ptargetSize.contains("Source")) //9:16
    {
        int indexOf = clipsSizeComboBox->currentText().indexOf(" x ");
        videoWidth = clipsSizeComboBox->currentText().mid(0,indexOf);
        videoHeight = clipsSizeComboBox->currentText().mid(indexOf + 3);
        targetSize = videoHeight;
    }
    else if (ptargetSize.contains("1K")) //9:16
    {
        targetSize = "1K";
        videoWidth = "1920";
        videoHeight = "1080";
    }
    else if (ptargetSize.contains("2.7K"))//9:16
    {
        targetSize = "2.7K";
        videoWidth = "2880";
        videoHeight = "1620";
    }
    else if (ptargetSize.contains("4K"))//9:16
    {
        targetSize = "4K";
        videoWidth = "3840";
        videoHeight = "2160";
    }
    else if (ptargetSize.contains("8K"))//9:16
    {
        targetSize = "8K";
        videoWidth = "7680";
        videoHeight = "4320";
    }
//    qDebug()<<"targetSize and framerate"<<ptargetSize<<targetSize<<videoWidth<<videoHeight<<pframeRate<<frameRate;

    if (videoWidth == "" || videoHeight == "" || frameRate == "")
    {
        QMessageBox::information(this, "Export", QString("One of the following values is not known. Videowidth %1 VideoHeight %2 framerate %3").arg(videoWidth, videoHeight, frameRate));
        return;
    }

    fileNameWithoutExtension = target;
    if (target != "Lossless")
    {
        fileNameWithoutExtension += targetSize;
        fileNameWithoutExtension += "@" + frameRate;
    }

    bool includingSRT = false;
    if (includingSRT)
    {
        QFile srtOutputFile(currentDirectory + "\\" + fileNameWithoutExtension + ".srt");
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

//    filesMap.clear();
    audioFilesMap.clear();
    videoFilesMap.clear();
    for (int i=0; i<timelineModel->rowCount();i++)
    {
        QString folderName = timelineModel->index(i,folderIndex).data().toString();
        QString fileName = timelineModel->index(i,fileIndex).data().toString();
//        filesMap[folderName + fileName].folderName = folderName;
//        filesMap[folderName + fileName].fileName = fileName;
//        filesMap[folderName + fileName].counter = filesMap.count() - 1;

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
    spinnerLabel->setMinimumWidth(spinnerLabel->height()*2);
    QMovie *movie = new QMovie(":/Spinner.gif");
    movie->setScaledSize(QSize(spinnerLabel->height()*2,spinnerLabel->height()*2));
    movie->start();

    spinnerLabel->setMovie(movie);

    //superview
    //https://intofpv.com/t-using-free-command-line-sorcery-to-fake-superview
    //https://github.com/Niek/superview

    if (target == "Lossless") //Lossless FFMpeg
    {
        losslessVideoAndAudio(true);

        muxVideoAndAudio();

        onPropertyUpdate(currentDirectory, videoFilesMap.first().fileName, fileNameWithoutExtension + ".mp4");

        removeTemporaryFiles();

        onReloadAll(includingSRT);

    }
    else if (target == "Encode") //FFMpeg encode
    {
        encodeVideoClips();

        losslessVideoAndAudio(false);

        muxVideoAndAudio();

        onPropertyUpdate(currentDirectory, videoFilesMap.first().fileName, fileNameWithoutExtension + ".mp4");

        removeTemporaryFiles();

        onReloadAll(includingSRT);
    }
    else if (target == "Shotcut")
    {
        QString *processId = new QString();
        emit addJobsEntry(currentDirectory, fileNameWithoutExtension, "file export", processId);

        QString fileName = fileNameWithoutExtension + ".mlt";
        QFile fileWrite(currentDirectory + "//" + fileName);
        fileWrite.open(QIODevice::WriteOnly);

//        QTextStream stream(&fileWrite);

        stream.setDevice(&fileWrite);

        emit addToJob(*processId, QString("Generating %1\n\n").arg(fileName));

        s("<?xml version=\"1.0\" encoding=\"utf-8\"?>");
        s("<mlt LC_NUMERIC=\"C\" version=\"6.17.0\" title=\"Shotcut v19.10.20 by ACVC v%1\" producer=\"main_bin\">", qApp->applicationVersion());
        s("  <profile description=\"automatic\" width=\"%1\" height=\"%2\" progressive=\"1\" sample_aspect_num=\"1\" sample_aspect_den=\"1\" display_aspect_num=\"%1\" display_aspect_den=\"%2\" frame_rate_num=\"%3\" frame_rate_den=\"1\"/>", videoWidth, videoHeight, frameRate);

        emit addToJob(*processId, "Producers\n");

//        QMapIterator<QString, FileStruct> videoFilesIterator(audioFilesMap);
//        while (videoFilesIterator.hasNext()) //all files
//        {
//            videoFilesIterator.next();

//            QString *durationPointer = new QString();
//            emit getPropertyValue(videoFilesIterator.value().fileName, "Duration", durationPointer); //format <30s: [ss.mm s] >30s: [h.mm:ss]
//            *durationPointer = QString(*durationPointer).replace(" (approx)", "");
//            QTime durationTime = QTime::fromString(*durationPointer,"h:mm:ss");
//            if (durationTime == QTime())
//            {
//                QString durationString = *durationPointer;
//                durationString = durationString.left(durationString.length() - 2); //remove " -s"
//                durationTime = QTime::fromMSecsSinceStartOfDay(int(durationString.toDouble() * 1000.0));
//            }

//            if (durationTime.msecsSinceStartOfDay() == 0)
//                durationTime = QTime::fromMSecsSinceStartOfDay(24 * 60 * 60 * 1000 - 1);


//            qDebug()<<"Duration"<<*durationPointer<<durationTime;

//            s("  <producer id=\"producer%1\" title=\"Anonymous Submission\" in=\"00:00:00.000\" out=\"%2\">", QString::number(videoFilesIterator.value().counter), durationTime.toString("hh:mm:ss.zzz"));
//            s("    <property name=\"length\">%1</property>", durationTime.toString("hh:mm:ss.zzz"));
//            s("    <property name=\"resource\">%1</property>", videoFilesIterator.key());
//            s("  </producer>");

//            emit addToJob(*processId, QString("  Producer%1 %2 %3\n").arg( QString::number(videoFilesIterator.value().counter), durationTime.toString("hh:mm:ss.zzz"), videoFilesIterator.key()));

//        }

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

            QString *durationPointer = new QString();
            emit getPropertyValue(fileName, "Duration", durationPointer); //format <30s: [ss.mm s] >30s: [h.mm:ss]
            *durationPointer = QString(*durationPointer).replace(" (approx)", "");
            QTime durationTime = QTime::fromString(*durationPointer,"h:mm:ss");
            if (durationTime == QTime())
            {
                QString durationString = *durationPointer;
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
//            int fileCounter = filesMap[timelineModel->index(row, folderIndex).data().toString() + timelineModel->index(row, fileIndex).data().toString()].counter;

            s("    <entry producer=\"producer%1\" in=\"%2\" out=\"%3\"/>"
              , QString::number(row)//QString::number(videoFileCounter)
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
//            QMap<QString, FileStruct> audioOrVideoFilesMap;

            if (mediaType == "A")
            {
//                audioOrVideoFilesMap = audioFilesMap;
                audioOrVideoClipsMap = audioClipsMap;
            }
            else
            {
//                audioOrVideoFilesMap = videoFilesMap;
                audioOrVideoClipsMap = videoClipsMap;
            }

            emit addToJob(*processId, "Transitions\n");
    //        int producerCounter = 0;
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
    //                    QString producerNr = QString::number(audioOrVideoFilesMap[timelineModel->index(row, folderIndex).data().toString() + timelineModel->index(row, fileIndex).data().toString()].counter);
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
            QMap<QString, FileStruct> audioOrVideoFilesMap;

            if (mediaType == "A")
            {
                audioOrVideoFilesMap = audioFilesMap;
                audioOrVideoClipsMap = audioClipsMap;
                s("    <property name=\"shotcut:audio\">1</property>");
                s("    <property name=\"shotcut:name\">A1</property>");
            }
            else
            {
                audioOrVideoFilesMap = videoFilesMap;
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

//                QString producerNr = QString::number(audioOrVideoFilesMap[timelineModel->index(row, folderIndex).data().toString() + timelineModel->index(row, fileIndex).data().toString()].counter);
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
        emit addJobsEntry(currentDirectory, fileNameWithoutExtension, "file export", processId);


        QString fileName = fileNameWithoutExtension + ".xml";
        QFile fileWrite(currentDirectory + "//" + fileName);
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
        if (exportVideoAudioValue > 0)
            mediaTypeList << "audio";

        foreach (QString mediaType, mediaTypeList) //for video and audio of video stream
        {
            s("   <%1>", mediaType);
            s("    <format>");
            s("     <samplecharacteristics>");
            if (mediaType == "video")
            {
                s("      <width>%1</width>", videoWidth);
                s("      <height>%1</height>", videoHeight);
            }
            else //audio
            {
                s("      <depth>16</depth>");
//                s("      <samplerate>44100</samplerate>");
            }
            s("     </samplecharacteristics>");
            s("    </format>");

            emit addToJob(*processId, "Timeline\n");

            for (int trackNr=0; trackNr<2; trackNr++) //even and odd tracks
            {
                s("    <track>");

                int totalFrames = 0;

                int iterationCounter = 0;

                QMapIterator<int, int> clipsIterator(videoClipsMap);
                while (clipsIterator.hasNext()) //all videos
                {
                    clipsIterator.next();
                    int row = clipsIterator.value();

                    QString folderName = timelineModel->index(row, folderIndex).data().toString();
                    QString fileName = timelineModel->index(row, fileIndex).data().toString();
                    QTime inTime = QTime::fromString(timelineModel->index(row, inIndex).data().toString(),"HH:mm:ss.zzz");
                    QTime outTime = QTime::fromString(timelineModel->index(row, outIndex).data().toString(),"HH:mm:ss.zzz");

                    int inFrames = qRound(inTime.msecsSinceStartOfDay() * frameRate.toInt() / 1000.0);
                    int outFrames = qRound(outTime.msecsSinceStartOfDay() * frameRate.toInt() / 1000.0);

                    QString *frameRatePointer = new QString();
                    emit getPropertyValue(fileName, "VideoFrameRate", frameRatePointer);

                    if (trackNr == 1 && iterationCounter%2 == 1 && transitionTimeFrames > 0) //track 2 and not first clip (never is)
                    {
                        addPremiereTransitionItem(totalFrames, totalFrames + transitionTimeFrames, *frameRatePointer, mediaType);
                    }

                    int deltaFrames = outFrames - inFrames + 1 - transitionTimeFrames;
                    if (iterationCounter%2 == trackNr) //even or odd tracks
                    {
                        addPremiereClipitem(QString::number(row), folderName, fileName, totalFrames, totalFrames + deltaFrames + transitionTimeFrames, inFrames, outFrames + 1, *frameRatePointer, mediaType, &videoFilesMap);
                    }
                    totalFrames += deltaFrames;

                    if (trackNr == 1 && iterationCounter%2 == 1 && iterationCounter != videoClipsMap.count()-1 && transitionTimeFrames > 0) //track 2 and not last clip
                    {
                        addPremiereTransitionItem(totalFrames, totalFrames + transitionTimeFrames, *frameRatePointer, mediaType);
                    }

                    emit addToJob(*processId, QString("  Producer%1 %2 %3\n"));

                    iterationCounter++;
                } // clipsiterator

                s("    </track>");

            } //for (int trackNr=0; trackNr<2; trackNr++) //even and odd tracks

            if (mediaType == "audio" && audioClipsMap.count() > 0) //add audio track if exists
            {
                for (int trackNr=0; trackNr<2; trackNr++) //even and odd tracks
                {
                    s("    <track>");

                    int totalFrames = 0;

                    int iterationCounter = 0;

                    QMapIterator<int, int> clipsIterator(audioClipsMap);
                    while (clipsIterator.hasNext()) //all audios
                    {
                        clipsIterator.next();
                        int row = clipsIterator.value();

                        QString folderName = timelineModel->index(row, folderIndex).data().toString();
                        QString fileName = timelineModel->index(row, fileIndex).data().toString();
                        QTime inTime = QTime::fromString(timelineModel->index(row, inIndex).data().toString(),"HH:mm:ss.zzz");
                        QTime outTime = QTime::fromString(timelineModel->index(row, outIndex).data().toString(),"HH:mm:ss.zzz");

                        int inFrames = qRound(inTime.msecsSinceStartOfDay() * frameRate.toInt() / 1000.0);
                        int outFrames = qRound(outTime.msecsSinceStartOfDay() * frameRate.toInt() / 1000.0);

//                        QString *frameRatePointer = new QString();
//                        emit getPropertyValue(fileName, "VideoFrameRate", frameRatePointer);

                        if (iterationCounter%2 == trackNr) //even or odd tracks
                        {
                            if (audioClipsMap.first() == row) //fade in
                            {
                                addPremiereTransitionItem(0, frameRate.toInt(), frameRate, mediaType);
                            }
                        }

                        if (trackNr == 1 && iterationCounter%2 == 1 && transitionTimeFrames > 0) //track 2 and not first clip (never is)
                        {
                            addPremiereTransitionItem(totalFrames, totalFrames + transitionTimeFrames, frameRate, mediaType);
                        }

                        int deltaFrames = outFrames - inFrames + 1 - transitionTimeFrames;
                        if (iterationCounter%2 == trackNr) //even or odd tracks
                        {
                            addPremiereClipitem(QString::number(row), folderName, fileName, totalFrames, totalFrames + deltaFrames + transitionTimeFrames, inFrames, outFrames + 1, frameRate, mediaType, &audioFilesMap);
                        }
                        totalFrames += deltaFrames;

                        if (trackNr == 1 && iterationCounter%2 == 1 && iterationCounter != audioClipsMap.count()-1 && transitionTimeFrames > 0) //track 2 and not last clip
                        {
                            addPremiereTransitionItem(totalFrames, totalFrames + transitionTimeFrames, frameRate, mediaType);
                        }

                        if (iterationCounter%2 == trackNr) //even or odd tracks
                        {
                            if (audioClipsMap.last() == row) //fade in
                            {
                                int audioFrames = audioOriginalDuration - ptransitionTimeFrames * (audioCountNrOfClips-1);

                                addPremiereTransitionItem(audioFrames - frameRate.toInt(), audioFrames, frameRate, mediaType);
                            }
                        }

                        emit addToJob(*processId, QString("  Producer%1 %2 %3\n"));

                        iterationCounter++;
                    } // clipsiterator

                    s("    </track>");

                } //for (int trackNr=0; trackNr<2; trackNr++) //even and odd tracks
            } //if audio (audio track)

            if (mediaType == "video" && watermarkFileName != "") //add audio track if exists
            {
                s("    <track>");

                int indexOf = watermarkFileName.lastIndexOf("/");
                QString WMFolderName = watermarkFileName.left(indexOf + 1);
                QString WMFileName = watermarkFileName.mid(indexOf + 1);

                addPremiereClipitem("WM", WMFolderName, WMFileName, 0, maxCombinedDuration, -1, -1, frameRate, mediaType, &videoFilesMap);

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

        if (exportWidget->spinnerLabel->movie() != nullptr)
        {
            exportWidget->spinnerLabel->movie()->stop();
            exportWidget->spinnerLabel->clear();
        }
        exportWidget->exportButton->setEnabled(true);

        QTimer::singleShot(3000, exportWidget, [exportWidget]()->void
        {
                               exportWidget->progressBar->setValue(0);
        });
    });
}

