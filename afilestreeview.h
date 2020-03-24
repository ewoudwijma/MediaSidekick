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
    void onRemux();
    void onDerperview();
    void onOpenInExplorer();
    void onOpenDefaultApplication();
private:
    void loadModel(QUrl folderUrl);

    QStandardItem *onRemux2(QStandardItem *parentItem, QString folderName, QString fileName);
    void copyClips(QStandardItem *parentItem, QString folderName, QString fileName, QString targetFileName);

    ADerperView *derperView;

    void recursiveFileRenameCopyIfExists(QString folderName, QString fileName);

public slots:
    void onClipIndexClicked(QModelIndex index);
    void onFolderSelected(QString folderName);
    void onStopThreadProcess();
    void onMoveFilesToACVCRecycleBin(QStandardItem *parentItem, QString folderName, QString fileName, bool supportingFilesOnly = false);

signals:
    void fileIndexClicked(QModelIndex index, QStringList filePathList);
    void releaseMedia(QString folderName, QString fileName);
    void trimAll(QStandardItem *parentItem, QStandardItem *&currentItem, QString folderName, QString fileName);
    void getPropertyValue(QString folderFileName, QString key, QVariant *value);

    void propertyCopy(QStandardItem *parentItem, QString folderNameSource, QString fileNameSource, QString folderNameTarget, QString fileNameTarget);
//    void moveFilesToACVCRecycleBin(QStandardItem *parentItem, QString folderName, QString fileName, bool supportingFilesOnly = false);
    void loadProperties(QStandardItem *parentItem);
    void loadClips(QStandardItem *parentItem);

    void jobAddLog(AJobParams jobParams, QString logMessage);

    void derperviewCompleted(QString errorString);

    void stopThreadProcess();

};

#endif // AFILESTREEVIEW_H
