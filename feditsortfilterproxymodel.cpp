#include "feditsortfilterproxymodel.h"
#include "fstarrating.h"

#include <QDebug>

#include "fglobal.h"

FEditSortFilterProxyModel::FEditSortFilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
}

bool FEditSortFilterProxyModel::filterAcceptsRow(int sourceRow,
        const QModelIndex &sourceParent) const
{
    QString expString = filterRegExp().pattern();
    QStringList expList = expString.split("|");
    QString starString = expList[0].toLower();
//    if (expList.count()>0)
    QString tag1String;
    QString tag2String;
    if (expList.count()>1)
        tag1String = expList[1].toLower();
    if (expList.count()>2)
        tag2String = expList[2].toLower();

    QStringList tag1List = tag1String.split(";");
    if (tag1List.count()==1 && tag1List[0] == "")
        tag1List.clear();
    QStringList tag2List = tag2String.split(";");
    if (tag2List.count()==1 && tag2List[0] == "")
        tag2List.clear();

    QModelIndex index0 = sourceModel()->index(sourceRow, ratingIndex, sourceParent);
    QString tags = sourceModel()->index(sourceRow, tagIndex, sourceParent).data().toString().toLower();

    FStarRating starRating = qvariant_cast<FStarRating>(index0.data());

//    qDebug()<<"FEditSortFilterProxyModel::filterAcceptsRow"<<starString<<starRating.starCount()<<expList.count()<<tag1List.count()<<tag2List.count()<<tag1String<<tag2String<<tags;

    bool allFound = true;
    bool found;
    if (tag1List.count()>0 && allFound) //check on tags 1
    {
        found = false;
        for (int j=0; j < tag1List.count(); j++)
        {
            if (tag1List[j] != "" && tags.contains(tag1List[j]))
            {
                found = true;
            }
//            qDebug()<<"tags1"<<tags<<j<<tag1List[j]<<tag1List.count()<<found;
        }
        allFound = allFound && found;
    }
    if ( tag2List.count()>0 && allFound) //check on tags 2
    {
        found = false;
        for (int j=0; j < tag2List.count(); j++)
        {
            if (tag2List[j] != "" && tags.contains(tag2List[j]))
            {
                found = true;
            }
//            qDebug()<<"tags2"<<tags<<j<<tag2List[j]<<tag2List.count()<<found;
        }
        allFound = allFound && found;
    }


    return starRating.starCount() >= starString.toInt() && allFound;
}

bool FEditSortFilterProxyModel::lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const
{
    if (source_left.column() == orderAtLoadIndex)
        return source_left.data().toInt() < source_right.data().toInt();
    else
        return QSortFilterProxyModel::lessThan(source_left, source_right);
}
