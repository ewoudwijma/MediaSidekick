#ifndef AClipsItemModel_H
#define AClipsItemModel_H

#include <QStandardItemModel>
#include <QItemSelection>

class AClipsItemModel  : public QStandardItemModel
{
    Q_OBJECT
public:
    AClipsItemModel(QObject *parent = nullptr);

    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);
    void currentChanged(const QModelIndex &current, const QModelIndex &previous);
    void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
};

#endif // AClipsItemModel_H
