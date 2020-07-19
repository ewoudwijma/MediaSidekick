#include "agcliprectitem.h"
#include "agviewrectitem.h"
#include "aexport.h"
#include "mgrouprectitem.h"
#include "ui_mexportdialog.h"

#include <QDebug>
#include <QDesktopServices>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>

#include <qmath.h>

MExportDialog::MExportDialog(QWidget *parent, AGViewRectItem *rootItem) :
    QDialog(parent),
    ui(new Ui::MExportDialog)
{
    ui->setupUi(this);

    this->rootItem = rootItem;

    changeUIProperties();

    loadSettings();

    allTooltips();
}

MExportDialog::~MExportDialog()
{
    delete ui;
}

void MExportDialog::changeUIProperties()
{
    //additional ui
    ui->transitionDial->setFixedSize(200,200);

    ui->transitionTimeSpinBox->setFixedWidth(120);

    ui->exportVideoAudioSlider->setMaximum(100);
    ui->exportVideoAudioSlider->setSingleStep(10);
//    ui->exportVideoAudioSlider->setValue(20);
    ui->exportVideoAudioSlider->setOrientation(Qt::Horizontal);
    ui->exportVideoAudioSlider->setTickPosition(QSlider::TicksBelow);
    ui->exportVideoAudioSlider->setTickInterval(10);

    ui->watermarkLabel->setText(QSettings().value("watermarkFileName").toString());
    watermarkFileNameChanged(ui->watermarkLabel->text());
}

void MExportDialog::loadSettings()
{
//    qDebug()<<"MExportDialog::loadSettings";
    //load clips (before exportsize and exportframerate calculated
    foreach (QGraphicsItem *item, rootItem->scene()->items())
    {
        if (item->data(mediaTypeIndex).toString() == "Clip" && item->data(itemTypeIndex).toString() == "Base")
        {
            AGClipRectItem *clipItem = (AGClipRectItem *)item;

            if (clipItem->mediaItem->groupItem->fileInfo.fileName() == "Video")
            {
                QString clipsSize = clipItem->mediaItem->exiftoolValueMap["ImageWidth"].value + " x " + clipItem->mediaItem->exiftoolValueMap["ImageHeight"].value;
                if (clipsSize != " x " && ui->clipsSizeComboBox->findText(clipsSize) == -1)
                    ui->clipsSizeComboBox->addItem(clipsSize);

                int clipsFrameRate = qRound(clipItem->mediaItem->exiftoolValueMap["VideoFrameRate"].value.toDouble());
                if (clipsFrameRate != 0 && ui->clipsFramerateComboBox->findText(QString::number(clipsFrameRate)) == -1)
                    ui->clipsFramerateComboBox->addItem(QString::number(clipsFrameRate));
            }
        }
    }

    if (ui->clipsSizeComboBox->count() == 0 || ui->clipsFramerateComboBox->count() == 0)
    {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(ui->clipsSizeComboBox, "Clips", tr("No clips sizes or framerates found. Cancel export?"), QMessageBox::Yes|QMessageBox::No);
        if (reply == QMessageBox::Yes)
            this->close();
    }

    int lframeRate = QSettings().value("frameRate").toInt();
    if (lframeRate < 1)
    {
            lframeRate = 25;
    }
//    qDebug()<<"Set framerate"<<lframeRate;
    ui->clipsFramerateComboBox->setCurrentText(QString::number(lframeRate));

    ui->transitionDial->setValue(ui->transitionTimeSpinBox->value());
    ui->transitionComboBox->setCurrentText(QSettings().value("transitionType").toString());
    ui->transitionTimeSpinBox->setValue(QSettings().value("transitionTime").toInt());

//    connect(exportTargetComboBox, &QComboBox::currentTextChanged, this, &MainWindow::on_exportTargetComboBox_currentTextChanged);
    if (ui->exportTargetComboBox->currentText() == QSettings().value("exportTarget").toString())
        on_exportTargetComboBox_currentTextChanged(ui->exportTargetComboBox->currentText());
    else
        ui->exportTargetComboBox->setCurrentText(QSettings().value("exportTarget").toString());

    if (QSettings().value("exportSize").toString() == "")
        ui->exportSizeComboBox->setCurrentText("Source"); //will trigger currentTextChanged
    else
    {
        if (ui->exportSizeComboBox->currentText() == QSettings().value("exportSize").toString())
            on_exportSizeComboBox_currentTextChanged(ui->exportSizeComboBox->currentText());
        else
            ui->exportSizeComboBox->setCurrentText(QSettings().value("exportSize").toString());
    }

    if (QSettings().value("exportFrameRate").toString() == "")
        ui->exportFramerateComboBox->setCurrentText("Source");  //will trigger currentTextChanged
    else
    {
        if (ui->exportFramerateComboBox->currentText() == QSettings().value("exportFrameRate").toString())
            on_exportFramerateComboBox_currentTextChanged(ui->exportFramerateComboBox->currentText());
        else
            ui->exportFramerateComboBox->setCurrentText(QSettings().value("exportFrameRate").toString());
    }

    if (QSettings().value("exportVideoAudioSlider").toInt() == 0)
        QSettings().setValue("exportVideoAudioSlider", 20); //default

    if (QSettings().value("exportVideoAudioSlider").toInt() != ui->exportVideoAudioSlider->value())
        ui->exportVideoAudioSlider->setValue(QSettings().value("exportVideoAudioSlider").toInt());
}

void MExportDialog::allTooltips()
{
    ui->clipsSizeComboBox->setToolTip(tr("<p><b>Video size</b></p>"
                                         "<p><i>Shows the sizes found in the media used for clips</i></p>"
                                         "<ul>"
                                         "<li>The size of the first file is selected as default.</li>"
                                         "</ul>"));

    ui->clipsFramerateComboBox->setToolTip(tr("<p><b>Framerate</b></p>"
                                        "<p><i>Shows the framerates found in the media used for clips</i></p>"
                                        "<ul>"
                                              "<li>The framerate of the first file is selected as default.</li>"
                                        "</ul>"));

    ui->transitionDial->setToolTip(tr("<p><b>Transitions</b></p>"
                                      "<p><i>Sets the transition time and type for the exported video</i></p>"
                                      "<ul>"
                                      "<li>No transitions: set the time to 0</li>"
                                      "<li>Transtion type: curently only Cross Dissolve supported</li>"
                                      "<li>Remark: Lossless and encode do not support transitions yet, the transition time is however subtracted from the exported video</li>"
                                      "</ul>"));
    ui->transitionComboBox->setToolTip( ui->transitionDial->toolTip());
    ui->transitionTimeSpinBox->setToolTip( ui->transitionDial->toolTip());


    ui->exportTargetComboBox->setToolTip(tr("<p><b>Export target</b></p>"
                                 "<p><i>Determines what will be exported</i></p>"
                                 "<ul>"
                                              "<li>Lossless: FFMpeg generated video file. Very fast! If codec, size and framerate of clips is not the same the result might not play correctly</li>"
                                            "<ul>"
                                                         "<li>Excluded: transitions (duration preserved by cut in the middle), watermark, audio fade in/out</li>"
                                            "</ul>"
                                              "<li>Encode: FFMpeg generated video file</li>"
    "<ul>"
                 "<li>Excluded: transitions (duration preserved by cut in the middle), audio fade in/out</li>"
    "</ul>"
                                              "<li>Premiere: Final cut XML project file for Adobe Premiere</li>"
                                              "<li>Shotcut: Mlt project file</li>"
                                 "</ul>"));

    ui->exportSizeComboBox->setToolTip(tr("<p><b>Export video size</b></p>"
                                          "<p><i>The video size of the exported files</i></p>"
                                          "<ul>"
                                          "<li>Source: if source selected then the selected video size of the clips is used</li>"
                                          "<li>Width: the exported width is based on the aspect ratio of the selected video size of the clips</li>"
                                          "</ul>"));

    ui->exportFramerateComboBox->setToolTip(tr("<p><b>Export framerate</b></p>"
                                                 "<p><i>The framerate of the exported files</i></p>"
                                                 "<ul>"
                                                 "<li>Source: if source selected than the framerate of the source is used (see clips window)</li>"
                                                 "</ul>"));

    ui->exportVideoAudioSlider->setToolTip(tr("<p><b>Video Audio volume</b></p>"
                                              "<p><i>Sets the volume of the original video</i></p>"
                                              "<ul>"
                                              "<li>Remark: This only applies to the videos. Audio files are always played at 100%</li>"
                                              "<li>Remark: Volume adjustments are also audible in Media Sidekick when playing video files</li>"
                                              "</ul>"));


    ui->watermarkLabel->setToolTip(tr("<p><b>Watermark</b></p>"
                                      "<p><i>Adds a watermark / logo on the bottom right of the exported video</i></p>"
                                      "<ul>"
                                      "<li>Button: If no watermark then browse for an image. If a watermark is already selected, clicking the button will remove the watermark</li>"
                                      "<li>Remark: No watermark on lossless encoding</li>"
                                      "</ul>"));

    ui->watermarkButton->setToolTip(ui->watermarkLabel->toolTip());
}

void MExportDialog::onProcessOutput(QTime time, QTime totalTime, QString event, QString outputString)
{
    AGProcessAndThread *processAndThread = (AGProcessAndThread *)sender();

//    qDebug()<<"MExportDialog::onProcessOutput"<<processAndThread->name<<outputString;//<<processAndThread->log.join("\n");

    emit processOutput(time, totalTime, event, outputString);
}

