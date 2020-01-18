#include "apropertysortfilterproxymodel.h"

#include <QDebug>

#include"aglobal.h"

APropertySortFilterProxyModel::APropertySortFilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
}

bool APropertySortFilterProxyModel::filterAcceptsRow(int sourceRow,
        const QModelIndex &sourceParent) const
{
//    qDebug()<<"APropertySortFilterProxyModel::filterAcceptsRow"<<sourceRow<<sourceParent.data().toString();
    QModelIndex indexProperty = sourceModel()->index(sourceRow, propertyIndex, sourceParent); //item name
    QModelIndex indexDiff = sourceModel()->index(sourceRow, diffIndex, sourceParent); //diff yes no

    QString expString = filterRegExp().pattern();
    QStringList expList = expString.split(";");
    QString filterString = expList.first().toLower();
    QString diffString = expList.last();

    if (sourceParent == QModelIndex()) //if toplevel
    {
        bool show = false;
        for (int i=0; i<sourceModel()->rowCount(indexProperty); i++) //check all sublevel rows
        {
//            qDebug()<<"filterAcceptsRow toplevel"<<sourceModel()->data(sourceModel()->index(i, propertyIndex, indexProperty)).toString();
            show = show || sourceModel()->index(i, propertyIndex, indexProperty).data().toString().toLower().contains(filterString);
//            show = show || sourceModel()->data(sourceModel()->index(i, propertyIndex, indexProperty)).toString().contains(filterRegExp());
//            qDebug()<<"diff"<<i<<sourceModel()->data(sourceModel()->index(i, propertyIndex, indexDiff)).toString();
        }
        return show;
    }
    else //sublevel
    {
//        if (sourceRow==4)
//            qDebug()<<"filterAcceptsRow sublevel"<<sourceRow<<sourceParent.data()<<indexProperty.data()<<sourceModel()->data(indexProperty).toString()<<indexDiff.data()<<filterRegExp()<<filterString<<diffString<<diffString.toInt()<<indexDiff.data().toBool();
        if (diffString.toInt() == Qt::Unchecked)
            return indexProperty.data().toString().toLower().contains(filterString) && !indexDiff.data().toBool();
        else if (diffString.toInt() == Qt::Checked)
            return indexProperty.data().toString().toLower().contains(filterString) && indexDiff.data().toBool();
        else
            return indexProperty.data().toString().toLower().contains(filterString);
//        return (sourceModel()->data(indexProperty).toString().contains(filterRegExp()));
    }
}
