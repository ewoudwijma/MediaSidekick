#ifndef FFILESTREEVIEW_H
#define FFILESTREEVIEW_H

#include <QFileSystemModel>
#include <QTreeView>
#include <QUrl>

class FFilesTreeView: public QTreeView
{
    Q_OBJECT
public:
    FFilesTreeView(QWidget *parent);

private slots:
    void this_customContextMenuRequested(const QPoint &point);
    void onTrim();
    void onFileDelete();
    void onProperties();
    void onIndexClicked(QModelIndex index);
private:
    QFileSystemModel *fileModel;
    void loadModel(QUrl folderUrl);
    QMenu *fileContextMenu;
public slots:
    void onEditIndexClicked(QModelIndex index);

    void onFolderIndexClicked(QModelIndex index);
signals:
    void indexClicked(QModelIndex index);

};

#endif // FFILESTREEVIEW_H
