#include "fgenerate.h"

#include <QDebug>
#include <QSettings>
#include <QTime>

#include "fglobal.h"

FGenerate::FGenerate(QWidget *parent) : QWidget(parent)
{

//    MainWindow *mainWindow = qobject_cast<MainWindow *>(parent);
//    parentLayout = qobject_cast<QVBoxLayout *>(parent->layout());
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

void FGenerate::generate(QStandardItemModel *timelineModel, QString target, QString size, double pframeRate, int transitionTimeFrames, QProgressBar *p_progressBar)
{
    int fileCounter = 0;
    int resultDurationMSec = 0;
    QString currentDirectory = QSettings().value("LastFolder").toString();

    progressBar = p_progressBar;
    progressBar->setRange(0, timelineModel->rowCount());

    bool includingSRT = false;

    if (target == "Preview") //Lossless FFMpeg
    {
        QString fileNameWithoutExtension = target;
        qDebug()<<"fileNameWithoutExtension"<<fileNameWithoutExtension;
        QFile vidlistFile(currentDirectory + "\\" + fileNameWithoutExtension + ".txt");
        QFile srtOutputFile(currentDirectory + "\\" + fileNameWithoutExtension + ".srt");
        if ( vidlistFile.open(QIODevice::WriteOnly) && srtOutputFile.open(QIODevice::WriteOnly) )
        {
            QTextStream vidlistStream( &vidlistFile );
            QTextStream srtStream( &srtOutputFile );

            int totalDuration = 0;

            qDebug()<<"timelineModel->rowCount()"<<timelineModel->rowCount();
            for (int row=0; row<timelineModel->rowCount();row++)
            {
                vidlistStream << "file '" << timelineModel->index(row, folderIndex).data().toString() + timelineModel->index(row, fileIndex).data().toString() << "'" << endl;
                QTime inTime = QTime::fromString(timelineModel->index(row, inIndex).data().toString(),"HH:mm:ss.zzz");
                QTime outTime = QTime::fromString(timelineModel->index(row, outIndex).data().toString(),"HH:mm:ss.zzz");
                qDebug()<<"gen"<<row<<inTime<<outTime;

                int duration = inTime.msecsTo(outTime)+1000/frameRate;

                vidlistStream << "inpoint " <<  QString::number(inTime.msecsSinceStartOfDay() / 1000.0, 'g', 6) << endl;
                vidlistStream << "outpoint " << QString::number((outTime.msecsSinceStartOfDay()) / 1000.0, 'g', 6) << endl;
                //      qDebug()<< videoUrl << srtItemModel->index(i,inIndex).data().toString() << " --> " << srtItemModel->index(i,outIndex).data().toString() << srtItemModel->index(i,tagIndex).data().toString();

                FStarRating starRating = qvariant_cast<FStarRating>(timelineModel->index(row, ratingIndex).data());

                QString srtContentString = "";
                srtContentString += "<o>" + timelineModel->index(row, orderAfterMovingIndex).data().toString() + "</o>";
                srtContentString += "<s>" + QString::number(starRating.starCount()) + "</s>";
                srtContentString += "<r>" + timelineModel->index(row,repeatIndex).data().toString() + "</r>";
                srtContentString += "<h>" + timelineModel->index(row, FGlobal().hintIndex).data().toString() + "</h>";
                srtContentString += "<t>" + timelineModel->index(row,tagIndex).data().toString() + "</t>";

                srtStream << row+1 << endl;
                srtStream << QTime::fromMSecsSinceStartOfDay(totalDuration).toString("HH:mm:ss.zzz") << " --> " << QTime::fromMSecsSinceStartOfDay(totalDuration + duration - 1000 / frameRate).toString("HH:mm:ss.zzz") << endl;
                srtStream << srtContentString << endl;//timelineModel->index(i, tagIndex).data().toString()
                srtStream << endl;

                totalDuration += duration;
            }

            vidlistFile.close();
            srtOutputFile.close();

            progressBar->setRange(0, 100);

            emit addLogEntry("lossless FFMpeg " + currentDirectory);

            QString code = "ffmpeg -f concat -safe 0 -i \"" + QString(currentDirectory).replace("/", "\\") + "\\" + fileNameWithoutExtension + ".txt\" -c copy -an -y \"" + QString(currentDirectory).replace("/", "\\") + "\\" + fileNameWithoutExtension + ".mp4\"";
            QMap<QString, QString> parameters;
            parameters["currentDirectory"] = currentDirectory;
            processManager->startProcess(code, parameters, [] (QWidget *parent, QMap<QString, QString> parameters, QString result)
            {
                FGenerate *generateWidget = qobject_cast<FGenerate *>(parent);
    //            MainWindow *mainWindow = qobject_cast<MainWindow *>(parent);
                emit generateWidget->addLogToEntry("lossless FFMpeg " + parameters["currentDirectory"], result);
            },  [] (QWidget *parent, QString command, QMap<QString, QString> parameters, QStringList result)
            {
                FGenerate *generateWidget = qobject_cast<FGenerate *>(parent);

                qDebug()<<"ffmpeg -f concat -safe 0 result"<<command << result;
                generateWidget->progressBar->setValue(generateWidget->progressBar->maximum());

                emit generateWidget->addLogToEntry("lossless FFMpeg " + parameters["currentDirectory"], "completed" + command.split(" ").last());
            });

            //adding audio (tbd)
            //ffmpeg -i video_input.mp4 -i audio_input.mp3 -c copy -map 0:v -map 1:a outputaudio.mp4

            //superview
            //https://intofpv.com/t-using-free-command-line-sorcery-to-fake-superview
            //https://github.com/Niek/superview

    //        mainWindow->ui->generateButton->setText("Generated");
    //        QTimer::singleShot(2000, this, [this]()->void{
    //                               mainWindow->ui->generateButton->setText("Generate");
    //                           });
        }


    }
//    //ffmpeg -f concat -safe 0 -i test.mp4 -i testTrim.mp4 -c copy output.mp4

    else if (target == "Premiere")
    {
        QString filesString = "";
        QString filterComplexString = "";

        QString code = nullptr;

        for (int i=0; i<timelineModel->rowCount();i++)
        {
            QString tags = timelineModel->index(i, tagIndex).data().toString();
            {
                QTime inTime = QTime::fromString(timelineModel->index(i, inIndex).data().toString(),"HH:mm:ss.zzz");
                QTime outTime = QTime::fromString(timelineModel->index(i, outIndex).data().toString(),"HH:mm:ss.zzz");
                int duration = inTime.msecsTo(outTime)+1000/frameRate;
                resultDurationMSec += duration;

                qDebug()<<i<<timelineModel->index(i, inIndex).data().toString()<<timelineModel->index(i, outIndex).data().toString();
    //            qDebug()<<"times"<<inTime<< deltaIn<< outTime<< deltaOut<< duration<<clipDurationMsec;

                QString targetFileName = currentDirectory + "Generated" + QString::number(i) + ".mp4";

                filesString = filesString + " -i \"" + targetFileName + "\"";
//                filterComplexString += "[" + QString::number(i) + ":v:0][" + QString::number(i) + ":a:0]";
                filterComplexString += "[" + QString::number(i) + ":v:0]";

                code = "ffmpeg -y -i \"" + QString(timelineModel->index(i, folderIndex).data().toString() + "//" + timelineModel->index(i, fileIndex).data().toString()).replace("/", "//") + "\" -ss " + inTime.toString("HH:mm:ss.zzz") + " -t " + QTime::fromMSecsSinceStartOfDay(duration).toString("hh:mm:ss.zzz") + " -map_metadata 0 -vcodec copy -acodec copy \"" + QString(targetFileName).replace("/", "//") + "\"";

                emit addLogEntry("FFMpeg encoding " + currentDirectory);
                emit addLogToEntry("FFMpeg encoding " + currentDirectory, code + "\n");
//                    emit addLogEntry("FFMpeg encoding " + timelineModel->index(i, folderIndex).data().toString() + "//" + timelineModel->index(i, fileIndex).data().toString());

                QMap<QString, QString> parameters;
                parameters["currentDirectory"] = currentDirectory;

                processManager->startProcess(code, parameters, nullptr,  [] (QWidget *parent, QString , QMap<QString, QString> parameters, QStringList )
                {
                    FGenerate *generateWidget = qobject_cast<FGenerate *>(parent);
                    emit generateWidget->addLogToEntry("FFMpeg encoding " + parameters["currentDirectory"], "Completed");
                });

                QString fileName;
//                fileName = QString(targetFileName).replace(".mp4",".srt").replace(".jpg",".srt").replace(".avi",".srt").replace(".wmv",".srt");
//                fileName.replace(".MP4",".srt").replace(".JPG",".srt").replace(".AVI",".srt").replace(".WMV",".srt");
                int lastIndex = targetFileName.lastIndexOf(".");
                if (lastIndex > -1)
                    fileName = targetFileName.left(lastIndex) + ".srt";

                QFile srtOutputFile(fileName);
                if ( srtOutputFile.open(QIODevice::WriteOnly) )
                {
                    QTextStream srtStream( &srtOutputFile );

                    srtStream << 1 << endl;
//                    srtStream << QTime::fromMSecsSinceStartOfDay(deltaIn).toString("HH:mm:ss.zzz") << " --> " << QTime::fromMSecsSinceStartOfDay(duration - deltaOut).toString("HH:mm:ss.zzz") << endl;
                    srtStream << QTime::fromMSecsSinceStartOfDay(resultDurationMSec).toString("HH:mm:ss.zzz") << " --> " << QTime::fromMSecsSinceStartOfDay(resultDurationMSec + duration).toString("HH:mm:ss.zzz") << endl;
                    srtStream << timelineModel->index(i, tagIndex).data().toString() << endl;
                    srtStream << endl;

                    srtOutputFile.close();

                }
            }
        }

        progressBar->setRange(0, resultDurationMSec);

        //ffmpeg -i "20190723 bentwoud2.mp4" -i ..\fiprelogo.ico -filter_complex "overlay = main_w-overlay_w-10:main_h-overlay_h-10" output.mp4

//        code = "ffmpeg " + filesString + " -filter_complex \"" + filterComplexString + " concat=n=" + QString::number(fileCounter) + ":v=1:a=1[outv][outa]\" -map \"[outv]\" -map \"[outa]\" -y \"" + QString(currentDirectory).replace("/", "\\") + "\\outputonesec.mp4\"";
        code = "ffmpeg " + filesString + " -i \"D:/photo/fiprelogo.ico\" -filter_complex \"" + filterComplexString + " concat=n=" + QString::number(timelineModel->rowCount()) + ":v=1[outv]\" -map \"[outv]\" -y \"" + QString(currentDirectory).replace("/", "\\") + "\\outputonesec.mp4\"";
//        code = "ffmpeg " + filesString + " -filter_complex \"[0]scale=2704x1520,setdar=16/9[a];[1]scale=2704x1520,setdar=16/9[b]; [a][b] concat=n=" + QString::number(timelineModel->rowCount()) + ":v=1\" -y \"" + QString(currentDirectory).replace("/", "\\") + "\\outputonesec.mp4\"";
        emit addLogEntry("FFMpeg encoding outputonesec " + currentDirectory);
        emit addLogToEntry("FFMpeg encoding outputonesec " + currentDirectory, code);
        QMap<QString, QString> parameters;
        parameters["currentDirectory"] = currentDirectory;
        processManager->startProcess(code, parameters, [] (QWidget *parent, QMap<QString, QString> parameters, QString result)
        {
            FGenerate *generateWidget = qobject_cast<FGenerate *>(parent);
//            MainWindow *mainWindow = qobject_cast<MainWindow *>(parent);
            emit generateWidget->addLogToEntry("FFMpeg encoding outputonesec " + parameters["currentDirectory"], result);

            int timeIndex = result.indexOf("time=");
            if (timeIndex > 0)
            {
                QString timeString = result.mid(timeIndex + 5, 11) + "0";
                QTime time = QTime::fromString(timeString,"HH:mm:ss.zzz");

//                qDebug()<<"Time progess"<<timeString<<time<<time.msecsSinceStartOfDay();
                generateWidget->progressBar->setValue(time.msecsSinceStartOfDay());
            }
//            mainWindow->logDialog->addLog(result);
            emit generateWidget->addLogToEntry("FFMpeg encoding outputonesec " + parameters["currentDirectory"], result);

        },  [] (QWidget *parent, QString , QMap<QString, QString> parameters, QStringList )
        {
            FGenerate *generateWidget = qobject_cast<FGenerate *>(parent);

//            qDebug()<<"ffmpeg filter result"<<command<<result;
            generateWidget->progressBar->setValue(generateWidget->progressBar->maximum());

//            QDir dir(parameters["currentDirectory"]);
//            dir.setFilter( QDir::NoDotAndDotDot | QDir::Files );
//            foreach( QString dirItem, dir.entryList() )
//            {
//                if (dirItem.contains("Generated"))
//                    dir.remove( dirItem );
//            }

            emit generateWidget->addLogToEntry("FFMpeg encoding outputonesec " + parameters["currentDirectory"], "completed");

        });

    }
    else if (target == "Shotcut")
    {

        emit addLogEntry("shotcut file generate " + currentDirectory);

        QFile fileWrite(currentDirectory + "//" + target + size + ".mlt");
        fileWrite.open(QIODevice::WriteOnly);

        QString width = "1920";
        QString height = "1080";
        if (size == "2.7K")
        {
            width = "2880";
            height = "1620";
        }
        else if (size == "4K")
        {
            width = "3840";
            height = "2160";
        }

//        QTextStream stream(&fileWrite);

        stream.setDevice(&fileWrite);

        s("<?xml version=\"1.0\" encoding=\"utf-8\"?>");
        s("<mlt LC_NUMERIC=\"C\" version=\"6.17.0\" title=\"Shotcut by Fipre\" producer=\"main_bin\">");
        s("  <profile description=\"automatic\" width=\"%1\" height=\"%2\" progressive=\"1\" sample_aspect_num=\"1\" sample_aspect_den=\"1\" display_aspect_num=\"%1\" display_aspect_den=\"%2\" frame_rate_num=\"%3\" frame_rate_den=\"1\"/>", width, height, QString::number(pframeRate));

        QMap<QString, int> filesMap;
        for (int i=0; i<timelineModel->rowCount();i++)
        {
            filesMap[timelineModel->index(i,folderIndex).data().toString() + timelineModel->index(i,fileIndex).data().toString()] = i;
        }


        int fileCounter = 0;
        QMapIterator<QString, int> filesIterator(filesMap);
        while (filesIterator.hasNext()) //all files
        {
            filesIterator.next();

            QString *durationString = new QString();
            emit getPropertyValue(timelineModel->index(filesIterator.value(),fileIndex).data().toString(), "Duration", durationString); //format <30s: [ss.mm s] >30s: [h.mm:ss]

            QTime durationTime = QTime::fromString(*durationString,"h:mm:ss");

            s("  <producer id=\"producer%1\" title=\"Anonymous Submission\" in=\"00:00:00.000\" out=\"%2\">", QString::number(fileCounter), durationTime.toString("hh:mm:ss.zzz"));
            s("    <property name=\"length\">%1</property>", durationTime.toString("hh:mm:ss.zzz"));
            s("    <property name=\"resource\">%1</property>", filesIterator.key());
            s("  </producer>");

            filesMap[filesIterator.key()] = fileCounter;
            fileCounter++;
        }


        s("  <playlist id=\"main_bin\" title=\"Main playlist\">");
        s("    <property name=\"xml_retain\">1</property>");
        for (int i=0; i<timelineModel->rowCount();i++)
        {
            QTime inTime = QTime::fromString(timelineModel->index(i, inIndex).data().toString(),"HH:mm:ss.zzz");
            QTime outTime = QTime::fromString(timelineModel->index(i, outIndex).data().toString(),"HH:mm:ss.zzz");

            s("    <entry producer=\"producer%1\" in=\"%2\" out=\"%3\"/>"
              , QString::number(filesMap[timelineModel->index(i, folderIndex).data().toString() + timelineModel->index(i, fileIndex).data().toString()]
              ), inTime.toString("HH:mm:ss.zzz"), outTime.toString("HH:mm:ss.zzz"));
        }

        s("  </playlist>"); //playlist main bin

        int transitionTimeMSecs = transitionTimeFrames * 1000 / frameRate;
        QTime transitionTime = QTime::fromMSecsSinceStartOfDay(transitionTimeMSecs);
        int tractorCounter = 0;
        int producerCounter = 0;
        //transitions
        if (transitionTimeMSecs > 0)
        {
            QTime previousInTime = QTime();
            QTime previousOutTime = QTime();
            int previousProducerNr = -1;
            for (int row=0; row<timelineModel->rowCount();row++)
            {
                QTime inTime = QTime::fromString(timelineModel->index(row, inIndex).data().toString(),"HH:mm:ss.zzz");
                QTime outTime = QTime::fromString(timelineModel->index(row, outIndex).data().toString(),"HH:mm:ss.zzz");
                int producerNr = filesMap[timelineModel->index(row, folderIndex).data().toString() + timelineModel->index(row, fileIndex).data().toString()];

                if (previousInTime != QTime())
                {
                    s("<tractor id=\"tractor%1\" title=\"%2\" global_feed=\"1\" in=\"00:00:00.000\" out=\"%3\">", QString::number(tractorCounter++), "Transition " + QString::number(row-1) + "-" + QString::number(row), transitionTime.toString("HH:mm:ss.zzz"));
                    s("   <property name=\"shotcut:transition\">lumaMix</property>");
                    s("   <track producer=\"producer%1\" in=\"%2\" out=\"%3\"/>", QString::number(previousProducerNr), previousOutTime.addMSecs(-transitionTimeMSecs + 1000 / frameRate).toString("HH:mm:ss.zzz"), previousOutTime.toString("HH:mm:ss.zzz"));
                    s("   <track producer=\"producer%1\" in=\"%2\" out=\"%3\"/>", QString::number(producerNr), inTime.toString("HH:mm:ss.zzz"), inTime.addMSecs(transitionTimeMSecs - 1000/frameRate).toString("HH:mm:ss.zzz"));
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
                }

                previousInTime = inTime;
                previousOutTime = outTime;
                previousProducerNr = producerNr;
            }

        }


        s("  <playlist id=\"playlist0\">");
        s("    <property name=\"shotcut:video\">1</property>");
        s("    <property name=\"shotcut:name\">V1</property>");
        for (int row=0; row<timelineModel->rowCount();row++)
        {
            QTime inTime = QTime::fromString(timelineModel->index(row, inIndex).data().toString(),"HH:mm:ss.zzz");
            QTime outTime = QTime::fromString(timelineModel->index(row, outIndex).data().toString(),"HH:mm:ss.zzz");

            int producerNr = filesMap[timelineModel->index(row, folderIndex).data().toString() + timelineModel->index(row, fileIndex).data().toString()];

            QString inString = inTime.toString("HH:mm:ss.zzz");
            QString outString = outTime.toString("HH:mm:ss.zzz");
            if (transitionTimeMSecs > 0)
            {
                if (row != 0) //first
                {
                    s("    <entry producer=\"tractor%1\" in=\"00:00:00.000\" out=\"%2\"/>", QString::number(row-1), transitionTime.toString("HH:mm:ss.zzz"));
                    inString = inTime.addMSecs(transitionTimeMSecs).toString("HH:mm:ss.zzz");
                }

                if (row != timelineModel->rowCount() - 1) //last
                    outString = outTime.addMSecs(-transitionTimeMSecs).toString("HH:mm:ss.zzz");
            }

            s("    <entry producer=\"producer%1\" in=\"%2\" out=\"%3\"/>"
              , QString::number(producerNr)
                   , inString, outString );
        }

        s("  </playlist>"); //playlist0

        s("  <tractor id=\"tractor%1\" title=\"Tractor V1\">", QString::number(tractorCounter++));
        s("    <property name=\"shotcut\">1</property>");
        s("    <track producer=\"\"/>");
        s("    <track producer=\"playlist0\"/>");
        s("  </tractor>");
        s("</mlt>");

        fileWrite.close();
\
        progressBar->setValue(progressBar->maximum());

        emit addLogToEntry("shotcut file generate " + currentDirectory, "Completed");

    }
    else
    {
        QString filesString = "";
        QString filterComplexString = "";
        for (int i = 0; i < timelineModel->rowCount(); i++)
        {
            QString tags = timelineModel->index(i,tagIndex).data().toString();
            if ( tags.contains("r8") || tags.contains("r9"))
            {
                filesString = filesString + " -i \"" + timelineModel->index(i,fileIndex).data().toString() + "\"";
                filterComplexString += "[" + QString::number(i) + ":v:0][" + QString::number(i) + ":a:0]";
            }
        }

        qDebug()<<"Method"<<target;
//        QString code = "ffmpeg -i \".\\2019-07-08 16.mp4\" -i .\\bp.mp4 -filter_complex \"[0:v]scale=640x640 [0:a] [1:v]scale=640x640 [1:a] concat=n=2:v=1:a=1 [v] [a]\" -map \"[v]\" -map \"[a]\" .\\outputreencode.mp4";
//        QString code = "ffmpeg -i \".\\2019-07-08 16.mp4\" -i .\\bp.mp4 -filter_complex \"[0]scale=2704x1520,setdar=16/9[a];[1]scale=2704x1520,setdar=16/9[b]; [a][b] concat=n=2:v=1\" -y D:\\output2.mp4";
        QString code = "ffmpeg " + filesString + " -filter_complex \"[0]scale=2704x1520,setdar=16/9[a];[1]scale=2704x1520,setdar=16/9[b]; [a][b] concat=n=2:v=1\" -y D:\\output2.mp4";
        QMap<QString, QString> parameters;
        parameters["currentDirectory"] = currentDirectory;
        emit addLogEntry("Else " + parameters["currentDirectory"]);

        processManager->startProcess(code, parameters, [] (QWidget *parent, QMap<QString, QString> parameters, QString result)
        {
            FGenerate *generateWidget = qobject_cast<FGenerate *>(parent);
//            MainWindow *mainWindow = qobject_cast<MainWindow *>(parent);
            emit generateWidget->addLogToEntry("Else " + parameters["currentDirectory"], result);

        }, nullptr);
    }
}

