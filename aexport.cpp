#include "agviewrectitem.h"
#include "aexport.h"
#include "mgrouprectitem.h"
#include "ui_mexportdialog.h"

#include <QDebug>
#include <QDesktopServices>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QTimer>

#include <qmath.h>

#include <QGraphicsProxyWidget>

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

    ui->transitionDurationTimeEdit->setFixedWidth(120);

    ui->watermarkLabel->setText(QSettings().value("watermarkFileName").toString());
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

void MExportDialog::loadSettings()
{
    timelineGroupList.clear();

    int foundClips = 0;
    int foundClipDuration = 0;

    foreach (QGraphicsItem *item, rootItem->scene()->items()) //Folders
    {
        if (item->data(mediaTypeIndex).toString() == "Folder" && item->data(itemTypeIndex).toString() == "Base")
        {
            AGFolderRectItem *folderItem = (AGFolderRectItem *)item;

//            qDebug()<<"Folder"<<folderItem->fileInfo.fileName();

            foreach (QGraphicsItem *item, rootItem->scene()->items(Qt::AscendingOrder)) //fileGroups
            {
                foundClipDuration = 0;
                if (item->parentItem() == folderItem && item->data(mediaTypeIndex).toString() == "FileGroup" && item->data(itemTypeIndex).toString() == "Base")
                {
                    MGroupRectItem *fileGroup = (MGroupRectItem *)item;
//                    qDebug()<<"Filegroup"<<fileGroup->fileInfo.fileName();

                    //check if descendent of rootitem

                    fileGroup->timelineGroupItem->filteredClips.clear();

                    if (fileGroup->fileInfo.fileName() != "Parking")
                    {
//                        qDebug()<<"  Group"<<fileGroup->fileInfo.fileName();

                        foreach (AGClipRectItem *clipItem, fileGroup->timelineGroupItem->clips)
                        {
                            bool cntnue;
                            if (strcmp(rootItem->metaObject()->className() , "AGMediaFileRectItem") == 0)
                                cntnue = clipItem->mediaItem == rootItem;
                            else
                                cntnue = isChild(rootItem, fileGroup);

                            if (cntnue && !clipItem->data(excludedInFilter).toBool())
                            {
//                                qDebug()<<"add to filteredClips"<<fileGroup->fileInfo.fileName()<<clipItem->fileInfo.fileName()<<clipItem->data(excludedInFilter).toBool()<<clipItem->clipIn;
                                fileGroup->timelineGroupItem->filteredClips << clipItem;

                                foundClips++;
                                foundClipDuration += clipItem->duration;

                                if (clipItem->mediaItem->groupItem->fileInfo.fileName() == "Video")
                                {
                                    QString clipsSize = clipItem->mediaItem->exiftoolPropertyMap["ImageWidth"].value + " x " + clipItem->mediaItem->exiftoolPropertyMap["ImageHeight"].value;
                                    if (clipsSize != " x " && ui->clipsSizeComboBox->findText(clipsSize) == -1)
                                        ui->clipsSizeComboBox->addItem(clipsSize);

                                    int clipsFrameRate = qRound(clipItem->mediaItem->exiftoolPropertyMap["VideoFrameRate"].value.toDouble());
                                    if (clipsFrameRate != 0 && ui->clipsFramerateComboBox->findText(QString::number(clipsFrameRate)) == -1)
                                        ui->clipsFramerateComboBox->addItem(QString::number(clipsFrameRate));
                                }
                            }
                        }

                        if (fileGroup->timelineGroupItem->filteredClips.count() > 0)
                        {
                            //sort the filtered clips in clipIn (they can be unsorted in srt files!!!
                            std::sort(fileGroup->timelineGroupItem->filteredClips.begin(), fileGroup->timelineGroupItem->filteredClips.end(), [](AGClipRectItem *v1, AGClipRectItem * v2)->bool
                            {
                                return v1->zValue() < v2->zValue();
                            });

                            timelineGroupList << fileGroup->timelineGroupItem;

                            QGroupBox *groupBox = new QGroupBox(ui->timelinesGroupBox);
                            groupBox->setTitle(fileGroup->fileInfo.fileName());
                            ui->timelinesGroupBox->layout()->addWidget(groupBox);

                            QFormLayout *groupBoxLayout = new QFormLayout(groupBox);
                            groupBox->setLayout(groupBoxLayout);

                            QSlider *audioLevelSlider = new QSlider(groupBox);
                            audioLevelSlider->setMaximum(100);
                            audioLevelSlider->setSingleStep(10);
                            audioLevelSlider->setOrientation(Qt::Horizontal);
                            audioLevelSlider->setTickPosition(QSlider::TicksBelow);
                            audioLevelSlider->setTickInterval(10);
                            if (QSettings().value(fileGroup->fileInfo.fileName() + "AudioLevelSlider").toInt() != audioLevelSlider->value())
                                audioLevelSlider->setValue(QSettings().value(fileGroup->fileInfo.fileName() + "AudioLevelSlider").toInt());

                            connect(audioLevelSlider, &QSlider::valueChanged, [=] (int value)
                            {
                                if (value != QSettings().value(fileGroup->fileInfo.fileName() + "AudioLevelSlider"))
                                {
                                    QSettings().setValue(fileGroup->fileInfo.fileName() + "AudioLevelSlider", value);
                                    QSettings().sync();
                                }
                                emit arrangeItems();
                            });

                            foundClipDuration -= (fileGroup->timelineGroupItem->filteredClips.count() - 1) * QSettings().value("transitionTime").toInt();

                            groupBoxLayout->addRow("# clips", new QLabel(QString::number(fileGroup->timelineGroupItem->filteredClips.count())));
                            groupBoxLayout->addRow("Duration", new QLabel(QTime::fromMSecsSinceStartOfDay(foundClipDuration).toString()));
                            groupBoxLayout->addRow("Audio level", audioLevelSlider);
                            if (fileGroup->fileInfo.fileName() == "Video")
                            {
                            }
                        }
                    }
                }
            }
        }
    }

    if (foundClips == 0)
    {
        QMessageBox::information(this, "Export", tr("No clips found to export"));

//        QTimer::singleShot(0, this, [=]()->void //first constructor needs to finish
//        {
//            qDebug()<<"MExportDialog::loadSettings close"<<this->close();
//        });

        return;
    }

    setWindowTitle(windowTitle() + " " + rootItem->fileInfo.fileName());

    ui->clipsSizeLabel->setText(ui->clipsSizeLabel->text() + " (" + QString::number(ui->clipsSizeComboBox->count()) + ")");
    ui->clipsFramerateLabel->setText(ui->clipsFramerateLabel->text() + " (" + QString::number(ui->clipsFramerateComboBox->count()) + ")");

    if (ui->clipsSizeComboBox->count() == 0 || ui->clipsFramerateComboBox->count() == 0)
    {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Clips", tr("No clip sizes or framerates found. Cancel export?"), QMessageBox::Yes|QMessageBox::No);
//        qDebug()<<"MExportDialog::loadSettings"<<reply;
        if (reply == QMessageBox::Yes)
        {
//            QTimer::singleShot(0, this, [=]()->void //first constructor needs to finish
//            {
//                qDebug()<<"MExportDialog::loadSettings close"<<this->close();
//            });

            return;
        }
    }

    if (QSettings().value("clipsFramerateComboBox").toString() != "")
        ui->clipsFramerateComboBox->setCurrentText(QSettings().value("clipsFramerateComboBox").toString());

    if (QSettings().value("clipsSizeComboBox").toString() != "")
        ui->clipsSizeComboBox->setCurrentText(QSettings().value("clipsSizeComboBox").toString());

    ui->transitionComboBox->setCurrentText(QSettings().value("transitionType").toString());
    ui->transitionDurationTimeEdit->setTime(QTime::fromMSecsSinceStartOfDay(QSettings().value("transitionTime").toInt()));
    ui->transitionDial->setValue(ui->transitionDurationTimeEdit->time().msecsSinceStartOfDay() / 100);

//    connect(exportTargetComboBox, &QComboBox::currentTextChanged, this, &MainWindow::on_exportTargetComboBox_currentTextChanged);
    if (ui->exportTargetComboBox->currentText() == QSettings().value("exportTarget").toString())
        on_exportTargetComboBox_currentTextChanged(ui->exportTargetComboBox->currentText());
    else
        ui->exportTargetComboBox->setCurrentText(QSettings().value("exportTarget").toString());

//    if (QSettings().value("exportSize").toString() == "")
//        ui->exportSizeComboBox->setCurrentText("Selected size"); //will trigger currentTextChanged
//    else
    {
        QStringList itemsInComboBox;
        for (int index = 0; index < ui->exportSizeComboBox->count(); index++)
            itemsInComboBox << ui->exportSizeComboBox->itemText(index);

        if (ui->exportSizeComboBox->currentText() == QSettings().value("exportSize").toString())
            on_exportSizeComboBox_currentTextChanged(ui->exportSizeComboBox->currentText());
        else if (itemsInComboBox.contains(QSettings().value("exportSize").toString()))
            ui->exportSizeComboBox->setCurrentText(QSettings().value("exportSize").toString());
        else
            ui->exportSizeComboBox->setCurrentText("Selected size");
    }

//    if (QSettings().value("exportFrameRate").toString() == "")
//        ui->exportFramerateComboBox->setCurrentText("Selected framerate");  //will trigger currentTextChanged
//    else
    {
        QStringList itemsInComboBox;
        for (int index = 0; index < ui->exportFramerateComboBox->count(); index++)
            itemsInComboBox << ui->exportFramerateComboBox->itemText(index);

        if (ui->exportFramerateComboBox->currentText() == QSettings().value("exportFrameRate").toString())
            on_exportFramerateComboBox_currentTextChanged(ui->exportFramerateComboBox->currentText());
        else if (itemsInComboBox.contains(QSettings().value("exportFrameRate").toString()))
            ui->exportFramerateComboBox->setCurrentText(QSettings().value("exportFrameRate").toString());
        else
            ui->exportFramerateComboBox->setCurrentText("Selected framerate");
    }

    ui->exportTableTextBrowser->setMinimumHeight(300);
    ui->exportTableTextBrowser->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse);

    QString text = "<h1>Features</h1><p><i>Features per target</i></p>"
            "<table border=1>\
            <tr><th></th><th>Lossless<sup><a href=\"#fn1\" id=\"ref1\">1</a></sup></th><th>Encode</th><th>Shotcut</th><th>Premiere</th></tr>\
            <tr><td><b>Track audio levels</b></td><td align=\"center\">✔</td><td align=\"center\">✔</td><td align=\"center\">✔</td><td align=\"center\">✔</td></tr>\
            <tr><td><b>Transitions</b></td><td align=\"center\">-<sup><a href=\"#fn2\" id=\"ref2\">2</a></sup></td><td align=\"center\">✔</td><td align=\"center\">✔</td><td align=\"center\">✔</td></tr>\
            <tr><td><b>Watermark</b></td><td align=\"center\">-</td><td align=\"center\">✔</td><td align=\"center\">✔</td><td align=\"center\">✔</td></tr>\
            <tr><td><b>Audio fade in/out</b></td><td align=\"center\">-</td><td align=\"center\">-</td><td align=\"center\">✔</td><td align=\"center\">✔</td></tr>\
            <tr><td><b>Speed</b></td><td align=\"center\">-</td><td align=\"center\">WIP<sup><a href=\"#fn3\" id=\"ref3\">3</a></sup></td><td align=\"center\">✔</td><td align=\"center\">✔</td></tr>\
            </table>\
            <ol>\
            <li><span id=\"fn1\">Lossless only works well if all clips used have the same codec, framerate, width and height</span><sup><a href=\"#ref1\" title=\"Jump back to footnote 1 in the text.\">↩</a></sup></li>\
            <li><span id=\"fn2\">duration preserved by cut in the middle</span><sup><a href=\"#ref2\" title=\"Jump back to footnote 2 in the text.\">↩</a></sup></li>\
            <li><span id=\"fn3\">In progress</span><sup><a href=\"#ref32\" title=\"Jump back to footnote 3 in the text.\">↩</a></sup></li>\
            </ol>\
            ";



    ui->exportTableTextBrowser->setHtml(text);

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
                                      "<p><i>Sets the transition duration and effect for the exported video</i></p>"
                                      "<ul>"
                                      "<li>Duration: Duration of the transition in seconds</li>"
                                      "<li>Effect: currently only Cross Dissolve supported</li>"
                                      "<li>No transitions if duration is set to 0</li>"
                                      "<li>Duration and effect apply to all transitions</li>"
                                      "</ul>"));
    ui->transitionComboBox->setToolTip( ui->transitionDial->toolTip());
    ui->transitionDurationTimeEdit->setToolTip( ui->transitionDial->toolTip());


    ui->exportTargetComboBox->setToolTip(tr("<p><b>Export target</b></p>"
                                 "<p><i>Determines what will be exported</i></p>"
                                 "<ul>"
                                              "<li>Lossless: FFMpeg generated video file. Very fast! If codec, size and framerate of clips is not the same the result might not play correctly</li>"
                                              "<li>Encode: FFMpeg generated video file</li>"
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
                                                 "<li>Source: if source selected than the selected framerate of the clips is used</li>"
                                                 "</ul>"));

