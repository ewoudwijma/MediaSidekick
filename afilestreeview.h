#ifndef AFILESTREEVIEW_H
#define AFILESTREEVIEW_H

#include <QFileSystemModel>
#include <QStandardItemModel>
#include <QTreeView>
#include <QUrl>

#include "afilessortfilterproxymodel.h"

class AFilesTreeView: public QTreeView
{
    Q_OBJECT
public:
    AFilesTreeView(QWidget *parent);
    QFileSystemModel *fileModel;
    AFilesSortFilterProxyModel *filesProxyModel;

    void setType(QString type);
private slots:
    void this_customContextMenuRequested(const QPoint &point);
    void onTrim();
    void onFileDelete();
    void onClipsDelete();
    void onIndexClicked(QModelIndex index);
    void onFileRename();

    void onIndexActivated(QModelIndex index);
    void onModelLoaded(const QString &path);
    void onSuperview();
    void onOpenInExplorer();
    void onOpenDefaultApplication();
private:
    void loadModel(QUrl folderUrl);
    QMenu *fileContextMenu;

    QModelIndex recursiveFiles(QAbstractItemModel *fileModel, QModelIndex parentIndex, QMap<QString, QString> parameters, void (*processOutput)(QWidget *, QMap<QString, QString>, QModelIndex));
public slots:
    void onClipIndexClicked(QModelIndex index);
    void onFolderIndexClicked(QModelIndex index);

signals:
    void indexClicked(QModelIndex index, QModelIndexList selectedIndices = QModelIndexList());
    void fileDelete(QString fileName);
    void clipsDelete(QString fileName);
    void fileRename();
    void trimF(QString fileName);
    void getPropertyValue(QString fileName, QString key, QString *value);

};

#endif // AFILESTREEVIEW_H
