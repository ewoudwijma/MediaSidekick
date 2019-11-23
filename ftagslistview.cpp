#include "ftagslistview.h"

#include <QSettings>
#include <QDebug>

#include "fglobal.h"

ATagsListView::ATagsListView(QWidget *parent) : QListView(parent)
{
    tagsItemModel = new QStandardItemModel(this);
    setModel(tagsItemModel);

    //from designer defaults
    setDragDropMode(DragDropMode::DragOnly);
//    setDefaultDropAction(Qt::MoveAction);

    setFlow(Flow::LeftToRight);
    setWrapping(true);
    setSpacing(5);
    setHorizontalScrollMode(ScrollPerPixel);
    setVerticalScrollMode(ScrollPerPixel);
//    setMaximumSize(100, size().height());
    connect(this, &ATagsListView::doubleClicked, this, &ATagsListView::onDoubleClicked);
}

void ATagsListView::onFolderIndexClicked(QAbstractItemModel *model)
{
//    QString lastFolder = QSettings().value("LastFolder").toString();
//    qDebug()<<"ATagsListView::onFolderIndexClicked"<<model->rowCount();
    loadModel(model);
}

bool ATagsListView::addTag(QString tagString)
{
    QList<QStandardItem *> foundItems = tagsItemModel->findItems(tagString);
    if (foundItems.count() > 0)
    {
//                    qDebug()<<"Taggg"<<foundItems.count() << foundItems.first()->row() <<foundItems.first()->column() << tagsItemModel->index(foundItems.first()->row(),1).data().toString();
        tagsItemModel->item(foundItems.first()->row(), 1)->setData(tagsItemModel->index(foundItems.first()->row(),1).data().toString() + "I" , Qt::DisplayRole);
        return false;
    }
    else
    {
        QList<QStandardItem *> items;
        QStandardItem *item = new QStandardItem(tagString);
        item->setBackground(QBrush(Qt::red));
//                item->setFont(QFont(font().family(), 8 * devicePixelRatio()));
        items.append(item);
        items.append(new QStandardItem("I")); //nr of occurrences
        tagsItemModel->appendRow(items);
        return true;
    }
}

void ATagsListView::loadModel(QAbstractItemModel *editItemModel)
{
//    qDebug() << "ATagsListView::loadModel" << editItemModel->rowCount();

    tagsItemModel->removeRows(0, tagsItemModel->rowCount());
    for (int i = 0; i < editItemModel->rowCount(); i++)
    {
        QStringList tagList = editItemModel->index(i,tagIndex).data().toString().split(";", QString::SkipEmptyParts);
//        if (tagList.count()==1 && tagList[0] == "")
//            tagList.clear();

        for (int j=0; j < tagList.count(); j++)
        {
            addTag(tagList[j].toLower());
        }
    }
//    qDebug() << "ATagsListView::loadModel done" << editItemModel->rowCount();
}

void ATagsListView::onDoubleClicked(const QModelIndex &index)
{
    qDebug()<<"ATagsListView::onDoubleClicked";
    tagsItemModel->takeRow(index.row());
    //tbd: move to addtagfield
}
