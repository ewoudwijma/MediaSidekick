#ifndef AClipsSortFilterProxyModel_H
#define AClipsSortFilterProxyModel_H


#include <QSortFilterProxyModel>

class AClipsSortFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    AClipsSortFilterProxyModel(QObject *parent = nullptr);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;
    bool lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const;

private:
};

#endif // AClipsSortFilterProxyModel_H
