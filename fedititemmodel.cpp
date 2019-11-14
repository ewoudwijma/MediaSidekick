#include "fedititemmodel.h"

#include <QDebug>
#include <QMimeData>

#include "fglobal.h"

FEditItemModel::FEditItemModel(QObject *parent)
    : QStandardItemModel(parent)
{
}

bool FEditItemModel::dropMimeData(const QMimeData *data, Qt::DropAction action,
                               int row, int column, const QModelIndex &parent)
{
    qDebug()<<"dropMimeData"<<row<<column<<parent.data();
    QByteArray encoded = data->data("application/x-qabstractitemmodeldatalist");
    QDataStream stream(&encoded, QIODevice::ReadOnly);

    if (parent != QModelIndex() && parent.column() == tagIndex)
    {
        while (!stream.atEnd())
        {
            int row, col;
            QMap<int,  QVariant> roleDataMap;
            stream >> row >> col >> roleDataMap;

    //        qDebug()<<"  roleDataMap"<<row<<col<<parent.data().toString()<<roleDataMap[0];
            if (parent.data().toString() == "")
                setData(parent, roleDataMap[0].toString());
            else
                setData(parent, parent.data().toString() + ";" + roleDataMap[0].toString());
            setData(parent.model()->index(parent.row(), changedIndex), "yes");
       }

        return QStandardItemModel::dropMimeData(data, action, row, column, parent);
    }
    return false;
}

void FEditItemModel::currentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    qDebug()<<"FEditItemModel::currentChanged"<<current.data()<<previous.data();

}

void FEditItemModel::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    qDebug()<<"FEditItemModel::selectionChanged"<<selected<<deselected;

}
