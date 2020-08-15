#ifndef AGFILESYSTEM_H
#define AGFILESYSTEM_H

#include "aderperviewvideo.h"
#include "agprocessthread.h"

#include <QFileSystemModel>
#include <QFileSystemWatcher>
#include <QGraphicsItem>

class AGFileSystem: public QObject
{
    Q_OBJECT

    void recursiveFirstFile(QModelIndex parentIndex);
    void loadClips(AGProcessAndThread *process, QFileInfo fileInfo);

    void loadItem(AGProcessAndThread *process, QFileInfo fileInfo, bool isNewFile);

    QList<AGProcessAndThread *> processes;

public:
    AGFileSystem(QObject *parent = nullptr);

    void loadFilesAndFolders(QDir dir, AGProcessAndThread *process);
    QFileSystemWatcher *fileSystemWatcher;

    bool processStopped = false;

private slots:
    void onFileChanged(const QString &path);
    void onDirectoryChanged(const QString &path);

public slots:
    void onStopThreadProcess();
    void onFileWatch(QString folderFileName, bool on);


signals:
    void addItem(bool changed, QString parentName, QString mediaType, QFileInfo fileInfo = QFileInfo(), int duration = 0, int clipIn = 0, int clipOut = 0, QString tag = "");
    void deleteItem(bool changed, QString mediaType, QFileInfo fileInfo, int clipIn = -1, QString tagName = "");

    void fileChanged(QFileInfo fileInfo);

};

#endif // AGFILESYSTEM_H
