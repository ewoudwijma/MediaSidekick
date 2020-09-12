#include "agviewrectitem.h"

#include <QSettings>

#include "agcliprectitem.h"
#include "aglobal.h"
#include "agtagtextitem.h"
#include "agview.h"
#include "aexport.h"

#include <QDesktopServices>
#include <QDir>
#include <QGraphicsEffect>
#include <QGraphicsItemAnimation>
#include <QGraphicsProxyWidget>
#include <QSlider>
#include <QStyle>
#include <QTabWidget>
#include <QTextBrowser>
#include <QVBoxLayout>

#include <QTimeLine>

#include <QMessageBox>

AGViewRectItem::AGViewRectItem(QGraphicsItem *parent, QFileInfo fileInfo) :
    QGraphicsRectItem(parent)
{
    this->mediaType = "Unknown";
    this->itemType = "Base";

    this->fileInfo = fileInfo;
//    setData(folderNameIndex, fileInfo.absolutePath());
//    setData(fileNameIndex, fileInfo.fileName());

    this->parentRectItem = (AGViewRectItem *)parent;

    this->setFlag(QGraphicsItem::ItemIsSelectable);
    this->setAcceptHoverEvents(true);

    fileContextMenu = new QMenu();
//    setContextMenuPolicy(Qt::CustomContextMenu);
    fileContextMenu->setToolTipsVisible(true);

//    fileContextMenu->eventFilter()

    QPalette pal = fileContextMenu->palette();
    QColor menuColor;
    if (QSettings().value("theme") == "Black")
    {
        menuColor = QColor(41, 42, 45);//80,80,80);
        fileContextMenu->setStyleSheet(R"(
                                   QMenu::separator {
                                    background-color: darkgray;
                                   }
                                       QMenu {
                                        border: 1px solid darkgray;
                                       }

                                 )");
    }
    else
        menuColor = pal.window().color();

    pal.setColor(QPalette::Base, menuColor);
    pal.setColor(QPalette::Window, menuColor);
    fileContextMenu->setPalette(pal);
}

void AGViewRectItem::newProgressItem()
{
    if (progressSliderProxy == nullptr)
    {
        QSlider *progressSlider = new QSlider();
        progressSlider->setOrientation(Qt::Horizontal);

        progressSlider->setMinimumWidth(this->rect().width());
        progressSlider->setMaximumWidth(this->rect().width());

//            progressSlider->setStyleSheet("QSlider::sub-page:Horizontal { background-color: blue; }"
//                          "QSlider::add-page:Horizontal { background-color: yellow; }"
//                          "QSlider::groove:Horizontal {border-image: url(:/images/MovieStrip.PNG) 0 12 0 10;border-left: 10px solid transparent;border-right: 12px solid transparent;}"
//                          "QSlider::handle:Horizontal { width:10px; border-radius:5px; background:magenta; margin: -5px 0px -5px 0px; }");

//        progressSlider->setStyleSheet("QSlider::sub-page:Horizontal { background-color: #222222; }"
//                      "QSlider::add-page:Horizontal { background-color: #222222; }"
//                      "QSlider::groove:Horizontal { background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #B1B1B1, stop:1 #c4c4c4)}; height:4px; }"
//                      "QSlider::handle:Horizontal { width:10px; border-radius:5px; background:#9F2425; margin: -5px 0px -5px 0px; }");

        progressSlider->setStyleSheet("background-color: rgba(0,0,0,0)");
//                                      "QSlider::sub-page:Horizontal { background-color: blue; }"
//                      "QSlider::add-page:Horizontal { background-color: yellow; }"
//                      "QSlider::groove:Horizontal { background: transparent; height:1px; }"
//                      "QSlider::handle:Horizontal { width:10px; border-radius:5px; background:magenta; margin: -5px 0px -5px 0px; }");


//            QSlider::groove {
//               border-image: url(:/slider-decoration.png) 0 12 0 10;
//               border-left: 10px solid transparent;
//               border-right: 12px solid transparent;
//            }
//            QSlider::handle {
//                width: 8px;
//                background-color: green;
//            }

        progressSliderProxy = new QGraphicsProxyWidget(this);
        progressSliderProxy->setWidget(progressSlider);

        progressSliderProxy->setData(mediaTypeIndex, mediaType);
        progressSliderProxy->setData(itemTypeIndex, "SubProgressSlider");
        progressSliderProxy->setPos(0, boundingRect().height() - progressSliderProxy->rect().height() ); //pos need to set here as arrangeitem not called here
    }
}