void MExportDialog::s(QString inputString, QString arg1, QString arg2, QString arg3, QString arg4)
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

void MExportDialog::losslessVideoAndAudio()
{
    foreach (AGViewRectItem *timelineItem, timelineGroupList)
    {
        AGViewRectItem *groupItem = (AGViewRectItem *)timelineItem->parentItem();
//        qDebug()<<"Timeline"<<groupItem->fileInfo<<timelineItem->fileInfo.absolutePath()<<timelineItem->fileInfo.fileName();

        QString recycleFolderName = QSettings().value("selectedFolderName").toString() + "MSKRecycleBin/";
        QDir recycleDir(recycleFolderName);
        if (!recycleDir.exists())
            recycleDir.mkpath(".");

        QFile vidlistFile(recycleFolderName + fileNameWithoutExtension + groupItem->fileInfo.completeBaseName() + ".txt");
//        qDebug()<<"opening vidlistfile"<<vidlistFile;

        if ( vidlistFile.open(QIODevice::WriteOnly) )
        {
            QTextStream vidlistStream( &vidlistFile );

            foreach (QGraphicsItem *item, timelineItem->filteredClips)
            {
                AGClipRectItem *clipItem = (AGClipRectItem *)item;
//                qDebug()<<"  Clip"<<clipItem->itemToString();

                QTime inTime = QTime::fromMSecsSinceStartOfDay(clipItem->clipIn);
                QTime outTime = QTime::fromMSecsSinceStartOfDay(clipItem->clipOut + AGlobal().frames_to_msec(1)); //add one frame

                vidlistStream << "file '" << clipItem->fileInfo.absoluteFilePath() << "'" << endl;
                if (ui->transitionTimeSpinBox->value() > 0)
                {
                    if (item != timelineItem->filteredClips.first())
                    {
                        inTime = inTime.addMSecs(AGlobal().frames_to_msec((ui->transitionTimeSpinBox->value()/2))); //subtract half of the transitionframes 2, not 2.0
                    }
                    if (item != timelineItem->filteredClips.last())
                    {
                        outTime = outTime.addMSecs(- AGlobal().frames_to_msec(qRound(ui->transitionTimeSpinBox->value()/2.0))); //subtract half of the transitionframes
                    }
                }
//                    qDebug()<<"gen"<<mediaType<<row<<transitionTimeFrames/2<<qRound(transitionTimeFrames/2.0)<<AGlobal().frames_to_msec((transitionTimeFrames/2))<<inTime<<outTime<<fileExtension;

                vidlistStream << "inpoint " <<  QString::number(inTime.msecsSinceStartOfDay() / 1000.0, 'g', 6) << endl;
                vidlistStream << "outpoint " << QString::number((outTime.msecsSinceStartOfDay()) / 1000.0, 'g', 6) << endl;

//                qDebug()<<"VideoAudioCheck"<<clipItem->fileInfo.fileName()<<clipItem->mediaItem->exiftoolValueMap["VideoFrameRate"].value<<clipItem->mediaItem->exiftoolValueMap["AudioSampleRate"].value<<clipItem->mediaItem->exiftoolValueMap["AudioBitrate"].value;

                timelineItem->containsVideo = clipItem->mediaItem->exiftoolValueMap["VideoFrameRate"].value != "";
                timelineItem->containsAudio = clipItem->mediaItem->exiftoolValueMap["AudioSampleRate"].value != "" || clipItem->mediaItem->exiftoolValueMap["AudioBitrate"].value != ""; //AudioBitrate for mp3, AudioSampleRate for mp4
                //tbd check other formats then mp3 and 4

            } //for each clip

            vidlistFile.close();

            QString sourceFolderFileName = recycleFolderName + fileNameWithoutExtension + groupItem->fileInfo.completeBaseName() + ".txt";
            QString targetFolderFileName = recycleFolderName + fileNameWithoutExtension + groupItem->fileInfo.completeBaseName() + ".mp4";

#ifdef Q_OS_WIN
            sourceFolderFileName = sourceFolderFileName.replace("/", "\\");
            targetFolderFileName = targetFolderFileName.replace("/", "\\");
#endif
            QString command = "ffmpeg -f concat -safe 0 -i \"" + sourceFolderFileName + "\" -c copy -y \"" + targetFolderFileName + "\"";

            if (timelineItem->containsAudio && ui->exportVideoAudioSlider->value() == 0) //remove audio
                command.replace("-c copy -y", " -an -c copy -y");

//            if (frameRate != "")
//                command.replace("-c copy -y", " -r " + frameRate + " -c copy -y");

            AGProcessAndThread *process = new AGProcessAndThread(this);
            process->command("FFMpeg lossless" + groupItem->fileInfo.completeBaseName(), command);
            *processes<<process;
            connect(process, &AGProcessAndThread::processOutput, this, &MExportDialog::onProcessOutput);
            connect(process, &AGProcessAndThread::processOutput, [=] (QTime time, QTime totalTime, QString event, QString outputString)
            {
                if (event == "finished") //go mux
                {
                    //check if all processes finished
                    int nrOfActiveJobs = 0;
                    foreach (AGProcessAndThread *process, *processes)
                    {
                        if ((process->process != nullptr && process->process->state() != QProcess::NotRunning) || (process->jobThread != nullptr && process->jobThread->isRunning()))
                            nrOfActiveJobs++;
                    }

                    if (nrOfActiveJobs == 0)
                        muxVideoAndAudio();
                }
            });
            process->start();

        } //if vidlist
        else
        {
            qDebug()<<"vidlistFile.error"<<vidlistFile.error()<< vidlistFile.errorString();
        }
    }
} //losslessVideoAndAudio

