#ifndef APROPERTYTREEVIEW_H
#define PROPERTYTREEVIEW_H

#include "aprocessmanager.h"
#include "apropertysortfilterproxymodel.h"

#include <QCheckBox>
#include <QStandardItemModel>
#include <QTreeView>
#include <QLineEdit>

class APropertyTreeView: public QTreeView
{
    Q_OBJECT

public:
    APropertyTreeView(QWidget *parent=nullptr);
    ~APropertyTreeView();

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
    APropertySortFilterProxyModel *propertyProxyModel;
    void loadModel(QString folderName);
    AProcessManager *processManager;
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

#endif // APROPERTYTREEVIEW_H