void AGViewRectItem::newSubLogItem()
{
    if (subLogItem == nullptr)
    {
        subLogItem = new QGraphicsTextItem(this);

        if (QSettings().value("theme") == "Black")
            subLogItem->setDefaultTextColor(Qt::white);
        else
            subLogItem->setDefaultTextColor(Qt::black);

        subLogItem->setData(itemTypeIndex, "SubLogItem");
        subLogItem->setData(mediaTypeIndex, "MediaFile");

        subLogItem->setPos(boundingRect().height() * 0.1, boundingRect().height());// - subLogItem->boundingRect().height() ); //pos need to set here as arrangeitem not called here

        subLogItem->setTextWidth(boundingRect().width() * 0.8);

        subLogItem->setZValue(9999);
    }
}

void AGViewRectItem::setTextItem(QTime time, QTime totalTime)
{
    if (subLogItem == nullptr)
    {
        newSubLogItem();
    }

    if (subLogItem != nullptr)
    {
        subLogItem->setHtml(tr("<p>%1</p><p><small><i>%2</i></small></p><p><small><i>%3</i></small></p>").arg(fileInfo.fileName(), time.toString() + " / " + totalTime.toString(), lastOutputString));
    }

//        subLogItem->setHtml(tr("<p>%1</p><p><small><i>%2</i></small></p>").arg(fileInfo.fileName(), QTime::fromMSecsSinceStartOfDay(progress).toString("hh:mm:ss.zzz") + " / " + QTime::fromMSecsSinceStartOfDay(m_player->duration()).toString("hh:mm:ss.zzz")));

}