void MExportDialog::encodeVideoClips()
{
    bool useTrimFiles = false;
    bool differentFrameRateFound = false;

    foreach (AGViewRectItem *timelineItem, timelineGroupList)
    {
        QStringList ffmpegFiles;
        QStringList ffmpegClips;
        QStringList ffmpegCombines;
        QStringList ffmpegMappings;

        AGViewRectItem *groupItem = (AGViewRectItem *)timelineItem->parentItem();
//        qDebug()<<"Timeline"<<groupItem->fileInfo<<timelineItem->fileInfo.absolutePath()<<timelineItem->fileInfo.fileName();

        QString recycleFolderName = QSettings().value("selectedFolderName").toString() + "MSKRecycleBin/";
        QDir recycleDir(recycleFolderName);
        if (!recycleDir.exists())
            recycleDir.mkpath(".");
        else
        {
            recycleDir.setFilter( QDir::NoDotAndDotDot | QDir::Files );
            foreach( QString dirItem, recycleDir.entryList() )
            {
                if (dirItem.contains("Clip"))
                    recycleDir.remove( dirItem );
            }
        }

        int totalDurationMsec = 0;
        QString lastV, lastA;

        foreach (QGraphicsItem *item, timelineItem->filteredClips)
        {
            AGClipRectItem *clipItem = (AGClipRectItem *)item;
//            qDebug()<<"  Clip"<<clipItem->itemToString();

            QTime inTime = QTime::fromMSecsSinceStartOfDay(clipItem->clipIn);
            QTime outTime = QTime::fromMSecsSinceStartOfDay(clipItem->clipOut + AGlobal().frames_to_msec(1)); //add one frame

//            AGMediaFileRectItem *mediaItem = clipItem->mediaItem;

            if (clipItem->mediaItem->exiftoolValueMap["VideoFrameRate"].value.toInt() != exportFramerate)
                     differentFrameRateFound = true;

            if (useTrimFiles && groupItem->fileInfo.fileName() == "Video")
            {
                QStandardItem *childItem = nullptr;
                QString recycleFileName = "Clip" + QString::number(ffmpegFiles.count()) + "." + clipItem->fileInfo.suffix();
                QFile file (recycleFolderName + recycleFileName);
                if (!file.exists())
                {
                    //    qDebug()<<"AExport::onTrimC"<<folderName<<fileNameSource<<fileNameTarget<<inTime<<outTime<<progressPercentage;
                    int duration = AGlobal().frames_to_msec(AGlobal().msec_to_frames(outTime.msecsSinceStartOfDay()) - AGlobal().msec_to_frames(inTime.msecsSinceStartOfDay()) + 1);

                    QString sourceFolderFileName = clipItem->fileInfo.absoluteFilePath();
                    QString targetFolderFileName = recycleFolderName + recycleFileName;

                #ifdef Q_OS_WIN
                    sourceFolderFileName = sourceFolderFileName.replace("/", "\\");
                    targetFolderFileName = targetFolderFileName.replace("/", "\\");
                #endif

                    AGProcessAndThread *process = new AGProcessAndThread(this);
                    process->command("Trim " + recycleFileName, "ffmpeg -i \"" + sourceFolderFileName + "\" -ss " + inTime.toString("HH:mm:ss.zzz") + " -t " + QTime::fromMSecsSinceStartOfDay(duration).toString("hh:mm:ss.zzz") + " -vcodec copy -acodec copy -y \"" + targetFolderFileName + "\""); // -map_metadata 0);

                    *processes<<process;

                    process->start();

//                    emit trimC(recycleFolderName, recycleFileName, inTime, outTime.addMSecs(AGlobal().frames_to_msec(1))); //one frame extra in case some more frames needed
                }

                ffmpegFiles << "-i \"" + recycleFolderName + recycleFileName + "\"";
            }
            else
                ffmpegFiles << "-i \"" + clipItem->fileInfo.absoluteFilePath() + "\"";

            int fileReference = ffmpegFiles.count() - 1;

//            qDebug()<<"VideoAudioCheck"<<clipItem->fileInfo.fileName()<<clipItem->mediaItem->exiftoolValueMap["VideoFrameRate"].value<<clipItem->mediaItem->exiftoolValueMap["AudioSampleRate"].value<<clipItem->mediaItem->exiftoolValueMap["AudioBitrate"].value;
            timelineItem->containsVideo = clipItem->mediaItem->exiftoolValueMap["VideoFrameRate"].value != "";
            timelineItem->containsAudio = clipItem->mediaItem->exiftoolValueMap["AudioSampleRate"].value != "" || clipItem->mediaItem->exiftoolValueMap["AudioBitrate"].value != ""; //AudioBitrate for mp3, AudioSampleRate for mp4
            //tbd check other formats then mp3 and 4

            QStringList tracks;

            if (timelineItem->containsVideo)
                tracks << "v";
            if (timelineItem->containsAudio)
                tracks << "a";
    //        if (groupItem->fileInfo.fileName() == "Video")
    //            tracks = QStringList()<<"v"<<"a";
    //        if (groupItem->fileInfo.fileName() == "Audio")
    //            tracks = QStringList()<<"a";

            foreach (QString track, tracks)
            {
                QString filterClip;

                if (track == "v")
                {
                    filterClip = tr("[%1]").arg(QString::number(fileReference));
                    if (!useTrimFiles)
                        filterClip += tr("trim=%2:%3[vv%1];[vv%1]").arg(QString::number(fileReference), QString::number(inTime.msecsSinceStartOfDay() / 1000.0) , QString::number((outTime.msecsSinceStartOfDay()) / 1000.0));
                    filterClip += tr("setpts=PTS-STARTPTS+%1/TB").arg(QString::number(totalDurationMsec / 1000.0));
                }
                else
                {
                    filterClip = tr("[%1:a]").arg(QString::number(fileReference));
                    if (!useTrimFiles)
                          filterClip += tr("atrim=%2:%3[aa%1];[aa%1]").arg(QString::number(fileReference), QString::number(inTime.msecsSinceStartOfDay() / 1000.0) , QString::number((outTime.msecsSinceStartOfDay()) / 1000.0)); // in seconds
                    filterClip += tr("volume=%1,").arg(QString::number(ui->exportVideoAudioSlider->value() / 100.0));
                    filterClip += tr("aformat=sample_fmts=fltp:sample_rates=44100:channel_layouts=stereo,asetpts=PTS-STARTPTS");
                }

                if (ui->transitionTimeSpinBox->value() != 0)
                {
                    if (track == "v")
                    {
                        QString alphaIn = "0"; //transparent= firstIn
                        QString alphaOut = "0"; //black = transitions
                        if (item != timelineItem->filteredClips.first())
                            alphaIn = "1";
                        if (item != timelineItem->filteredClips.last())
                            alphaOut = "1";

                        //in frames
                        filterClip += tr(",fade=in:0:%1:alpha=%2").arg(QString::number(ui->transitionTimeSpinBox->value()), alphaIn);
                        filterClip += tr(",fade=out:%1:%2:alpha=%3").arg(QString::number(AGlobal().msec_to_frames(inTime.msecsTo(outTime)) - ui->transitionTimeSpinBox->value()), QString::number(ui->transitionTimeSpinBox->value()), alphaOut);
                    }
                    else
                    {
                        //in seconds
                        filterClip += tr(",afade=t=in:ss=0:d=%1").arg(QString::number(AGlobal().frames_to_msec(ui->transitionTimeSpinBox->value()) / 1000.0));
                        filterClip += tr(",afade=t=out:st=%1:d=%2").arg(QString::number((inTime.msecsTo(outTime) - AGlobal().frames_to_msec(ui->transitionTimeSpinBox->value())) / 1000.0), QString::number(AGlobal().frames_to_msec(ui->transitionTimeSpinBox->value()) / 1000.0));
                    }
                }

                if (track == "v")
                {
                    if (clipItem->mediaItem->exiftoolValueMap["ImageWidth"].value.toInt() != exportWidth || clipItem->mediaItem->exiftoolValueMap["ImageHeight"].value.toInt() != exportHeight)
                        filterClip += ",scale=" + QString::number(exportWidth) + "x" + QString::number(exportHeight);
                    if (clipItem->mediaItem->exiftoolValueMap["VideoFrameRate"].value.toInt() != exportFramerate)
                        filterClip += ",fps=" + QString::number(exportFramerate);

                    filterClip += tr("[v%1]").arg(QString::number(fileReference));
                }
                else
                    filterClip += tr("[a%1]").arg(QString::number(fileReference));

//                if (tags.toLower().contains("backwards"))
//                    filterClip.replace("setpts=PTS-STARTPTS", "setpts=PTS-STARTPTS, reverse");
//                //https://stackoverflow.com/questions/42257354/concat-a-video-with-itself-but-in-reverse-using-ffmpeg
//                if (tags.toLower().contains("slowmotion"))
//                    filterClip.replace("setpts=PTS-STARTPTS", "setpts=2.0*PTS");
//                if (tags.toLower().contains("fastmotion"))
//                    filterClip.replace("setpts=PTS-STARTPTS", "setpts=0.5*PTS");

                ffmpegClips << filterClip;

                if (track == "v")
                {
                    if (item != timelineItem->filteredClips.first())
                    {
                        ffmpegCombines << tr("[%1][v%2]overlay[vo%2]").arg(lastV, QString::number(fileReference));
                        lastV = tr("vo%1").arg(QString::number(fileReference));
                    }
                    else
                        lastV = tr("v%1").arg(QString::number(fileReference));
                }
                else
                {
                    if (item != timelineItem->filteredClips.first())
                    {
                        if (ui->transitionTimeSpinBox->value() > 0)
                            ffmpegCombines << tr("[%1][a%2]acrossfade=d=%3[ao%2]").arg( lastA, QString::number(fileReference), QString::number(AGlobal().frames_to_msec(ui->transitionTimeSpinBox->value())/1000.0));
                        else
                            ffmpegCombines << tr("[%1][a%2]concat=n=2:v=0:a=1[ao%2]").arg( lastA, QString::number(fileReference));
                        lastA = tr("ao%1").arg(QString::number(fileReference));
                    }
                    else
                        lastA = tr("a%1").arg(QString::number(fileReference));
                }
            }

            totalDurationMsec += inTime.msecsTo(outTime) - AGlobal().frames_to_msec(ui->transitionTimeSpinBox->value());
//            qDebug()<<"  Clip"<<clipItem->itemToString()<<inTime.msecsTo(outTime)<<totalDurationMsec;
        } //for each clip

        //add the watermark to the (first) video track
//        foreach (QString track, tracks)
        if (timelineItem->containsVideo)
        {
            if (groupItem->fileInfo.fileName() == "Video" && QSettings().value("watermarkFileName").toString() != "No Watermark")
            {
                ffmpegFiles << "-i \"" + QSettings().value("watermarkFileName").toString() + "\"";
                ffmpegClips << "[" + QString::number(ffmpegFiles.count() -1) + ":v]scale=" + QString::number(exportWidth/10) + "x" + QString::number(exportHeight/10) + "[wtm]";
                ffmpegCombines << "[" + lastV + "][wtm]overlay = main_w-overlay_w-10:main_h-overlay_h-10[video]";
                ffmpegMappings << "-map [video]";
            }
            else
                ffmpegMappings << tr("-map [%1]").arg(lastV);
        }
        if (timelineItem->containsAudio)
        {
            ffmpegMappings << tr("-map [%1]").arg(lastA);
        }

//        QString sourceFolderFileName = recycleFolderName + fileNameWithoutExtension + groupItem->fileInfo.completeBaseName() + ".txt";
        QString targetFolderFileName = recycleFolderName + fileNameWithoutExtension + groupItem->fileInfo.completeBaseName() + ".mp4";

#ifdef Q_OS_WIN
//        sourceFolderFileName = sourceFolderFileName.replace("/", "\\");
        targetFolderFileName = targetFolderFileName.replace("/", "\\");
#endif
//        QString command = "ffmpeg -f concat -safe 0 -i \"" + sourceFolderFileName + "\" -c copy -y \"" + targetFolderFileName + "\"";

//        if (ui->exportVideoAudioSlider->value() == 0) //remove audio
//            command.replace("-c copy -y", " -an -c copy -y");

////            if (frameRate != "")
////                command.replace("-c copy -y", " -r " + frameRate + " -c copy -y");

        ffmpegMappings << "-y \"" + targetFolderFileName + "\"";

        QString command = "ffmpeg";

        if (ffmpegFiles.count() > 0)
            command += " " + ffmpegFiles.join(" ");

        if (ffmpegClips.count() + ffmpegCombines.count() > 0)
        {
            command += " -filter_complex \"";
            QString sep = "";
            if (ffmpegClips.count() > 0)
            {
                command += ffmpegClips.join(";");
                sep = ";";
            }

            if (ffmpegCombines.count() > 0)
                    command += sep + ffmpegCombines.join(";");
            command += "\"";
        }

        if (ffmpegMappings.count() > 0)
            command += " " + ffmpegMappings.join(" ");

        //for each timeline, if clip processes (trim) are finished
        if (false)
        {
            AGProcessAndThread *process0 = new AGProcessAndThread(this);
            *processes<<process0;
            process0->command("Check " + groupItem->fileInfo.fileName() + " finished", [=]()
            {
                bool busy = true;
                while (busy)
                {
                    int nrOfActiveJobs = 0;
                    foreach (AGProcessAndThread *process, *processes)
                    {
                        //tbd: check only processes for this timeline...
                        if ((process->process != nullptr && process->process->state() != QProcess::NotRunning) || (process->jobThread != nullptr && process->jobThread->isRunning()))
                            nrOfActiveJobs++;
                    }

                    busy = nrOfActiveJobs > 1; //> 1 to exclude itself
                }
            });
            connect(process0, &AGProcessAndThread::processOutput, [=] (QTime time, QTime totalTime, QString event, QString outputString)
            {
                if (event == "finished")
                {
                    //time to start encode
                    qDebug()<<"time to start encode"<<groupItem->fileInfo.fileName();
                }
            });
            process0->start();
        }

        AGProcessAndThread *process = new AGProcessAndThread(this);
        process->command("FFMpeg encode" + groupItem->fileInfo.completeBaseName(), command);
        process->totalTime = QTime::fromMSecsSinceStartOfDay(totalDurationMsec);
        *processes<<process;
        connect(process, &AGProcessAndThread::processOutput, this, &MExportDialog::onProcessOutput);
        connect(process, &AGProcessAndThread::processOutput, [=] (QTime time, QTime totalTime, QString event, QString outputString)
        {
            if (event == "started")
            {
                process->addProcessLog("output", ffmpegFiles.join("\n"));
                process->addProcessLog("output", ffmpegClips.join("\n"));
                process->addProcessLog("output", ffmpegCombines.join("\n"));
                process->addProcessLog("output", ffmpegMappings.join("\n"));
            }
            if (event == "finished") //go mux
            {
                //check if all processes finished
                int nrOfActiveJobs = 0;
                foreach (AGProcessAndThread *process, *processes)
                {
                    if ((process->process != nullptr && process->process->state() != QProcess::NotRunning) || (process->jobThread != nullptr && process->jobThread->isRunning()))
                        nrOfActiveJobs++;
                }

                if (nrOfActiveJobs == 0)
                    muxVideoAndAudio();
            }
        });
        process->start();

    } //for each timeline
} //encodeVideoClips

