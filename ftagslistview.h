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
    bool addTag(QString tagString);
private:
    void loadModel(QAbstractItemModel *editItemModel);

public slots:
    void onFolderIndexClicked(QAbstractItemModel *model);
private slots:
    void onDoubleClicked(const QModelIndex &index);
};

#endif // FTAGSLISTVIEW_H
