#ifndef AFOLDERTREEVIEW_H
#define AFOLDERTREEVIEW_H

#include "ajobtreeview.h"

#include <QFileSystemModel>
#include <QSettings>
#include <QTreeView>

class AFolderTreeView : public QTreeView
{
    Q_OBJECT
    void recursiveFileRenameCopyIfExists(QString folderName, QString fileName);
public:
    AFolderTreeView(QWidget *parent);

    QFileSystemModel *directoryModel;
    void onIndexClicked(const QModelIndex &index);

    AJobTreeView *jobTreeView;

    void simulateIndexClicked(QString folderName);
signals:
    void indexClicked(QModelIndex index);
    void jobAddLog(AJobParams jobParams, QString logMessage);

public slots:
    void onMoveFilesToACVCRecycleBin(QStandardItem *parentItem, QString folderName, QString fileName, bool supportingFilesOnly = false);

};

#endif // AFOLDERTREEVIEW_H
