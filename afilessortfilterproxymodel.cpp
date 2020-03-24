#include "afilessortfilterproxymodel.h"
#include "aglobal.h"

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

    bool exportFileFound = false;
    foreach (QString exportMethod, AGlobal().exportMethods)
        if (fileNameIndex.data().toString().toLower().contains(exportMethod))
            exportFileFound = true;

//    qDebug()<<"AFilesSortFilterProxyModel::filterAcceptsRow"<<sourceRow<<sourceParent.data().toString()<<fileNameIndex.data().toString()<<expString<<containsGeneratedNames;

    if (expString == "Video")
        return !exportFileFound;
    else if (expString == "Audio")
    {
//        qDebug()<<"  Audio"<<fileNameIndex.data().toString()<<containsGeneratedNames;
        return !exportFileFound;
    }
    else if (expString == "Export")
    {
//        qDebug()<<"  Export"<<fileNameIndex.data().toString()<<containsGeneratedNames;
        return true; //not containsGeneratedNames because in that case parentfolders not accepted
    }
    else
        return false; //QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
}
