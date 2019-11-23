#ifndef FFOLDERTREEVIEW_H
#define FFOLDERTREEVIEW_H

#include <QFileSystemModel>
#include <QSettings>
#include <QTreeView>

class AFolderTreeView : public QTreeView
{
    Q_OBJECT
public:
    AFolderTreeView(QWidget *parent);

    QFileSystemModel *directoryModel;
    void onIndexClicked(const QModelIndex &index);

signals:
    void indexClicked(QModelIndex index);
};

#endif // FFOLDERTREEVIEW_H
