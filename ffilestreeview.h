#ifndef FFILESTREEVIEW_H
#define FFILESTREEVIEW_H

#include <QFileSystemModel>
#include <QStandardItemModel>
#include <QTreeView>
#include <QUrl>

class AFilesTreeView: public QTreeView
{
    Q_OBJECT
public:
    AFilesTreeView(QWidget *parent);

private slots:
    void this_customContextMenuRequested(const QPoint &point);
    void onTrim();
    void onFileDelete();
    void onClipsDelete();
    void onIndexClicked(QModelIndex index);
    void onFileRename();

    void onIndexActivated(QModelIndex index);
    void onDirectoryLoaded(const QString &path);
    void onSuperview();
private:
    void loadModel(QUrl folderUrl);
    QFileSystemModel *fileModel;
    QMenu *fileContextMenu;

public slots:
    void onClipIndexClicked(QModelIndex index);
    void onFolderIndexClicked(QModelIndex index);

signals:
    void indexClicked(QModelIndex index, QModelIndexList selectedIndices = QModelIndexList());
    void fileDelete(QString fileName);
    void clipsDelete(QString fileName);
    void fileRename();
    void trim(QString fileName);
    void getPropertyValue(QString fileName, QString key, QString *value);

};

#endif // FFILESTREEVIEW_H
