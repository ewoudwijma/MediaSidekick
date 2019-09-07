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
private:
    QStandardItemModel *tagsItemModel;
    void loadModel(QStandardItemModel *editItemModel);

public slots:
    void onFolderIndexClicked(FEditItemModel *model);
};

#endif // FTAGSLISTVIEW_H
