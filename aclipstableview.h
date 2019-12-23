#ifndef AClipsTableView_H
#define AClipsTableView_H

#include "aclipsitemmodel.h"
#include "aclipssortfilterproxymodel.h"
#include "aprocessmanager.h"
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
    void onFolderIndexClicked(QModelIndex index);
    void onFileIndexClicked(QModelIndex index, QModelIndexList selectedIndices = QModelIndexList());
    void onScrubberInChanged(QString AV, int row, int in);
    void onScrubberOutChanged(QString AV, int row, int out);
    void onVideoPositionChanged(int progress, int row, int relativeProgress);
    void onClipsFilterChanged(QComboBox *ratingFilterComboBox, QCheckBox *alikeCheckBox, QListView *tagFilter1ListView, QListView *tagFilter2ListView, QCheckBox *fileOnlyCheckBox);
    void onClipsDelete(QString fileName);
    void onTrimF(QString fileName);
    void onPropertiesLoaded();
    void onSectionMoved(int logicalIndex, int oldVisualIndex, int newVisualIndex);
    void onReloadClips();

private slots:
    void onIndexClicked(QModelIndex index);
    void onClipRightClickMenu(const QPoint &point);
    void onClipDelete();
    void onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);

    void onIndexActivated(QModelIndex index);
    void onSectionEntered(int logicalIndex);
signals:
    void indexClicked(QModelIndex index);
    void clipAdded(QModelIndex clipInIndex);
    void clipsChangedToVideo(QAbstractItemModel *itemModel);
    void clipsChangedToTimeline(AClipsSortFilterProxyModel *clipProxyModel);
    void folderIndexClickedItemModel(QAbstractItemModel *itemModel);
    void folderIndexClickedProxyModel(QAbstractItemModel *itemModel);
    void fileIndexClicked(QModelIndex index, QModelIndexList selectedIndices = QModelIndexList());
    void addJobsEntry(QString folder, QString file, QString action, QString *id);
    void addToJob(QString id, QString log);
    void getPropertyValue(QString fileName, QString key, QString *value);
    void reloadProperties(QString fileName);
    void setIn(int frames);
    void setOut(int frames);
    void frameRateChanged(int frameRate);
    void propertiesLoaded();
    void propertyUpdate(QString selectedFolderName, QString fileName, QString targetFileName);
    void trimC(QString folderName, QString fileNameSource, QString fileNameTarget, QTime inTime, QTime outTime, int progressPercentage);
    void reloadAll(bool includingSRT);

};

#endif // s_H
