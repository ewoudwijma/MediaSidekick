#ifndef FEDITTABLEVIEW_H
#define FEDITTABLEVIEW_H

#include "fedititemmodel.h"
#include "feditsortfilterproxymodel.h"
#include "fprocessmanager.h"
#include "fstareditor.h"

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
    void toggleRepeat();
    void giveStars(int starCount);
    FEditItemModel *editItemModel;
    FEditSortFilterProxyModel *editProxyModel;
    int highLightedRow;
    QStandardItemModel *srtFileItemModel;
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

public slots:
    void onFolderIndexClicked(QModelIndex index);
    void onFileIndexClicked(QModelIndex index, QModelIndexList selectedIndices = QModelIndexList());

    void onInChanged(int row, int in);
    void onOutChanged(int row, int out);
    void onVideoPositionChanged(int progress, int row, int relativeProgress);
    void onEditFilterChanged(FStarEditor *starEditorFilterWidget, QListView *tagFilter1ListView, QListView *tagFilter2ListView);
    void onFileDelete(QString fileName);
    void onTrim(QString fileName);
    void onPropertiesLoaded();

private slots:
    void onIndexClicked(QModelIndex index);
    void onEditRightClickMenu(const QPoint &point);
    void onEditDelete();
    void onSectionMoved(int logicalIndex, int oldVisualIndex, int newVisualIndex);

signals:
    void indexClicked(QModelIndex index);
    void editAdded(QModelIndex editInIndex);
    void editsChanged(FEditSortFilterProxyModel *editProxyModel);
    void editsChangedFromVideo(FEditSortFilterProxyModel *editProxyModel);
    void folderIndexClickedItemModel(FEditItemModel *model);
    void folderIndexClickedProxyModel(FEditSortFilterProxyModel *editProxyModel);
    void fileIndexClicked(QModelIndex index, QModelIndexList selectedIndices = QModelIndexList());
    void addLogEntry(QString function);
    void addLogToEntry(QString function, QString log);
    void getPropertyValue(QString fileName, QString key, QString *value);

};

#endif // FEDITTABLEVIEW_H
