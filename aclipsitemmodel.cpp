#include "aclipsitemmodel.h"

#include <QDebug>
#include <QMimeData>

#include "fglobal.h"

AClipsItemModel::AClipsItemModel(QObject *parent)
    : QStandardItemModel(parent)
{
}

bool AClipsItemModel::dropMimeData(const QMimeData *data, Qt::DropAction action,
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

void AClipsItemModel::currentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    qDebug()<<"AClipsItemModel::currentChanged"<<current.data()<<previous.data();

}

void AClipsItemModel::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    qDebug()<<"AClipsItemModel::selectionChanged"<<selected<<deselected;

}
