#ifndef FTAGSLISTVIEW_H
#define FTAGSLISTVIEW_H

#include "fedititemmodel.h"

#include <QListView>
#include <QStandardItemModel>


class FTagsListView: public QListView
{
    Q_OBJECT
public:
    FTagsListView(QWidget *parent = nullptr);
    QStandardItemModel *tagsItemModel;
private:
    void loadModel(QStandardItemModel *editItemModel);

public slots:
    void onFolderIndexClicked(FEditItemModel *model);
private slots:
    void onDoubleClicked(const QModelIndex &index);
};

#endif // FTAGSLISTVIEW_H
