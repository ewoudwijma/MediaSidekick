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
    void onPropertyFilterChanged(QLineEdit *propertyFilterLineEdit, QCheckBox *propertyDiffCheckBox);
    void onFolderIndexClicked(QModelIndex index);
    void onGetPropertyValue(QString fileName, QString key, QString *value);
    void onFileIndexClicked(QModelIndex index, QModelIndexList selectedIndices = QModelIndexList());
    void onEditIndexClicked(QModelIndex index);
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
protected:
      void resizeEvent(QResizeEvent *event) override;
      QModelIndex moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers) override;
      void scrollTo (const QModelIndex & index, ScrollHint hint = EnsureVisible) override;

signals:
      void addLogEntry(QString function);
      void addLogToEntry(QString function, QString log);
      void propertiesLoaded();
};

#endif // FPROPERTYTREEVIEW_H