void AGViewRectItem::updateToolTip()
{
    QString tooltipText = "";

    QString commandControl = "Ctrl-";
#ifdef Q_OS_MAC
    commandControl = "⌘-";
#endif

    AGViewRectItem *tooltipItem = this;
    if (data(itemTypeIndex).toString().contains("Sub"))
        tooltipItem = (AGViewRectItem *)parentItem();

    if (tooltipItem->data(mediaTypeIndex) == "Folder")
    {
        tooltipText += tr("<p><b>Folder %1</b></p>"
                       "<p><i>Folder with its subfolders and media files. Only <b>folders containing mediafiles</b> are shown</i></p>"
                       ).arg(tooltipItem->fileInfo.absoluteFilePath());
        tooltipText += tr(
                    "<ul>"
                          "<li><b>Media Sidekick recycle bin</b>: Right click / Open in explorer will show the Media Sidekick recycle bin</li>"
                      "</ul>"
        );
    }
    else if (tooltipItem->data(mediaTypeIndex) == "FileGroup")
    {
        tooltipText += tr("<p><b>%1 Files in folder %2</b></p>"
                          "<ul>"
                          ).arg(tooltipItem->fileInfo.fileName(), tooltipItem->fileInfo.absolutePath());
        if (tooltipItem->fileInfo.fileName() == "Video")
            tooltipText += tr("<li><b>Supported Video files</b>: %1</li>"
                              "</ul>").arg(AGlobal().videoExtensions.join(","));
        else if (tooltipItem->fileInfo.fileName() == "Audio")
            tooltipText += tr("<li><b>Supported Audio files</b>: %1</li>"
                              "</ul>").arg(AGlobal().audioExtensions.join(","));
        else if (tooltipItem->fileInfo.fileName() == "Image")
            tooltipText += tr("<li><b>Supported Image files</b>: %1</li>"
                              "</ul>").arg(AGlobal().imageExtensions.join(","));
        else if (tooltipItem->fileInfo.fileName() == "Export")
            tooltipText += tr("<li><b>Export files</b>: Videos generated by Media Sidekick (lossless or encode). Can be openened by common video players, uploaded to social media etc.</li>"
                              "<li><b>Supported Export methods</b>: %1</li>"
                              "<li><b>Supported Export files</b>: %2</li>"
                              "</ul>").arg(AGlobal().exportMethods.join(","), AGlobal().exportExtensions.join(","));
        else if (tooltipItem->fileInfo.fileName() == "Project")
            tooltipText += tr("<li><b>Project files</b>: generated by Media Sidekick. Can be openened by video editors. Currently Shotcut and Premiere.</li>"
                              "<li><b>Supported Export methods</b>: %1</li>"
                              "<li><b>Supported Export files</b>: %2</li>"
                              "</ul>").arg(AGlobal().exportMethods.join(","), AGlobal().projectExtensions.join(","));
        else if (tooltipItem->fileInfo.fileName() == "Parking")
            tooltipText += tr("<li><b>Parking</b>: Mediafiles or clips which do not match the filter criteria are moved here (see the search field).</li>"
                              "<li><b>Timeline view</b>: Mediafiles without clips are moved to the parking (In spotview they are shown in the video and audio group).</li>"
                              "</ul>");
        else
            tooltipText += tr("<li><b>Group</b>: %2</li>"
                              "</ul>").arg(tooltipItem->fileInfo.fileName());
    }
    else if (tooltipItem->data(mediaTypeIndex) == "TimelineGroup")
    {
        tooltipText += tr("<p><b>Timeline group of %1 of folder %2</b></p>"
                       "<p><i>Showing all clips next to each other</i></p>"
                          "<ul>"
                          "<li><b>Sorting</b>: Currently sorted in order of mediafiles and within a mediafile, sorted chronologically</li>"
                          "<li><b>Transition</b>: If transition defined (%3) then clips overlapping.</li>"
                          ).arg(tooltipItem->parentRectItem->fileInfo.fileName(), tooltipItem->fileInfo.absolutePath(), AGlobal().frames_to_time(QSettings().value("transitionTime").toInt()));
//        tooltipText += tr("<li><b>zValue</b>: %1</li>").arg(QString::number(tooltipItem->zValue()));

    }
    else if (tooltipItem->data(mediaTypeIndex) == "MediaFile")
    {
    }
    else if (tooltipItem->data(mediaTypeIndex) == "Clip")
    {
        AGClipRectItem *clipItem = (AGClipRectItem *)tooltipItem;
        tooltipText += tr("<p><b>Clip from %3 to %4</b></p>"
                       "<ul>"
             "<li><b>Duration</b>: %1 (%2 s)</li>"
                       "</ul>").arg(AGlobal().msec_to_timeNoFPS(clipItem->duration), QString::number(clipItem->duration / 1000.0)
                                    , AGlobal().msec_to_timeNoFPS(clipItem->clipIn), AGlobal().msec_to_timeNoFPS(clipItem->clipOut)
                         );

//        tooltipText += tr("<li><b>zValue</b>: %1</li>").arg(QString::number(tooltipItem->zValue()));
    }
    else if (tooltipItem->data(mediaTypeIndex) == "Tag")
    {
        AGTagTextItem *tagItem = (AGTagTextItem *)tooltipItem;
        tooltipText += tr("<p><b>Rating, Tags and alike %1</b></p>"
                       "<p><i>Rating from * to *****, alike indicator or tag</i></p>"
                       "<ul>"
                       "</ul>"
            "<p><b>Actions</p>"
            "<ul>"
                          "<li><b>Remove</b>: Drag to bin (in next version of Media Sidekick)</li>"
            "</ul>").arg(tagItem->tagName);
    }

    if (tooltipText != "")
        setToolTip(tooltipText);
}

QString AGViewRectItem::itemToString()
{
    return fileInfo.absoluteFilePath() + " " + data(itemTypeIndex).toString() + " " + data(mediaTypeIndex).toString();// + " " + QString::number(item->zValue());
}

QVariant AGViewRectItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemPositionHasChanged && scene())
    {
        qDebug()<<"AGViewRectItem::itemChange"<<change<<value;
//        emit agItemChanged(this);
    }

    return QGraphicsRectItem::itemChange(change, value);
}

//https://forum.qt.io/topic/32869/solved-resize-qgraphicsitem-with-drag-and-drop/3

void AGViewRectItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
//    qDebug()<<"AGViewRectItem::mousePressEvent"<<event;
    QGraphicsRectItem::mousePressEvent(event); //triggers itemselected
}

void AGViewRectItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
//    qDebug()<<"AGViewRectItem::hoverMoveEvent"<<event<<this->fileInfo.fileName();

    if (event->modifiers() == Qt::ShiftModifier)
    {
//        if (event->pos().y() > rect().height() * 0.7) //only hover on timeline
        {
            int progress;

            if (mediaType == "Clip")
            {
                AGClipRectItem *clipItem = (AGClipRectItem *)this;

//                qDebug()<<"clipItem"<<clipItem->itemToString()<<clipItem->clipIn<<clipItem->clipOut;

                progress = clipItem->clipIn + (clipItem->clipOut - clipItem->clipIn) * event->pos().x()/rect().width();
            }
            else
                progress = duration * event->pos().x()/rect().width();

//            QGraphicsItem *senderItem = (QGraphicsItem *)sender();

//            qDebug()<<"AGViewRectItem::hoverMoveEvent"<<"emit hoverPositionChanged"<<senderItem->data(fileNameIndex).toString()<<fileInfo.fileName()<<mediaType<<itemType<<progress<<duration<<event->pos().x()/rect().width();
            emit hoverPositionChanged(this, progress);
        }
    }

    QGraphicsRectItem::hoverMoveEvent(event);
}

void AGViewRectItem::recursiveFileRenameCopyIfExists(QString folderName, QString fileName)
{
    bool success;
    QFile file(folderName + fileName);
    if (file.exists())
    {
//        qDebug()<<"AFilesTreeView::recursiveFileRenameCopyIfExists"<<fileName<<fileName.left(fileName.lastIndexOf(".")) + "BU." + fileInfo.suffix();

        recursiveFileRenameCopyIfExists(folderName, fileName.left(fileName.lastIndexOf(".")) + "BU." + fileInfo.suffix());

        success = file.rename(folderName + fileName.left(fileName.lastIndexOf(".")) + "BU." + fileInfo.suffix());
    }
}

void AGViewRectItem::processAction(QString action)
{
    AGView *agView = (AGView *)scene()->views().first();

    if (action.contains("Export"))
    {
        if (agView->isLoading)
            QMessageBox::information(agView, "Export", tr("Files are loading, wait until finished"));
        else
        {
            MExportDialog *exportDialog = new MExportDialog(nullptr, this);
            exportDialog->processes = &processes;
            exportDialog->show();
            connect(exportDialog, &MExportDialog::processOutput, this, &AGViewRectItem::onProcessOutput);
            connect(exportDialog, &MExportDialog::arrangeItems, agView, &AGView::onArrangeItems);
            connect(exportDialog, &MExportDialog::fileWatch, agView, &AGView::fileWatch);
        }
    }
}