void MExportDialog::muxVideoAndAudio()
{
    QStringList ffmpegFiles;
    QStringList ffmpegClips;
    QStringList ffmpegCombines;
    QStringList ffmpegMappings;

    QStringList audioStreamList;
    QStringList videoStreamList;
    foreach (AGViewRectItem *timelineItem, timelineGroupList)
    {
        AGViewRectItem *groupItem = (AGViewRectItem *)timelineItem->parentItem();

        QString recycleFolderName = QSettings().value("selectedFolderName").toString() + "MSKRecycleBin/";
        QDir recycleDir(recycleFolderName);
        if (!recycleDir.exists())
            recycleDir.mkpath(".");

        QString targetFolderFileName = recycleFolderName + fileNameWithoutExtension + groupItem->fileInfo.completeBaseName() + ".mp4";;

    #ifdef Q_OS_WIN
        targetFolderFileName = targetFolderFileName.replace("/", "\\");
    #endif

        ffmpegFiles << "-i \"" + targetFolderFileName + "\"";

        int fileReference = ffmpegFiles.count() - 1;

        if (timelineItem->containsVideo)
        {
            videoStreamList << "[" + QString::number(fileReference) + "]";
        }
        if (timelineItem->containsAudio)
        {
            if (groupItem->fileInfo.completeBaseName() == "Video")
            {
                if (ui->exportVideoAudioSlider->value() == 100)
                    audioStreamList << "[" + QString::number(fileReference) + "]";
    //                                                command += " -filter_complex \"[0][1]amix=inputs=2[a]\" -map 0:v -map \"[a]\" -c:v copy";
                else if (ui->exportVideoAudioSlider->value() > 0)
                    audioStreamList << "[" + QString::number(fileReference) + "]" + "volume=" + QString::number(ui->exportVideoAudioSlider->value() / 100.0) + "[a" + QString::number(fileReference) + "];[a" + QString::number(fileReference) + "]";
    //                                                command += " -filter_complex \"[1]amix=inputs=2[a]\" -map 0:v -map \"[a]\" -c:v copy";

    //                                            ffmpegMappings << "-map " + QString::number(fileReference) + ":v";
            }
            else
            {
                audioStreamList << "[" + QString::number(fileReference) + "]";
            }

        }
        if (timelineItem->containsImage) //tbd
        {
            videoStreamList << "[" + QString::number(fileReference) + "]";
        }

    } //for each timeline

    if (videoStreamList.count() > 0)
    {
        ffmpegMappings << "-map 0:v";
    }

    if (audioStreamList.count() > 0)
    {
        {
            ffmpegCombines << audioStreamList.join("") + "amix=inputs=" + QString::number(audioStreamList.count()) + "[audio]";
            ffmpegMappings << "-map \"[audio]\"";
        }
    }

    ffmpegMappings << " -c:v copy";

    QString targetFolderFileName = QSettings().value("selectedFolderName").toString() +  fileNameWithoutExtension + ".mp4";

#ifdef Q_OS_WIN
    targetFolderFileName = targetFolderFileName.replace("/", "\\");
#endif

    ffmpegMappings << "-y \"" + targetFolderFileName + "\"";

    QString command = "ffmpeg";

    if (ffmpegFiles.count() > 0)
        command += " " + ffmpegFiles.join(" ");

    if (ffmpegClips.count() + ffmpegCombines.count() > 0)
    {
        command += " -filter_complex \"";
        QString sep = "";
        if (ffmpegClips.count() > 0)
        {
            command += ffmpegClips.join(";");
            sep = ";";
        }

        if (ffmpegCombines.count() > 0)
                command += sep + ffmpegCombines.join(";");
        command += "\"";
    }

    if (ffmpegMappings.count() > 0)
        command += " " + ffmpegMappings.join(" ");

    AGProcessAndThread *process = new AGProcessAndThread(this);
    process->command("FFMpeg mux " + rootItem->fileInfo.completeBaseName(), command);
    *processes<<process;
    connect(process, &AGProcessAndThread::processOutput, this, &MExportDialog::onProcessOutput);
    connect(process, &AGProcessAndThread::processOutput, [=] (QTime time, QTime totalTime, QString event, QString outputString)
    {
        if (event == "started")
        {
            process->addProcessLog("output", "Files\n" + ffmpegFiles.join("\n"));
            process->addProcessLog("output", "Clips\n" + ffmpegClips.join("\n"));
            process->addProcessLog("output", "Combines\n" + ffmpegCombines.join("\n"));
            process->addProcessLog("output", "Mappings\n" + ffmpegMappings.join("\n"));
        }
    });
    process->start();

}

