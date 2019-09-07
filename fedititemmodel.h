#ifndef FEDITITEMMODEL_H
#define FEDITITEMMODEL_H

#include <QStandardItemModel>

class FEditItemModel  : public QStandardItemModel
{
    Q_OBJECT
public:
    FEditItemModel(QObject *parent = nullptr);

    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);
};

#endif // FEDITITEMMODEL_H
