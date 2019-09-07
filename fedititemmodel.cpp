#include "fedititemmodel.h"

#include <QDebug>
#include <QMimeData>

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

    while (!stream.atEnd())
    {
        int row, col;
        QMap<int,  QVariant> roleDataMap;
        stream >> row >> col >> roleDataMap;

        qDebug()<<"  roleDataMap"<<row<<col<<roleDataMap[0];
        setData(parent, parent.data().toString() + ";" + roleDataMap[0].toString());

   }

    return QStandardItemModel::dropMimeData(data, action, row, column, parent);
}
