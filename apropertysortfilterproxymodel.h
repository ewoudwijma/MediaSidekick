#ifndef APROPERTYSORTFILTERPROXYMODEL_H
#define APROPERTYSORTFILTERPROXYMODEL_H

#include <QSortFilterProxyModel>

class APropertySortFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    APropertySortFilterProxyModel(QObject *parent = nullptr);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;

private:
};

#endif // APROPERTYSORTFILTERPROXYMODEL_H
