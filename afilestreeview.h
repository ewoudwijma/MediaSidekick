#ifndef AFILESTREEVIEW_H
#define AFILESTREEVIEW_H

#include <QFileSystemModel>
#include <QStandardItemModel>
#include <QTreeView>
#include <QUrl>

#include "aderperviewmain.h"
#include "afilessortfilterproxymodel.h"
#include "ajobtreeview.h"

class AFilesTreeView: public QTreeView
{
    Q_OBJECT
public:
    AFilesTreeView(QWidget *parent);
    QFileSystemModel *fileModel;
    AFilesSortFilterProxyModel *filesProxyModel;

    void setType(QString type);
    QMenu *fileContextMenu;

    AJobTreeView *jobTreeView;

private slots:
    void this_customContextMenuRequested(const QPoint &point);
    void onTrim();
    void onArchiveFiles();
    void onArchiveClips();
    void onIndexClicked(QModelIndex index);

    void onIndexActivated(QModelIndex index);
    void onModelLoaded(const QString &path);
    void onRemux();
    void onDerperview();
    void onOpenInExplorer();
    void onOpenDefaultApplication();
private:
    void loadModel(QUrl folderUrl);

    //    QModelIndex recursiveFiles(QAbstractItemModel *fileModel, QModelIndex parentIndex, QMap<QString, QString> parameters, void (*processOutput)(QWidget *, QMap<QString, QString>, QModelIndex));
    QStandardItem *onRemux2(QStandardItem *parentItem, QString folderName, QString fileName);
    void copyClips(QStandardItem *parentItem, QString folderName, QString fileName, QString targetFileName);

    ADerperView *derperView;
public slots:
    void onClipIndexClicked(QModelIndex index);
    void onFolderIndexClicked(QModelIndex index);
    void onStopThreadProcess();

signals:
    void indexClicked(QModelIndex index, QModelIndexList selectedIndices = QModelIndexList());
    void releaseMedia(QString fileName);
//    void archiveClips(QString fileName);
//    void archiveFiles(QString fileName);
    void trimF(QStandardItem *parentItem, QStandardItem *&currentItem, QString folderName, QString fileName);
//    void derperView(QString folderName, QString fileName);
    void getPropertyValue(QString fileName, QString key, QVariant *value);

    void propertyCopy(QStandardItem *parentItem, QString selectedFolderName, QString fileName, QString targetFileName);
    void moveFilesToACVCRecycleBin(QStandardItem *parentItem, QString folderName, QString fileName, bool supportingFilesOnly = false);
    void loadProperties(QStandardItem *parentItem);
    void loadClips(QStandardItem *parentItem);

    void jobAddLog(AJobParams jobParams, QString logMessage);

    void derperviewCompleted(QString errorString);

    void stopThreadProcess();

};

#endif // AFILESTREEVIEW_H
