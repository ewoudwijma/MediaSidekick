#include "fdragdroplineedit.h"
#include "ftagslistview.h"

#include <QKeyEvent>
#include <QMimeData>
#include <QDebug>

FDragDropLineEdit::FDragDropLineEdit(QWidget *parent)
    : QLineEdit(parent)
{

}

void FDragDropLineEdit::dragEnterEvent(QDragEnterEvent *e)
{
    qDebug()<<"FDragDropLineEdit::dragEnterEvent"<<e->pos();
    if(e->mimeData()->hasFormat("application/x-qabstractitemmodeldatalist")){
        e->acceptProposedAction();
    }
}

void FDragDropLineEdit::dropEvent(QDropEvent *e)
{


    QByteArray encoded = e->mimeData()->data("application/x-qabstractitemmodeldatalist");
    QDataStream strm(&encoded, QIODevice::ReadOnly);
    while(!strm.atEnd()){
        int row, col;
        QMap<int,  QVariant> data;
        strm >> row >> col >> data;
        this->setText(data[0].toString());

        //implement move
        QListView *qlv = qobject_cast<QListView *>(e->source());

        qDebug()<<"FDragDropLineEdit::dropEvent"<<e->pos()<<e->source()<<data[0].toString()<<qlv;

        if (qlv != nullptr)
        {
            for (int row = qlv->model()->rowCount() - 1; row >=0 ;row--)
                if (qlv->model()->index(row, 0).data().toString() == data[0].toString() )
                    qlv->model()->removeRow(row);
        }
    }
}
