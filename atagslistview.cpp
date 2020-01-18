#include "atagslistview.h"

#include <QSettings>
#include <QDebug>

#include "aglobal.h"

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

    setDragDropMode(QListView::DragDrop);
    setDefaultDropAction(Qt::MoveAction);
    setSpacing(4);
    setEditTriggers(QAbstractItemView::NoEditTriggers);

    connect(tagsItemModel, &QStandardItemModel::dataChanged,  this, &ATagsListView::onTagChanged);
    connect(tagsItemModel, &QStandardItemModel::rowsRemoved,  this, &ATagsListView::onTagChanged); //datachanged not signalled when removing
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
        item->setBackground(QBrush(Qt::darkGray));
        item->setForeground(QBrush(Qt::white));
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
        stringToModel(editItemModel->index(i,tagIndex).data().toString());
    }
//    qDebug() << "ATagsListView::loadModel done" << editItemModel->rowCount();
}

void ATagsListView::stringToModel(QString string)
{
    QStringList tagList = string.split(";", QString::SkipEmptyParts);

    for (int j=0; j < tagList.count(); j++)//tbd: add as method of tagslistview
    {
        addTag(tagList[j].toLower());
    }
}

QString ATagsListView::modelToString()
{
    QString string = "";
    QString sep = "";
    for (int i=0; i < tagsItemModel->rowCount();i++)
    {
        string += sep + tagsItemModel->index(i,0).data().toString();
        sep = ";";
    }
    return string;
}

void ATagsListView::onDoubleClicked(const QModelIndex &index)
{
    qDebug()<<"ATagsListView::onDoubleClicked";
    tagsItemModel->takeRow(index.row());
    //tbd: move to addtagfield
}

void ATagsListView::onTagChanged()
{
    emit tagChanged(modelToString());
}
