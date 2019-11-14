#include "fgenerate.h"

#include <QDebug>
#include <QSettings>
#include <QTime>

#include "fglobal.h"

#include <QMessageBox>

#include <QMovie>

FGenerate::FGenerate(QWidget *parent) : QWidget(parent)
{
    processManager = new FProcessManager(this);
}

void FGenerate::s(QString inputString, QString arg1, QString arg2, QString arg3)
{
    if (arg3 != "")
        stream<<inputString.arg(arg1, arg2, arg3)<<endl;
    else if (arg2 != "")
        stream<<inputString.arg(arg1, arg2)<<endl;
    else if (arg1 != "")
        stream<<inputString.arg(arg1)<<endl;
    else
        stream<<inputString<<endl;
}

void FGenerate::onPropertyUpdate(QString folderName, QString fileNameSource, QString fileNameTarget)
{
    QString *processId = new QString();
    QMap<QString, QString> parameters;

    emit addLogEntry(folderName, fileNameTarget, "Property update", processId);
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
    emit addLogToEntry(*processId, command + "\n");
    parameters["processId"] = *processId;
    processManager->startProcess(command, parameters, [](QWidget *parent, QMap<QString, QString> parameters, QString result)
    {
        FGenerate *generateWidget = qobject_cast<FGenerate *>(parent);
        emit generateWidget->addLogToEntry(parameters["processId"], result);
    }, [] (QWidget *parent, QString, QMap<QString, QString> parameters, QStringList result)
    {
        FGenerate *generateWidget = qobject_cast<FGenerate *>(parent);
        emit generateWidget->addLogToEntry(parameters["processId"], "Completed");
    });
}

void FGenerate::onTrim(QString folderName, QString fileNameSource, QString fileNameTarget, QTime inTime, QTime outTime, int progressPercentage)
{
    qDebug()<<"FGenerate::onTrim"<<folderName<<fileNameSource<<fileNameTarget<<inTime<<outTime<<progressPercentage<<progressBar;
    int duration = FGlobal().frames_to_msec(FGlobal().msec_to_frames(outTime.msecsSinceStartOfDay()) - FGlobal().msec_to_frames(inTime.msecsSinceStartOfDay()) + 1);

    QMap<QString, QString> parameters;

    QString *processId = new QString();
    emit addLogEntry(folderName, fileNameTarget, "Trim", processId);
    parameters["processId"] = *processId;

    QString code = "ffmpeg -y -i \"" + QString(folderName + "//" + fileNameSource).replace("/", "//") + "\" -ss " + inTime.toString("HH:mm:ss.zzz") + " -t " + QTime::fromMSecsSinceStartOfDay(duration).toString("hh:mm:ss.zzz") + " -map_metadata 0 -vcodec copy -acodec copy \"" + QString(folderName + fileNameTarget).replace("/", "//") + "\"";
//    code = "ffmpeg -y -i \"" + QString(selectedFolderName + fileName).replace("/", "//") + "\" -ss " + inTime.toString("HH:mm:ss.zzz") + " -t " + QTime::fromMSecsSinceStartOfDay(duration).toString("hh:mm:ss.zzz") + " -map_metadata 0 -vcodec copy \"" + QString(selectedFolderName + targetFileName).replace("/", "//") + "\"";

    emit addLogToEntry(parameters["processId"], code + "\n");

    parameters["percentage"] = QString::number(progressPercentage);

    processManager->startProcess(code, parameters, nullptr,  [] (QWidget *parent, QString , QMap<QString, QString> parameters, QStringList )
    {
        FGenerate *generateWidget = qobject_cast<FGenerate *>(parent);
        emit generateWidget->addLogToEntry(parameters["processId"], "Completed");
        if (generateWidget->progressBar != nullptr)
            generateWidget->progressBar->setValue(parameters["percentage"].toInt());
    });

}

void FGenerate::onReloadAll(bool includingSRT)
{
    QMap<QString, QString> parameters;
    parameters["includingSRT"] = QString::number(includingSRT);
    processManager->startProcess(parameters, [] (QWidget *parent, QString command, QMap<QString, QString> parameters, QStringList result)
    {
        FGenerate *generateWidget = qobject_cast<FGenerate *>(parent);
        if (parameters["includingSRT"].toInt())
            emit generateWidget->reloadEdits();
        emit generateWidget->reloadProperties();
    });

}

typedef struct {
    QString folderName;
    QString fileName;
    int counter;
} FileStruct;

