#include "alogitemdelegate.h"
#include "alogtableview.h"

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
static const int timestampIndex = 1;
static const int folderIndex = 2;
static const int fileIndex = 3;
//static const int actionIndex = 4;
static const int logIndex = 5;
static const int allIndex = 6;

ALogTableView::ALogTableView(QWidget *parent) : QTableView(parent)
{
    logItemModel = new QStandardItemModel(this);
    QStringList labels;
    labels << "ID"<<"Timestamp"<<"Folder"<<"File"<<"Action"<<"Log"<<"All";
    logItemModel->setHorizontalHeaderLabels(labels);

    setModel(logItemModel);

//    ALogItemDelegate *logItemDelegate = new ALogItemDelegate(this);
//    setItemDelegate(logItemDelegate);

    horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    verticalHeader()->setSectionsMovable(true);
    horizontalHeader()->setSectionsClickable(true);
    horizontalHeader()->setStretchLastSection( true );
    setColumnWidth(idIndex, 1);
    setColumnWidth(folderIndex,int(columnWidth(folderIndex) * 1.5));
    setColumnWidth(fileIndex,int(columnWidth(fileIndex) * 3));

    setColumnHidden(idIndex, true);
    setColumnHidden(allIndex, true);

    setSortingEnabled(true);

    verticalHeader()->setSectionResizeMode (QHeaderView::Fixed);
}

void ALogTableView::onAddEntry(QString folder, QString file, QString action, QString* id)
{
    *id =QString::number(logItemModel->rowCount()).rightJustified(5, '0');

    QList<QStandardItem *> items;


    items.append(new QStandardItem(*id));
    items.append(new QStandardItem(QTime().currentTime().toString("hh:mm:ss.zzz"))); //timestamp

    QStringList folderlist = folder.split("/", QString::SkipEmptyParts);

    QStandardItem *item = new QStandardItem(folderlist.last());
//    item->setTextAlignment(Qt::AlignRight);
    items.append(item);
    items.append(new QStandardItem(file));
    items.append(new QStandardItem(action));

    logItemModel->appendRow(items);

    sortByColumn(idIndex, Qt::DescendingOrder);
}

void ALogTableView::onAddLogToEntry(QString id, QString log)
{
    for (int row = 0; row < logItemModel->rowCount();row++)
    {
        if (logItemModel->index(row, idIndex).data().toString() == id)
        {
            logItemModel->setData(logItemModel->index(row, logIndex), log); //last entry
            logItemModel->setData(logItemModel->index(row, allIndex), logItemModel->index(row, allIndex).data().toString() + log); //all entries
        }
    }
}

void ALogTableView::mouseMoveEvent(QMouseEvent *event)
{
    QModelIndex index = indexAt(event->pos());
//    qDebug()<<"ALogTableView::mouseMoveEvent"<<index.data().toString();
}

void ALogTableView::mousePressEvent(QMouseEvent *event)
{
    QModelIndex index = indexAt(event->pos());
//    qDebug()<<"ALogTableView::mousePressEvent"<<index.data().toString();

    if (index.data().toString() != "") //only show if an item is clicked on
    {
        QDialog *dialog = new QDialog(this);
    //    dialog->mapFromGlobal(QCursor::pos());
        dialog->setWindowTitle("Log details of " + logItemModel->index(index.row(), 1).data().toString());

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
        textBrowser->setText(logItemModel->index(index.row(), allIndex).data().toString());

        QVBoxLayout *m_pDialogLayout = new QVBoxLayout(this);
          m_pDialogLayout->addWidget(textBrowser);

         dialog->setLayout(m_pDialogLayout);

        dialog->show();
    }
}
