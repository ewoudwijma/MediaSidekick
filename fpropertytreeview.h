#ifndef FPROPERTYTREEVIEW_H
#define FPROPERTYTREEVIEW_H

#include "fprocessmanager.h"
#include "fpropertysortfilterproxymodel.h"

#include <QCheckBox>
#include <QStandardItemModel>
#include <QTreeView>
#include <QLineEdit>

class FPropertyTreeView: public QTreeView
{
    Q_OBJECT

public:
    FPropertyTreeView(QWidget *parent=nullptr);
    ~FPropertyTreeView();

public slots:
    void onPropertyFilterChanged(QLineEdit *propertyFilterLineEdit, QCheckBox *propertyDiffCheckBox, QCheckBox *locationCheckBox, QCheckBox *cameraCheckBox, QCheckBox *authorCheckBox);
    void onFolderIndexClicked(QModelIndex index);
    void onGetPropertyValue(QString fileName, QString key, QString *value);
    void onFileIndexClicked(QModelIndex index, QModelIndexList selectedIndices = QModelIndexList());
    void onClipIndexClicked(QModelIndex index);
    void onRemoveFile(QString fileName);
    void onReloadProperties();
private slots:
    void updateSectionWidth(int logicalIndex, int, int newSize);
    void updateSectionHeight(int logicalIndex, int oldSize, int newSize);
private:
    QStandardItemModel *propertyItemModel;
    FPropertySortFilterProxyModel *propertyProxyModel;
    void loadModel(QString folderName);
    FProcessManager *processManager;
    void addSublevelItem(QUrl fileUrl, QString itemName, QString type, QString value);
    QStandardItem *getToplevelItem(QString itemName);
    QTreeView *frozenTableView;
    void init();
    void updateFrozenTableGeometry();
    void setCellStyle(QStringList fileNames);
    void onPropertyChanged(QStandardItem *item);
    bool isLoading;
    void updateSuggestedName(QModelIndex index);

    bool locationInName;
    bool cameraInName;
    bool authorInName;
protected:
      void resizeEvent(QResizeEvent *event) override;
      QModelIndex moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers) override;
      void scrollTo (const QModelIndex & index, ScrollHint hint = EnsureVisible) override;

signals:
      void addLogEntry(QString folder, QString file, QString action, QString* id);
      void addLogToEntry(QString id, QString log);
      void propertiesLoaded();
      void fileDelete(QString fileName);
};

#endif // FPROPERTYTREEVIEW_H
