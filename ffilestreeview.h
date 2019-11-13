#ifndef FFILESTREEVIEW_H
#define FFILESTREEVIEW_H

#include <QFileSystemModel>
#include <QStandardItemModel>
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
    void onEditsDelete();
    void onIndexClicked(QModelIndex index);
    void onFileRename();
private:
    void loadModel(QUrl folderUrl);
    QFileSystemModel *fileModel;
    QMenu *fileContextMenu;
public slots:
    void onEditIndexClicked(QModelIndex index);

    void onFolderIndexClicked(QModelIndex index);
signals:
    void indexClicked(QModelIndex index, QModelIndexList selectedIndices = QModelIndexList());
    void fileDelete(QString fileName);
    void editsDelete(QString fileName);
    void fileRename();
    void trim(QString fileName);
    void getPropertyValue(QString fileName, QString key, QString *value);

};

#endif // FFILESTREEVIEW_H
