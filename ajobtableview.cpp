#include "ajobitemdelegate.h"
#include "ajobtableview.h"

#include <QApplication>
#include <QDialog>
#include <QHeaderView>
#include <QKeyEvent>
#include <QMessageBox>
#include <QSettings>
#include <QTextBrowser>
#include <QTime>
#include <QVBoxLayout>

#include <QDebug>

#include <QDesktopWidget>

static const int idIndex = 0;
//static const int timestampIndex = 1;
static const int folderIndex = 2;
static const int fileIndex = 3;
//static const int actionIndex = 4;
static const int logIndex = 5;
static const int allIndex = 6;

AJobTableView::AJobTableView(QWidget *parent) : QTableView(parent)
{
    jobItemModel = new QStandardItemModel(this);
    QStringList labels;
    labels << "ID"<<"Timestamp"<<"Folder"<<"File"<<"Action"<<"Log"<<"All";
    jobItemModel->setHorizontalHeaderLabels(labels);

    setModel(jobItemModel);

//    AJobItemDelegate *logItemDelegate = new AJobItemDelegate(this);
//    setItemDelegate(logItemDelegate);

    horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    verticalHeader()->setSectionsMovable(true);
    horizontalHeader()->setSectionsClickable(true);
    horizontalHeader()->setStretchLastSection( true );
    setColumnWidth(idIndex, 1);
    setColumnWidth(folderIndex,int(columnWidth(folderIndex) * 1.5));
    setColumnWidth(fileIndex,int(columnWidth(fileIndex) * 1.5));

    setColumnHidden(idIndex, true);
    setColumnHidden(allIndex, true);

    setSortingEnabled(true);

    verticalHeader()->setSectionResizeMode (QHeaderView::Fixed);
}

void AJobTableView::onAddJob(QString folder, QString file, QString action, QString* id)
{
    *id =QString::number(jobItemModel->rowCount()).rightJustified(5, '0');

    QList<QStandardItem *> items;


    items.append(new QStandardItem(*id));
    items.append(new QStandardItem(QTime().currentTime().toString("hh:mm:ss.zzz"))); //timestamp

    QStringList folderlist = folder.split("/", QString::SkipEmptyParts);

    QStandardItem *item = new QStandardItem(folderlist.last());
//    item->setTextAlignment(Qt::AlignRight);
    items.append(item);
    items.append(new QStandardItem(file));
    items.append(new QStandardItem(action));

    jobItemModel->appendRow(items);

    sortByColumn(idIndex, Qt::DescendingOrder);
}

void AJobTableView::onAddLogToJob(QString id, QString log)
{
    for (int row = 0; row < jobItemModel->rowCount();row++)
    {
        if (jobItemModel->index(row, idIndex).data().toString() == id)
        {
            jobItemModel->setData(jobItemModel->index(row, logIndex), log); //last entry
            jobItemModel->setData(jobItemModel->index(row, allIndex), jobItemModel->index(row, allIndex).data().toString() + log); //all entries
        }
    }
}

void AJobTableView::mouseMoveEvent(QMouseEvent *event)
{
    QModelIndex index = indexAt(event->pos());
//    qDebug()<<"AJobTableView::mouseMoveEvent"<<index.data().toString();
}

void AJobTableView::mousePressEvent(QMouseEvent *event)
{
    QModelIndex index = indexAt(event->pos());
//    qDebug()<<"AJobTableView::mousePressEvent"<<index.data().toString();

    if (index.data().toString() != "") //only show if an item is clicked on
    {
        QDialog *dialog = new QDialog(this);
    //    dialog->mapFromGlobal(QCursor::pos());
        dialog->setWindowTitle("Log details of " + jobItemModel->index(index.row(), 1).data().toString());

        QRect savedGeometry = QSettings().value("Geometry").toRect();
        savedGeometry.setX(savedGeometry.x() + savedGeometry.width()/4);
        savedGeometry.setY(savedGeometry.y() + savedGeometry.height()/4);
        savedGeometry.setWidth(savedGeometry.width()/2);
        savedGeometry.setHeight(savedGeometry.height()/2);
        dialog->setGeometry(savedGeometry);

    //    QApplication;
    //    dialog->resize(QApplication.desktop()->width()/2, parentWidget()->size().height()/2);
        QTextBrowser *textBrowser = new QTextBrowser(dialog);
        textBrowser->setWordWrapMode(QTextOption::NoWrap);
        textBrowser->setText(jobItemModel->index(index.row(), allIndex).data().toString());

        QVBoxLayout *m_pDialogLayout = new QVBoxLayout(this);
          m_pDialogLayout->addWidget(textBrowser);

         dialog->setLayout(m_pDialogLayout);

        dialog->show();
    }
}
