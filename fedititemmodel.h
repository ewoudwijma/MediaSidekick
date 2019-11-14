#ifndef FEDITITEMMODEL_H
#define FEDITITEMMODEL_H

#include <QStandardItemModel>
#include <QItemSelection>

class FEditItemModel  : public QStandardItemModel
{
    Q_OBJECT
public:
    FEditItemModel(QObject *parent = nullptr);

    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);
    void currentChanged(const QModelIndex &current, const QModelIndex &previous);
    void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
};

#endif // FEDITITEMMODEL_H