void AGViewRectItem::onItemRightClicked(QPoint pos)
{
    QGraphicsView *view = scene()->views().first();

    fileContextMenu->addAction(new QAction("Zoom to item",fileContextMenu));
    fileContextMenu->actions().last()->setIcon(QIcon(QPixmap::fromImage(QImage(":/images/zoomin.png"))));
    connect(fileContextMenu->actions().last(), &QAction::triggered, [=]()
    {
        view->fitInView(boundingRect()|childrenBoundingRect(), Qt::KeepAspectRatio);
    });

    fileContextMenu->actions().last()->setToolTip(tr("<p><b>%1</b></p>"
                                        "<p><i>Zooms in to %2 and it's details</i></p>"
                                              ).arg(fileContextMenu->actions().last()->text(), fileInfo.fileName()));


    fileContextMenu->addSeparator();

    fileContextMenu->addAction(new QAction("Export",fileContextMenu));
    fileContextMenu->actions().last()->setIcon(QIcon(QPixmap::fromImage(QImage(":/Spinner.gif"))));
    fileContextMenu->actions().last()->setShortcut(QKeySequence(tr("Ctrl+E")));
    connect(fileContextMenu->actions().last(), &QAction::triggered, [=]()
    {
        processAction("Export");
    });

    fileContextMenu->actions().last()->setToolTip(tr("<p><b>%1</b></p>"
                                        "<p><i>Export all clips of %2</i></p>"
                                                     "<ul>"
                                                     "<li><b>Select</b>: An export window will open where export parameters can be selected.</li>"
                                                     "</ul>"
                                              ).arg(fileContextMenu->actions().last()->text(), fileInfo.fileName())); //not effective!

    int nrOfActiveJobs = 0;
    foreach (AGProcessAndThread *process, processes)
    {
        if ((process->process != nullptr && process->process->state() != QProcess::NotRunning) || (process->jobThread != nullptr && process->jobThread->isRunning()))
        {
            nrOfActiveJobs++;
        }
    }

    if (nrOfActiveJobs > 0)
    {
        fileContextMenu->addAction(new QAction("Cancel jobs",fileContextMenu));
        fileContextMenu->actions().last()->setIcon(qApp->style()->standardIcon(QStyle::SP_BrowserStop));
        connect(fileContextMenu->actions().last(), &QAction::triggered, [=]()
        {
            foreach (AGProcessAndThread *process, processes)
            {
                if ((process->process != nullptr && process->process->state() != QProcess::NotRunning) || (process->jobThread != nullptr && process->jobThread->isRunning()))
                {
                    qDebug()<<"AGViewRectItem::onItemRightClicked Killing process"<<fileInfo.fileName()<<process->name<<process->process<<process->jobThread;
                    process->kill();
                }
            }
        });
        fileContextMenu->actions().last()->setToolTip(tr("<p><b>%1</b></p>"
                                                         "<p><i>Cancel running jobs of %2</i></p>"
                                                               ).arg(fileContextMenu->actions().last()->text(), fileInfo.fileName()));
    }

    fileContextMenu->addSeparator();

    fileContextMenu->addAction(new QAction("Open in explorer",fileContextMenu));
    fileContextMenu->actions().last()->setIcon(qApp->style()->standardIcon(QStyle::SP_DirOpenIcon));
    connect(fileContextMenu->actions().last(), &QAction::triggered, [=]()
    {
        if (strcmp(metaObject()->className() , "AGMediaFileRectItem") == 0)
        {
#ifdef Q_OS_MAC
            QStringList args;
            args << "-e";
            args << "tell application \"Finder\"";
            args << "-e";
            args << "activate";
            args << "-e";
            args << "select POSIX file \"" + fileInfo.absolutePath() + "/" + fileInfo.fileName() + "\"";
            args << "-e";
            args << "end tell";
            QProcess::startDetached("osascript", args);
#endif

#ifdef Q_OS_WIN
            QStringList args;
            args << "/select," << QDir::toNativeSeparators(fileInfo.absoluteFilePath());
            QProcess::startDetached("explorer", args);
#endif
        }
        else if (strcmp(metaObject()->className() , "MGroupRectItem") == 0)
            QDesktopServices::openUrl( QUrl::fromLocalFile( fileInfo.absolutePath()) );
        else
            QDesktopServices::openUrl( QUrl::fromLocalFile( fileInfo.absoluteFilePath()) );

    });

    fileContextMenu->actions().last()->setToolTip(tr("<p><b>%1</b></p>"
                                        "<p><i>Shows the current file or folder %2 in the explorer of your computer</i></p>"
                                              ).arg(fileContextMenu->actions().last()->text(), fileInfo.fileName())); //not effective!

    if (strcmp(metaObject()->className() , "AGMediaFileRectItem") == 0)
    {
        fileContextMenu->addAction(new QAction("Open in default application",fileContextMenu));
        fileContextMenu->actions().last()->setIcon(QIcon(QPixmap::fromImage(QImage(":/images/shotcut-logo-320x320.png"))));
        connect(fileContextMenu->actions().last(), &QAction::triggered, [=]()
        {
            QDesktopServices::openUrl( QUrl::fromLocalFile( fileInfo.absoluteFilePath()) );
        });
    }

    fileContextMenu->addSeparator();

    fileContextMenu->addAction(new QAction("Jobs",fileContextMenu));
    fileContextMenu->actions().last()->setIcon(QIcon(QPixmap::fromImage(QImage(":/MediaSidekick.ico"))));
    connect(fileContextMenu->actions().last(), &QAction::triggered, [=]()
    {
        QDialog * dialog = new QDialog(view);
        dialog->setWindowTitle("Media Sidekick Jobs");
    #ifdef Q_OS_MAC
        dialog->setWindowFlag(Qt::WindowStaysOnTopHint); //needed for MAC / OSX
    #endif

        QRect savedGeometry = QSettings().value("Geometry").toRect();
        savedGeometry.setX(savedGeometry.x() + savedGeometry.width()/4);
        savedGeometry.setY(savedGeometry.y() + savedGeometry.height()/4);
        savedGeometry.setWidth(savedGeometry.width()/2);
        savedGeometry.setHeight(savedGeometry.height()/2);
        dialog->setGeometry(savedGeometry);

        QVBoxLayout *mainLayout = new QVBoxLayout(dialog);
        QTabWidget *tabWidget = new QTabWidget(dialog);
        mainLayout->addWidget(tabWidget);

        foreach (AGProcessAndThread *process, processes)
        {
            QTextBrowser *textBrowser = new QTextBrowser(tabWidget);
            textBrowser->setWordWrapMode(QTextOption::NoWrap);
            textBrowser->setText(process->log.join("\n"));
            tabWidget->insertTab(0, textBrowser, process->name.replace(" " + fileInfo.fileName(), ""));
        }
        tabWidget->setCurrentIndex(0);

        dialog->show();

    });
    fileContextMenu->actions().last()->setToolTip(tr("<p><b>Jobs</b></p>"
                                    "<p><i>Show jobs for %1</i></p>"
                                          "<ul>"
                                          "<li><b>Log</b>: Show the output of processes for %1 (e.g. load all items, export)</li>"
                                          "</ul>").arg(fileInfo.fileName()));
}

