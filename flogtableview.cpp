#include "flogtableview.h"

#include <QHeaderView>
#include <QTime>

FLogTableView::FLogTableView(QWidget *parent) : QTableView(parent)
{
    logItemModel = new QStandardItemModel(this);
    QStringList labels;
    labels << "Timestamp"<<"Function"<<"Log";
    logItemModel->setHorizontalHeaderLabels(labels);

    setModel(logItemModel);

    horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    verticalHeader()->setSectionsMovable(true);
    horizontalHeader()->setSectionsClickable(true);
    horizontalHeader()->setStretchLastSection( true );

    setSortingEnabled(true);


    onAddEntry("Test1");
    onAddLogToEntry("Test1", "Row1");
    onAddLogToEntry("Test1", "Row2");
    onAddEntry("Test2");
    onAddLogToEntry("Test2", "Row1");
    onAddLogToEntry("Test2", "Row2");
}

void FLogTableView::onAddEntry(QString function)
{
    QList<QStandardItem *> items;

    items.append(new QStandardItem(QTime().currentTime().toString()));
    items.append(new QStandardItem(function));

    logItemModel->appendRow(items);
    sortByColumn(0, Qt::DescendingOrder);
}

void FLogTableView::onAddLogToEntry(QString function, QString log)
{
    for (int row = 0; row < logItemModel->rowCount();row++)
    {
        if (logItemModel->index(row, 1).data().toString() == function)
            logItemModel->setData(logItemModel->index(row, 2), logItemModel->index(row, 2).data().toString() + log);
    }
}
