#include "apropertysortfilterproxymodel.h"

#include <QDebug>

APropertySortFilterProxyModel::APropertySortFilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
}

bool APropertySortFilterProxyModel::filterAcceptsRow(int sourceRow,
        const QModelIndex &sourceParent) const
{
//    qDebug()<<"APropertySortFilterProxyModel::filterAcceptsRow"<<sourceRow<<sourceParent.data().toString();
    QModelIndex index0 = sourceModel()->index(sourceRow, 0, sourceParent); //item name
    QModelIndex index2 = sourceModel()->index(sourceRow, 2, sourceParent); //diff yes no

    QString expString = filterRegExp().pattern();
    QStringList expList = expString.split(";");
    QString filterString = expList.first().toLower();
    QString diffString = expList.last();

    if (sourceParent == QModelIndex()) //if toplevel
    {
        bool show = false;
        for (int i=0; i<sourceModel()->rowCount(index0); i++) //check all sublevel rows
        {
//            qDebug()<<"filterAcceptsRow toplevel"<<sourceModel()->data(sourceModel()->index(i, 0, index0)).toString();
            show = show || sourceModel()->index(i, 0, index0).data().toString().toLower().contains(filterString);
//            show = show || sourceModel()->data(sourceModel()->index(i, 0, index0)).toString().contains(filterRegExp());
//            qDebug()<<"diff"<<i<<sourceModel()->data(sourceModel()->index(i, 0, index2)).toString();
        }
        return show;
    }
    else //sublevel
    {
//        if (sourceRow==4)
//            qDebug()<<"filterAcceptsRow sublevel"<<sourceRow<<sourceParent.data()<<index0.data()<<sourceModel()->data(index0).toString()<<index2.data()<<filterRegExp()<<filterString<<diffString<<diffString.toInt()<<index2.data().toBool();
        if (diffString.toInt() == Qt::Unchecked)
            return index0.data().toString().toLower().contains(filterString) && !index2.data().toBool();
        else if (diffString.toInt() == Qt::Checked)
            return index0.data().toString().toLower().contains(filterString) && index2.data().toBool();
        else
            return index0.data().toString().toLower().contains(filterString);
//        return (sourceModel()->data(index0).toString().contains(filterRegExp()));
    }
}
