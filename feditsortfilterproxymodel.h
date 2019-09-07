#ifndef FEDITSORTFILTERPROXYMODEL_H
#define FEDITSORTFILTERPROXYMODEL_H


#include <QSortFilterProxyModel>

class FEditSortFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    FEditSortFilterProxyModel(QObject *parent = nullptr);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;
    bool lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const;

private:
};

#endif // FEDITSORTFILTERPROXYMODEL_H