void AGViewRectItem::onProcessOutput(QTime time, QTime totalTime, QString event, QString outputString)
{
    AGProcessAndThread *processAndThread = (AGProcessAndThread *)sender();

//    qDebug()<<"AGFolderRectItem::onProcessOutput"<<processAndThread->name<<time<<event<<outputString;//thread->log.join("\n");

    lastOutputString = outputString;
    setTextItem(time, totalTime); //needed here as onPositionChanged not applicable here

    if (progressSliderProxy == nullptr)
    {
        newProgressItem();
    }

    //update progressLine (as arrangeitem not called here)
    if (progressSliderProxy != nullptr && 60 != 0)
    {
        QSlider *progressSlider = (QSlider *)progressSliderProxy->widget();
        progressSlider->setMaximum(totalTime.msecsSinceStartOfDay());
        progressSlider->setSingleStep(progressSlider->maximum() / 10);
        progressSlider->setValue(time.msecsSinceStartOfDay());
    }

    if (event == "started")
    {
        if (strcmp(metaObject()->className() , "AGMediaFileRectItem") != 0)
        {
            QPixmap base = QPixmap::fromImage(QImage(":/images/Folder.png")).scaledToWidth(56);
            QPixmap overlay = QPixmap::fromImage(QImage(":/Spinner.gif")).scaledToWidth(32);
            QPixmap result(base.width(), base.height());
            result.fill(Qt::transparent); // force alpha channel
            QPainter painter(&result);
            painter.drawPixmap(0, 0, base);
            painter.drawPixmap((base.width() - overlay.width()) / 2, (base.height() - overlay.height()) / 2, overlay);

            pictureItem->setPixmap(result);
            if (base.height() != 0)
                pictureItem->setScale(200.0 * 9.0 / 16.0 / base.height() * 0.8);
        }
    }
    if (event == "finished" || event == "error")
    {
        if (strcmp(metaObject()->className() , "AGMediaFileRectItem") != 0)
        {
            QImage image = QImage(":/images/Folder.png");
            QPixmap pixmap = QPixmap::fromImage(image);//.scaled(QSize(200,200 * myImage.height() / myImage.width()))
            pictureItem->setPixmap(pixmap);
            if (image.height() != 0)
                pictureItem->setScale(200.0 * 9.0 / 16.0 / image.height() * 0.8);
        }
    }

    if (event == "finished")
    {
        if (progressSliderProxy != nullptr)
        {
            delete progressSliderProxy;
            progressSliderProxy = nullptr;
        }

//        subLogItem->setHtml("");
        lastOutputString = "";
        setTextItem(QTime(), QTime());
    }
}
