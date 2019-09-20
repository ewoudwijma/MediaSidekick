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

void FGenerate::generate(FEditSortFilterProxyModel *editProxyModel, QString target, QString size, QProgressBar *p_progressBar)
{
    int fileCounter = 0;
    int resultDurationMSec = 0;
    QString currentDirectory = QSettings().value("LastFolder").toString();

    progressBar = p_progressBar;
    progressBar->setRange(0, editProxyModel->rowCount());

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

            qDebug()<<"editProxyModel->rowCount()"<<editProxyModel->rowCount();
            for (int row=0; row<editProxyModel->rowCount();row++)
            {
                vidlistStream << "file '" << editProxyModel->index(row, folderIndex).data().toString() + editProxyModel->index(row, fileIndex).data().toString() << "'" << endl;
                QTime inTime = QTime::fromString(editProxyModel->index(row, inIndex).data().toString(),"HH:mm:ss.zzz");
                QTime outTime = QTime::fromString(editProxyModel->index(row, outIndex).data().toString(),"HH:mm:ss.zzz");
                qDebug()<<"gen"<<row<<inTime<<outTime;

                int duration = inTime.msecsTo(outTime)+1000/frameRate;

                vidlistStream << "inpoint " <<  QString::number(inTime.msecsSinceStartOfDay() / 1000.0, 'g', 6) << endl;
                vidlistStream << "outpoint " << QString::number((outTime.msecsSinceStartOfDay()+1000.0/frameRate) / 1000.0, 'g', 6) << endl;
                //      qDebug()<< videoUrl << srtItemModel->index(i,inIndex).data().toString() << " --> " << srtItemModel->index(i,outIndex).data().toString() << srtItemModel->index(i,tagIndex).data().toString();

                FStarRating starRating = qvariant_cast<FStarRating>(editProxyModel->index(row, ratingIndex).data());

                QString srtContentString = "";
                srtContentString += "<o>" + editProxyModel->index(row, orderAfterMovingIndex).data().toString() + "</o>";
                srtContentString += "<s>" + QString::number(starRating.starCount()) + "</s>";
                srtContentString += "<r>" + editProxyModel->index(row,repeatIndex).data().toString() + "</r>";
                srtContentString += "<h>" + editProxyModel->index(row, FGlobal().hintIndex).data().toString() + "</h>";
                srtContentString += "<t>" + editProxyModel->index(row,tagIndex).data().toString() + "</t>";

                srtStream << row+1 << endl;
                srtStream << QTime::fromMSecsSinceStartOfDay(totalDuration).toString("HH:mm:ss.zzz") << " --> " << QTime::fromMSecsSinceStartOfDay(totalDuration + duration - 1000 / frameRate).toString("HH:mm:ss.zzz") << endl;
                srtStream << srtContentString << endl;//editProxyModel->index(i, tagIndex).data().toString()
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

        for (int i=0; i<editProxyModel->rowCount();i++)
        {
            QString tags = editProxyModel->index(i, tagIndex).data().toString();
            {
                QTime inTime = QTime::fromString(editProxyModel->index(i, inIndex).data().toString(),"HH:mm:ss.zzz");
                QTime outTime = QTime::fromString(editProxyModel->index(i, outIndex).data().toString(),"HH:mm:ss.zzz");
                int duration = inTime.msecsTo(outTime)+1000/frameRate;
                resultDurationMSec += duration;

                qDebug()<<i<<editProxyModel->index(i, inIndex).data().toString()<<editProxyModel->index(i, outIndex).data().toString();
    //            qDebug()<<"times"<<inTime<< deltaIn<< outTime<< deltaOut<< duration<<clipDurationMsec;

                QString targetFileName = currentDirectory + "Generated" + QString::number(i) + ".mp4";

                filesString = filesString + " -i \"" + targetFileName + "\"";
//                filterComplexString += "[" + QString::number(i) + ":v:0][" + QString::number(i) + ":a:0]";
                filterComplexString += "[" + QString::number(i) + ":v:0]";

                code = "ffmpeg -y -i \"" + QString(editProxyModel->index(i, folderIndex).data().toString() + "//" + editProxyModel->index(i, fileIndex).data().toString()).replace("/", "//") + "\" -ss " + inTime.toString("HH:mm:ss.zzz") + " -t " + QTime::fromMSecsSinceStartOfDay(duration).toString("hh:mm:ss.zzz") + " -map_metadata 0 -vcodec copy -acodec copy \"" + QString(targetFileName).replace("/", "//") + "\"";

                emit addLogEntry("FFMpeg encoding " + currentDirectory);
                emit addLogToEntry("FFMpeg encoding " + currentDirectory, code + "\n");
//                    emit addLogEntry("FFMpeg encoding " + editProxyModel->index(i, folderIndex).data().toString() + "//" + editProxyModel->index(i, fileIndex).data().toString());

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
                    srtStream << editProxyModel->index(i, tagIndex).data().toString() << endl;
                    srtStream << endl;

                    srtOutputFile.close();

                }
            }
        }

        progressBar->setRange(0, resultDurationMSec);

        //ffmpeg -i "20190723 bentwoud2.mp4" -i ..\fiprelogo.ico -filter_complex "overlay = main_w-overlay_w-10:main_h-overlay_h-10" output.mp4

//        code = "ffmpeg " + filesString + " -filter_complex \"" + filterComplexString + " concat=n=" + QString::number(fileCounter) + ":v=1:a=1[outv][outa]\" -map \"[outv]\" -map \"[outa]\" -y \"" + QString(currentDirectory).replace("/", "\\") + "\\outputonesec.mp4\"";
        code = "ffmpeg " + filesString + " -i \"D:/photo/fiprelogo.ico\" -filter_complex \"" + filterComplexString + " concat=n=" + QString::number(editProxyModel->rowCount()) + ":v=1[outv]\" -map \"[outv]\" -y \"" + QString(currentDirectory).replace("/", "\\") + "\\outputonesec.mp4\"";
//        code = "ffmpeg " + filesString + " -filter_complex \"[0]scale=2704x1520,setdar=16/9[a];[1]scale=2704x1520,setdar=16/9[b]; [a][b] concat=n=" + QString::number(editProxyModel->rowCount()) + ":v=1\" -y \"" + QString(currentDirectory).replace("/", "\\") + "\\outputonesec.mp4\"";
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

            QDir dir(parameters["currentDirectory"]);
            dir.setFilter( QDir::NoDotAndDotDot | QDir::Files );
            foreach( QString dirItem, dir.entryList() )
            {
                if (dirItem.contains("Generated"))
                    dir.remove( dirItem );
            }

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
        s("  <profile description=\"automatic\" width=\"%1\" height=\"%2\" progressive=\"1\" sample_aspect_num=\"1\" sample_aspect_den=\"1\" display_aspect_num=\"%1\" display_aspect_den=\"%2\" frame_rate_num=\"%3\" frame_rate_den=\"1\"/>", width, height, QString::number(frameRate));

        QMap<QString, int> filesMap;
        for (int i=0; i<editProxyModel->rowCount();i++)
        {
            filesMap[editProxyModel->index(i,folderIndex).data().toString() + editProxyModel->index(i,fileIndex).data().toString()] = i;
        }


        int fileCounter = 0;
        QMapIterator<QString, int> filesIterator(filesMap);
        while (filesIterator.hasNext()) //all files
        {
            filesIterator.next();

            QString *duration = new QString();
            emit getPropertyValue(editProxyModel->index(filesIterator.value(),fileIndex).data().toString(), "Duration", duration); //format <30s: [ss.mm s] >30s: [h.mm:ss]

            QTime durationTime = QTime::fromString(*duration,"h:mm:ss");

            s("  <producer id=\"producer%1\" title=\"Anonymous Submission\" in=\"00:00:00.000\" out=\"%2\">", QString::number(fileCounter), durationTime.toString("hh:mm:ss.zzz"));
            s("    <property name=\"length\">%1</property>", durationTime.toString("hh:mm:ss.zzz"));
            s("    <property name=\"resource\">%1</property>", filesIterator.key());
            s("  </producer>");

            filesMap[filesIterator.key()] = fileCounter;
            fileCounter++;
        }


        s("  <playlist id=\"main_bin\" title=\"Shotcut by Fipre\">");
        s("    <property name=\"xml_retain\">1</property>");
        for (int i=0; i<editProxyModel->rowCount();i++)
        {
            QTime inTime = QTime::fromString(editProxyModel->index(i, inIndex).data().toString(),"HH:mm:ss.zzz");
            QTime outTime = QTime::fromString(editProxyModel->index(i, outIndex).data().toString(),"HH:mm:ss.zzz");

            s("    <entry producer=\"producer%1\" in=\"%2\" out=\"%3\"/>"
              , QString::number(filesMap[editProxyModel->index(i, folderIndex).data().toString() + editProxyModel->index(i, fileIndex).data().toString()]
              ), inTime.toString("HH:mm:ss.zzz"), outTime.toString("HH:mm:ss.zzz"));
        }

        s("  </playlist>");
        s("  <playlist id=\"playlist0\">");
        s("    <property name=\"shotcut:video\">1</property>");
        s("    <property name=\"shotcut:name\">V1</property>");
        for (int i=0; i<editProxyModel->rowCount();i++)
        {
            QTime inTime = QTime::fromString(editProxyModel->index(i, inIndex).data().toString(),"HH:mm:ss.zzz");
            QTime outTime = QTime::fromString(editProxyModel->index(i, outIndex).data().toString(),"HH:mm:ss.zzz");

            if (true)
            {
                s("    <entry producer=\"producer%1\" in=\"%2\" out=\"%3\"/>"
                  , QString::number(filesMap[editProxyModel->index(i, folderIndex).data().toString() + editProxyModel->index(i, fileIndex).data().toString()])
                       , inTime.toString("HH:mm:ss.zzz"), outTime.toString("HH:mm:ss.zzz") );
            }
//            else
//                shotcutTransitionTractor(stream, 500);
        }

        s("  </playlist>");
        s("  <tractor id=\"tractor0\" title=\"Shotcut by Fipre\">");
        s("    <property name=\"shotcut\">1</property>");
        s("    <track producer=\"\"/>");
        s("    <track producer=\"playlist0\"/>");
        s("  </tractor>");
        s("</mlt>");

        if (false)
        {
            QXmlStreamWriter *stream2 = new QXmlStreamWriter(&fileWrite);
            stream2->setAutoFormatting(true);
            stream2->writeStartDocument();
            stream2->writeStartElement("mlt");
            stream2->writeAttribute("LC_NUMERIC", "en_US");
            stream2->writeAttribute("version", "6.15.0");
            stream2->writeAttribute("title", "Shotcut generated by Fipre");
            stream2->writeAttribute("producer", "main_bin");
            {
                stream2->writeStartElement("profile");
                stream2->writeAttribute("description", "HD 1080p 25 fps");
                if (size == "1K")
                {
                    stream2->writeAttribute("width", "1920");
                    stream2->writeAttribute("height", "1080");
                }
                else if (size == "2.7K")
                {
                    stream2->writeAttribute("width", "2880");
                    stream2->writeAttribute("height", "1620");
                }
                else if (size == "4K")
                {
                    stream2->writeAttribute("width", "3840");
                    stream2->writeAttribute("height", "2160");
                }
                else
                {
                    stream2->writeAttribute("width", "1920");
                    stream2->writeAttribute("height", "1080");
                }
                stream2->writeAttribute("progressive", "1");
                stream2->writeAttribute("sample_aspect_num", "1");
                stream2->writeAttribute("sample_aspect_den", "1");
                stream2->writeAttribute("display_aspect_num", "16");
                stream2->writeAttribute("display_aspect_den", "9");
                stream2->writeAttribute("frame_rate_num", "25");
                stream2->writeAttribute("frame_rate_den", "1");
                stream2->writeAttribute("colorspace", "709");
                stream2->writeEndElement();
            }

            QMap<QString, int> filesMap;
            for (int i=0; i<editProxyModel->rowCount();i++)
            {
                filesMap[editProxyModel->index(i,folderIndex).data().toString() + editProxyModel->index(i,fileIndex).data().toString()] = i;
            }


            int fileCounter = 0;
            QMapIterator<QString, int> filesIterator(filesMap);
            while (filesIterator.hasNext()) //all files
            {
                filesIterator.next();

                QString *duration = new QString();
                emit getPropertyValue(editProxyModel->index(filesIterator.value(),fileIndex).data().toString(), "Duration", duration); //format <30s: [ss.mm s] >30s: [h.mm:ss]

                QTime durationTime = QTime::fromString(*duration,"h:mm:ss");

                stream2->writeStartElement("producer");
                stream2->writeAttribute("id", "producer" + QString::number(fileCounter));
                stream2->writeAttribute("title", "Anonymous Submission");
                stream2->writeAttribute("in", "00:00:00.000");
                stream2->writeAttribute("out", durationTime.toString("hh:mm:ss.zzz"));
                {
                    stream2->writeStartElement("property");
                    stream2->writeAttribute("name", "length");
                    stream2->writeCharacters( durationTime.toString("hh:mm:ss.zzz"));
                    stream2->writeEndElement();

                    stream2->writeStartElement("property");
                    stream2->writeAttribute("name", "resource");
                    stream2->writeCharacters(filesIterator.key());
                    stream2->writeEndElement();
                }
                stream2->writeEndElement();

                filesMap[filesIterator.key()] = fileCounter;
                fileCounter++;
            }

            {
                stream2->writeStartElement("playlist");
                stream2->writeAttribute("id", "main_bin");
                stream2->writeAttribute("title", "Shotcut generated by Fipre");
                {
    //                stream2->writeStartElement("property");
    //                stream2->writeAttribute("name", "shotcut:projectAudioChannels");
    //                stream2->writeCharacters("2");
    //                stream2->writeEndElement();

    //                stream2->writeStartElement("property");
    //                stream2->writeAttribute("name", "shotcut:projectFolder");
    //                stream2->writeCharacters("1");
    //                stream2->writeEndElement();

                    stream2->writeStartElement("property");
                    stream2->writeAttribute("name", "xml_retain"); //causes this playlist to show in the playlist
                    stream2->writeCharacters("1");
                    stream2->writeEndElement();

                    for (int i=0; i<editProxyModel->rowCount();i++)
                    {
                        QTime inTime = QTime::fromString(editProxyModel->index(i, inIndex).data().toString(),"HH:mm:ss.zzz");
                        QTime outTime = QTime::fromString(editProxyModel->index(i, outIndex).data().toString(),"HH:mm:ss.zzz");

                        stream2->writeStartElement("entry");
                        stream2->writeAttribute("producer", "producer" + QString::number(filesMap[editProxyModel->index(i, folderIndex).data().toString() + editProxyModel->index(i, fileIndex).data().toString()]));
                        stream2->writeAttribute("in", inTime.toString("HH:mm:ss.zzz"));
                        stream2->writeAttribute("out", outTime.toString("HH:mm:ss.zzz"));
                        stream2->writeEndElement();
                    }

                }
    //            stream2->writeAttribute("", "");
                stream2->writeEndElement();
            }
            {
                stream2->writeStartElement("playlist");
                stream2->writeAttribute("id", "playlist0");
                stream2->writeAttribute("title", "Shotcut generated by Fipre");
                {
                    stream2->writeStartElement("property");
                    stream2->writeAttribute("name", "shotcut:video"); //mandatory
                    stream2->writeCharacters("1");
                    stream2->writeEndElement();


                    stream2->writeStartElement("property");
                    stream2->writeAttribute("name", "shotcut:name"); //mandatory
                    stream2->writeCharacters("V1");
                    stream2->writeEndElement();

                    for (int i=0; i<editProxyModel->rowCount();i++)
                    {
                        QTime inTime = QTime::fromString(editProxyModel->index(i, inIndex).data().toString(),"HH:mm:ss.zzz");
                        QTime outTime = QTime::fromString(editProxyModel->index(i, outIndex).data().toString(),"HH:mm:ss.zzz");

                        if (true)
                        {
                            stream2->writeStartElement("entry");
                            stream2->writeAttribute("producer", "producer" + QString::number(filesMap[editProxyModel->index(i, folderIndex).data().toString() + editProxyModel->index(i, fileIndex).data().toString()]));
                            stream2->writeAttribute("in", inTime.toString("HH:mm:ss.zzz"));
                            stream2->writeAttribute("out", outTime.toString("HH:mm:ss.zzz"));
                            stream2->writeEndElement();
                        }
                        else
                            shotcutTransitionTractor(stream2, 500);

                    }

                }
    //            stream2->writeAttribute("", "");
                stream2->writeEndElement(); //playlist0
            }
            //main tractor
            {
                stream2->writeStartElement("tractor");
                stream2->writeAttribute("id", "tractor0");
                stream2->writeAttribute("title", "Shotcut generated by Fipre");
    //            stream2->writeAttribute("global_feed", "1");
                stream2->writeAttribute("in", "00:00:00.000");
                stream2->writeAttribute("out", "00:00:13.960");
                {
                    stream2->writeStartElement("property");
                    stream2->writeAttribute("name", "shotcut"); //mandatory
                    stream2->writeCharacters("1");
                    stream2->writeEndElement();

    //                stream2->writeStartElement("property");
    //                stream2->writeAttribute("name", "shotcut:projectAudioChannels");
    //                stream2->writeCharacters("2");
    //                stream2->writeEndElement();

    //                stream2->writeStartElement("property");
    //                stream2->writeAttribute("name", "shotcut:projectFolder");
    //                stream2->writeCharacters("1");
    //                stream2->writeEndElement();

                    stream2->writeStartElement("track");
                    stream2->writeAttribute("producer", ""); //apparently no need to define the background producer
                    stream2->writeEndElement();

                    stream2->writeStartElement("track");
                    stream2->writeAttribute("producer", "playlist0");
                    stream2->writeEndElement();

                }
                stream2->writeEndElement(); //tractor

    //            <tractor id="tractor0" title="Shotcut version 19.09.14" global_feed="1" in="00:00:00.000" out="00:00:13.960">
    //                <property name="shotcut">1</property>
    //                <property name="shotcut:projectAudioChannels">2</property>
    //                <property name="shotcut:projectFolder">1</property>
    //                <track producer="background"/>
    //                <track producer="playlist0"/>
    //                <transition id="transition0">
    //                  <property name="a_track">0</property>
    //                  <property name="b_track">1</property>
    //                  <property name="mlt_service">mix</property>
    //                  <property name="always_active">1</property>
    //                  <property name="sum">1</property>
    //                </transition>
    //                <transition id="transition1">
    //                  <property name="a_track">0</property>
    //                  <property name="b_track">1</property>
    //                  <property name="version">0.9</property>
    //                  <property name="mlt_service">frei0r.cairoblend</property>
    //                  <property name="disable">1</property>
    //                </transition>
    //              </tractor>
            }

            stream2->writeEndElement(); //mlt
            stream2->writeEndDocument();
        }
        fileWrite.close();
\
        progressBar->setValue(progressBar->maximum());

        emit addLogToEntry("shotcut file generate " + currentDirectory, "Completed");

    }
    else
    {
        QString filesString = "";
        QString filterComplexString = "";
        for (int i = 0; i < editProxyModel->rowCount(); i++)
        {
            QString tags = editProxyModel->index(i,tagIndex).data().toString();
            if ( tags.contains("r8") || tags.contains("r9"))
            {
                filesString = filesString + " -i \"" + editProxyModel->index(i,fileIndex).data().toString() + "\"";
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

void FGenerate::shotcutTransitionTractor(QXmlStreamWriter *stream2, int transitionTime)
{
    //transition tractor
    {
        stream2->writeStartElement("tractor");
        stream2->writeAttribute("id", "tractor1");
        stream2->writeAttribute("title", "Anonymous Submission");
//            stream2->writeAttribute("global_feed", "1");
        stream2->writeAttribute("in", "00:00:00.000");
        stream2->writeAttribute("out", "00:00:00.240"); //transition time
        {
            stream2->writeStartElement("property");
            stream2->writeAttribute("name", "shotcut:transition"); //mandatory
            stream2->writeCharacters("lumaMix");
            stream2->writeEndElement();

            stream2->writeStartElement("track");
            stream2->writeAttribute("producer", "producer0");
            stream2->writeAttribute("in", "00:00:00.000");
            stream2->writeAttribute("out", "00:00:00.240"); //transition time
            stream2->writeEndElement();

            stream2->writeStartElement("track");
            stream2->writeAttribute("producer", "producer0");
            stream2->writeAttribute("in", "00:00:00.000");
            stream2->writeAttribute("out", "00:00:00.240"); //transition time
            stream2->writeEndElement();
            {
                stream2->writeStartElement("transition");
                stream2->writeAttribute("id", "transition0");
                stream2->writeAttribute("out", "00:00:00.240"); //transition time
                {
                    stream2->writeStartElement("property");
                    stream2->writeAttribute("name", "a_track"); //mandatory
                    stream2->writeCharacters("0");
                    stream2->writeEndElement();

                    stream2->writeStartElement("property");
                    stream2->writeAttribute("name", "b_track"); //mandatory
                    stream2->writeCharacters("1");
                    stream2->writeEndElement();

                    stream2->writeStartElement("property");
                    stream2->writeAttribute("name", "factory"); //mandatory
                    stream2->writeCharacters("loader");
                    stream2->writeEndElement();

                    stream2->writeStartElement("property");
                    stream2->writeAttribute("name", "mlt_service"); //mandatory
                    stream2->writeCharacters("luma");
                    stream2->writeEndElement();

                }
                stream2->writeEndElement(); //transition

                stream2->writeStartElement("transition");
                stream2->writeAttribute("id", "transition1");
                stream2->writeAttribute("out", "00:00:00.240"); //transition time
                {
                    stream2->writeStartElement("property");
                    stream2->writeAttribute("name", "a_track"); //mandatory
                    stream2->writeCharacters("0");
                    stream2->writeEndElement();

                    stream2->writeStartElement("property");
                    stream2->writeAttribute("name", "b_track"); //mandatory
                    stream2->writeCharacters("1");
                    stream2->writeEndElement();

                    stream2->writeStartElement("property");
                    stream2->writeAttribute("name", "start"); //mandatory
                    stream2->writeCharacters("1");
                    stream2->writeEndElement();

                    stream2->writeStartElement("property");
                    stream2->writeAttribute("name", "accept_blanks"); //mandatory
                    stream2->writeCharacters("1");
                    stream2->writeEndElement();

                    stream2->writeStartElement("property");
                    stream2->writeAttribute("name", "mlt_service"); //mandatory
                    stream2->writeCharacters("mix");
                    stream2->writeEndElement();

                }
                stream2->writeEndElement(); //transition

            }

        }
        stream2->writeEndElement(); //tractor


//            <tractor id="tractor0" title="Anonymous Submission" global_feed="1" in="00:00:00.000" out="00:00:00.240">
//              <property name="shotcut:transition">lumaMix</property>
//              <track producer="producer0" in="00:00:21.920" out="00:00:22.160"/>
//              <track producer="producer0" in="00:00:24.360" out="00:00:24.600"/>
//              <transition id="transition0" out="00:00:00.240">
//                <property name="a_track">0</property>
//                <property name="b_track">1</property>
//                <property name="factory">loader</property>
//                <property name="mlt_service">luma</property>
//              </transition>
//              <transition id="transition1" out="00:00:00.240">
//                <property name="a_track">0</property>
//                <property name="b_track">1</property>
//                <property name="start">-1</property>
//                <property name="accepts_blanks">1</property>
//                <property name="mlt_service">mix</property>
//              </transition>
//            </tractor>
    }

}
