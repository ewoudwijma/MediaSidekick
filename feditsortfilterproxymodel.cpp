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
    QString alikeString = expList[1];
    QString tag1String;
    QString tag2String;
    if (expList.count()>1)
        tag1String = expList[2].toLower();
    if (expList.count()>2)
        tag2String = expList[3].toLower();

    QStringList tag1List = tag1String.split(";", QString::SkipEmptyParts);
    QStringList tag2List = tag2String.split(";", QString::SkipEmptyParts);

    QString fileNameString = expList[4];

    FStarRating starRating = qvariant_cast<FStarRating>(sourceModel()->index(sourceRow, ratingIndex, sourceParent).data());
    QString alikeValue = sourceModel()->index(sourceRow, alikeIndex, sourceParent).data().toString();
    QString tags = sourceModel()->index(sourceRow, tagIndex, sourceParent).data().toString().toLower();
    QString fileNameValue = sourceModel()->index(sourceRow, fileIndex, sourceParent).data().toString();

//    qDebug()<<"FEditSortFilterProxyModel::filterAcceptsRow"<<starString<<starRating.starCount()<<expList.count()<<tag1List.count()<<tag2List.count()<<tag1String<<tag2String<<tags;
//    qDebug()<<"FEditSortFilterProxyModel::filterAcceptsRow"<<alikeString<<alikeValue<<fileNameString<<fileNameValue;

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

    if (alikeString == "true")
        allFound = allFound && alikeValue == "true";

    if (fileNameString != "")
        allFound = allFound && fileNameString == fileNameValue;

    return starRating.starCount() >= starString.toInt() && allFound;
}

bool FEditSortFilterProxyModel::lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const
{
    if (source_left.column() == orderAtLoadIndex)
        return source_left.data().toInt() < source_right.data().toInt();
    else
        return QSortFilterProxyModel::lessThan(source_left, source_right);
}
