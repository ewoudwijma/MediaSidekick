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


class FEditTableView: public QTableView
{
    Q_OBJECT
public:
    FEditTableView(QWidget *parent = nullptr);
    void addEdit(int minus, int plus);
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
    FProcessManager *processManager;
    bool doNotUpdate;

public slots:
    void onFolderIndexClicked(QModelIndex index);
    void onFileIndexClicked(QModelIndex index, QModelIndexList selectedIndices = QModelIndexList());

    void onScrubberInChanged(int row, int in);
    void onScrubberOutChanged(int row, int out);
    void onVideoPositionChanged(int progress, int row, int relativeProgress);
    void onEditFilterChanged(FStarEditor *starEditorFilterWidget, QCheckBox *alikeCheckBox, QListView *tagFilter1ListView, QListView *tagFilter2ListView, QCheckBox *fileOnlyCheckBox);
    void onFileDelete(QString fileName);
    void onTrim(QString fileName);
    void onPropertiesLoaded();
    void onSectionMoved(int logicalIndex, int oldVisualIndex, int newVisualIndex);
    void onFileRename();
private slots:
    void onIndexClicked(QModelIndex index);
    void onEditRightClickMenu(const QPoint &point);
    void onEditDelete();

//    void onSpinnerChanged(STimeSpinBox *timeSpinBox);
    void onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
signals:
    void indexClicked(QModelIndex index);
    void editAdded(QModelIndex editInIndex);
    void editsChangedToVideo(QAbstractItemModel *itemModel);
    void editsChangedToTimeline(FEditSortFilterProxyModel *editProxyModel);
    void folderIndexClickedItemModel(QAbstractItemModel *itemModel);
    void folderIndexClickedProxyModel(QAbstractItemModel *itemModel);
    void fileIndexClicked(QModelIndex index, QModelIndexList selectedIndices = QModelIndexList());
    void addLogEntry(QString function);
    void addLogToEntry(QString function, QString log);
    void getPropertyValue(QString fileName, QString key, QString *value);
    void trim(QString fileName);
    void updateIn(int frames);
    void updateOut(int frames);
    void frameRateChanged(int frameRate);
};

#endif // FEDITTABLEVIEW_H