void MExportDialog::addPremiereTrack(QString mediaType, AGViewRectItem *timelineItem, QMap<QString, FileStruct> filesMap)
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
            int totalDurationMsec = 0;

            foreach (QGraphicsItem *item, timelineItem->filteredClips)
            {
                AGClipRectItem *clipItem = (AGClipRectItem *)item;
//                qDebug()<<"  Clip"<<clipItem->itemToString();

                AGMediaFileRectItem *mediaItem = clipItem->mediaItem;

                QTime inTime = QTime::fromMSecsSinceStartOfDay(clipItem->clipIn);
                QTime outTime = QTime::fromMSecsSinceStartOfDay(clipItem->clipOut); //not needed to add one frame...

//                QString folderName = timelineModel->index(row, folderIndex).data().toString();
//                QString fileName = timelineModel->index(row, fileIndex).data().toString();
//                QString imageWidth = timelineModel->index(row, imageWidthIndex).data().toString();
//                QString imageHeight = timelineModel->index(row, imageHeightIndex).data().toString();

                int inFrames = qRound(inTime.msecsSinceStartOfDay() * exportFramerate / 1000.0);
                int outFrames = qRound(outTime.msecsSinceStartOfDay() * exportFramerate / 1000.0);

                QString clipAudioChannels = "";

                if (mediaItem->exiftoolValueMap["AudioChannels"].value != "")
                    clipAudioChannels = mediaItem->exiftoolValueMap["AudioChannels"].value;
                else
                {
                    if (mediaType == "audio")
                        clipAudioChannels = "2";  //assume stereo for audiofiles
                    else if (timelineItem->containsAudio)
                        clipAudioChannels = "1";  //assume mono for videofiles
                    else
                        clipAudioChannels = "0";  //assume mono for videofiles
                }

                //                qDebug()<<"props"<<fileName<<mediaType<<clipFrameRate<<clipAudioChannels<<*audioChannelsPointer;

                QString frameRate = mediaItem->exiftoolValueMap["VideoFrameRate"].value;
                if (frameRate.toInt() == 0) //for audio
                    frameRate = QString::number(exportFramerate);

                if (mediaType == "audio")
                {
                    if (iterationCounter%2 == trackNr) //even or odd tracks
                    {
                        if (item == timelineItem->filteredClips.first()) //fade in
                        {
                            addPremiereTransitionItem(0, frameRate.toInt(), frameRate, mediaType, "start");
                        }
                    }
                }

                if (trackNr == 1 && iterationCounter%2 == 1 && ui->transitionTimeSpinBox->value() > 0) //track 2 and not first clip (never is)
                {
                    addPremiereTransitionItem(totalFrames, totalFrames + ui->transitionTimeSpinBox->value(), frameRate, mediaType, "start");
                }

                int deltaFrames = outFrames - inFrames + 1 - ui->transitionTimeSpinBox->value();
                if (iterationCounter%2 == trackNr) //even or odd tracks
                {
                    addPremiereClipitem(QString::number(iterationCounter), mediaItem->fileInfo, totalFrames, totalFrames + deltaFrames + ui->transitionTimeSpinBox->value(), inFrames, outFrames + 1, frameRate, mediaType, &filesMap, channelTrackNr, clipAudioChannels, mediaItem->exiftoolValueMap["ImageWidth"].value, mediaItem->exiftoolValueMap["ImageHeight"].value);
                }

                totalFrames += deltaFrames;

                if (trackNr == 1 && iterationCounter%2 == 1 && item != timelineItem->filteredClips.last() && ui->transitionTimeSpinBox->value() > 0) //track 2 and not last clip
                {
                    addPremiereTransitionItem(totalFrames, totalFrames + ui->transitionTimeSpinBox->value(), frameRate, mediaType, "end");
                }

                totalDurationMsec += inTime.msecsTo(outTime) - AGlobal().frames_to_msec(ui->transitionTimeSpinBox->value() - 1); //-1: add 1 frame

                if (mediaType == "audio")
                {
                    if (iterationCounter%2 == trackNr) //even or odd tracks
                    {
                        if (item == timelineItem->filteredClips.last()) //fade out 1 second
                        {
                            addPremiereTransitionItem(AGlobal().msec_to_frames(totalDurationMsec) - frameRate.toInt(), AGlobal().msec_to_frames(totalDurationMsec), frameRate, mediaType, "end");
                        }
                    }
                }

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

void MExportDialog::addPremiereTransitionItem(int startFrames, int endFrames, QString frameRate, QString mediaType, QString startOrEnd)
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

void MExportDialog::addPremiereClipitem(QString clipId, QFileInfo fileInfo, int startFrames, int endFrames, int inFrames, int outFrames, QString frameRate, QString mediaType, QMap<QString, FileStruct> *filesMap, int channelTrackNr, QString clipAudioChannels, QString imageWidth, QString imageHeight)
{
    if (mediaType == "video")
        s("     <clipitem>");
    else //audio
    {
        if (clipAudioChannels == "2")
            s("     <clipitem premiereChannelType=\"%1\">", "stereo");
        else
            s("     <clipitem premiereChannelType=\"%1\">", "mono");
    }

    s("      <masterclipid>masterclip-%1</masterclipid>", clipId);
    s("      <name>%1</name>", fileInfo.fileName());
//            s("      <enabled>TRUE</enabled>");
//                        s("      <duration>%1</duration>", QString::number( outFrames - inFrames + 1));
    s("      <start>%1</start>", QString::number(startFrames)); //will be reassigned in premiere to -1

    s("      <end>%1</end>", QString::number(endFrames)); //will be reassigned in premiere to -1

    if (inFrames != -1)
        s("      <in>%1</in>", QString::number(inFrames));
    if (outFrames != -1)
        s("      <out>%1</out>", QString::number(outFrames));

    QString AVType;

    QString fileNameLow = fileInfo.fileName().toLower();
    int lastIndexOf = fileNameLow.lastIndexOf(".");
    QString extension = fileNameLow.mid(lastIndexOf + 1);

    if (AGlobal().audioExtensions.contains(extension, Qt::CaseInsensitive))
        AVType = "A";
    else if (clipId == "WM") //watermark
        AVType = "W";
    else
        AVType = "V";

    if (!(*filesMap)[fileInfo.absoluteFilePath()].definitionGenerated)
    {
        s("      <file id=\"file-%1%2\">", AVType, QString::number((*filesMap)[fileInfo.absoluteFilePath()].counter)); //define file
        s("       <name>%1</name>", fileInfo.fileName());
        s("       <pathurl>file://localhost/%1</pathurl>", fileInfo.absoluteFilePath());
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

        if ((AVType == "A" || (clipAudioChannels != "0" && ui->exportVideoAudioSlider->value() > 0)) && AVType != "W")
        {
            s("        <audio>");
//                                s("         <samplecharacteristics>");
//                                s("          <depth>16</depth>");
////                                s("          <samplerate>44100</samplerate>");
//                                s("         </samplecharacteristics>");
            s("         <channelcount>%1</channelcount>", clipAudioChannels);
            s("        </audio>");
        }
        s("       </media>");
        s("      </file>");

        (*filesMap)[fileInfo.absoluteFilePath()].definitionGenerated = true;
    }
    else //already defined
    {
        s("      <file id=\"file-%1%2\"/>", AVType, QString::number((*filesMap)[fileInfo.absoluteFilePath()].counter));  //refer to file if already defined
    }

    if (mediaType == "video")
    {
        if (AVType == "V")
        {
            if (imageHeight.toInt() != exportHeight || imageWidth.toInt() != exportWidth)
            {
                double heightRatio = 100.0 * exportHeight / imageHeight.toInt();
                double widthRatio = 100.0 * exportWidth / imageWidth.toInt();
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
            myImage.load(QSettings().value("watermarkFileName").toString());
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
            s("                  <value>%1</value>", QString::number(exportHeight / myImage.height() * 10));
            s("              </parameter>");
            s("              <parameter authoringApp=\"PremierePro\">");
            s("                  <parameterid>center</parameterid>");
            s("                  <name>Center</name>");
            s("                  <value>");
            s("                      <horiz>%1</horiz>", QString::number(exportWidth / myImage.width() * 1.7)); //don't know why 1.7 and 1.4 but looks ok...
            s("                      <vert>%1</vert>", QString::number(exportHeight / myImage.height() * 1.4));
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

        if (AVType == "V"  && ui->exportVideoAudioSlider->value() > 0 && ui->exportVideoAudioSlider->value() < 100 && clipAudioChannels != "0")
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
            s("         <value>%1</value>", QString::number(ui->exportVideoAudioSlider->value() / 100.0));
            s("        </parameter>");
            s("       </effect>");
            s("      </filter>");
        }
    }
    s("     </clipitem>");
} //addPremiereClipItem

void MExportDialog::exportShotcut()
{

    QFile fileWrite(QSettings().value("selectedFolderName").toString() + fileNameWithoutExtension + ".mlt");
    fileWrite.open(QIODevice::WriteOnly);

    stream.setDevice(&fileWrite);

    s("<?xml version=\"1.0\" encoding=\"utf-8\"?>");
    s("<mlt LC_NUMERIC=\"C\" version=\"6.17.0\" title=\"Shotcut v19.10.20 by Media Sidekick v%1\" producer=\"main_bin\">", qApp->applicationVersion());
    s("  <profile description=\"automatic\" width=\"%1\" height=\"%2\" progressive=\"1\" sample_aspect_num=\"1\" sample_aspect_den=\"1\" display_aspect_num=\"%1\" display_aspect_den=\"%2\" frame_rate_num=\"%3\" frame_rate_den=\"1\"/>", QString::number(exportWidth), QString::number(exportHeight), QString::number(exportFramerate));

    int iterationCounter = 0;
    int maxDurationMsec = 0;
    foreach (AGViewRectItem *timelineItem, timelineGroupList)
    {
        AGViewRectItem *groupItem = (AGViewRectItem *)timelineItem->parentItem();
//        qDebug()<<"Timeline"<<groupItem->fileInfo<<timelineItem->fileInfo.absolutePath()<<timelineItem->fileInfo.fileName();

        int totalDurationMsec = 0;

        foreach (QGraphicsItem *item, timelineItem->filteredClips)
        {
            AGClipRectItem *clipItem = (AGClipRectItem *)item;

            QTime inTime = QTime::fromMSecsSinceStartOfDay(clipItem->clipIn);
            QTime outTime = QTime::fromMSecsSinceStartOfDay(clipItem->clipOut); //not needed to add one frame...
            int clipDuration = clipItem->data(mediaDurationIndex).toInt();

            s("  <producer id=\"producer%1\" title=\"Anonymous Submission\" in=\"00:00:00.000\" out=\"%2\">", QString::number(iterationCounter), QTime::fromMSecsSinceStartOfDay(clipItem->mediaItem->data(mediaDurationIndex).toInt()).toString("hh:mm:ss.zzz"));
            s("    <property name=\"length\">%1</property>", QTime::fromMSecsSinceStartOfDay(clipItem->mediaItem->data(mediaDurationIndex).toInt()).toString("hh:mm:ss.zzz"));// fileDuration);
            s("    <property name=\"resource\">%1</property>", clipItem->fileInfo.absoluteFilePath());

            if (groupItem->fileInfo.fileName() == "Audio")
            {
                if (item == timelineItem->filteredClips.first() || item == timelineItem->filteredClips.last())
                {
                    s("    <filter id=\"filter%1\" in=\"%2\" out=\"%3\">", QString::number(iterationCounter), inTime.toString("HH:mm:ss.zzz"), outTime.toString("HH:mm:ss.zzz"));
                    s("          <property name=\"window\">75</property>");
                    s("          <property name=\"max_gain\">20dB</property>");
                    s("          <property name=\"mlt_service\">volume</property>");

                    //fadein and out of 1 second.
                    if (item == timelineItem->filteredClips.first())
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
                if (ui->exportVideoAudioSlider->value() > 0)
                {
    //                    qDebug()<<"qLn"<<qLn(10)<<exportVideoAudioSlider->value()<<10 * qLn(exportVideoAudioValue / 100.0);
                    s("          <filter id=\"filter%1\" in=\"%2\" out=\"%3\">", QString::number(iterationCounter), inTime.toString("HH:mm:ss.zzz"), outTime.toString("HH:mm:ss.zzz"));
                    s("             <property name=\"window\">75</property>");
                    s("             <property name=\"max_gain\">20dB</property>");
                    s("             <property name=\"level\">%1</property>", QString::number(10 * qLn(ui->exportVideoAudioSlider->value() / 100.0))); //percentage to dB
                    s("             <property name=\"mlt_service\">volume</property>");
                    s("           </filter>");
                }
            }

            s("  </producer>");

            totalDurationMsec += inTime.msecsTo(outTime) - AGlobal().frames_to_msec(ui->transitionTimeSpinBox->value() - 1); //-1: add 1 frame
//            qDebug()<<"  Clip"<<clipItem->itemToString()<<clipDuration<<inTime.msecsTo(outTime)<<totalDurationMsec;
            iterationCounter++;
        }

        maxDurationMsec = qMax(maxDurationMsec, totalDurationMsec);
//        qDebug()<<"DurationMsec"<<groupItem->fileInfo.fileName()<<totalDurationMsec<<maxDurationMsec;
    }

    iterationCounter = 0;
    s("  <playlist id=\"main_bin\" title=\"Main playlist\">");
    s("    <property name=\"xml_retain\">1</property>");
    foreach (AGViewRectItem *timelineItem, timelineGroupList)
    {

        foreach (QGraphicsItem *item, timelineItem->filteredClips)
        {
            AGClipRectItem *clipItem = (AGClipRectItem *)item;
//            qDebug()<<"  Clip"<<clipItem->itemToString();

            QTime inTime = QTime::fromMSecsSinceStartOfDay(clipItem->clipIn);
            QTime outTime = QTime::fromMSecsSinceStartOfDay(clipItem->clipOut); //not needed to add one frame...

            s("    <entry producer=\"producer%1\" in=\"%2\" out=\"%3\"/>"
              , QString::number(iterationCounter)
              , inTime.toString("HH:mm:ss.zzz"), outTime.toString("HH:mm:ss.zzz"));

            iterationCounter++;
        }
    }

    s("  </playlist>"); //playlist main bin

    if (ui->transitionTimeSpinBox->value() > 0)
    {
        QTime previousInTime = QTime();
        QTime previousOutTime = QTime();
        int previousProducerNr = -1;

        iterationCounter = 0;
        foreach (AGViewRectItem *timelineItem, timelineGroupList)
        {
            foreach (QGraphicsItem *item, timelineItem->filteredClips)
            {
                AGClipRectItem *clipItem = (AGClipRectItem *)item;
//                qDebug()<<"  Clip"<<clipItem->itemToString();

                QTime inTime = QTime::fromMSecsSinceStartOfDay(clipItem->clipIn);
                QTime outTime = QTime::fromMSecsSinceStartOfDay(clipItem->clipOut); //not needed to add one frame...

                if (item != timelineItem->filteredClips.first())
                {
                    s("<tractor id=\"tractor%1\" title=\"%2\" global_feed=\"1\" in=\"00:00:00.000\" out=\"%3\">", QString::number(previousProducerNr) + "-" + QString::number(iterationCounter), "Transition " + QString::number(previousProducerNr) + "-" + QString::number(iterationCounter), QTime::fromMSecsSinceStartOfDay(AGlobal().frames_to_msec(ui->transitionTimeSpinBox->value())).toString("HH:mm:ss.zzz"));
                    s("   <property name=\"shotcut:transition\">lumaMix</property>");
                    s("   <track producer=\"producer%1\" in=\"%2\" out=\"%3\"/>", QString::number(previousProducerNr), previousOutTime.addMSecs(AGlobal().frames_to_msec(-ui->transitionTimeSpinBox->value() + 1)).toString("HH:mm:ss.zzz"), previousOutTime.toString("HH:mm:ss.zzz"));
                    s("   <track producer=\"producer%1\" in=\"%2\" out=\"%3\"/>", QString::number(iterationCounter), inTime.toString("HH:mm:ss.zzz"), inTime.addMSecs(AGlobal().frames_to_msec(ui->transitionTimeSpinBox->value() - 1)).toString("HH:mm:ss.zzz"));
                    s("   <transition id=\"transition0\" out=\"%1\">", QTime::fromMSecsSinceStartOfDay(AGlobal().frames_to_msec(ui->transitionTimeSpinBox->value())).toString("HH:mm:ss.zzz"));
                    s("     <property name=\"a_track\">0</property>");
                    s("     <property name=\"b_track\">1</property>");
                    s("     <property name=\"factory\">loader</property>");
                    s("     <property name=\"mlt_service\">luma</property>");
                    s("   </transition>");
                    s("   <transition id=\"transition1\" out=\"%1\">", QTime::fromMSecsSinceStartOfDay(AGlobal().frames_to_msec(ui->transitionTimeSpinBox->value())).toString("HH:mm:ss.zzz"));
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
                previousProducerNr = iterationCounter;
                iterationCounter++;
            }
        }
    }

    iterationCounter = 0;
    int previousProducerNr = -1;

    foreach (AGViewRectItem *timelineItem, timelineGroupList)
    {
        AGViewRectItem *groupItem = (AGViewRectItem *)timelineItem->parentItem();

        s("  <playlist id=\"playlist%1\">", groupItem->fileInfo.fileName());

        if (groupItem->fileInfo.fileName() == "Audio")
        {
            s("    <property name=\"shotcut:audio\">1</property>");
            s("    <property name=\"shotcut:name\">A1</property>");
        }
        else
        {
            s("    <property name=\"shotcut:video\">1</property>");
            s("    <property name=\"shotcut:name\">V1</property>");
        }


        foreach (QGraphicsItem *item, timelineItem->filteredClips)
        {
            AGClipRectItem *clipItem = (AGClipRectItem *)item;
//            qDebug()<<"  Clip"<<clipItem->itemToString();

            QTime inTime = QTime::fromMSecsSinceStartOfDay(clipItem->clipIn);
            QTime outTime = QTime::fromMSecsSinceStartOfDay(clipItem->clipOut); //not needed to add one frame...

            if (ui->transitionTimeSpinBox->value() > 0)
            {
                if (item != timelineItem->filteredClips.first())
                {
                    s("    <entry producer=\"tractor%1\" in=\"00:00:00.000\" out=\"%2\"/>", QString::number(previousProducerNr) + "-" + QString::number(iterationCounter), QTime::fromMSecsSinceStartOfDay(AGlobal().frames_to_msec(ui->transitionTimeSpinBox->value())).toString("HH:mm:ss.zzz"));
                    inTime = inTime.addMSecs(AGlobal().frames_to_msec(ui->transitionTimeSpinBox->value())); //subtract half of the transitionframes
                }
                if (item != timelineItem->filteredClips.last())
                {
                    outTime = outTime.addMSecs(- AGlobal().frames_to_msec(ui->transitionTimeSpinBox->value())); //subtract half of the transitionframes
                }
            }

            s("    <entry producer=\"producer%1\" in=\"%2\" out=\"%3\"/>", QString::number(iterationCounter), inTime.toString("HH:mm:ss.zzz"), outTime.toString("HH:mm:ss.zzz") );

            previousProducerNr = iterationCounter;
            iterationCounter++;
        }

        s("  </playlist>"); //playlist0..x
    }

    if (QSettings().value("watermarkFileName").toString() != "No Watermark")
    {
//            QImage myImage;
//            myImage.load(watermarkFileName);

//            myImage.width();

        s("  <producer id=\"producer%1\" title=\"Anonymous Submission\" in=\"00:00:00.000\" out=\"%2\">", "WM", "03:59:59.960");
        s("    <property name=\"length\">%1</property>", "04:00:00.000");
        s("    <property name=\"resource\">%1</property>", QSettings().value("watermarkFileName").toString());

        s("     <filter id=\"filter%1\" out=\"%2\">", "WM", QTime::fromMSecsSinceStartOfDay(maxDurationMsec).toString("hh:mm:ss.zzz"));
        s("       <property name=\"background\">colour:0</property>");
        s("       <property name=\"mlt_service\">affine</property>");
        s("       <property name=\"shotcut:filter\">affineSizePosition</property>");
        s("       <property name=\"transition.fill\">1</property>");
        s("       <property name=\"transition.distort\">0</property>");
        s("       <property name=\"transition.rect\">%1 %2 %3 %4 1</property>", QString::number(exportWidth * 0.85),  QString::number(exportHeight * 0.85), QString::number(exportWidth * 0.15),  QString::number(exportHeight * 0.15));
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
        s("      <entry producer=\"producer%1\" in=\"00:00:00.000\" out=\"%2\"/>", "WM", QTime::fromMSecsSinceStartOfDay(maxDurationMsec).toString("hh:mm:ss.zzz"));
        s("  </playlist>");
    }

    s("  <tractor id=\"tractor%1\" title=\"Shotcut v19.10.20 by Media Sidekick v%2\">", "Main", qApp->applicationVersion());
    s("    <property name=\"shotcut\">1</property>");
    s("    <track producer=\"\"/>");

    foreach (AGViewRectItem *timelineItem, timelineGroupList)
    {
        AGViewRectItem *groupItem = (AGViewRectItem *)timelineItem->parentItem();

        if (groupItem->fileInfo.fileName() == "Video" && ui->exportVideoAudioSlider->value() == 0)
            s("    <track producer=\"playlist%1\" hide=\"audio\"/>", groupItem->fileInfo.fileName());
        else
            s("    <track producer=\"playlist%1\"/>", groupItem->fileInfo.fileName());
    }

    if (QSettings().value("watermarkFileName").toString() != "No Watermark")
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

void MExportDialog::exportPremiere()
{
    int maxDurationMsec = 0;
    foreach (AGViewRectItem *timelineItem, timelineGroupList)
    {
        int totalDurationMsec = 0;

        foreach (QGraphicsItem *item, timelineItem->filteredClips)
        {
            AGClipRectItem *clipItem = (AGClipRectItem *)item;

            QTime inTime = QTime::fromMSecsSinceStartOfDay(clipItem->clipIn);
            QTime outTime = QTime::fromMSecsSinceStartOfDay(clipItem->clipOut); //not needed to add one frame...

            totalDurationMsec += inTime.msecsTo(outTime) - AGlobal().frames_to_msec(ui->transitionTimeSpinBox->value() - 1); //-1: add 1 frame

            timelineItem->containsVideo = clipItem->mediaItem->exiftoolValueMap["VideoFrameRate"].value != "";
            timelineItem->containsAudio = clipItem->mediaItem->exiftoolValueMap["AudioSampleRate"].value != "" || clipItem->mediaItem->exiftoolValueMap["AudioBitrate"].value != ""; //AudioBitrate for mp3, AudioSampleRate for mp4
            //tbd check other formats then mp3 and 4
        }

        maxDurationMsec = qMax(maxDurationMsec, totalDurationMsec);
    }

    QFile fileWrite(QSettings().value("selectedFolderName").toString() + fileNameWithoutExtension + ".xml");
    fileWrite.open(QIODevice::WriteOnly);

    stream.setDevice(&fileWrite);

    s("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
    s("<!DOCTYPE xmeml>");
    s("<xmeml version=\"4\">");
//        s(" <project>");
//        s("  <name>%1</name>", fileNameWithoutExtension);
//        s("  <children>");
    s(" <sequence>");
//        s("  <duration>114</duration>");
    s("  <rate>");
    s("   <timebase>%1</timebase>", QString::number(exportFramerate));
//        s("   <ntsc>FALSE</ntsc>");
    s("  </rate>");
    s("  <name>%1</name>", fileNameWithoutExtension);
    s("  <media>");

    QStringList mediaTypeList;
//        if (groupItem->fileInfo.fileName() == "Video")
//            tracks = QStringList()<<"v"<<"a";
//        if (groupItem->fileInfo.fileName() == "Audio")
//            tracks = QStringList()<<"a";

//    if (groupItem->fileInfo.fileName() == "Video")
//    {
        mediaTypeList << "video";
//        if (ui->exportVideoAudioSlider->value() > 0)
//            tracks << "audio";
//    }
//    if (groupItem->fileInfo.fileName() == "Audio")
//    {
        mediaTypeList << "audio";
//    }


    foreach (QString mediaType, mediaTypeList)
    {
        s("   <%1>", mediaType);

        if (mediaType == "audio")
            s("    <numOutputChannels>%1</numOutputChannels>", "1");

        s("    <format>");
        s("     <samplecharacteristics>");

        if (mediaType == "video")
        {
            s("      <width>%1</width>", QString::number(exportWidth));
            s("      <height>%1</height>", QString::number(exportHeight));
        }
        else //audio
        {
//                s("      <depth>16</depth>");
//                s("      <samplerate>44100</samplerate>");
        }
        s("     </samplecharacteristics>");
        s("    </format>");

        foreach (AGViewRectItem *timelineItem, timelineGroupList)
        {
            AGViewRectItem *groupItem = (AGViewRectItem *)timelineItem->parentItem();

            if ((mediaType == "video" && timelineItem->containsVideo) || (mediaType == "audio" && timelineItem->containsAudio))
//            if ((groupItem->fileInfo.fileName() == "Video") || (groupItem->fileInfo.fileName() == "Audio" && mediaType == "audio"))
            {
//                qDebug()<<"Timeline"<<groupItem->fileInfo<<timelineItem->fileInfo.absolutePath()<<timelineItem->fileInfo.fileName();

                QMap<QString, FileStruct> filesMap;
                foreach (QGraphicsItem *item, timelineItem->filteredClips)
                {
                    AGClipRectItem *clipItem = (AGClipRectItem *)item;

                    filesMap[clipItem->fileInfo.absoluteFilePath()].folderName = clipItem->fileInfo.absolutePath();
                    filesMap[clipItem->fileInfo.absoluteFilePath()].fileName = clipItem->fileInfo.fileName();
                    filesMap[clipItem->fileInfo.absoluteFilePath()].counter = filesMap.count() - 1;
                    filesMap[clipItem->fileInfo.absoluteFilePath()].definitionGenerated = false;
                }

                addPremiereTrack(mediaType, timelineItem, filesMap);
            }

        } //for each timeline

        if (mediaType == "video" && QSettings().value("watermarkFileName").toString() != "No Watermark")
        {
            s("    <track>");

            QMap<QString, FileStruct> filesMap; //empty as definitionGenerated used only once

            addPremiereClipitem("WM", QFileInfo(QSettings().value("watermarkFileName").toString()), 0, AGlobal().msec_to_frames(maxDurationMsec), -1, -1, QString::number(exportFramerate), "video", &filesMap, 1, "", "", "");

            s("    </track>");
        }

        s("   </%1>", mediaType);

    } //for each mediatype

    s("  </media>");

    s(" </sequence>");
//        s("  </children>");
//        s(" </project>");
    s("</xmeml>");

    fileWrite.close();
} //exportPremiere

void MExportDialog::on_clipsFramerateComboBox_currentTextChanged(const QString &arg1)
{
    if (QSettings().value("frameRate") != arg1)
    {
        qDebug()<<"on_clipsFramerateComboBox_currentTextChanged"<<QSettings().value("frameRate")<<arg1;
        QSettings().setValue("frameRate", arg1);
        QSettings().sync();
    }
}

void MExportDialog::on_transitionDial_valueChanged(int value)
{
    if (transitionValueChangedBy != "SpinBox") //do not change the spinbox if the spinbox triggers the change
    {
        ui->transitionTimeSpinBox->setValue(value);
//        transitionValueChangedBy = "";
    }
}

void MExportDialog::on_transitionDial_sliderMoved(int position)
{
    transitionValueChangedBy = "Dial";
}

void MExportDialog::on_transitionTimeSpinBox_valueChanged(int arg1)
{
    if (QSettings().value("transitionTime") != arg1)
    {
//        qDebug()<<"MExportDialog::on_transitionTimeSpinBox_valueChanged"<<arg1<<transitionValueChangedBy;
        QSettings().setValue("transitionTime", arg1);
        QSettings().sync();

//                    emit timelineWidgetsChanged(ui->transitionTimeSpinBox->value(), ui->transitionComboBox->currentText(), ui->clipsTableView);
        transitionValueChangedBy = "SpinBox";
        ui->transitionDial->setValue(arg1);
    }
}

void MExportDialog::on_transitionComboBox_currentTextChanged(const QString &arg1)
{
    if (QSettings().value("transitionType") != arg1)
    {
//        qDebug()<<"MExportDialog::on_transitionComboBox_currentTextChanged"<<arg1;
        QSettings().setValue("transitionType", arg1);
        QSettings().sync();
//                    emit timelineWidgetsChanged(transitionTimeSpinBox->value(), transitionComboBox->currentText(), ui->clipsTableView);
    }
}

void MExportDialog::on_exportTargetComboBox_currentTextChanged(const QString &arg1)
{
//    qDebug()<<"MExportDialog::on_exportTargetComboBox_currentTextChanged"<<arg1;
    if (QSettings().value("exportTarget") != arg1)
    {
        QSettings().setValue("exportTarget", arg1);
        QSettings().sync();
    }

    ui->exportSizeComboBox->setEnabled(arg1 != "Lossless");
    ui->exportFramerateComboBox->setEnabled(arg1 != "Lossless");
    ui->watermarkButton->setEnabled(arg1 != "Lossless");
    ui->watermarkLabel->setEnabled(arg1 != "Lossless");
    ui->transitionComboBox->setEnabled(arg1 != "Lossless" && arg1 != "Encode");
}

void MExportDialog::on_exportFramerateComboBox_currentTextChanged(const QString &arg1)
{
    if (QSettings().value("exportFramerate") != arg1)
    {
        QSettings().setValue("exportFramerate", arg1);
        QSettings().sync();
    }

    if (arg1 != "Source" || ui->clipsFramerateComboBox->currentText().toInt() == 0)
    {
        exportFramerate = arg1.toInt();
    }
    else //source
    {
        exportFramerate = ui->clipsFramerateComboBox->currentText().toInt();
    }
}

void MExportDialog::on_exportSizeComboBox_currentTextChanged(const QString &arg1)
{
    if (QSettings().value("exportSize") != arg1)
    {
        QSettings().setValue("exportSize", arg1);
        QSettings().sync();
    }

    int indexOfX = ui->clipsSizeComboBox->currentText().indexOf(" x ");
    int clipsWidth = ui->clipsSizeComboBox->currentText().left(indexOfX).toInt();
    int clipsHeight = ui->clipsSizeComboBox->currentText().mid(indexOfX + 3).toInt();
//    qDebug()<<"exportSizeComboBox::currentTextChanged clips"<< ui->clipsSizeComboBox->currentText()<<indexOfX<<clipsWidth<<clipsHeight;

    if (arg1 != "Source" || clipsWidth == 0)
    {
        int indexOfX = arg1.indexOf(" x ");
        int indexOfLeftBracket = arg1.indexOf("(");
        int indexOfRightBracket = arg1.indexOf(")");

        exportHeight = arg1.mid(indexOfX + 3, indexOfRightBracket - (indexOfX + 3)).toInt();

        if (clipsWidth != 0 && clipsHeight != 0)
            exportWidth = int(exportHeight * clipsWidth / clipsHeight / 2.0) * 2; //maintain aspect ratio. Should be dividable by 2
        else
            exportWidth = arg1.mid(indexOfLeftBracket, indexOfX - indexOfLeftBracket + 1).toInt();

        exportSizeShortName = arg1.left(indexOfLeftBracket - 1);
//        qDebug()<<"exportSizeComboBox::currentTextChanged export"<<arg1<<indexOfX<<indexOfLeftBracket<<indexOfRightBracket<<exportWidth<<exportHeight<<exportSizeShortName;
    }
    else //source
    {
        exportHeight = clipsHeight;
        exportWidth = clipsWidth;

        exportSizeShortName = QString::number(clipsHeight) + "p";
//        qDebug()<<"exportSizeComboBox::currentTextChanged source"<<arg1<<exportWidth<<exportHeight<<exportSizeShortName;
    }
}

void MExportDialog::on_exportVideoAudioSlider_valueChanged(int value)
{
//    qDebug()<<"MExportDialog::on_exportVideoAudioSlider_valueChanged"<<QSettings().value("exportVideoAudioSlider")<<value;
    if (value != QSettings().value("exportVideoAudioSlider"))
    {
        QSettings().setValue("exportVideoAudioSlider", value);
        QSettings().sync();
    }

}

void MExportDialog::watermarkFileNameChanged(QString newFileName)
{
    if (newFileName == "")
        newFileName = "No Watermark";

    if (newFileName != QSettings().value("watermarkFileName"))
    {
        QSettings().setValue("watermarkFileName", newFileName);
        QSettings().sync();
    }

    if (newFileName != "No Watermark")
    {

        QImage myImage;
        myImage.load(newFileName);

        ui->watermarkLabel->setPixmap(QPixmap::fromImage(myImage).scaled(QSize(50,50)));

        ui->watermarkLabel->show();
    }
    else
    {
        ui->watermarkLabel->setText("No Watermark");
    }
}

void MExportDialog::on_watermarkButton_clicked()
{
    if (ui->watermarkLabel->text() == "No Watermark")
    {
#ifdef Q_OS_WIN
        ui->watermarkLabel->setText(QFileDialog::getOpenFileName(ui->watermarkButton,
            tr("Open Image"), QSettings().value("selectedFolderName").toString(), tr("Image Files (*.png *.jpg *.bmp *.ico)")));
#else
        ui->watermarkLabel->setText(QFileDialog::getOpenFileName(ui->watermarkButton,
            tr("Open Image"), QDir::home().homePath(), tr("Image Files (*.png *.jpg *.bmp *.ico)")));
#endif
    }
    else
    {
        ui->watermarkLabel->setText("No Watermark");
    }
    watermarkFileNameChanged(ui->watermarkLabel->text());

}

bool isChild(QGraphicsItem *parent, QGraphicsItem *child)
{
    bool answer;
    if (parent == child)
        answer = true;
    else if (child->parentItem() == nullptr)
        answer = false;
    else
        answer = isChild(parent, child->parentItem());

//    qDebug()<<"isChild"<<parent->data(fileNameIndex).toString()<<child->data(fileNameIndex).toString()<<child->parentItem()->data(fileNameIndex).toString()<<answer;
    return answer;
}


void MExportDialog::on_exportButton_clicked()
{
    //                createContextSensitiveHelp("Export started");

    int transitionTime = 0;
//    if (transitionComboBox->currentText() != "No transition")
    transitionTime = ui->transitionTimeSpinBox->value();
//    qDebug()<<"MExportDialog::on_exportButton_clicked";

    timelineGroupList.clear();

    foreach (QGraphicsItem *item, rootItem->scene()->items()) //Folders
    {
        if (item->data(mediaTypeIndex).toString() == "Folder" && item->data(itemTypeIndex).toString() == "Base")
        {
            AGFolderRectItem *folderItem = (AGFolderRectItem *)item;
//                qDebug()<<"Folder"<<folderItem->fileInfo.fileName()<<folderItem->focusProxy()->data(mediaTypeIndex)<<folderItem->focusProxy()->data(fileNameIndex);

            foreach (QGraphicsItem *item, rootItem->scene()->items(Qt::AscendingOrder)) //fileGroups
            {
                if (item->parentItem() == folderItem && item->data(mediaTypeIndex).toString() == "FileGroup" && item->data(itemTypeIndex).toString() == "Base")
                {
                    MGroupRectItem *fileGroup = (MGroupRectItem *)item;
                    AGViewRectItem *timelineItem = (AGViewRectItem *)fileGroup->focusProxy();

                    //check if descendent of rootitem

                    timelineItem->filteredClips.clear();

                    if (fileGroup->fileInfo.fileName() != "Parking" && isChild(rootItem, fileGroup))
                    {
//                            qDebug()<<"  Group"<<fileGroup->fileInfo.fileName()<<fileGroup->focusProxy()->data(mediaTypeIndex)<<fileGroup->focusProxy()->data(fileNameIndex);

                        foreach (QGraphicsItem *item, timelineItem->clips)
                        {
                            AGClipRectItem *clipItem = (AGClipRectItem *)item;

//                            qDebug()<<"excludedInFilter"<<clipItem->fileInfo.fileName()<<clipItem->data(excludedInFilter).toBool();

                            if (!clipItem->data(excludedInFilter).toBool())
                                timelineItem->filteredClips << clipItem;
                        }

                        if (timelineItem->filteredClips.count() > 0)
                            timelineGroupList << timelineItem;
                    }
                }
            }
        }
    }

    fileNameWithoutExtension = ui->exportTargetComboBox->currentText();
    if (ui->exportTargetComboBox->currentText() != "Lossless")
    {
        fileNameWithoutExtension += exportSizeShortName;
        fileNameWithoutExtension += "@" + QString::number(exportFramerate);
    }

    if (ui->exportTargetComboBox->currentText() == "Lossless")
    {
        losslessVideoAndAudio();
    }
    else if (ui->exportTargetComboBox->currentText() == "Encode")
    {
        encodeVideoClips();
    }
    else if (ui->exportTargetComboBox->currentText() == "Shotcut")
    {
        exportShotcut();
    }
    else if (ui->exportTargetComboBox->currentText() == "Premiere")
    {
        exportPremiere();
    }


//                AExport *exportWidget = new AExport;

//                connect(exportWidget, &AExport::jobAddLog, jobTreeView, &AJobTreeView::onJobAddLog);
//                connect(exportWidget, &AExport::propertyCopy, propertyTreeView, &APropertyTreeView::onPropertyCopy);
//                connect(exportWidget, &AExport::releaseMedia, videoWidget, &AVideoWidget::onReleaseMedia);

//                exportWidget->exportClips(scene()->items(), exportTargetComboBox->currentText(), exportSizeComboBox->currentText(), exportFramerateComboBox->currentText(), transitionTime, exportVideoAudioSlider, watermarkLabel, clipsFramerateComboBox, clipsSizeComboBox);

//    if (QSettings().value("firstUsedDate") == QVariant())
//    {
//        QSettings().setValue("firstUsedDate", QDateTime::currentDateTime().addDays(-1));
//        QSettings().sync();
//    }
//    qint64 days = QSettings().value("firstUsedDate").toDateTime().daysTo(QDateTime::currentDateTime());

    QSettings().setValue("exportCounter", QSettings().value("exportCounter").toInt()+1);
    QSettings().sync();
//    qDebug()<<"exportCounter"<<QSettings().value("exportCounter").toInt();
    if (QSettings().value("exportCounter").toInt() == 50)
    {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(ui->exportButton, "Donate", tr("If you like this software, consider to make a donation. Go to the Donate page on the Media Sidekick website for more information (%1 exports made)").arg(QSettings().value("exportCounter").toString()), QMessageBox::Yes|QMessageBox::No);
        if (reply == QMessageBox::Yes)
            QDesktopServices::openUrl(QUrl("https://www.mediasidekick.org/support"));
    }
    this->close();

}