void FGenerate::generate(QAbstractItemModel *timelineModel, QString target, QString size, QString pframeRate, int transitionTimeFrames, QProgressBar *p_progressBar, bool includingSRT, bool includeAudio, QLabel *pSpinnerLabel)
{
    if (timelineModel->rowCount()==0)
    {
        QMessageBox::information(this, "Generate", "No edits");
        return;
    }
    int resultDurationMSec = 0;
    QString currentDirectory = QSettings().value("LastFolder").toString();

    int transitionTimeMSecs = transitionTimeFrames * FGlobal().frames_to_msec(1);
    QTime transitionTime = QTime::fromMSecsSinceStartOfDay(transitionTimeMSecs);

    progressBar = p_progressBar;
    progressBar->setRange(0, timelineModel->rowCount());
    progressBar->setStyleSheet("QProgressBar::chunk {background: " + palette().highlight().color().name() + "}");

    QString fileNameWithoutExtension = target;
    if (target != "Lossless")
    {
        fileNameWithoutExtension += size;
        fileNameWithoutExtension += "@" + pframeRate;
    }

    if (includingSRT)
    {
        QFile srtOutputFile(currentDirectory + "\\" + fileNameWithoutExtension + ".srt");
        if (srtOutputFile.open(QIODevice::WriteOnly) )
        {
            QTextStream srtStream( &srtOutputFile );
            int totalDuration = 0;
            for (int row=0; row<timelineModel->rowCount();row++)
            {
                QString srtContentString = "";

                QTime inTime = QTime::fromString(timelineModel->index(row, inIndex).data().toString(),"HH:mm:ss.zzz");
                QTime outTime = QTime::fromString(timelineModel->index(row, outIndex).data().toString(),"HH:mm:ss.zzz");
                FStarRating starRating = qvariant_cast<FStarRating>(timelineModel->index(row, ratingIndex).data());

                srtContentString += "<o>" + QString::number(timelineModel->index(row, orderAfterMovingIndex).data().toInt()+2) + "</o>"; //+1 for file trim, +2 for generate
                srtContentString += "<r>" + QString::number(starRating.starCount()) + "</r>";
                srtContentString += "<a>" + timelineModel->index(row, alikeIndex).data().toString() + "</a>";
                srtContentString += "<h>" + timelineModel->index(row, hintIndex).data().toString() + "</h>";
                srtContentString += "<t>" + timelineModel->index(row, tagIndex).data().toString() + "</t>";

                int duration = FGlobal().frames_to_msec(FGlobal().msec_to_frames(outTime.msecsSinceStartOfDay()) - FGlobal().msec_to_frames(inTime.msecsSinceStartOfDay()) + 1);

                srtStream << row+1 << endl;
                srtStream << QTime::fromMSecsSinceStartOfDay(totalDuration).toString("HH:mm:ss.zzz") << " --> " << QTime::fromMSecsSinceStartOfDay(totalDuration + duration - FGlobal().frames_to_msec(1)).toString("HH:mm:ss.zzz") << endl;
                srtStream << srtContentString << endl;//timelineModel->index(i, tagIndex).data().toString()
                srtStream << endl;

                totalDuration += duration;
            }
            srtOutputFile.close();
        }
    }

    QMap<QString, FileStruct> filesMap;
    for (int i=0; i<timelineModel->rowCount();i++)
    {
        filesMap[timelineModel->index(i,folderIndex).data().toString() + timelineModel->index(i,fileIndex).data().toString()].folderName = timelineModel->index(i,folderIndex).data().toString();
        filesMap[timelineModel->index(i,folderIndex).data().toString() + timelineModel->index(i,fileIndex).data().toString()].fileName = timelineModel->index(i,fileIndex).data().toString();
    }

    int fileCounter = 0;
    QMapIterator<QString, FileStruct> filesIterator(filesMap);
    while (filesIterator.hasNext()) //all files
    {
        filesIterator.next();
        filesMap[filesIterator.key()].counter = fileCounter;
        fileCounter++;
    }

    QString width;
    QString height;
    if (size == "240p") //3:4
    {
        width = "320"; //352 of 427?
        height = "240";
    }
    else if (size == "360p") //3:4
    {
        width = "480"; //640
        height = "360";
    }
    else if (size == "480p")
    {
        width = "640"; //853 or 858
        height = "480";
    }
    else if (size == "720p")//9:16
    {
        width = "1280";
        height = "720";
    }
    else if (size == "1K") //9:16
    {
        width = "1920";
        height = "1080";
    }
    else if (size == "2.7K")//9:16
    {
        width = "2880";
        height = "1620";
    }
    else if (size == "4K")//9:16
    {
        width = "3840";
        height = "2160";
    }
    else if (size == "8K")//9:16
    {
        width = "7680";
        height = "4320";
    }

    spinnerLabel = pSpinnerLabel;
    QMovie *movie = new QMovie(":/spinner.gif");
    spinnerLabel->setMovie(movie);
    spinnerLabel->show();
    movie->start();

    if (target == "Lossless") //Lossless FFMpeg
    {
        qDebug()<<"fileNameWithoutExtension"<<fileNameWithoutExtension;
        QFile vidlistFile(currentDirectory + "\\" + fileNameWithoutExtension + ".txt");
        if ( vidlistFile.open(QIODevice::WriteOnly) )
        {
            QTextStream vidlistStream( &vidlistFile );

            qDebug()<<"timelineModel->rowCount()"<<timelineModel->rowCount();
            for (int row=0; row<timelineModel->rowCount();row++)
            {
                vidlistStream << "file '" << timelineModel->index(row, folderIndex).data().toString() + timelineModel->index(row, fileIndex).data().toString() << "'" << endl;
                QTime inTime = QTime::fromString(timelineModel->index(row, inIndex).data().toString(),"HH:mm:ss.zzz");
                QTime outTime = QTime::fromString(timelineModel->index(row, outIndex).data().toString(),"HH:mm:ss.zzz");

                if (transitionTimeMSecs > 0)
                {
                    if (row != 0)
                    {
                        inTime = inTime.addMSecs(FGlobal().frames_to_msec((transitionTimeFrames/2))); //subtract half of the transitionframes
                    }
                    if (row != timelineModel->rowCount()-1)
                    {
                        outTime = outTime.addMSecs(- FGlobal().frames_to_msec(qRound(transitionTimeFrames/2.0))); //subtract half of the transitionframes
                    }
                }
                qDebug()<<"gen"<<row<<transitionTimeMSecs<<transitionTimeFrames/2<<qRound(transitionTimeFrames/2.0)<<FGlobal().frames_to_msec((transitionTimeFrames/2))<<inTime<<outTime;

                vidlistStream << "inpoint " <<  QString::number(inTime.msecsSinceStartOfDay() / 1000.0, 'g', 6) << endl;
                vidlistStream << "outpoint " << QString::number((outTime.msecsSinceStartOfDay()) / 1000.0, 'g', 6) << endl;
                //      qDebug()<< videoUrl << srtItemModel->index(i,inIndex).data().toString() << " --> " << srtItemModel->index(i,outIndex).data().toString() << srtItemModel->index(i,tagIndex).data().toString();

            }

            vidlistFile.close();

            progressBar->setRange(0, 100);

            QString *processId = new QString();

            int lastIndex = filesMap.first().fileName.lastIndexOf(".");
            QString fileExtension = filesMap.first().fileName.mid(lastIndex);

            emit addLogEntry(currentDirectory, fileNameWithoutExtension + fileExtension, "FFMpeg lossless", processId);

            QString code = "ffmpeg -f concat -safe 0 -i \"" + QString(currentDirectory).replace("/", "\\") + "\\" + fileNameWithoutExtension + ".txt\" -c copy -y \"" + QString(currentDirectory).replace("/", "\\") + "\\" + fileNameWithoutExtension + fileExtension + "\"";

            if (!includeAudio)
                code.replace("-c copy -y", " -an -c copy -y");

//            if (pframeRate != "")
//                code.replace("-c copy -y", " -r " + pframeRate + " -c copy -y");

            emit addLogToEntry(*processId, code + "\n");

            QMap<QString, QString> parameters;
            parameters["processId"] = *processId;

            processManager->startProcess(code, parameters, [] (QWidget *parent, QMap<QString, QString> parameters, QString result)
            {
                FGenerate *generateWidget = qobject_cast<FGenerate *>(parent);
                emit generateWidget->addLogToEntry(parameters["processId"], result);

                if (result.contains("Could not"))
                {
                    foreach (QString resultLine, result.split("\n"))
                    {
                        if (resultLine.contains("Could not"))
                        {
                            generateWidget->processError = resultLine;
                            qDebug()<<"error"<<generateWidget->processError;
                        }
                    }
                }

            },  [] (QWidget *parent, QString command, QMap<QString, QString> parameters, QStringList result)
            {
                FGenerate *generateWidget = qobject_cast<FGenerate *>(parent);

                generateWidget->progressBar->setValue(generateWidget->progressBar->maximum());
                generateWidget->progressBar->setStyleSheet("QProgressBar::chunk {background: green}");

                qDebug()<<"error"<<parameters["error"];
                if (generateWidget->processError != "")
                    emit generateWidget->addLogToEntry(parameters["processId"], generateWidget->processError);
                else
                    emit generateWidget->addLogToEntry(parameters["processId"], "Completed");

            });

            onPropertyUpdate(currentDirectory, filesMap.first().fileName, fileNameWithoutExtension + fileExtension);

            onReloadAll(includingSRT);

//            if (vidlistFile.exists())
//               vidlistFile.remove();

            //adding audio (tbd)
            //ffmpeg -i video_input.mp4 -i audio_input.mp3 -c copy -map 0:v -map 1:a outputaudio.mp4

            //superview
            //https://intofpv.com/t-using-free-command-line-sorcery-to-fake-superview
            //https://github.com/Niek/superview
        }
    }
    else if (target == "Encode") //FFMpeg encode
    {
        QString filesString = "";
        QString filterComplexString = "";
        QString filterComplexString2 = "";

        progressBar->setRange(0, 100);

        QMapIterator<QString, FileStruct> filesIterator(filesMap);
        while (filesIterator.hasNext()) //all files
        {
            filesIterator.next();
            filesString = filesString + " -i \"" + filesIterator.value().folderName + filesIterator.value().fileName + "\"";
        }

        for (int row=0; row<timelineModel->rowCount();row++)
        {
            {
                QTime inTime = QTime::fromString(timelineModel->index(row, inIndex).data().toString(),"HH:mm:ss.zzz");
                QTime outTime = QTime::fromString(timelineModel->index(row, outIndex).data().toString(),"HH:mm:ss.zzz");
                QString folderName = timelineModel->index(row, folderIndex).data().toString();
                QString fileName = timelineModel->index(row, fileIndex).data().toString();

                if (transitionTimeMSecs > 0)
                {
                    if (row != 0)
                    {
                        inTime = inTime.addMSecs(FGlobal().frames_to_msec((transitionTimeFrames/2))); //subtract half of the transitionframes
                    }
                    if (row != timelineModel->rowCount()-1)
                    {
                        outTime = outTime.addMSecs(- FGlobal().frames_to_msec(qRound(transitionTimeFrames/2.0))); //subtract half of the transitionframes
                    }
                }

                int duration = FGlobal().frames_to_msec(FGlobal().msec_to_frames(outTime.msecsSinceStartOfDay()) - FGlobal().msec_to_frames(inTime.msecsSinceStartOfDay()) + 1);
                resultDurationMSec += duration;

                qDebug()<<row<<timelineModel->index(row, inIndex).data().toString()<<timelineModel->index(row, outIndex).data().toString();

                double inSeconds = inTime.msecsSinceStartOfDay() / 1000.0;
                double outSeconds = outTime.msecsSinceStartOfDay() / 1000.0;
                int fileCounter = filesMap[folderName + fileName].counter;

                filterComplexString += "[" + QString::number(fileCounter) + ":v]trim=" + QString::number(inSeconds, 'g', 6) + ":" + QString::number(outSeconds, 'g', 6) + ",setpts=PTS-STARTPTS,scale=" + width + "x" + height + ",setdar=16/9[v" + QString::number(row) + "];";

                filterComplexString2 += "[v" + QString::number(row) + "]";

                if (includeAudio)
                {
                    filterComplexString += "[" + QString::number(fileCounter) + ":a]atrim=" + QString::number(inSeconds, 'g', 6) + ":" + QString::number(outSeconds, 'g', 6) + ",asetpts=PTS-STARTPTS[a" + QString::number(row) + "];";
                    filterComplexString2 += "[a" + QString::number(row) + "]";
                }

//                QString targetFileName = fileNameWithoutExtension + "-" + QString::number(row) + ".mp4";
//                onTrim(timelineModel->index(i, folderIndex).data().toString(),  timelineModel->index(i, fileIndex).data().toString(), targetFileName, inTime, outTime, 10*i/(timelineModel->rowCount())); //not -1 to avoid divby0
            }
        }

        //ffmpeg -i "20190723 bentwoud2.mp4" -i ..\acvclogo.ico -filter_complex "overlay = main_w-overlay_w-10:main_h-overlay_h-10" output.mp4

        //ffmpeg -i "2000-01-19 00-00-00 +53632ms.MP4" -i "Numbered Frames.mp4" -filter_complex "[0:v]trim=1.2:2.5,setpts=PTS-STARTPTS,scale=270x152,setdar=16/9[v0]; [0:a]atrim=1.2:2.5,asetpts=PTS-STARTPTS[a0]; [1:v]trim=10:13.3,setpts=PTS-STARTPTS,scale=270x152,setdar=16/9[v1]; [1:a]atrim=10:13.3,asetpts=PTS-STARTPTS[a1];
        //              [v0][a0][v1][a1]concat=n=2:v=1:a=1[out]" -map "[out]" -r 50 -y out.mp4


        QString code = "ffmpeg " + filesString + " -filter_complex \"" + filterComplexString + " " + filterComplexString2 + " concat=n=" + QString::number(timelineModel->rowCount()) + ":v=1";

        if (includeAudio)
            code += ":a=1";

        code += "[out]\" -map \"[out]\"";

        if (pframeRate != "")
            code += " -r " + pframeRate;

//        code += " flags=bicubic -vf scale=1920:-1";

        code +=  " -y \"" + QString(currentDirectory).replace("/", "\\") + "\\" + fileNameWithoutExtension + ".mp4\"";

        qDebug()<<"filter_complex"<<code;

        //        -filter_complex \"[0:v]scale=640x640 [0:a] [1:v]scale=640x640 [1:a] concat=n=2:v=1:a=1 [v] [a]\" -map \"[v]\" -map \"[a]\" .\\outputreencode.mp4";
        //        -filter_complex \"[0]scale=2704x1520,setdar=16/9[a];[1]scale=2704x1520,setdar=16/9[b]; [a][b] concat=n=2:v=1\" -y D:\\output2.mp4";

        //https://ffmpeg.org/ffmpeg-filters.html#trim

        QString *processId = new QString();

        emit addLogEntry(currentDirectory, fileNameWithoutExtension + ".mp4", "FFMPeg Encode", processId);
        QMap<QString, QString> parameters;
        parameters["processId"] = *processId;
        emit addLogToEntry(parameters["processId"], code + "\n");

        parameters["currentDirectory"] = currentDirectory;
        parameters["resultDurationMSec"] = QString::number(resultDurationMSec);
        parameters["fileNameWithoutExtension"] = fileNameWithoutExtension;
        processManager->startProcess(code, parameters, [] (QWidget *parent, QMap<QString, QString> parameters, QString result)
        {
            FGenerate *generateWidget = qobject_cast<FGenerate *>(parent);

            emit generateWidget->addLogToEntry(parameters["processId"], result);

            int timeIndex = result.indexOf("time=");
            if (timeIndex > 0)
            {
                QString timeString = result.mid(timeIndex + 5, 11) + "0";
                QTime time = QTime::fromString(timeString,"HH:mm:ss.zzz");
                generateWidget->progressBar->setValue(10 + 90*time.msecsSinceStartOfDay()/parameters["resultDurationMSec"].toInt());
            }

            emit generateWidget->addLogToEntry(parameters["processId"], result);
        },  [] (QWidget *parent, QString , QMap<QString, QString> parameters, QStringList )
        {
            FGenerate *generateWidget = qobject_cast<FGenerate *>(parent);

            generateWidget->progressBar->setValue(generateWidget->progressBar->maximum());
            generateWidget->progressBar->setStyleSheet("QProgressBar::chunk {background: green}");

            QDir dir(parameters["currentDirectory"]);
            dir.setFilter( QDir::NoDotAndDotDot | QDir::Files );
            foreach( QString dirItem, dir.entryList() )
            {
                if (dirItem.contains(parameters["fileNameWithoutExtension"] + "-"))
                    dir.remove( dirItem );
            }

            emit generateWidget->addLogToEntry(parameters["processId"], "completed");
        });

        onPropertyUpdate(currentDirectory, filesMap.first().fileName, fileNameWithoutExtension + ".mp4");

        onReloadAll(includingSRT);
    }
    else if (target == "Shotcut")
    {
        QString *processId = new QString();
        emit addLogEntry(currentDirectory, fileNameWithoutExtension, "file generate", processId);

        QString fileName = fileNameWithoutExtension + ".mlt";
        QFile fileWrite(currentDirectory + "//" + fileName);
        fileWrite.open(QIODevice::WriteOnly);

//        QTextStream stream(&fileWrite);

        stream.setDevice(&fileWrite);

        emit addLogToEntry(*processId, QString("Generating %1\n\n").arg(fileName));

        s("<?xml version=\"1.0\" encoding=\"utf-8\"?>");
        s("<mlt LC_NUMERIC=\"C\" version=\"6.17.0\" title=\"Shotcut by ACVC\" producer=\"main_bin\">");
        s("  <profile description=\"automatic\" width=\"%1\" height=\"%2\" progressive=\"1\" sample_aspect_num=\"1\" sample_aspect_den=\"1\" display_aspect_num=\"%1\" display_aspect_den=\"%2\" frame_rate_num=\"%3\" frame_rate_den=\"1\"/>", width, height, pframeRate);

        emit addLogToEntry(*processId, "Producers\n");
        QMapIterator<QString, FileStruct> filesIterator(filesMap);
        while (filesIterator.hasNext()) //all files
        {
            filesIterator.next();

            QString *durationPointer = new QString();
            emit getPropertyValue(filesIterator.value().fileName, "Duration", durationPointer); //format <30s: [ss.mm s] >30s: [h.mm:ss]
            QTime durationTime = QTime::fromString(*durationPointer,"h:mm:ss");
            if (durationTime == QTime())
            {
                QString durationString = *durationPointer;
                durationString = durationString.left(durationString.length()-2); //remove " -s"
                durationTime = QTime::fromMSecsSinceStartOfDay(durationString.toDouble()*1000.0);
            }

            s("  <producer id=\"producer%1\" title=\"Anonymous Submission\" in=\"00:00:00.000\" out=\"%2\">", QString::number(filesIterator.value().counter), durationTime.toString("hh:mm:ss.zzz"));
            s("    <property name=\"length\">%1</property>", durationTime.toString("hh:mm:ss.zzz"));
            s("    <property name=\"resource\">%1</property>", filesIterator.key());
            s("  </producer>");

            emit addLogToEntry(*processId, QString("  Producer%1 %2 %3\n").arg( QString::number(filesIterator.value().counter), durationTime.toString("hh:mm:ss.zzz"), filesIterator.key()));

        }
        emit addLogToEntry(*processId, "\n");

        emit addLogToEntry(*processId, "Playlist\n");
        s("  <playlist id=\"main_bin\" title=\"Main playlist\">");
        s("    <property name=\"xml_retain\">1</property>");
        for (int i=0; i<timelineModel->rowCount();i++)
        {
            QTime inTime = QTime::fromString(timelineModel->index(i, inIndex).data().toString(),"HH:mm:ss.zzz");
            QTime outTime = QTime::fromString(timelineModel->index(i, outIndex).data().toString(),"HH:mm:ss.zzz");
            int fileCounter = filesMap[timelineModel->index(i, folderIndex).data().toString() + timelineModel->index(i, fileIndex).data().toString()].counter;

            s("    <entry producer=\"producer%1\" in=\"%2\" out=\"%3\"/>"
              , QString::number(fileCounter) //fileCounter
              , inTime.toString("HH:mm:ss.zzz"), outTime.toString("HH:mm:ss.zzz"));
            emit addLogToEntry(*processId, QString("  Producer%1 %2 %3\n").arg( QString::number(fileCounter), inTime.toString("HH:mm:ss.zzz"), outTime.toString("HH:mm:ss.zzz")));
        }
        s("  </playlist>"); //playlist main bin
        emit addLogToEntry(*processId, "\n");

        emit addLogToEntry(*processId, "Transitions\n");
        int tractorCounter = 0;
        int producerCounter = 0;
        //transitions
        if (transitionTimeMSecs > 0)
        {
            QTime previousInTime = QTime();
            QTime previousOutTime = QTime();
            QString previousProducerNr = "-1""";
            for (int row=0; row<timelineModel->rowCount();row++)
            {
                QTime inTime = QTime::fromString(timelineModel->index(row, inIndex).data().toString(),"HH:mm:ss.zzz");
                QTime outTime = QTime::fromString(timelineModel->index(row, outIndex).data().toString(),"HH:mm:ss.zzz");
                QString producerNr = QString::number(filesMap[timelineModel->index(row, folderIndex).data().toString() + timelineModel->index(row, fileIndex).data().toString()].counter);

                if (previousInTime != QTime())
                {
                    s("<tractor id=\"tractor%1\" title=\"%2\" global_feed=\"1\" in=\"00:00:00.000\" out=\"%3\">", QString::number(tractorCounter), "Transition " + QString::number(row-1) + "-" + QString::number(row), transitionTime.toString("HH:mm:ss.zzz"));
                    s("   <property name=\"shotcut:transition\">lumaMix</property>");
                    s("   <track producer=\"producer%1\" in=\"%2\" out=\"%3\"/>", previousProducerNr, previousOutTime.addMSecs(-transitionTimeMSecs + FGlobal().frames_to_msec(1)).toString("HH:mm:ss.zzz"), previousOutTime.toString("HH:mm:ss.zzz"));
                    s("   <track producer=\"producer%1\" in=\"%2\" out=\"%3\"/>", producerNr, inTime.toString("HH:mm:ss.zzz"), inTime.addMSecs(transitionTimeMSecs - FGlobal().frames_to_msec(1)).toString("HH:mm:ss.zzz"));
                    s("   <transition id=\"transition0\" out=\"%1\">", transitionTime.toString("HH:mm:ss.zzz"));
                    s("     <property name=\"a_track\">0</property>");
                    s("     <property name=\"b_track\">1</property>");
                    s("     <property name=\"factory\">loader</property>");
                    s("     <property name=\"mlt_service\">luma</property>");
                    s("   </transition>");
                    s("   <transition id=\"transition1\" out=\"%1\">", transitionTime.toString("HH:mm:ss.zzz"));
                    s("     <property name=\"a_track\">0</property>");
                    s("     <property name=\"b_track\">1</property>");
                    s("     <property name=\"start\">-1</property>");
                    s("     <property name=\"accepts_blanks\">1</property>");
                    s("     <property name=\"mlt_service\">mix</property>");
                    s("   </transition>");
                    s(" </tractor>");

                    emit addLogToEntry(*processId, QString("  Transition%1 %2 %3\n").arg( QString::number(tractorCounter), "Transition " + QString::number(row-1) + "-" + QString::number(row), transitionTime.toString("HH:mm:ss.zzz")));

                    tractorCounter++;
                }

                previousInTime = inTime;
                previousOutTime = outTime;
                previousProducerNr = producerNr;
            }

        }
        emit addLogToEntry(*processId, "\n");

        emit addLogToEntry(*processId, "Timeline\n");
        s("  <playlist id=\"playlist0\">");
        s("    <property name=\"shotcut:video\">1</property>");
        s("    <property name=\"shotcut:name\">V1</property>");
        for (int row=0; row<timelineModel->rowCount();row++)
        {
            QTime inTime = QTime::fromString(timelineModel->index(row, inIndex).data().toString(),"HH:mm:ss.zzz");
            QTime outTime = QTime::fromString(timelineModel->index(row, outIndex).data().toString(),"HH:mm:ss.zzz");

            QString producerNr = QString::number(filesMap[timelineModel->index(row, folderIndex).data().toString() + timelineModel->index(row, fileIndex).data().toString()].counter);

            QString inString = inTime.toString("HH:mm:ss.zzz");
            QString outString = outTime.toString("HH:mm:ss.zzz");
            if (transitionTimeMSecs > 0)
            {
                if (row != 0) //first
                {
                    s("    <entry producer=\"tractor%1\" in=\"00:00:00.000\" out=\"%2\"/>", QString::number(row-1), transitionTime.toString("HH:mm:ss.zzz"));
                    inString = inTime.addMSecs(transitionTimeMSecs).toString("HH:mm:ss.zzz");
                    emit addLogToEntry(*processId, QString("  Transition%1 %2\n").arg( QString::number(row-1), transitionTime.toString("HH:mm:ss.zzz")));
                }

                if (row != timelineModel->rowCount() - 1) //last
                    outString = outTime.addMSecs(-transitionTimeMSecs).toString("HH:mm:ss.zzz");
            }

            s("    <entry producer=\"producer%1\" in=\"%2\" out=\"%3\"/>", producerNr, inString, outString );
            emit addLogToEntry(*processId, QString("  Producer%1 %2 %3\n").arg( producerNr, inString, outString ));
        }

        s("  </playlist>"); //playlist0

        s("  <tractor id=\"tractor%1\" title=\"Tractor V1\">", QString::number(tractorCounter++));
        s("    <property name=\"shotcut\">1</property>");
        s("    <track producer=\"\"/>");
        s("    <track producer=\"playlist0\"/>");
        if (includeAudio)
            s("    <track producer=\"playlist0\"/>");
        else
            s("    <track producer=\"playlist0\"  hide=\"audio\"/>");

        s("  </tractor>");
        s("</mlt>");

        fileWrite.close();
\
        progressBar->setValue(progressBar->maximum());
        progressBar->setStyleSheet("QProgressBar::chunk {background: green}");

        emit addLogToEntry(*processId, "\nSuccesfully completed");

    }
    else if (target == "Premiere")
    {
        QString *processId = new QString();
        emit addLogEntry(currentDirectory, fileNameWithoutExtension, "file generate", processId);


        QString fileName = fileNameWithoutExtension + ".xml";
        QFile fileWrite(currentDirectory + "//" + fileName);
        fileWrite.open(QIODevice::WriteOnly);

        stream.setDevice(&fileWrite);

        emit addLogToEntry(*processId, QString("Generating %1\n\n").arg(fileName));

        s("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
        s("<!DOCTYPE xmeml>");
        s("<xmeml version=\"4\">");
        s(" <sequence>");
//        s("  <duration>114</duration>");
        s("  <rate>");
        s("  	<timebase>%1</timebase>", pframeRate);
        s("  	<ntsc>FALSE</ntsc>");
        s("  </rate>");
        s("  <name>%1</name>", fileNameWithoutExtension);
        s("  <media>");

        QStringList mediaTypeList;
        mediaTypeList << "video";
        if (includeAudio)
            mediaTypeList << "audio";
        foreach (QString mediaType, mediaTypeList)
        {
            s("   <%1>", mediaType);
            s("    <format>");
            s("       <samplecharacteristics>");
            if (mediaType == "video")
            {
                s("           <width>%1</width>", width);
                s("           <height>%1</height>", height);
            }
            else
            {
                s("          <depth>16</depth>");
                s("          <samplerate>44100</samplerate>");
            }
            s("       </samplecharacteristics>");
            s("    </format>");

            emit addLogToEntry(*processId, "Timeline\n");

            for (int trackNr=0; trackNr<2;trackNr++)
            {
                s("    <track>");

                int totalFrames = 0;
                for (int row=0; row<timelineModel->rowCount();row++)
                {
                    QString folderName = timelineModel->index(row, folderIndex).data().toString();
                    QString fileName = timelineModel->index(row, fileIndex).data().toString();
                    QTime inTime = QTime::fromString(timelineModel->index(row, inIndex).data().toString(),"HH:mm:ss.zzz");
                    QTime outTime = QTime::fromString(timelineModel->index(row, outIndex).data().toString(),"HH:mm:ss.zzz");

                    int inFrames = qRound(inTime.msecsSinceStartOfDay() * pframeRate.toInt() / 1000.0);
                    int outFrames = qRound(outTime.msecsSinceStartOfDay() * pframeRate.toInt() / 1000.0);
                    int transitionTimeFramesExported = qRound(transitionTimeMSecs * pframeRate.toInt() / 1000.0);

                    QString *frameRatePointer = new QString();
                    emit getPropertyValue(fileName, "VideoFrameRate", frameRatePointer);

                    if (trackNr == 1 && row%2 == 1 && transitionTimeFramesExported > 0) //track 2 and not first edit (never is)
                    {
                        s("     <transitionitem>");
                        s("     	<start>%1</start>", QString::number(totalFrames));
                        s("     	<end>%1</end>", QString::number(totalFrames + transitionTimeFramesExported));
                        s("     	<alignment>start-black</alignment>");
                        s("     	<cutPointTicks>0</cutPointTicks>");
                        s("     	<rate>");
                        s("     		<timebase>%1</timebase>", *frameRatePointer);
                        s("     		<ntsc>FALSE</ntsc>");
                        s("     	</rate>");
                        if (mediaType == "video")
                        {
                            s("     	<effect>");
                            s("     		<name>Cross Dissolve</name>");
                            s("     		<effectid>Cross Dissolve</effectid>");
                            s("     		<effectcategory>Dissolve</effectcategory>");
                            s("     		<effecttype>transition</effecttype>");
                            s("     		<mediatype>video</mediatype>");
                            s("     		<wipecode>0</wipecode>");
                            s("     		<wipeaccuracy>100</wipeaccuracy>");
                            s("     		<startratio>0</startratio>");
                            s("     		<endratio>1</endratio>");
                            s("     		<reverse>FALSE</reverse>");
                            s("     	</effect>");
                        }
                        else
                        {
                            s("     	<effect>");
                            s("     		<name>Cross Fade (+3dB)</name>");
                            s("     		<effectid>KGAudioTransCrossFade3dB</effectid>");
                            s("     		<effecttype>transition</effecttype>");
                            s("     		<mediatype>audio</mediatype>");
                            s("     		<wipecode>0</wipecode>");
                            s("     		<wipeaccuracy>100</wipeaccuracy>");
                            s("     		<startratio>0</startratio>");
                            s("     		<endratio>1</endratio>");
                            s("     		<reverse>FALSE</reverse>");
                            s("     	</effect>");
                        }
                        s("     </transitionitem>");

                    }

                    if (row%2 == trackNr) //even or odd tracks
                    {
                        s("     <clipitem>");
                        s("      <masterclipid>masterclip-%1</masterclipid>", QString::number(row));
                        s("      <name>%1</name>", fileName);
            //            s("      <enabled>TRUE</enabled>");
                        s("      <duration>%1</duration>", QString::number( outFrames - inFrames + 1));
                        s("      <start>%1</start>", QString::number(totalFrames));
                    }

                    totalFrames += outFrames - inFrames + 1 - transitionTimeFramesExported;

                    if (row%2 == trackNr) //even or odd tracks
                    {
                        QString *widthPointer = new QString();
                        emit getPropertyValue(fileName, "ImageWidth", widthPointer);
                        QString *heightPointer = new QString();
                        emit getPropertyValue(fileName, "ImageHeight", heightPointer);

                        s("      <end>%1</end>", QString::number( totalFrames + transitionTimeFramesExported));
                        s("      <in>%1</in>", QString::number( inFrames));
                        s("      <out>%1</out>", QString::number( outFrames +1));
                        if (mediaType == "video")
                        {
                            s("      <file id=\"file-%1\">", QString::number(row));
                            s("       <name>%1</name>", fileName);
                            s("       <pathurl>file://localhost/%1</pathurl>", folderName + fileName);
                            s("       <rate>");
                            s("  	   <timebase>%1</timebase>", *frameRatePointer);
                            s("  	   <ntsc>FALSE</ntsc>");
                            s("       </rate>");
                            s("       <duration>%1</duration>", QString::number( outFrames - inFrames));
                            s("       <media>");
                            s("        <video>");
                            s("         <samplecharacteristics>");
                            s("          <width>%1</width>", *widthPointer);
                            s("          <height>%1</height>", *heightPointer);
                            s("         </samplecharacteristics>");
                            s("        </video>");
                            if (includeAudio)
                            {
                                s("        <audio>");
                                s("         <samplecharacteristics>");
                                s("          <depth>16</depth>");
                                s("          <samplerate>44100</samplerate>");
                                s("         </samplecharacteristics>");
                                s("         <channelcount>2</channelcount>");
                                s("        </audio>");
                            }
                            s("       </media>");
                            s("      </file>");

                            if (*heightPointer != height || *widthPointer != width)
                            {
                                double heightRatio = 100.0 * height.toInt() / (*heightPointer).toInt();
                                double widthRatio = 100.0 * width.toInt() / (*widthPointer).toInt();
                                s("      <filter>");
                                s("       <effect>");
                                s("        <name>Basic Motion</name>");
                                s("         <effectid>basic</effectid>");
                                s("         <effectcategory>motion</effectcategory>");
                                s("         <effecttype>motion</effecttype>");
                                s("         <mediatype>video</mediatype>");
                                s("         <pproBypass>false</pproBypass>");
                                s("         <parameter authoringApp=\"PremierePro\">");
                                s("          <parameterid>scale</parameterid>");
                                s("          <name>Scale</name>");
                                s("          <valuemin>0</valuemin>");
                                s("          <valuemax>1000</valuemax>");
                                s("          <value>%1</value>", QString::number(qMin(heightRatio, widthRatio), 'g', 6));
                                s("         </parameter>");
                                s("        </effect>");
                                s("       </filter>");
                            }
                        }
                        else
                        {
                            s("      <file id=\"file-%1\"/>", QString::number(row));
                            s("      <sourcetrack>");
                            s("          <mediatype>audio</mediatype>");
                            s("          <trackindex>%1</trackindex>", QString::number(trackNr));
                            s("      </sourcetrack>");
                        }
                        s("     </clipitem>");
                    }

                    if (trackNr == 1 && row%2 == 1 && row != timelineModel->rowCount() && transitionTimeFramesExported > 0) //track 2 and not last edit
                    {
                        s("     <transitionitem>");
                        s("     	<start>%1</start>", QString::number(totalFrames));
                        s("     	<end>%1</end>", QString::number(totalFrames + transitionTimeFramesExported));
                        s("     	<alignment>end-black</alignment>");
    //                    s("     	<cutPointTicks>0</cutPointTicks>");
                        s("     	<rate>");
                        s("     		<timebase>%1</timebase>", *frameRatePointer);
                        s("     		<ntsc>FALSE</ntsc>");
                        s("     	</rate>");
                        if (mediaType == "video")
                        {
                            s("     	<effect>");
                            s("     		<name>Cross Dissolve</name>");
                            s("     		<effectid>Cross Dissolve</effectid>");
                            s("     		<effectcategory>Dissolve</effectcategory>");
                            s("     		<effecttype>transition</effecttype>");
                            s("     		<mediatype>video</mediatype>");
                            s("     		<wipecode>0</wipecode>");
                            s("     		<wipeaccuracy>100</wipeaccuracy>");
                            s("     		<startratio>0</startratio>");
                            s("     		<endratio>1</endratio>");
                            s("     		<reverse>FALSE</reverse>");
                            s("     	</effect>");
                        }
                        else
                        {
                            s("     	<effect>");
                            s("     		<name>Cross Fade (+3dB)</name>");
                            s("     		<effectid>KGAudioTransCrossFade3dB</effectid>");
                            s("     		<effecttype>transition</effecttype>");
                            s("     		<mediatype>audio</mediatype>");
                            s("     		<wipecode>0</wipecode>");
                            s("     		<wipeaccuracy>100</wipeaccuracy>");
                            s("     		<startratio>0</startratio>");
                            s("     		<endratio>1</endratio>");
                            s("     		<reverse>FALSE</reverse>");
                            s("     	</effect>");
                        }

                        s("     </transitionitem>");
                    }

                    emit addLogToEntry(*processId, QString("  Producer%1 %2 %3\n"));
                }


                s("    </track>");
            }
    //        s("     <transitionitem>");
    //        s("      <start></start>");
    //        s("      <effect>");
    //        s("       <name></name>");
    //        s("      </effect>");
    //        s("      <end></end>");
    //        s("     </transitionitem>");
            s("   </%1>", mediaType);
        }

        s("  </media>");

        s("  </sequence>");
        s("</xmeml>");

        fileWrite.close();
\

        emit addLogToEntry(*processId, "\nSuccesfully completed");
    }

    QMap<QString, QString> parameters;
    processManager->startProcess(parameters, [] (QWidget *parent, QString , QMap<QString, QString> , QStringList ) //command, parameters, result
    {
        FGenerate *generateWidget = qobject_cast<FGenerate *>(parent);
        generateWidget->progressBar->setValue(generateWidget->progressBar->maximum());
        generateWidget->progressBar->setStyleSheet("QProgressBar::chunk {background: green}");
        generateWidget->spinnerLabel->movie()->stop();
//        generateWidget->spinnerLabel->setMovie(nullptr);
    });
    //https://stackoverflow.com/questions/26958644/qt-loading-indicator-widget/26958738
}

