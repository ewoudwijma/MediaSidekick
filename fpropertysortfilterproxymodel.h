#ifndef FPROPERTYSORTFILTERPROXYMODEL_H
#define FPROPERTYSORTFILTERPROXYMODEL_H

#include <QSortFilterProxyModel>

class FPropertySortFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    FPropertySortFilterProxyModel(QObject *parent = nullptr);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;

private:
};

#endif // FPROPERTYSORTFILTERPROXYMODEL_H
