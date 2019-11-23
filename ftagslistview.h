#ifndef FTAGSLISTVIEW_H
#define FTAGSLISTVIEW_H

#include <QListView>
#include <QStandardItemModel>

class ATagsListView: public QListView
{
    Q_OBJECT
public:
    ATagsListView(QWidget *parent = nullptr);
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
