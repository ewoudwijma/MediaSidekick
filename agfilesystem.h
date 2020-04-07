#ifndef AGFILESYSTEM_H
#define AGFILESYSTEM_H

#include "aderperviewvideo.h"
#include "ajobtreeview.h"

#include <QFileSystemModel>
#include <QFileSystemWatcher>
#include <QGraphicsItem>

class AGFileSystem: public QObject
{
    Q_OBJECT

public:
    AGFileSystem(QObject *parent = nullptr);

    AJobTreeView *jobTreeView;

    void loadMedia(AJobParams jobParams, QString folderName, QString fileName, bool isNewFile, bool loadMediaInJob);
    int loadMediaTotal = 0;

    void loadFilesAndFolders(QDir dir, AJobParams jobParams);
    QFileSystemWatcher *fileSystemWatcher;

private slots:
    void onFileChanged(const QString &path);
    void onDirectoryChanged(const QString &path);
private:

    void recursiveFirstFile(QModelIndex parentIndex);
    void loadClips(QString parentName, QString folderName, QString fileName);

    void loadOrModifyItem(AJobParams jobParams, QString folderName, QString fileName, bool isNewFile, bool loadMediaInJob);
signals:
    void mediaLoaded(QString folderName, QString fileName, QImage image = QImage(), int duration = 0, QSize mediaSize = QSize(), QString ffmpegMeta = "", QList<int> samples = QList<int>());
    void addItem(QString parentName, QString mediaType, QString folderName, QString fileName, int duration = 0, int clipIn = 0, int clipOut = 0, QString tag = "");
    void deleteItem(QString mediaType, QString folderName, QString fileName);
    void jobAddLog(AJobParams jobParams, QString logMessage);
    void arrangeItems(QGraphicsItem *parentItem = nullptr);

};

#endif // AGFILESYSTEM_H
