#include "ftagslistview.h"

#include <QSettings>
#include <QDebug>

static const int orderBeforeLoadIndex = 0;
static const int orderAtLoadIndex = 1;
static const int orderAfterMovingIndex = 2;
static const int folderIndex = 3;
static const int fileIndex = 4;
static const int inIndex = 5;
static const int outIndex = 6;
static const int durationIndex = 7;
static const int ratingIndex = 8;
static const int repeatIndex = 9;
static const int hintIndex = 10;
static const int tagIndex = 11;

FTagsListView::FTagsListView(QWidget *parent) : QListView(parent)
{
    tagsItemModel = new QStandardItemModel(this);
    setModel(tagsItemModel);

    //from designer defaults
    setDragDropMode(DragDropMode::DragDrop);
    setFlow(Flow::LeftToRight);
    setWrapping(true);
    setSpacing(5);
    setHorizontalScrollMode(ScrollPerPixel);
    setVerticalScrollMode(ScrollPerPixel);
//    setMaximumSize(100, size().height());
}

void FTagsListView::onFolderIndexClicked(FEditItemModel *model)
{
//    QString lastFolder = QSettings().value("LastFolder").toString();
    qDebug()<<"FTagsListView::onFolderIndexClicked"<<model->rowCount();
    loadModel(model);
}

void FTagsListView::loadModel(QStandardItemModel *editItemModel)
{
    tagsItemModel->removeRows(0, tagsItemModel->rowCount());
    for (int i = 0; i < editItemModel->rowCount(); i++)
    {
        QStringList splitted = editItemModel->index(i,tagIndex).data().toString().split(";");
        for (int j=0; j < splitted.count(); j++)
        {
            QList<QStandardItem *> foundItems = tagsItemModel->findItems(splitted[j].toLower());
            if (foundItems.count() > 0)
            {
//                    qDebug()<<"Taggg"<<foundItems.count() << foundItems.first()->row() <<foundItems.first()->column() << tagsItemModel->index(foundItems.first()->row(),1).data().toString();
                tagsItemModel->item(foundItems.first()->row(), 1)->setData(tagsItemModel->index(foundItems.first()->row(),1).data().toString() + "I" , Qt::DisplayRole);
            }
            else
            {
                QList<QStandardItem *> items;
                QStandardItem *item = new QStandardItem(splitted[j].toLower());
                item->setBackground(QBrush(Qt::red));
                item->setFont(QFont(font().family(), 8 * devicePixelRatio()));
                items.append(item);
                items.append(new QStandardItem("I"));
                tagsItemModel->appendRow(items);
            }
        }
    }

}
