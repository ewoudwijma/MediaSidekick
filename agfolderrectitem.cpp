#include "agfolderrectitem.h"

#include <QPen>

#include <QDesktopServices>

#include "agview.h" //for the constants

#include <QDialog>
#include <QSettings>
#include <QStyle>
#include <QTabWidget>
#include <QTextBrowser>
#include <QVBoxLayout>

AGFolderRectItem::AGFolderRectItem(QGraphicsItem *parent, QFileInfo fileInfo) :
    AGViewRectItem(parent, fileInfo)
{
    this->mediaType = "Folder";
    this->itemType = "Base";

    QPen pen(Qt::transparent);
    setPen(pen);

    setRect(QRectF(0, 0, 200 * 9 / 16 + 4, 200 * 9 / 16)); //+2 is minimum to get dd-mm-yyyy not wrapped (+2 extra to be sure)

    setItemProperties("Folder", "Base", 0, QSize());

    pictureItem = new QGraphicsPixmapItem(this);
    QImage image = QImage(":/images/Folder.png");
    QPixmap pixmap = QPixmap::fromImage(image);//.scaled(QSize(200,200 * myImage.height() / myImage.width()))
    pictureItem->setPixmap(pixmap);
    if (image.height() != 0)
        pictureItem->setScale(200.0 * 9.0 / 16.0 / image.height() * 0.8);

//    setItemProperties(pictureItem, "Folder", "SubPicture", folderName, fileName, 0);
    pictureItem->setData(mediaTypeIndex, "Folder");
    pictureItem->setData(itemTypeIndex, "SubPicture");
    pictureItem->setData(folderNameIndex, fileInfo.absolutePath());
    pictureItem->setData(fileNameIndex, fileInfo.fileName());

    pictureItem->setData(mediaDurationIndex, 0);
    pictureItem->setData(mediaWithIndex, 0);
    pictureItem->setData(mediaHeightIndex, 0);
}

void AGFolderRectItem::onItemRightClicked(QGraphicsView *view, QPoint pos)
{
    fileContextMenu->clear();

    fileContextMenu->addAction(new QAction("Zoom to item",fileContextMenu));
    connect(fileContextMenu->actions().last(), &QAction::triggered, [=]()
    {
        view->fitInView(boundingRect()|childrenBoundingRect(), Qt::KeepAspectRatio);
    });
    fileContextMenu->actions().last()->setToolTip(tr("<p><b>%1</b></p>"
                                        "<p><i>Zooms in to %2 and it's details</i></p>"
                                              ).arg(fileContextMenu->actions().last()->text(), fileInfo.fileName()));

    fileContextMenu->addSeparator();

    fileContextMenu->addAction(new QAction("Export",fileContextMenu));
    fileContextMenu->actions().last()->setIcon(qApp->style()->standardIcon(QStyle::SP_DirOpenIcon));
    connect(fileContextMenu->actions().last(), &QAction::triggered, [=]()
    {
        //for each folder
        //for each timeline in folder
        //for each file in folder
        //for each clip in timeline
    });

    fileContextMenu->actions().last()->setToolTip(tr("<p><b>%1</b></p>"
                                        "<p><i>To be done (%2)</i></p>"
                                              ).arg(fileContextMenu->actions().last()->text(), fileInfo.fileName())); //not effective!

    fileContextMenu->addSeparator();

    fileContextMenu->addAction(new QAction("Open in explorer",fileContextMenu));
    fileContextMenu->actions().last()->setIcon(qApp->style()->standardIcon(QStyle::SP_DirOpenIcon));
    connect(fileContextMenu->actions().last(), &QAction::triggered, [=]()
    {
        QDesktopServices::openUrl( QUrl::fromLocalFile( fileInfo.absoluteFilePath()) );
    });

    fileContextMenu->actions().last()->setToolTip(tr("<p><b>%1</b></p>"
                                        "<p><i>Shows the current file or folder %2 in the explorer of your computer</i></p>"
                                              ).arg(fileContextMenu->actions().last()->text(), fileInfo.fileName())); //not effective!

    fileContextMenu->addSeparator();

    fileContextMenu->addAction(new QAction("Properties",fileContextMenu));
    fileContextMenu->actions().last()->setIcon(QIcon(QPixmap::fromImage(QImage(":/MediaSidekick.ico"))));
    connect(fileContextMenu->actions().last(), &QAction::triggered, [=]()
    {
        QDialog * propertiesDialog = new QDialog(view);
        propertiesDialog->setWindowTitle("Media Sidekick Properties");
    #ifdef Q_OS_MAC
        propertiesDialog->setWindowFlag(Qt::WindowStaysOnTopHint); //needed for MAC / OSX
    #endif

        QRect savedGeometry = QSettings().value("Geometry").toRect();
        savedGeometry.setX(savedGeometry.x() + savedGeometry.width()/4);
        savedGeometry.setY(savedGeometry.y() + savedGeometry.height()/4);
        savedGeometry.setWidth(savedGeometry.width()/2);
        savedGeometry.setHeight(savedGeometry.height()/2);
        propertiesDialog->setGeometry(savedGeometry);

        QVBoxLayout *mainLayout = new QVBoxLayout(propertiesDialog);
        QTabWidget *tabWidget = new QTabWidget(propertiesDialog);
        mainLayout->addWidget(tabWidget);

        QTabWidget *subTabWidget = new QTabWidget(tabWidget);
        tabWidget->addTab(subTabWidget, "Jobs");

        foreach (AGProcessAndThread *process, processes)
        {
            QTextBrowser *textBrowser = new QTextBrowser(subTabWidget);
            textBrowser->setWordWrapMode(QTextOption::NoWrap);
            textBrowser->setText(process->log.join("\n"));
            subTabWidget->insertTab(0, textBrowser, process->name);
        }
        subTabWidget->setCurrentIndex(0);


//        if (data(logIndex).toString() != "")
//        {
//            QTextBrowser *textBrowser = new QTextBrowser(tabWidget);
//            textBrowser->setWordWrapMode(QTextOption::NoWrap);
//            textBrowser->setText(data(logIndex).toString());
//            tabWidget->addTab(textBrowser, "Log");
//        }

        propertiesDialog->show();

        fileContextMenu->actions().last()->setToolTip(tr("<p><b>Properties</b></p>"
                                        "<p><i>Show properties for the currently selected folder</i></p>"
                                              "<ul>"
                                              "<li><b>Log</b>: Show the output of folder processes (e.g. load all items, export)</li>"
                                              "</ul>"));

    }  );

    QPointF map = view->mapToGlobal(QPoint(pos.x()+10, pos.y()));
    fileContextMenu->popup(QPoint(map.x(), map.y()));
}
