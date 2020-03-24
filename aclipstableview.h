#ifndef AClipsTableView_H
#define AClipsTableView_H

#include "aclipsitemmodel.h"
#include "aclipssortfilterproxymodel.h"
#include "ajobtreeview.h"
#include "astareditor.h"
#include "stimespinbox.h"

#include <QCheckBox>
#include <QDir>
#include <QListView>
#include <QTableView>
#include <QTime>
#include <QComboBox>

class AClipsTableView: public QTableView
{
    Q_OBJECT
public:
    AClipsTableView(QWidget *parent = nullptr);
    void addClip(int rating, bool alike, QAbstractItemModel *tagFilter1Model, QAbstractItemModel *tagFilter2Model);
    void saveModel(QString folderName, QString fileName);
    void toggleAlike();
    void giveStars(int starCount);
    AClipsItemModel *clipsItemModel;
    AClipsSortFilterProxyModel *clipsProxyModel;
    int highLightedRow;
    QStandardItemModel *srtFileItemModel;
    bool checkSaveIfClipsChanged();
    int originalDuration;
    void selectClips();

    void saveModels();
    int nrOfDeletedItems;

    AJobTreeView *jobTreeView;

private:
    QString selectedFolderName;
    QString selectedFileName;
    void loadModel(QString folderName);
    void scanDir(QDir dir, QStringList extensionList);
    QStandardItemModel *read(QString folderName, QString fileName);
    QMenu *clipContextMenu;
    int position;
    void onTagsChanged(const QModelIndex &parent, int first, int last);
    int clipCounter;
    int fileCounter;
    bool doNotUpdate;
    bool continueLoading;

public slots:
    void onFolderSelected(QString folderName);
    void onFileIndexClicked(QModelIndex index, QStringList filePathList);
    void onScrubberInChanged(QString AV, int row, int in);
    void onScrubberOutChanged(QString AV, int row, int out);
    void onVideoPositionChanged(int progress, int row, int relativeProgress);
    void onClipsFilterChanged(QComboBox *ratingFilterComboBox, QCheckBox *alikeCheckBox, QListView *tagFilter1ListView, QListView *tagFilter2ListView, QCheckBox *fileOnlyCheckBox);
    void onTrimAll(QStandardItem *parentItem, QStandardItem *&currentItem, QString folderName, QString fileName);
    void onPropertiesLoaded();
    void onSectionMoved(int logicalIndex, int oldVisualIndex, int newVisualIndex);
    void onLoadClips(QStandardItem *parentItem);
    void onTrimC(QStandardItem *parentItem, QStandardItem *&currentItem, QString folderNameSource, QString fileNameSource, QString folderNameTarget, QString fileNameTarget, QTime inTime, QTime outTime);

    void onTrimF(QStandardItem *parentItem, QStandardItem *&currentItem, QString folderNameSource, QString fileNameSource, QString folderNameTarget);
private slots:
    void onIndexClicked(QModelIndex index);
    void onClipRightClickMenu(const QPoint &point);
    void onClipDelete();
    void onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);

    void onIndexActivated(QModelIndex index);
    void onSectionEntered(int logicalIndex);
signals:
    void clipIndexClicked(QModelIndex index);
    void clipAdded(QModelIndex clipInIndex);
    void clipsChangedToVideo(QAbstractItemModel *itemModel);
    void clipsChangedToTimeline(AClipsSortFilterProxyModel *clipProxyModel);
    void folderSelectedItemModel(QAbstractItemModel *itemModel);
    void folderSelectedProxyModel(QAbstractItemModel *itemModel);
    void fileIndexClicked(QModelIndex index, QStringList filePathList);
    void getPropertyValue(QString folderFileName, QString key, QVariant *value);
    void loadProperties(QStandardItem *parentItem);
    void setIn(int frames);
    void setOut(int frames);
    void frameRateChanged(int frameRate);
    void propertiesLoaded();
    void propertyCopy(QStandardItem *parentItem, QString folderNameSource, QString fileNameSource, QString folderNameTarget, QString fileNameTarget);
    void moveFilesToACVCRecycleBin(QStandardItem *parentItem, QString folderName, QString fileName, bool supportingFilesOnly = false);
    void showInStatusBar(QString message, int timeout);
    void releaseMedia(QString folderName, QString fileName);

};

#endif // s_H
