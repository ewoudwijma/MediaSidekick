#include "aglobal.h"
#include "agtagtextitem.h"

#include <QSettings>

#include <QDebug>
#include <QGraphicsSceneMouseEvent>

#include "agview.h"

AGTagTextItem::AGTagTextItem(QGraphicsItem *parent, QFileInfo fileInfo, QString tagName) :
    QGraphicsTextItem(parent)
{
//    changed = true;
    this->mediaType = "Tag";
    this->itemType = "Base";

    this->fileInfo = fileInfo;

    this->tagName = tagName;

    clipItem = (AGClipRectItem *) parent;

    this->clipItem->tags << this;

    //    setItemProperties(tagItem, mediaType, "Base", folderName, fileName, duration, QSize(), clipIn, clipOut, tagName);

    setData(itemTypeIndex, itemType);
    setData(mediaTypeIndex, mediaType);
    setData(folderNameIndex, fileInfo.absolutePath());
    setData(fileNameIndex, fileInfo.fileName());

//    qDebug()<<"AGTagTextItem::AGTagTextItem"<<fileInfo.fileName()<<tagName;

    if (tagName == "*" || tagName == "**" || tagName == "***" || tagName == "****" || tagName == "*****")
        setDefaultTextColor(Qt::yellow);
    else if (QSettings().value("theme") == "Black")
        setDefaultTextColor(Qt::white);
    else
        setDefaultTextColor(Qt::black);

    if (AGlobal().audioExtensions.contains(fileInfo.suffix(), Qt::CaseInsensitive))
        setHtml(QString("<div align=\"center\">") + tagName + "</div>");//#008000 darkgreen style='background-color: rgba(0, 128, 0, 0.5);'
    else
        setHtml(QString("<div align=\"center\">") + tagName + "</div>");//blue-ish #2a82da  style='background-color: rgba(42, 130, 218, 0.5);'
//        setPlainText(tagName);

    setFlag(QGraphicsItem::ItemIsMovable, true); //to put in bin

//    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
//    this->setFlag(QGraphicsItem::ItemIsSelectable);
//    this->setAcceptHoverEvents(true);
//    this->setAcceptDrops(true);

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

void AGTagTextItem::onItemRightClicked(QPoint pos)
{
    QGraphicsView *view = scene()->views().first();

    fileContextMenu->clear();

    fileContextMenu->addAction(new QAction("Zoom to item",fileContextMenu));
    connect(fileContextMenu->actions().last(), &QAction::triggered, [=]()
    {
        view->fitInView(boundingRect()|childrenBoundingRect(), Qt::KeepAspectRatio);
    });

    fileContextMenu->actions().last()->setToolTip(tr("<p><b>%1</b></p>"
                                        "<p><i>Zooms in to %2 and it's details</i></p>"
                                              ).arg(fileContextMenu->actions().last()->text(), fileInfo.fileName()));

    fileContextMenu->addAction(new QAction("Delete tag",fileContextMenu));
    connect(fileContextMenu->actions().last(), &QAction::triggered, [=]()
    {
        emit deleteItem(true, "Tag", fileInfo, clipItem->clipIn, tagName);
    });

    fileContextMenu->actions().last()->setToolTip(tr("<p><b>%1</b></p>"
                                        "<p><i>Remove %2</i></p>"
                                              ).arg(fileContextMenu->actions().last()->text(), fileInfo.fileName()));

    QPointF map = view->mapToGlobal(QPoint(pos.x()+10, pos.y()));
    fileContextMenu->popup(QPoint(map.x(), map.y()));
}

void AGTagTextItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    qDebug()<<"AGTagTextItem::mousePressEvent"<<event;
    if (event->button() == Qt::LeftButton)
    {

    }
    else
        QGraphicsTextItem::mousePressEvent(event);

}

void AGTagTextItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
//    qDebug()<<"AGTagTextItem::hoverMoveEvent"<<event->pos();

    if(event->modifiers() == Qt::ShiftModifier)
    {
//        if (event->pos().y() > rect().height() * 0.7) //only hover on timeline
        {
            int progress;

            if (mediaType == "Tag") //it always is
            {
//                qDebug()<<"clipItem"<<clipItem->itemToString()<<clipItem->clipIn<<clipItem->clipOut;

                progress = clipItem->clipIn + (clipItem->clipOut - clipItem->clipIn) * event->pos().x()/clipItem->rect().width();
            }
            else
                progress = data(mediaDurationIndex).toInt() * event->pos().x()/clipItem->rect().width();

//            qDebug()<<"AGTagTextItem::hoverMoveEvent"<<progress<<data(mediaDurationIndex).toInt()<<event->pos().x()/clipItem->rect().width();

            emit hoverPositionChanged(clipItem, progress);
        }
    }

    QGraphicsTextItem::hoverMoveEvent(event);

}
