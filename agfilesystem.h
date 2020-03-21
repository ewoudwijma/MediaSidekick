#ifndef AGFILESYSTEM_H
#define AGFILESYSTEM_H

#include "aderperviewvideo.h"
#include "ajobtreeview.h"

#include <QFileSystemModel>
#include <QFileSystemWatcher>

class AGFileSystem: public QObject
{
    Q_OBJECT

public:
    AGFileSystem(QObject *parent = nullptr);

    AJobTreeView *jobTreeView;

    void loadMedia(QString folderName, QString fileName);
    void loadFilesAndFolders(QDir dir);
private slots:
    void onFileChanged(const QString &path);
    void onDirectoryChanged(const QString &path);
private:
    QFileSystemWatcher *fileSystemWatcher;

    void recursiveFirstFile(QModelIndex parentIndex);
    void loadClips(QFileInfo fileInfo);

signals:
    void mediaLoaded(QString folderName, QString fileName, QImage image, int duration, QSize mediaSize, QString ffmpegMeta);
    void addItem(QString parentFileName, QString mediaType, QString folderName, QString fileName, int duration = 0, int clipIn = 0, int clipOut = 0, QString tag = "");

};

#endif // AGFILESYSTEM_H
