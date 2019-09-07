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
private slots:
    void updateSectionWidth(int logicalIndex, int, int newSize);
    void updateSectionHeight(int logicalIndex, int oldSize, int newSize);
private:
    QStandardItemModel *propertyItemModel;
    QStandardItemModel *pivotItemModel;
    FPropertySortFilterProxyModel *propertyProxyModel;
    void loadModel(QString folderName);
    FProcessManager *processManager;
    void addSublevelItem(QUrl fileUrl, QString itemName, QString type, QString value);
    QStandardItem *getToplevelItem(QString itemName);
    void diffData(QModelIndex parent);
    QTreeView *frozenTableView;
    void init();
    void updateFrozenTableGeometry();
protected:
      void resizeEvent(QResizeEvent *event) override;
      QModelIndex moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers) override;
      void scrollTo (const QModelIndex & index, ScrollHint hint = EnsureVisible) override;

};

#endif // FPROPERTYTREEVIEW_H
