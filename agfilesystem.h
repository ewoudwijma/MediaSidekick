#ifndef AGFILESYSTEM_H
#define AGFILESYSTEM_H

#include "aderperviewvideo.h"
#include "agview.h"
#include "ajobtreeview.h"

#include <QFileSystemModel>
#include <QFileSystemWatcher>




class AGFileSystem: public QObject
{
    Q_OBJECT

    QGraphicsItem *videoParentItem = nullptr;
    QGraphicsItem *audioParentItem = nullptr;
    QGraphicsItem *imageParentItem = nullptr;
    QGraphicsItem *exportParentItem = nullptr;

    QGraphicsItem *videoTimelineParentItem = nullptr;
    QGraphicsItem *audioTimelineParentItem = nullptr;
    QGraphicsItem *imageTimelineParentItem = nullptr;
    QGraphicsItem *exportTimelineParentItem = nullptr;


public:
    AGFileSystem(QObject *parent = nullptr);

    AJobTreeView *jobTreeView;

    void loadMedia(QString folderName, QString fileName);
    QGraphicsItem *loadFilesAndFolders(AGView *view, QGraphicsItem *parentItem, QDir dir);
private slots:
    void onFileChanged(const QString &path);
    void onDirectoryChanged(const QString &path);
private:
    QFileSystemWatcher *fileSystemWatcher;

    void recursiveFirstFile(QModelIndex parentIndex);
    void loadClips(QGraphicsItem *parentItem, QFile &file, AGView *view, QFileInfo fileInfo);

signals:
    void mediaLoaded(QString folderName, QString fileName, QImage image, int duration, QSize mediaSize);

};

#endif // AGFILESYSTEM_H
