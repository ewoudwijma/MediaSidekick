#ifndef FEDITTABLEVIEW_H
#define FEDITTABLEVIEW_H

#include "fedititemmodel.h"
#include "feditsortfilterproxymodel.h"
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
    void saveModel();
    void toggleRepeat();
    void giveStars(int starCount);
private slots:
    void onIndexClicked(QModelIndex index);
    void onEditRightClickMenu(const QPoint &point);
    void onEditDelete();
    void onSectionMoved(int logicalIndex, int oldVisualIndex, int newVisualIndex);
public slots:
    void onFolderIndexClicked(QModelIndex index);
    void onFileIndexClicked(QModelIndex index);

    void onInChanged(int row, int in);
    void onOutChanged(int row, int out);
    void onPositionChanged(int progress);
    void onEditFilterChanged(FStarEditor *starEditorFilterWidget, QListView *tagFilter1ListView, QListView *tagFilter2ListView);
private:
    QString selectedFolderName;
    QString selectedFileName;
    FEditItemModel *editItemModel;
    void loadModel(QString folderName);
    void scanDir(QDir dir);
    QStandardItemModel *read(QUrl mediaFilePath);
    QMenu *editContextMenu;
    int position;
    void selectEdits();
    void onTagsChanged(const QModelIndex &parent, int first, int last);
    FEditSortFilterProxyModel *editProxyModel;
    int highLightedRow;
    int editCounter;
signals:
    void indexClicked(QModelIndex index);
    void editAdded(QModelIndex editInIndex);
    void editsChanged(FEditSortFilterProxyModel *editProxyModel);
    void editsChangedFromVideo(FEditSortFilterProxyModel *editProxyModel);
    void folderIndexClickedItemModel(FEditItemModel *model);
    void folderIndexClickedProxyModel(FEditSortFilterProxyModel *editProxyModel);
    void fileIndexClicked(QModelIndex index);

};

#endif // FEDITTABLEVIEW_H
