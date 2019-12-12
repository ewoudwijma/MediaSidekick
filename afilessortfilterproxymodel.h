#ifndef AFILESSORTFILTERPROXYMODEL_H
#define AFILESSORTFILTERPROXYMODEL_H

#include <QSortFilterProxyModel>

class AFilesSortFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    AFilesSortFilterProxyModel(QObject *parent = nullptr);
protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;
};

#endif // AFILESSORTFILTERPROXYMODEL_H