//    ui->exportVideoAudioSlider->setToolTip(tr("<p><b>Video Audio volume</b></p>"
//                                              "<p><i>Sets the volume of the original video</i></p>"
//                                              "<ul>"
//                                              "<li>Remark: This only applies to the videos. Audio files are always played at 100%</li>"
//                                              "<li>Remark: Volume adjustments are also effective in Media Sidekick when playing video files</li>"
//                                              "</ul>"));


    ui->watermarkLabel->setToolTip(tr("<p><b>Watermark</b></p>"
                                      "<p><i>Adds a watermark / logo on the bottom right of the exported video</i></p>"
                                      "<ul>"
                                      "<li>Button: If no watermark then browse for an image. If a watermark is already selected, clicking the button will remove the watermark</li>"
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
    foreach (MTimelineGroupRectItem *timelineItem, timelineGroupList)
    {
//        MGroupRectItem *groupItem = timelineItem->groupItem;
//        qDebug()<<"Timeline"<<groupItem->fileInfo<<timelineItem->fileInfo.absolutePath()<<timelineItem->fileInfo.fileName();

        QString recycleFolderName = QSettings().value("selectedFolderName").toString() + "MSKRecycleBin/";
        QDir recycleDir(recycleFolderName);
        if (!recycleDir.exists())
            recycleDir.mkpath(".");

        QFile vidlistFile(recycleFolderName + fileNameWithoutExtension + timelineItem->groupItem->fileInfo.completeBaseName() + ".txt");
//        qDebug()<<"opening vidlistfile"<<vidlistFile;

        if ( vidlistFile.open(QIODevice::WriteOnly) )
        {
            QTextStream vidlistStream( &vidlistFile );

            foreach (QGraphicsItem *item, timelineItem->filteredClips)
            {
                AGClipRectItem *clipItem = (AGClipRectItem *)item;
//                qDebug()<<"  Clip"<<clipItem->itemToString();

                QTime inTime = QTime::fromMSecsSinceStartOfDay(clipItem->clipIn);
                QTime outTime = QTime::fromMSecsSinceStartOfDay(clipItem->clipOut);

                vidlistStream << "file '" << clipItem->fileInfo.absoluteFilePath() << "'" << Qt::endl;
                if (ui->transitionDurationTimeEdit->time().msecsSinceStartOfDay() > 0)
                {
                    if (item != timelineItem->filteredClips.first())
                    {
                        inTime = inTime.addMSecs(ui->transitionDurationTimeEdit->time().msecsSinceStartOfDay() / 2); //subtract half of the transitionframes 2, not 2.0
                    }
                    if (item != timelineItem->filteredClips.last())
                    {
                        outTime = outTime.addMSecs(- qRound(ui->transitionDurationTimeEdit->time().msecsSinceStartOfDay() / 2.0)); //subtract half of the transitionframes
                    }
                }

                vidlistStream << "inpoint " <<  QString::number(inTime.msecsSinceStartOfDay() / 1000.0, 'g', 6) << Qt::endl;
                vidlistStream << "outpoint " << QString::number((outTime.msecsSinceStartOfDay()) / 1000.0, 'g', 6) << Qt::endl;

//                qDebug()<<"VideoAudioCheck"<<clipItem->fileInfo.fileName()<<clipItem->mediaItem->exiftoolValueMap["VideoFrameRate"].value<<clipItem->mediaItem->exiftoolValueMap["AudioSampleRate"].value<<clipItem->mediaItem->exiftoolValueMap["AudioBitrate"].value;

                bool containsVideo = clipItem->mediaItem->exiftoolPropertyMap["VideoFrameRate"].value != "";
                bool containsAudio = clipItem->mediaItem->exiftoolPropertyMap["AudioSampleRate"].value != "" || clipItem->mediaItem->exiftoolPropertyMap["AudioBitrate"].value != "" || clipItem->mediaItem->exiftoolPropertyMap["AudioChannels"].value != ""; //AudioBitrate for mp3, AudioSampleRate for mp4, AudioChannels for mkv;
                if (clipItem->mediaItem->exiftoolPropertyMap["MIMEType"].value.contains("video"))
                {
                    timelineItem->containsVideo = timelineItem->containsVideo || containsVideo;
                    timelineItem->containsAudio = timelineItem->containsAudio || containsAudio;
                }
                else if (clipItem->mediaItem->exiftoolPropertyMap["MIMEType"].value.contains("audio"))
                {
                    timelineItem->containsVideo = false;
                    timelineItem->containsAudio = timelineItem->containsAudio || containsAudio;
                }

                //tbd check other formats then mp3 and 4

            } //for each clip

            vidlistFile.close();

            QString sourceFolderFileName = recycleFolderName + fileNameWithoutExtension + timelineItem->groupItem->fileInfo.completeBaseName() + ".txt";
            QString targetFolderFileName = recycleFolderName + fileNameWithoutExtension + timelineItem->groupItem->fileInfo.completeBaseName() + ".mp4";

#ifdef Q_OS_WIN
            sourceFolderFileName = sourceFolderFileName.replace("/", "\\");
            targetFolderFileName = targetFolderFileName.replace("/", "\\");
#endif
            QString command = "ffmpeg -f concat -safe 0 -i \"" + sourceFolderFileName + "\" -c copy -y \"" + targetFolderFileName + "\"";

            QSlider *audioLevelSlider = (QSlider *)timelineItem->groupItem->audioLevelSliderProxy->widget();
            if (timelineItem->containsAudio && audioLevelSlider->value() == 0) //remove audio
                command.replace("-c copy -y", " -an -c copy -y");

//            if (frameRate != "")
//                command.replace("-c copy -y", " -r " + frameRate + " -c copy -y");

            AGProcessAndThread *process = new AGProcessAndThread(this);
            process->command("FFMpeg lossless" + timelineItem->groupItem->fileInfo.completeBaseName(), command);
            *processes<<process;
            connect(process, &AGProcessAndThread::processOutput, this, &MExportDialog::onProcessOutput);
            connect(process, &AGProcessAndThread::processOutput, [=] (QTime time, QTime totalTime, QString event, QString outputString)
            {
                if (event == "finished") //go mux
                {
                    if (process->errorMessage == "")
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
                    else
                        QMessageBox::information(this, "Error " + process->name, process->errorMessage);
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
    bool differentFrameRateFound = false;

    QString recycleFolderName = QSettings().value("selectedFolderName").toString() + "MSKRecycleBin/";

    foreach (MTimelineGroupRectItem *timelineItem, timelineGroupList)
    {
        QStringList ffmpegFiles;
        QStringList ffmpegClips;
        QStringList ffmpegCombines;
        QStringList ffmpegMappings;

//        MGroupRectItem *groupItem = (MGroupRectItem *)timelineItem->parentItem();
//        qDebug()<<"Timeline"<<groupItem->fileInfo<<timelineItem->fileInfo.absolutePath()<<timelineItem->fileInfo.fileName();

        int totalDurationMsec = 0;
        QString lastV, lastA;

        foreach (QGraphicsItem *item, timelineItem->filteredClips)
        {
            AGClipRectItem *clipItem = (AGClipRectItem *)item;
//            qDebug()<<"  Clip"<<clipItem->itemToString();

            qreal speed = 1;
            foreach (AGTagTextItem *tagItem, clipItem->tags)
            {
                if (tagItem->tagName.contains("speed", Qt::CaseInsensitive))
                {
                    qreal foundSpeed = tagItem->tagName.mid(5).toDouble();
                    if (foundSpeed != 0)
                        speed = foundSpeed;
                }
            }

            QTime inTime = QTime::fromMSecsSinceStartOfDay(clipItem->clipIn / speed);
            QTime outTime = QTime::fromMSecsSinceStartOfDay(clipItem->clipOut / speed);

            int clipInRounded = clipItem->clipIn / 1000 / 10 * 10; //round to 10s of seconds

//            AGMediaFileRectItem *mediaItem = clipItem->mediaItem;

            if (qRound(clipItem->mediaItem->exiftoolPropertyMap["VideoFrameRate"].value.toDouble()) != exportFramerate)
                 differentFrameRateFound = true;

            QString trimOffsetFFMpeg = "";
            if (clipInRounded > 0)
                trimOffsetFFMpeg = "-ss " + QString::number(clipInRounded) + " ";

            ffmpegFiles << trimOffsetFFMpeg + "-i \"" + clipItem->fileInfo.absoluteFilePath() + "\"";

            int fileReference = ffmpegFiles.count() - 1;

//            qDebug()<<"VideoAudioCheck"<<clipItem->fileInfo.fileName()<<clipItem->mediaItem->exiftoolValueMap["VideoFrameRate"].value<<clipItem->mediaItem->exiftoolValueMap["AudioSampleRate"].value<<clipItem->mediaItem->exiftoolValueMap["AudioBitrate"].value;
            bool containsVideo = clipItem->mediaItem->exiftoolPropertyMap["VideoFrameRate"].value != "";
            bool containsAudio = clipItem->mediaItem->exiftoolPropertyMap["AudioSampleRate"].value != "" || clipItem->mediaItem->exiftoolPropertyMap["AudioBitrate"].value != "" || clipItem->mediaItem->exiftoolPropertyMap["AudioChannels"].value != ""; //AudioBitrate for mp3, AudioSampleRate for mp4, AudioChannels for mkv;
            if (clipItem->mediaItem->exiftoolPropertyMap["MIMEType"].value.contains("video"))
            {
                timelineItem->containsVideo = timelineItem->containsVideo || containsVideo;
                timelineItem->containsAudio = timelineItem->containsAudio || containsAudio;
            }
            else if (clipItem->mediaItem->exiftoolPropertyMap["MIMEType"].value.contains("audio"))
            {
                timelineItem->containsVideo = false;
                timelineItem->containsAudio = timelineItem->containsAudio || containsAudio;
            }
            //tbd check other formats then mp3 and 4

            QStringList tracks;

            if (timelineItem->containsVideo)
                tracks << "v";

            QSlider *audioLevelSlider = (QSlider *)timelineItem->groupItem->audioLevelSliderProxy->widget();
            if (timelineItem->containsAudio && audioLevelSlider->value() > 0)
                tracks << "a";

            int transitionTimeMSec = ui->transitionDurationTimeEdit->time().msecsSinceStartOfDay();

            foreach (QString track, tracks)
            {
                QString filterClip;

                if (track == "v")
                {
                    filterClip = tr("[%1]").arg(QString::number(fileReference));

                    //trim using seperate output trim%1
                    filterClip += tr("trim=%2:%3[vv%1];[vv%1]").arg(QString::number(fileReference), QString::number(clipItem->clipIn / 1000.0 - clipInRounded) , QString::number(clipItem->clipOut / 1000.0 - clipInRounded));
                    filterClip += tr("setpts=(PTS-STARTPTS)");
                }
                else
                {
                    filterClip = tr("[%1:a]").arg(QString::number(fileReference));

                    filterClip += tr("atrim=%2:%3[aa%1];[aa%1]").arg(QString::number(fileReference), QString::number(clipItem->clipIn / 1000.0 - clipInRounded) , QString::number(clipItem->clipOut / 1000.0 - clipInRounded)); // in seconds
//                    if (audioLevelSlider->value() < 100) //done in mux
//                        filterClip += tr("volume=%1,").arg(QString::number(audioLevelSlider->value() / 100.0));
                    filterClip += tr("aformat=sample_fmts=fltp:sample_rates=44100:channel_layouts=stereo,asetpts=(PTS-STARTPTS)");
                }

                if (speed != 1)
                {
                    if (track == "v")
                        filterClip += "/" + QString::number(speed);
//                    filterClip.replace("setpts=PTS-STARTPTS", "setpts=(PTS-STARTPTS)/" + QString::number(speed));
                    else //audio
                        filterClip += ",asetrate=r=" + QString::number(44100.0 * speed);
                }

                if (track == "v" && totalDurationMsec != 0) //audio uses acrossfade, not overlay
                    filterClip += tr("+%1/TB").arg(QString::number(totalDurationMsec / 1000.0));

                if (ui->transitionDurationTimeEdit->time().msecsSinceStartOfDay() != 0)
                {
                    if (track == "v")
                    {
                        QString alphaIn = "0"; //transparent= firstIn
                        QString alphaOut = "0"; //black = transitions
                        if (item != timelineItem->filteredClips.first())
                            alphaIn = "1";
                        if (item != timelineItem->filteredClips.last())
                            alphaOut = "1";

                        qreal frameRateMedia = qRound(clipItem->mediaItem->exiftoolPropertyMap["VideoFrameRate"].value.toDouble());

//                        qDebug()<<__func__<<transitionTimeMSec<<frameRateMedia;
                        //in frames
                        filterClip += tr(",fade=in:0:%1:alpha=%2").arg(QString::number(transitionTimeMSec * frameRateMedia / 1000.0), alphaIn);
                        filterClip += tr(",fade=out:%1:%2:alpha=%3").arg(QString::number(qRound((clipItem->clipOut - clipItem->clipIn - transitionTimeMSec) * frameRateMedia / 1000.0)), QString::number(transitionTimeMSec * frameRateMedia / 1000.0), alphaOut);

                        //in seconds is not working for some reason
//                        filterClip += tr(",fade=t=in:st=0:d=%1:alpha=%2").arg(QString::number(transitionTimeMSec / 1000.0), alphaIn);
//                        filterClip += tr(",fade=t=out:st=%1:d=%2:alpha=%3").arg(QString::number((inTime.msecsTo(outTime) * speed - transitionTimeMSec) / 1000.0), QString::number(transitionTimeMSec / 1000.0), alphaOut);
                    }
                    else //audio (no fadein and out in between
                    {
                        //in seconds
                        if (item == timelineItem->filteredClips.first())
                            filterClip += tr(",afade=t=in:st=0:d=%1").arg(QString::number(transitionTimeMSec / 1000.0));
                        if (item == timelineItem->filteredClips.last())
                            filterClip += tr(",afade=t=out:st=%1:d=%2").arg(QString::number((inTime.msecsTo(outTime) * speed - transitionTimeMSec) / 1000.0), QString::number(transitionTimeMSec / 1000.0));
                    }
                }

                if (track == "v")
                {
                    //maintain aspect ratio of original media: 638 * 480 to export:2704 * 1520

                    int mediaWidth = clipItem->mediaItem->exiftoolPropertyMap["ImageWidth"].value.toInt();
                    int mediaHeight = clipItem->mediaItem->exiftoolPropertyMap["ImageHeight"].value.toInt();

                    if (mediaWidth != exportWidth || mediaHeight != exportHeight)
                    {
                        if (qreal(mediaWidth) / qreal(mediaHeight) < qreal(exportWidth) / qreal(exportHeight))
                        {
                            int wwidth = int(exportHeight * mediaWidth / mediaHeight / 2.0) * 2; //maintain aspect ratio. Should be dividable by 2

                            filterClip += ",scale=w=" + QString::number(wwidth) + ":h=" + QString::number(exportHeight) + ",pad=" + QString::number(exportWidth) + ":ih:(ow-iw)/2";
                        }
                        else
                        {
                            int hheight = int(exportWidth * mediaHeight / mediaWidth / 2.0) * 2; //maintain aspect ratio. Should be dividable by 2

                            filterClip += ",scale=w=" + QString::number(exportWidth) + ":h=" + QString::number(hheight) + ",pad=(oh-ih)/2:ih:" + QString::number(exportHeight);
                        }
//                    filterClip += ",scale=w=" + QString::number(exportWidth) + ":h=-1";// + QString::number(height);
                    }

                    if (qRound(clipItem->mediaItem->exiftoolPropertyMap["VideoFrameRate"].value.toDouble()) != exportFramerate)
                    {
//                        qDebug()<<__func__<<"fps"<<clipItem->fileInfo.fileName()<<clipItem->mediaItem->exiftoolValueMap["VideoFrameRate"].value.toDouble()<<exportFramerate<<QString("50.0").toInt();
                        filterClip += ",fps=" + QString::number(exportFramerate);
                    }

                    filterClip += tr("[v%1]").arg(QString::number(fileReference));
                }
                else
                    filterClip += tr("[a%1]").arg(QString::number(fileReference));

                QStringList tags;
                foreach (AGTagTextItem *tagItem, clipItem->tags)
                {
                    tags << tagItem->tagName;
                }

                if (false && tags.count() > 0)
                {
                    qDebug()<<__func__<<"tags"<<track<<tags.join(";")<<filterClip;

                    if (tags.join(";").toLower().contains("backwards"))
                        filterClip.replace("setpts=PTS-STARTPTS", "setpts=PTS-STARTPTS, reverse");
                    //https://stackoverflow.com/questions/42257354/concat-a-video-with-itself-but-in-reverse-using-ffmpeg
                    if (tags.join(";").toLower().contains("slowmotion"))
                        filterClip.replace("setpts=PTS-STARTPTS", "setpts=2.0*PTS");
                    if (tags.join(";").toLower().contains("fastmotion"))
                        filterClip.replace("setpts=PTS-STARTPTS", "setpts=0.5*PTS");
                }

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
                        if (ui->transitionDurationTimeEdit->time().msecsSinceStartOfDay() > 0)
                            ffmpegCombines << tr("[%1][a%2]acrossfade=d=%3[ao%2]").arg( lastA, QString::number(fileReference), QString::number(transitionTimeMSec / 1000.0));
                        else
                            ffmpegCombines << tr("[%1][a%2]concat=n=2:v=0:a=1[ao%2]").arg( lastA, QString::number(fileReference));
                        lastA = tr("ao%1").arg(QString::number(fileReference));
                    }
                    else
                        lastA = tr("a%1").arg(QString::number(fileReference));
                }
            }

            totalDurationMsec += inTime.msecsTo(outTime) - transitionTimeMSec;
//            qDebug()<<"  Clip"<<clipItem->itemToString()<<inTime.msecsTo(outTime)<<totalDurationMsec;
        } //for each clip

        //add the watermark to the (first) video track
//        foreach (QString track, tracks)
        if (timelineItem->containsVideo)
        {
            if (timelineItem->groupItem->fileInfo.fileName() == "Video" && QSettings().value("watermarkFileName").toString() != "No Watermark")
            {
                ffmpegFiles << "-i \"" + QSettings().value("watermarkFileName").toString() + "\"";
                ffmpegClips << "[" + QString::number(ffmpegFiles.count() -1) + ":v]scale=" + QString::number(exportWidth/10) + "x" + QString::number(exportHeight/10) + "[wtm]";
                ffmpegCombines << "[" + lastV + "][wtm]overlay = main_w-overlay_w-10:main_h-overlay_h-10[video]";
                ffmpegMappings << "-map [video]";
            }
            else
                ffmpegMappings << tr("-map [%1]").arg(lastV);
        }

        QSlider *audioLevelSlider = (QSlider *)timelineItem->groupItem->audioLevelSliderProxy->widget();
        if (timelineItem->containsAudio && audioLevelSlider->value() > 0)
        {
            ffmpegMappings << tr("-map [%1]").arg(lastA);
        }

//        QString sourceFolderFileName = recycleFolderName + fileNameWithoutExtension + groupItem->fileInfo.completeBaseName() + ".txt";
        QString targetFolderFileName = recycleFolderName + fileNameWithoutExtension + timelineItem->groupItem->fileInfo.completeBaseName() + ".mp4";

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
            process0->command("Check " + timelineItem->groupItem->fileInfo.fileName() + " finished", [=]()
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
            process0->start();
        }

        AGProcessAndThread *process = new AGProcessAndThread(this);
        process->command("FFMpeg encode" + timelineItem->groupItem->fileInfo.completeBaseName(), command);
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
                if (process->errorMessage != "")
                    QMessageBox::information(this, "Error " + process->name, process->errorMessage);
                else
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
            }
        });
        process->start();

    } //for each timeline
}

void MExportDialog::muxVideoAndAudio()
{
    QStringList ffmpegFiles;
    QStringList ffmpegClips;
    QStringList ffmpegCombines;
    QStringList ffmpegMappings;

    QStringList audioStreamList;
    QStringList audioStreamListAliases;
    QStringList videoStreamList;
    foreach (MTimelineGroupRectItem *timelineItem, timelineGroupList)
    {
//        MGroupRectItem *groupItem = (MGroupRectItem *)timelineItem->parentItem();

        QString recycleFolderName = QSettings().value("selectedFolderName").toString() + "MSKRecycleBin/";
        QDir recycleDir(recycleFolderName);
        if (!recycleDir.exists())
            recycleDir.mkpath(".");

        QString targetFolderFileName = recycleFolderName + fileNameWithoutExtension + timelineItem->groupItem->fileInfo.completeBaseName() + ".mp4";;

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
//            if (timelineItem->groupItem->fileInfo.completeBaseName() == "Video")
            {
                QSlider *audioLevelSlider = (QSlider *)timelineItem->groupItem->audioLevelSliderProxy->widget();
                if (audioLevelSlider->value() == 100)
                    audioStreamListAliases << "[" + QString::number(fileReference) + "]";
    //                                                command += " -filter_complex \"[0][1]amix=inputs=2[a]\" -map 0:v -map \"[a]\" -c:v copy";
                else if (audioLevelSlider->value() > 0)
                {
                    audioStreamList << "[" + QString::number(fileReference) + "]" + "volume=" + QString::number(audioLevelSlider->value() / 100.0) + "[a" + QString::number(fileReference) + "]";
                    audioStreamListAliases << "[a" + QString::number(fileReference) + "]";
                }

    //                                                command += " -filter_complex \"[1]amix=inputs=2[a]\" -map 0:v -map \"[a]\" -c:v copy";

    //                                            ffmpegMappings << "-map " + QString::number(fileReference) + ":v";
            }
//            else
//            {
//                audioStreamList << "[" + QString::number(fileReference) + "]";
//            }

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
        ffmpegCombines << audioStreamList.join(";") + ";" + audioStreamListAliases.join("") + "amix=inputs=" + QString::number(audioStreamListAliases.count()) + "[audio]";
        ffmpegMappings << "-map \"[audio]\"";
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

        emit fileWatch(QSettings().value("selectedFolderName").toString() +  fileNameWithoutExtension + ".mp4", false);

        if (event == "started")
        {
            process->addProcessLog("output", "Files\n" + ffmpegFiles.join("\n"));
            process->addProcessLog("output", "Clips\n" + ffmpegClips.join("\n"));
            process->addProcessLog("output", "Combines\n" + ffmpegCombines.join("\n"));
            process->addProcessLog("output", "Mappings\n" + ffmpegMappings.join("\n"));
        }
        if (event == "finished")
        {
            emit fileWatch(QSettings().value("selectedFolderName").toString() +  fileNameWithoutExtension + ".mp4", true, true);
            if (process->errorMessage == "")
            {
            }
            else
                QMessageBox::information(this, "Error " + process->name, process->errorMessage);
        }
    });
    process->start();

}

