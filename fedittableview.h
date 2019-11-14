#ifndef FEDITTABLEVIEW_H
#define FEDITTABLEVIEW_H

#include "fedititemmodel.h"
#include "feditsortfilterproxymodel.h"
#include "fprocessmanager.h"
#include "fstareditor.h"
#include "stimespinbox.h"

#include <QCheckBox>
#include <QDir>
#include <QListView>
#include <QTableView>
#include <QTime>
#include <QComboBox>

class FEditTableView: public QTableView
{
    Q_OBJECT
public:
    FEditTableView(QWidget *parent = nullptr);
    void addEdit();
    void saveModel(QString folderName, QString fileName);
    void toggleAlike();
    void giveStars(int starCount);
    FEditItemModel *editItemModel;
    FEditSortFilterProxyModel *editProxyModel;
    int highLightedRow;
    QStandardItemModel *srtFileItemModel;
    bool checkSaveIfEditsChanged();
    int originalDuration;

private:
    QString selectedFolderName;
    QString selectedFileName;
    void loadModel(QString folderName);
    void scanDir(QDir dir);
    QStandardItemModel *read(QString folderName, QString fileName);
    QMenu *editContextMenu;
    int position;
    void selectEdits();
    void onTagsChanged(const QModelIndex &parent, int first, int last);
    int editCounter;
    int fileCounter;
    bool doNotUpdate;
    bool continueLoading;

public slots:
    void onFolderIndexClicked(QModelIndex index);
    void onFileIndexClicked(QModelIndex index, QModelIndexList selectedIndices = QModelIndexList());
    void onScrubberInChanged(int row, int in);
    void onScrubberOutChanged(int row, int out);
    void onVideoPositionChanged(int progress, int row, int relativeProgress);
    void onEditFilterChanged(QComboBox *ratingFilterComboBox, QCheckBox *alikeCheckBox, QListView *tagFilter1ListView, QListView *tagFilter2ListView, QCheckBox *fileOnlyCheckBox);
    void onEditsDelete(QString fileName);
    void onTrim(QString fileName);
    void onPropertiesLoaded();
    void onSectionMoved(int logicalIndex, int oldVisualIndex, int newVisualIndex);
    void onReloadEdits();

private slots:
    void onIndexClicked(QModelIndex index);
    void onEditRightClickMenu(const QPoint &point);
    void onEditDelete();
    void onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);

    void onIndexActivated(QModelIndex index);
    void onSectionEntered(int logicalIndex);
signals:
    void indexClicked(QModelIndex index);
    void editAdded(QModelIndex editInIndex);
    void editsChangedToVideo(QAbstractItemModel *itemModel);
    void editsChangedToTimeline(FEditSortFilterProxyModel *editProxyModel);
    void folderIndexClickedItemModel(QAbstractItemModel *itemModel);
    void folderIndexClickedProxyModel(QAbstractItemModel *itemModel);
    void fileIndexClicked(QModelIndex index, QModelIndexList selectedIndices = QModelIndexList());
    void addLogEntry(QString folder, QString file, QString action, QString *id);
    void addLogToEntry(QString id, QString log);
    void getPropertyValue(QString fileName, QString key, QString *value);
    void reloadProperties(QString fileName);
    void updateIn(int frames);
    void updateOut(int frames);
    void frameRateChanged(int frameRate);
    void propertiesLoaded();
    void propertyUpdate(QString selectedFolderName, QString fileName, QString targetFileName);
    void trim(QString folderName, QString fileNameSource, QString fileNameTarget, QTime inTime, QTime outTime, int progressPercentage);
    void reloadAll(bool includingSRT);

};

#endif // FEDITTABLEVIEW_H
