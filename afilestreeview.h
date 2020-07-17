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
    void onIndexClicked(QModelIndex index);

    void onIndexActivated(QModelIndex index);
private:
    void loadModel(QUrl folderUrl);

public slots:
    void onClipIndexClicked(QModelIndex index);
    void onFolderSelected(QString folderName);

signals:
    void fileIndexClicked(QModelIndex index, QStringList filePathList);
    void releaseMedia(QString folderName, QString fileName);
};

#endif // AFILESTREEVIEW_H
