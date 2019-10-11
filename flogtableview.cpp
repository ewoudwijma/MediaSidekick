#include "flogitemdelegate.h"
#include "flogtableview.h"

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

FLogTableView::FLogTableView(QWidget *parent) : QTableView(parent)
{
    logItemModel = new QStandardItemModel(this);
    QStringList labels;
    labels << "Timestamp"<<"Function"<<"Log"<<"All";
    logItemModel->setHorizontalHeaderLabels(labels);

    setModel(logItemModel);

    FLogItemDelegate *logItemDelegate = new FLogItemDelegate(this);
    setItemDelegate(logItemDelegate);

    horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    verticalHeader()->setSectionsMovable(true);
    horizontalHeader()->setSectionsClickable(true);
    horizontalHeader()->setStretchLastSection( true );
    setColumnWidth(1,int(columnWidth(1) * 4));
    setColumnHidden(3, true);

    setSortingEnabled(true);
}

void FLogTableView::onAddEntry(QString function)
{
    QList<QStandardItem *> items;

    items.append(new QStandardItem(QTime().currentTime().toString("hh:mm:ss.zzz")));
    items.append(new QStandardItem(function));

    logItemModel->appendRow(items);
    sortByColumn(0, Qt::DescendingOrder);
}

void FLogTableView::onAddLogToEntry(QString function, QString log)
{
    for (int row = 0; row < logItemModel->rowCount();row++)
    {
        if (logItemModel->index(row, 1).data().toString() == function)
        {
            logItemModel->setData(logItemModel->index(row, 2), log);
            logItemModel->setData(logItemModel->index(row, 3), logItemModel->index(row, 3).data().toString() +log);
        }
    }
}

void FLogTableView::mouseMoveEvent(QMouseEvent *event)
{
    QModelIndex index = indexAt(event->pos());
    qDebug()<<"FLogTableView::mouseMoveEvent"<<index.data().toString();
}

void FLogTableView::mousePressEvent(QMouseEvent *event)
{
    QModelIndex index = indexAt(event->pos());
//    qDebug()<<"FLogTableView::mousePressEvent"<<index.data().toString();


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
    textBrowser->setText(logItemModel->index(index.row(), 3).data().toString());

    QVBoxLayout *m_pDialogLayout = new QVBoxLayout(this);
      m_pDialogLayout->addWidget(textBrowser);

     dialog->setLayout(m_pDialogLayout);

    dialog->show();
}
