#ifndef AFOLDERTREEVIEW_H
#define AFOLDERTREEVIEW_H

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

#endif // AFOLDERTREEVIEW_H
