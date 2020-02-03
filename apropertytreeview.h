#ifndef APROPERTYTREEVIEW_H
#define APROPERTYTREEVIEW_H

#include "aprocessmanager.h"
#include "apropertysortfilterproxymodel.h"
#include "aspinnerlabel.h"

#include <QCheckBox>
#include <QStandardItemModel>
#include <QTreeView>
#include <QLineEdit>
#include <QProgressBar>

class APropertyTreeView: public QTreeView
{
    Q_OBJECT

public:
    APropertyTreeView(QWidget *parent=nullptr);
    ~APropertyTreeView();
    QStandardItemModel *propertyItemModel;
    APropertySortFilterProxyModel *propertyProxyModel;
    bool editMode;
    void onPropertyColumnFilterChanged(QString filter);

    void onSuggestedNameFiltersClicked(QCheckBox *locationCheckBox, QCheckBox *cameraCheckBox, QCheckBox *artistCheckBox);
    void calculateMinimumDeltaMaximum();
    void saveChanges(QProgressBar *progressBar);
    QMap<QString, QMap<QString, QModelIndex>> changedIndexesMap;
    QString colorChanged;

    void setupModel();

    void initModel(QStandardItemModel *itemModel);

    bool isLoading;

private:
    void loadModel(QString folderName);
    AProcessManager *processManager;
    void addSublevelItem(QUrl fileUrl, QString itemName, QString type, QString value);
    QStandardItem *getToplevelItem(QString itemName);
    QTreeView *frozenTableView;
    void init();
    void updateFrozenTableGeometry();
    void setCellStyle(QStringList fileNames);
    void onPropertyChanged(QStandardItem *item);
    void updateSuggestedNames(int column);

    bool locationInName;
    bool cameraInName;
    bool artistInName;
    QModelIndex findIndex(QString fileName, QString propertyName);

    ASpinnerLabel *spinnerLabel;

    QString valueChangedBy;

    QProgressBar *progressBar;

protected:
      void resizeEvent(QResizeEvent *event) override;
      QModelIndex moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers) override;
      void scrollTo (const QModelIndex & index, ScrollHint hint = EnsureVisible) override;

public slots:
    void onPropertyFilterChanged(QLineEdit *propertyFilterLineEdit, QCheckBox *propertyDiffCheckBox);
    void onFolderIndexClicked(QModelIndex index);
    bool onGetPropertyValue(QString fileName, QString propertyName, QVariant *value);
    bool onSetPropertyValue(QString fileName, QString propertyName, QVariant value, int role = Qt::EditRole);
    void onFileIndexClicked(QModelIndex index, QModelIndexList selectedIndices = QModelIndexList());
    void onClipIndexClicked(QModelIndex index);
    void onRemoveFile(QString fileName);
    void onReloadProperties();

private slots:
    void updateSectionWidth(int logicalIndex, int, int newSize);
    void updateSectionHeight(int logicalIndex, int oldSize, int newSize);

signals:
      void addJob(QString folder, QString file, QString action, QString* id);
      void addToJob(QString id, QString log);
      void propertiesLoaded();
      void releaseMedia(QString fileName);
      void redrawMap();
};

#endif // APROPERTYTREEVIEW_H