void MExportDialog::addPremiereTrack(QString mediaType, MTimelineGroupRectItem *timelineItem, QMap<QString, FileStruct> filesMap)
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

            int totalTime = 0;

            int iterationCounter = 0;
            int totalDurationMsec = 0;

            foreach (QGraphicsItem *item, timelineItem->filteredClips)
            {
                AGClipRectItem *clipItem = (AGClipRectItem *)item;
//                qDebug()<<"  Clip"<<clipItem->itemToString();

                qreal speed = 1;
                foreach (AGTagTextItem *tagItem, clipItem->tags)
                {
                    if (tagItem->tagName.contains("speed", Qt::CaseInsensitive))
                    {
                        qreal foundSpeed = tagItem->tagName.mid(5).toDouble();
                        if (foundSpeed != 0)
                            speed = foundSpeed;
                    }
                }

                QTime inTime = QTime::fromMSecsSinceStartOfDay(clipItem->clipIn / speed);
                QTime outTime = QTime::fromMSecsSinceStartOfDay(clipItem->clipOut / speed); //not needed to add one frame...

//                int inFrames = qRound(inTime.msecsSinceStartOfDay() * exportFramerate / 1000.0);
//                int outFrames = qRound(outTime.msecsSinceStartOfDay() * exportFramerate / 1000.0);

                //                qDebug()<<"props"<<fileName<<mediaType<<clipFrameRate<<clipAudioChannels<<*audioChannelsPointer;

                int frameRate = qRound(clipItem->mediaItem->exiftoolPropertyMap["VideoFrameRate"].value.toDouble());
                if (frameRate == 0) //for audio
                    frameRate = exportFramerate;

                if (mediaType == "audio")
                {
                    if (iterationCounter%2 == trackNr) //even or odd tracks
                    {
                        if (item == timelineItem->filteredClips.first()) //fade in 1 second
                        {
                            addPremiereTransitionItem(0, 1000 / frameRate, QString::number(frameRate), mediaType, "start");
                        }
                    }
                }

                if (trackNr == 1 && iterationCounter%2 == 1 && ui->transitionDurationTimeEdit->time().msecsSinceStartOfDay() > 0) //track 2 and not first clip (never is)
                {
                    addPremiereTransitionItem(totalTime, totalTime + ui->transitionDurationTimeEdit->time().msecsSinceStartOfDay(), QString::number(frameRate), mediaType, "start");
                }

                int deltaTime = outTime.msecsSinceStartOfDay() - inTime.msecsSinceStartOfDay() - ui->transitionDurationTimeEdit->time().msecsSinceStartOfDay();

                if (iterationCounter%2 == trackNr) //even or odd tracks
                {
                    addPremiereClipitem(clipItem, QString::number(iterationCounter), clipItem->mediaItem->fileInfo, totalTime, totalTime + deltaTime + ui->transitionDurationTimeEdit->time().msecsSinceStartOfDay(), inTime.msecsSinceStartOfDay(), outTime.msecsSinceStartOfDay(), QString::number(frameRate), mediaType, &filesMap, channelTrackNr);
                }

                totalTime += deltaTime;

                if (trackNr == 1 && iterationCounter%2 == 1 && item != timelineItem->filteredClips.last() && ui->transitionDurationTimeEdit->time().msecsSinceStartOfDay() > 0) //track 2 and not last clip
                {
                    addPremiereTransitionItem(totalTime, totalTime + ui->transitionDurationTimeEdit->time().msecsSinceStartOfDay(), QString::number(frameRate), mediaType, "end");
                }

                totalDurationMsec += inTime.msecsTo(outTime) - ui->transitionDurationTimeEdit->time().msecsSinceStartOfDay();

                if (mediaType == "audio")
                {
                    if (iterationCounter%2 == trackNr) //even or odd tracks
                    {
                        if (item == timelineItem->filteredClips.last()) //fade out 1 second
                        {
                            addPremiereTransitionItem(totalDurationMsec - 1000 / frameRate, totalDurationMsec, QString::number(frameRate), mediaType, "end");
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

void MExportDialog::addPremiereTransitionItem(int startTime, int endTime, QString frameRate, QString mediaType, QString startOrEnd)
{
    s("     <transitionitem>");
    s("      <start>%1</start>", QString::number(startTime * frameRate.toInt() / 1000));
    s("      <end>%1</end>", QString::number(endTime * frameRate.toInt() / 1000));
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

void MExportDialog::addPremiereClipitem(AGClipRectItem *clipItem, QString clipId, QFileInfo fileInfo, int startTime, int endTime, int inTime, int outTime, QString frameRate, QString mediaType, QMap<QString, FileStruct> *filesMap, int channelTrackNr)
{
    QString clipAudioChannels = "2";

    if (mediaType == "video")
        s("     <clipitem>");
    else //audio
    {
        if (clipItem->mediaItem->exiftoolPropertyMap["AudioChannels"].value != "")
            clipAudioChannels = clipItem->mediaItem->exiftoolPropertyMap["AudioChannels"].value;
//        else
//        {
//            if (mediaType == "audio")
//                clipAudioChannels = "2";  //assume stereo for audiofiles
//            else if (timelineItem->containsAudio)
//                clipAudioChannels = "1";  //assume mono for videofiles
//            else
//                clipAudioChannels = "0";  //assume mono for videofiles
//        }

        if (clipAudioChannels == "1")
            s("     <clipitem premiereChannelType=\"%1\">", "mono");
        else if (clipAudioChannels == "2")
            s("     <clipitem premiereChannelType=\"%1\">", "stereo");
        else
            s("     <clipitem premiereChannelType=\"%1\">", "multichannel");
    }

    s("      <masterclipid>masterclip-%1</masterclipid>", clipId);
    s("      <name>%1</name>", fileInfo.fileName().toHtmlEscaped());
//            s("      <enabled>TRUE</enabled>");
    s("      <duration>%1</duration>", QString::number(outTime * frameRate.toInt() / 1000 - inTime * frameRate.toInt() / 1000 + 1)); //20200818 added again as audio without duration not shown correctly
    s("      <start>%1</start>", QString::number(startTime * frameRate.toInt() / 1000)); //will be reassigned in premiere to -1

    s("      <end>%1</end>", QString::number(endTime * frameRate.toInt() / 1000)); //will be reassigned in premiere to -1

    if (inTime != -1)
        s("      <in>%1</in>", QString::number(inTime * frameRate.toInt() / 1000));
    if (outTime != -1)
        s("      <out>%1</out>", QString::number(outTime * frameRate.toInt() / 1000));

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

    QSlider *audioLevelSlider = nullptr;
    if (clipItem != nullptr)
        audioLevelSlider = (QSlider *)clipItem->mediaItem->groupItem->audioLevelSliderProxy->widget();

    if (!(*filesMap)[fileInfo.absoluteFilePath()].definitionGenerated)
    {
        s("      <file id=\"file-%1%2\">", AVType, QString::number((*filesMap)[fileInfo.absoluteFilePath()].counter)); //define file
        s("       <name>%1</name>", fileInfo.fileName().toHtmlEscaped());
        s("       <pathurl>file://localhost/%1</pathurl>", fileInfo.absoluteFilePath().toHtmlEscaped());
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

        if ((AVType == "A" || clipAudioChannels != "0") && AVType != "W")
        {
            if (audioLevelSlider != nullptr && audioLevelSlider->value() > 0)
            {
                s("        <audio>");
    //                                s("         <samplecharacteristics>");
    //                                s("          <depth>16</depth>");
    ////                                s("          <samplerate>44100</samplerate>");
    //                                s("         </samplecharacteristics>");
                s("         <channelcount>%1</channelcount>", clipAudioChannels);
                s("        </audio>");
            }
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
            int imageWidth = clipItem->mediaItem->exiftoolPropertyMap["ImageWidth"].value.toInt();
            int imageHeight = clipItem->mediaItem->exiftoolPropertyMap["ImageHeight"].value.toInt();
            if (imageHeight != exportHeight || imageWidth != exportWidth)
            {
                double heightRatio = 100.0 * exportHeight / imageHeight;
                double widthRatio = 100.0 * exportWidth / imageWidth;
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

            qreal speed = 1;
            foreach (AGTagTextItem *tagItem, clipItem->tags)
            {
                if (tagItem->tagName.contains("speed", Qt::CaseInsensitive))
                {
                    qreal foundSpeed = tagItem->tagName.mid(5).toDouble();
                    if (foundSpeed != 0)
                        speed = foundSpeed;
                }
            }

            if (speed != 1)
            {
                s("      <filter>");
                s("          <effect>");
                s("              <name>Time Remap</name>");
                s("              <effectid>timeremap</effectid>");
                s("              <effectcategory>motion</effectcategory>");
                s("              <effecttype>motion</effecttype>");
                s("              <mediatype>video</mediatype>");
                s("              <parameter authoringApp=\"PremierePro\">");
                s("                  <parameterid>variablespeed</parameterid>");
                s("                  <name>variablespeed</name>");
                s("                  <valuemin>0</valuemin>");
                s("                  <valuemax>1</valuemax>");
                s("                  <value>0</value>");
                s("              </parameter>");
                s("              <parameter authoringApp=\"PremierePro\">");
                s("                  <parameterid>speed</parameterid>");
                s("                  <name>speed</name>");
                s("                  <valuemin>-100000</valuemin>");
                s("                  <valuemax>100000</valuemax>");
                s("                  <value>%1</value>", QString::number(speed * 100));
                s("              </parameter>");
                s("              <parameter authoringApp=\"PremierePro\">");
                s("                  <parameterid>graphdict</parameterid>");
                s("                  <name>graphdict</name>");
                s("                  <valuemin>0</valuemin>");
                s("                  <valuemax>15969</valuemax>");
                s("                  <value>0</value>");
                s("                  <keyframe>");
                s("                      <when>0</when>");
                s("                      <value>0</value>");
                s("                      <speedvirtualkf>TRUE</speedvirtualkf>");
                s("                      <speedkfstart>TRUE</speedkfstart>");
                s("                  </keyframe>");
                s("                  <keyframe>");
                s("                      <when>369</when>");
                s("                      <value>3505</value>");
                s("                      <speedvirtualkf>TRUE</speedvirtualkf>");
                s("                      <speedkfin>TRUE</speedkfin>");
                s("                  </keyframe>");
                s("                  <keyframe>");
                s("                      <when>1681</when>");
                s("                      <value>15969</value>");
                s("                      <speedvirtualkf>TRUE</speedvirtualkf>");
                s("                      <speedkfout>TRUE</speedkfout>");
                s("                  </keyframe>");
                s("                  <interpolation>");
                s("                      <name>FCPCurve</name>");
                s("                  </interpolation>");
                s("              </parameter>");
                s("         </effect>");
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

        if (audioLevelSlider != nullptr && audioLevelSlider->value() > 0 && audioLevelSlider->value() < 100 && clipAudioChannels != "0") //AVType == "V"  &&
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
            s("         <value>%1</value>", QString::number(audioLevelSlider->value() / 100.0));
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
    foreach (MTimelineGroupRectItem *timelineItem, timelineGroupList)
    {
//        MGroupRectItem *groupItem = (MGroupRectItem *)timelineItem->parentItem();
//        qDebug()<<"Timeline"<<groupItem->fileInfo<<timelineItem->fileInfo.absolutePath()<<timelineItem->fileInfo.fileName();

        int totalDurationMsec = 0;

        foreach (QGraphicsItem *item, timelineItem->filteredClips)
        {
            AGClipRectItem *clipItem = (AGClipRectItem *)item;

            qreal speed = 1;
            foreach (AGTagTextItem *tagItem, clipItem->tags)
            {
                if (tagItem->tagName.contains("speed", Qt::CaseInsensitive))
                {
                    qreal foundSpeed = tagItem->tagName.mid(5).toDouble();
                    if (foundSpeed != 0)
                        speed = foundSpeed;
                }
            }

            QTime inTime = QTime::fromMSecsSinceStartOfDay(clipItem->clipIn / speed);
            QTime outTime = QTime::fromMSecsSinceStartOfDay(clipItem->clipOut / speed); //remove 1 frame below
            int clipDuration = (clipItem->clipOut - clipItem->clipIn) / speed;

            int frameRate = qRound(clipItem->mediaItem->exiftoolPropertyMap["VideoFrameRate"].value.toDouble());
            if (frameRate == 0) //for audio
                frameRate = exportFramerate;

            s("  <producer id=\"producer%1\" title=\"Anonymous Submission\" in=\"00:00:00.000\" out=\"%2\">", QString::number(iterationCounter), QTime::fromMSecsSinceStartOfDay(clipItem->mediaItem->duration / speed - 1000 / frameRate).toString("hh:mm:ss.zzz"));
            s("    <property name=\"length\">%1</property>", QTime::fromMSecsSinceStartOfDay(clipItem->mediaItem->duration / speed - 1000 / frameRate).toString("hh:mm:ss.zzz"));// fileDuration);

            if (speed != 1)
            {
                s("    <property name=\"resource\">%1:%2</property>", QString::number(speed), clipItem->fileInfo.absoluteFilePath().toHtmlEscaped());
                s("    <property name=\"warp_speed\">%1</property>", QString::number(speed));
                s("    <property name=\"warp_resource\">%1</property>", clipItem->fileInfo.absoluteFilePath().toHtmlEscaped());
                s("    <property name=\"mlt_service\">timewarp</property>");
            }
            else
                s("    <property name=\"resource\">%1</property>", clipItem->fileInfo.absoluteFilePath().toHtmlEscaped());

            if (timelineItem->groupItem->fileInfo.fileName() == "Audio")
            {
                //fadein and out of 1 second.
                if (item == timelineItem->filteredClips.first())
                {
                    s("    <filter id=\"filter%1\" in=\"%2\" out=\"%3\">", QString::number(iterationCounter), inTime.toString("HH:mm:ss.zzz"), outTime.addMSecs(-1000 / frameRate).toString("HH:mm:ss.zzz"));
                    s("          <property name=\"window\">75</property>");
                    s("          <property name=\"max_gain\">20dB</property>");
                    s("          <property name=\"mlt_service\">volume</property>");
                    s("          <property name=\"level\">00:00:00.000=-60;00:00:00.960=0</property>");
                    s("          <property name=\"shotcut:filter\">fadeInVolume</property>");
                    s("          <property name=\"shotcut:animIn\">00:00:01.000</property>");
                    s("        </filter>");
                }
                if (item == timelineItem->filteredClips.last()) //fade out 1 second
                {
                    s("    <filter id=\"filter%1\" in=\"%2\" out=\"%3\">", QString::number(iterationCounter), inTime.toString("HH:mm:ss.zzz"), outTime.addMSecs(-1000 / frameRate).toString("HH:mm:ss.zzz"));
                    s("          <property name=\"window\">75</property>");
                    s("          <property name=\"max_gain\">20dB</property>");
                    s("          <property name=\"mlt_service\">volume</property>");
                    s("          <property name=\"level\">%1=0;%2=-60</property>", QTime::fromMSecsSinceStartOfDay(clipDuration - 1000).toString("hh:mm:ss.zzz"), QTime::fromMSecsSinceStartOfDay(clipDuration).toString("hh:mm:ss.zzz"));
                    s("          <property name=\"shotcut:filter\">fadeOutVolume</property>");
                    s("          <property name=\"shotcut:animOut\">00:00:01.000</property>");
                    s("        </filter>");
                }
            }
            else //video
            {
                if (false)
                {
                    s("          <filter id=\"filter%1-2\" in=\"%2\" out=\"%3\">", QString::number(iterationCounter), inTime.toString("HH:mm:ss.zzz"), outTime.addMSecs(-1000 / frameRate).toString("HH:mm:ss.zzz"));
                    s("            <property name=\"mlt_service\">avfilter.hue</property>");
                    s("            <property name=\"av.h\">%1</property>", QString::number(32.4)); //hue
                    s("            <property name=\"av.b\">%1</property>", QString::number(-0.8)); //lightness / brightness
                    s("            <property name=\"av.s\">%1</property>", QString::number(2.58)); //saturation
//                    s("            <property name=\"shotcut:animIn\">00:00:00.000</property>");
//                    s("            <property name=\"shotcut:animOut\">00:00:00.000</property>");
                    s("          </filter>");

                }
            }
            QSlider *audioLevelSlider = (QSlider *)clipItem->mediaItem->groupItem->audioLevelSliderProxy->widget();
            if (audioLevelSlider->value() > 0 && audioLevelSlider->value() < 100)
            {
//                    qDebug()<<"qLn"<<qLn(10)<<audioLevelSlider->value()<<10 * qLn(audioLevelSlider->value() / 100.0);
                s("          <filter id=\"filter%1-1\" in=\"%2\" out=\"%3\">", QString::number(iterationCounter), inTime.toString("HH:mm:ss.zzz"), outTime.addMSecs(-1000 / frameRate).toString("HH:mm:ss.zzz"));
                s("             <property name=\"window\">75</property>");
                s("             <property name=\"max_gain\">20dB</property>");
                s("             <property name=\"level\">%1</property>", QString::number(10 * qLn(audioLevelSlider->value() / 100.0))); //percentage to dB
                s("             <property name=\"mlt_service\">volume</property>");
                s("           </filter>");
            }

            s("  </producer>");

            totalDurationMsec += inTime.msecsTo(outTime);

            if (item != timelineItem->filteredClips.first())
                totalDurationMsec -= ui->transitionDurationTimeEdit->time().msecsSinceStartOfDay();

            //            qDebug()<<"  Clip"<<clipItem->itemToString()<<clipDuration<<inTime.msecsTo(outTime)<<totalDurationMsec;
            iterationCounter++;
        }

        maxDurationMsec = qMax(maxDurationMsec, totalDurationMsec);
//        qDebug()<<"DurationMsec"<<groupItem->fileInfo.fileName()<<totalDurationMsec<<maxDurationMsec;
    }

    iterationCounter = 0;
    s("  <playlist id=\"main_bin\" title=\"Main playlist\">");
    s("    <property name=\"xml_retain\">1</property>");
    foreach (MTimelineGroupRectItem *timelineItem, timelineGroupList)
    {

        foreach (QGraphicsItem *item, timelineItem->filteredClips)
        {
            AGClipRectItem *clipItem = (AGClipRectItem *)item;
//            qDebug()<<"  Clip"<<clipItem->itemToString();

            qreal speed = 1;
            foreach (AGTagTextItem *tagItem, clipItem->tags)
            {
                if (tagItem->tagName.contains("speed", Qt::CaseInsensitive))
                {
                    qreal foundSpeed = tagItem->tagName.mid(5).toDouble();
                    if (foundSpeed != 0)
                        speed = foundSpeed;
                }
            }

            QTime inTime = QTime::fromMSecsSinceStartOfDay(clipItem->clipIn / speed);
            QTime outTime = QTime::fromMSecsSinceStartOfDay(clipItem->clipOut / speed); //remove 1 frame below
            int frameRate = qRound(clipItem->mediaItem->exiftoolPropertyMap["VideoFrameRate"].value.toDouble());
            if (frameRate == 0) //for audio
                frameRate = exportFramerate;

            s("    <entry producer=\"producer%1\" in=\"%2\" out=\"%3\"/>"
              , QString::number(iterationCounter)
              , inTime.toString("HH:mm:ss.zzz"), outTime.addMSecs(-1000 / frameRate).toString("HH:mm:ss.zzz"));

            iterationCounter++;
        }
    }

    s("  </playlist>"); //playlist main bin

    if (ui->transitionDurationTimeEdit->time().msecsSinceStartOfDay() > 0)
    {
        QTime previousInTime = QTime();
        QTime previousOutTime = QTime();
        int previousProducerNr = -1;

        iterationCounter = 0;
        foreach (MTimelineGroupRectItem *timelineItem, timelineGroupList)
        {
            foreach (QGraphicsItem *item, timelineItem->filteredClips)
            {
                AGClipRectItem *clipItem = (AGClipRectItem *)item;
//                qDebug()<<"  Clip"<<clipItem->itemToString();

                qreal speed = 1;
                foreach (AGTagTextItem *tagItem, clipItem->tags)
                {
                    if (tagItem->tagName.contains("speed", Qt::CaseInsensitive))
                    {
                        qreal foundSpeed = tagItem->tagName.mid(5).toDouble();
                        if (foundSpeed != 0)
                            speed = foundSpeed;
                    }
                }

                QTime inTime = QTime::fromMSecsSinceStartOfDay(clipItem->clipIn / speed);
                QTime outTime = QTime::fromMSecsSinceStartOfDay(clipItem->clipOut / speed); //remove 1 frame below
                int frameRate = qRound(clipItem->mediaItem->exiftoolPropertyMap["VideoFrameRate"].value.toDouble());
                if (frameRate == 0) //for audio
                    frameRate = exportFramerate;

                if (item != timelineItem->filteredClips.first())
                {
                    s("<tractor id=\"tractor%1\" title=\"%2\" global_feed=\"1\" in=\"00:00:00.000\" out=\"%3\">", QString::number(previousProducerNr) + "-" + QString::number(iterationCounter), "Transition " + QString::number(previousProducerNr) + "-" + QString::number(iterationCounter), ui->transitionDurationTimeEdit->time().toString("HH:mm:ss.zzz"));
                    s("   <property name=\"shotcut:transition\">lumaMix</property>");
                    s("   <track producer=\"producer%1\" in=\"%2\" out=\"%3\"/>", QString::number(previousProducerNr), previousOutTime.addMSecs(-ui->transitionDurationTimeEdit->time().msecsSinceStartOfDay()).toString("HH:mm:ss.zzz"), previousOutTime.addMSecs(-1000 / frameRate).toString("HH:mm:ss.zzz"));
                    s("   <track producer=\"producer%1\" in=\"%2\" out=\"%3\"/>", QString::number(iterationCounter), inTime.toString("HH:mm:ss.zzz"), inTime.addMSecs(ui->transitionDurationTimeEdit->time().msecsSinceStartOfDay() - 1000 / frameRate).toString("HH:mm:ss.zzz"));
                    s("   <transition id=\"transition0\" out=\"%1\">", ui->transitionDurationTimeEdit->time().toString("HH:mm:ss.zzz"));
                    s("     <property name=\"a_track\">0</property>");
                    s("     <property name=\"b_track\">1</property>");
                    s("     <property name=\"factory\">loader</property>");
                    s("     <property name=\"mlt_service\">luma</property>");
                    s("   </transition>");
                    s("   <transition id=\"transition1\" out=\"%1\">", ui->transitionDurationTimeEdit->time().toString("HH:mm:ss.zzz"));
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

    foreach (MTimelineGroupRectItem *timelineItem, timelineGroupList)
    {
//        MGroupRectItem *groupItem = (MGroupRectItem *)timelineItem->parentItem();

        s("  <playlist id=\"playlist%1\">", timelineItem->groupItem->fileInfo.fileName());

        if (timelineItem->groupItem->fileInfo.fileName() == "Audio")
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

            qreal speed = 1;
            foreach (AGTagTextItem *tagItem, clipItem->tags)
            {
                if (tagItem->tagName.contains("speed", Qt::CaseInsensitive))
                {
                    qreal foundSpeed = tagItem->tagName.mid(5).toDouble();
                    if (foundSpeed != 0)
                        speed = foundSpeed;
                }
            }

            QTime inTime = QTime::fromMSecsSinceStartOfDay(clipItem->clipIn / speed);
            QTime outTime = QTime::fromMSecsSinceStartOfDay(clipItem->clipOut / speed); //remove 1 frame below
            int frameRate = qRound(clipItem->mediaItem->exiftoolPropertyMap["VideoFrameRate"].value.toDouble());
            if (frameRate == 0) //for audio
                frameRate = exportFramerate;

            if (ui->transitionDurationTimeEdit->time().msecsSinceStartOfDay() > 0)
            {
                if (item != timelineItem->filteredClips.first())
                {
                    s("    <entry producer=\"tractor%1\" in=\"00:00:00.000\" out=\"%2\"/>", QString::number(previousProducerNr) + "-" + QString::number(iterationCounter), ui->transitionDurationTimeEdit->time().toString("HH:mm:ss.zzz"));
                    inTime = inTime.addMSecs(ui->transitionDurationTimeEdit->time().msecsSinceStartOfDay()); //subtract half of the transitionframes
                }
                if (item != timelineItem->filteredClips.last())
                {
                    outTime = outTime.addMSecs(- ui->transitionDurationTimeEdit->time().msecsSinceStartOfDay()); //subtract half of the transitionframes
                }
            }

            s("    <entry producer=\"producer%1\" in=\"%2\" out=\"%3\"/>", QString::number(iterationCounter), inTime.toString("HH:mm:ss.zzz"), outTime.addMSecs(-1000 / frameRate).toString("HH:mm:ss.zzz") );

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

    int trackCounter = 1;
    int audioTrack = -1;
    int videoTrack = -1;
    int watermarkTrack = -1;
    foreach (MTimelineGroupRectItem *timelineItem, timelineGroupList)
    {
//        MGroupRectItem *groupItem = (MGroupRectItem *)timelineItem->parentItem();

        QSlider *audioLevelSlider = (QSlider *)timelineItem->groupItem->audioLevelSliderProxy->widget();
        if (audioLevelSlider->value() == 0) //timelineItem->groupItem->fileInfo.fileName() == "Video" &&
            s("    <track producer=\"playlist%1\" hide=\"audio\"/>", timelineItem->groupItem->fileInfo.fileName());
        else
            s("    <track producer=\"playlist%1\"/>", timelineItem->groupItem->fileInfo.fileName());

        if (timelineItem->groupItem->fileInfo.fileName() == "Video")
            videoTrack = trackCounter;
        if (timelineItem->groupItem->fileInfo.fileName() == "Audio")
            audioTrack = trackCounter;

        trackCounter++;

    }

    if (QSettings().value("watermarkFileName").toString() != "No Watermark")
    {
        s("    <track producer=\"playlist%1\"/>", "WM");
//        s("    <transition id=\"transition%1\">", "Mix"); //mix V1 and V2?
//        s("      <property name=\"a_track\">0</property>");
//        s("      <property name=\"b_track\">1</property>");
//        s("      <property name=\"mlt_service\">mix</property>");
//        s("      <property name=\"always_active\">1</property>");
//        s("      <property name=\"sum\">1</property>");
//        s("    </transition>");
//        s("    <transition id=\"transition%1\">", "Mix"); //mix audio from V1 and A1?
//        s("      <property name=\"a_track\">0</property>");
//        s("      <property name=\"b_track\">2</property>");
//        s("      <property name=\"mlt_service\">mix</property>");
//        s("      <property name=\"always_active\">1</property>");
//        s("      <property name=\"sum\">1</property>");
//        s("    </transition>");
        watermarkTrack = trackCounter;
    }

    if (videoTrack != -1 && watermarkTrack != -1) //blend watermark with video
    {
        s("    <transition id=\"transition%1\">", "Blend"); //blend A1 and V2?
        s("      <property name=\"a_track\">%1</property>", QString::number(videoTrack));
        s("      <property name=\"b_track\">%1</property>", QString::number(watermarkTrack));
        s("      <property name=\"version\">0.9</property>");
        s("      <property name=\"mlt_service\">frei0r.cairoblend</property>");
        s("      <property name=\"disable\">0</property>");
        s("    </transition>");
    }
    if (videoTrack != -1 && audioTrack != -1) //mix audio
    {
        s("    <transition id=\"transitionMix\">");
        s("      <property name=\"a_track\">%1</property>", QString::number(videoTrack));
        s("      <property name=\"b_track\">%1</property>", QString::number(audioTrack));
        s("      <property name=\"mlt_service\">mix</property>");
        s("      <property name=\"always_active\">1</property>");
        s("      <property name=\"sum\">1</property>");
        s("    </transition>");
    }

    s("  </tractor>");

    s("</mlt>");

    fileWrite.close();

} //exportShotcut

void MExportDialog::exportPremiere()
{
    int maxDurationMsec = 0;
    foreach (MTimelineGroupRectItem *timelineItem, timelineGroupList)
    {
        int totalDurationMsec = 0;

//        timelineItem->containsVideo = false;
//        timelineItem->containsAudio = false;

        foreach (QGraphicsItem *item, timelineItem->filteredClips)
        {
            AGClipRectItem *clipItem = (AGClipRectItem *)item;

            qreal speed = 1;
            foreach (AGTagTextItem *tagItem, clipItem->tags)
            {
                if (tagItem->tagName.contains("speed", Qt::CaseInsensitive))
                {
                    qreal foundSpeed = tagItem->tagName.mid(5).toDouble();
                    if (foundSpeed != 0)
                        speed = foundSpeed;
                }
            }

            QTime inTime = QTime::fromMSecsSinceStartOfDay(clipItem->clipIn / speed);
            QTime outTime = QTime::fromMSecsSinceStartOfDay(clipItem->clipOut / speed);

            totalDurationMsec += inTime.msecsTo(outTime);

            if (item != timelineItem->filteredClips.first())
                totalDurationMsec -= ui->transitionDurationTimeEdit->time().msecsSinceStartOfDay();

            bool containsVideo = clipItem->mediaItem->exiftoolPropertyMap["VideoFrameRate"].value != "";
            bool containsAudio = clipItem->mediaItem->exiftoolPropertyMap["AudioSampleRate"].value != "" || clipItem->mediaItem->exiftoolPropertyMap["AudioBitrate"].value != "" || clipItem->mediaItem->exiftoolPropertyMap["AudioChannels"].value != ""; //AudioBitrate for mp3, AudioSampleRate for mp4, AudioChannels for mkv;
            if (clipItem->mediaItem->exiftoolPropertyMap["MIMEType"].value.contains("video"))
            {
                timelineItem->containsVideo = timelineItem->containsVideo || containsVideo;
                timelineItem->containsAudio = timelineItem->containsAudio || containsAudio;
            }
            else if (clipItem->mediaItem->exiftoolPropertyMap["MIMEType"].value.contains("audio"))
            {
                timelineItem->containsVideo = false;
                timelineItem->containsAudio = timelineItem->containsAudio || containsAudio;
            }

            qDebug()<<"MExportDialog::exportPremiere() - contains"<<clipItem->fileInfo.fileName()<<inTime<<clipItem->mediaItem->exiftoolPropertyMap["VideoFrameRate"].value<<clipItem->mediaItem->exiftoolPropertyMap["MIMEType"].value<<containsVideo<<containsAudio;
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

        foreach (MTimelineGroupRectItem *timelineItem, timelineGroupList)
        {
//            qDebug()<<"MExportDialog::exportPremiere Timeline"<<timelineItem->groupItem->fileInfo<<timelineItem->fileInfo.absolutePath()<<timelineItem->fileInfo.fileName()<<mediaType<<timelineItem->containsVideo<<timelineItem->containsAudio;
            if ((mediaType == "video" && timelineItem->containsVideo) || (mediaType == "audio" && timelineItem->containsAudio))
            {
//                qDebug()<<"  MExportDialog::exportPremiere Timeline"<<timelineItem->groupItem->fileInfo<<timelineItem->fileInfo.absolutePath()<<timelineItem->fileInfo.fileName();

                QMap<QString, FileStruct> filesMap;
                foreach (QGraphicsItem *item, timelineItem->filteredClips)
                {
                    AGClipRectItem *clipItem = (AGClipRectItem *)item;

//                    filesMap[clipItem->fileInfo.absoluteFilePath()].folderName = clipItem->fileInfo.absolutePath();
//                    filesMap[clipItem->fileInfo.absoluteFilePath()].fileName = clipItem->fileInfo.fileName();
                    filesMap[clipItem->fileInfo.absoluteFilePath()].counter = 0;
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

            addPremiereClipitem(nullptr, "WM", QFileInfo(QSettings().value("watermarkFileName").toString()), 0, maxDurationMsec, -1, -1, QString::number(exportFramerate), "video", &filesMap, 1);

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
//    qDebug()<<"on_clipsFramerateComboBox_currentTextChanged"<<QSettings().value("clipsFramerateComboBox")<<arg1;
    if (QSettings().value("clipsFramerateComboBox") != arg1)
    {
        QSettings().setValue("clipsFramerateComboBox", arg1);
        QSettings().sync();
//        on_exportFramerateComboBox_currentTextChanged(ui->exportFramerateComboBox->currentText());
    }
}

void MExportDialog::on_transitionDial_valueChanged(int value)
{
//    qDebug()<<__func__<<transitionValueChangedBy<<value;
    if (transitionValueChangedBy != "TimeEdit") //do not change the TimeEdit if the TimeEdit triggers the change
        ui->transitionDurationTimeEdit->setTime(QTime::fromMSecsSinceStartOfDay(value * 100));
}

void MExportDialog::on_transitionDurationTimeEdit_timeChanged(const QTime &time)
{
    if (QSettings().value("transitionTime") != time.msecsSinceStartOfDay())
    {
//        qDebug()<<"MExportDialog::on_transitionDurationTimeEdit_timeChanged"<<arg1<<transitionValueChangedBy;
        QSettings().setValue("transitionTime", time.msecsSinceStartOfDay());
        QSettings().sync();

        transitionValueChangedBy = "TimeEdit";
        ui->transitionDial->setValue(time.msecsSinceStartOfDay() / 100);
        transitionValueChangedBy = "";

        emit arrangeItems();
    }
}

void MExportDialog::on_transitionComboBox_currentTextChanged(const QString &arg1)
{
    if (QSettings().value("transitionType") != arg1)
    {
//        qDebug()<<"MExportDialog::on_transitionComboBox_currentTextChanged"<<arg1;
        QSettings().setValue("transitionType", arg1);
        QSettings().sync();
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

    if (arg1 != "Selected framerate" || ui->clipsFramerateComboBox->currentText().toInt() == 0)
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

void MExportDialog::on_exportButton_clicked()
{
    //                createContextSensitiveHelp("Export started");

    int indexOfX = ui->clipsSizeComboBox->currentText().indexOf(" x ");
    int clipsWidth = ui->clipsSizeComboBox->currentText().left(indexOfX).toInt();
    int clipsHeight = ui->clipsSizeComboBox->currentText().mid(indexOfX + 3).toInt();
//    qDebug()<<"exportSizeComboBox::currentTextChanged clips"<< ui->clipsSizeComboBox->currentText()<<indexOfX<<clipsWidth<<clipsHeight;

    if (ui->exportSizeComboBox->currentText() != "Selected size" || clipsWidth == 0)
    {
        int indexOfX = ui->exportSizeComboBox->currentText().indexOf(" x ");
        int indexOfLeftBracket = ui->exportSizeComboBox->currentText().indexOf("(");
        int indexOfRightBracket = ui->exportSizeComboBox->currentText().indexOf(")");

        exportHeight = ui->exportSizeComboBox->currentText().mid(indexOfX + 3, indexOfRightBracket - (indexOfX + 3)).toInt();

        if (clipsWidth != 0 && clipsHeight != 0)
            exportWidth = int(exportHeight * clipsWidth / clipsHeight / 2.0) * 2; //maintain aspect ratio. Should be dividable by 2
        else
            exportWidth = ui->exportSizeComboBox->currentText().mid(indexOfLeftBracket, indexOfX - indexOfLeftBracket + 1).toInt();

        exportSizeShortName = ui->exportSizeComboBox->currentText().left(indexOfLeftBracket - 1);
//        qDebug()<<"exportSizeComboBox::currentTextChanged export"<<ui->exportSizeComboBox->currentText()<<indexOfX<<indexOfLeftBracket<<indexOfRightBracket<<exportWidth<<exportHeight<<exportSizeShortName;
    }
    else //source
    {
        exportHeight = clipsHeight;
        exportWidth = clipsWidth;

        exportSizeShortName = QString::number(clipsHeight) + "p";
//        qDebug()<<"exportSizeComboBox::currentTextChanged source"<<ui->exportSizeComboBox->currentText()<<exportWidth<<exportHeight<<exportSizeShortName;
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

void MExportDialog::on_clipsSizeComboBox_currentTextChanged(const QString &arg1)
{
//    qDebug()<<"on_clipsSizeComboBox_currentTextChanged"<<QSettings().value("clipsSizeComboBox")<<arg1;
    if (QSettings().value("clipsSizeComboBox") != arg1)
    {
        QSettings().setValue("clipsSizeComboBox", arg1);
        QSettings().sync();
//        on_exportSizeComboBox_currentTextChanged(ui->exportSizeComboBox->currentText());
    }
}

