#include "afilessortfilterproxymodel.h"

#include <QDebug>

AFilesSortFilterProxyModel::AFilesSortFilterProxyModel(QObject *parent)
    :QSortFilterProxyModel(parent)
{

}

bool AFilesSortFilterProxyModel::filterAcceptsRow(int sourceRow,
        const QModelIndex &sourceParent) const
{
    QString expString = filterRegExp().pattern();

    QModelIndex fileNameIndex = sourceModel()->index(sourceRow, 0, sourceParent);

    bool containsGeneratedNames = fileNameIndex.data().toString().toLower().contains("lossless") || fileNameIndex.data().toString().toLower().contains("encode") || fileNameIndex.data().toString().toLower().contains("premiere") || fileNameIndex.data().toString().toLower().contains("shotcut");

//    qDebug()<<"AFilesSortFilterProxyModel::filterAcceptsRow"<<sourceRow<<sourceParent.data().toString()<<fileNameIndex.data().toString()<<expString<<containsGeneratedNames;

    if (expString == "Video")
        return !containsGeneratedNames;
    else if (expString == "Audio")
    {
//        qDebug()<<"  Audio"<<fileNameIndex.data().toString()<<containsGeneratedNames;
        return !containsGeneratedNames;
    }
    else if (expString == "Export")
    {
//        qDebug()<<"  Export"<<fileNameIndex.data().toString()<<containsGeneratedNames;
        return true; //not containsGeneratedNames because in that case parentfolders not accepted
    }
    else
        return false; //QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
}
