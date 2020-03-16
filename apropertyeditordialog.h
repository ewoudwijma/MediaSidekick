#ifndef PROPERTYEDITORDIALOG_H
#define PROPERTYEDITORDIALOG_H

#include "ajobtreeview.h"
#include "aspinnerlabel.h"

#include <QDialog>
#include <QGeoCoordinate>
#include <QStandardItemModel>

namespace Ui {
class propertyEditorDialog;
}

class APropertyEditorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit APropertyEditorDialog(QWidget *parent = nullptr);
    ~APropertyEditorDialog();

    void setProperties(QStandardItemModel *itemModel);

    void closeEvent(QCloseEvent *event);

    AJobTreeView *jobTreeView;
private:
    Ui::propertyEditorDialog *ui;
    ASpinnerLabel *spinnerLabel;

    void onRedrawMap();
    bool checkExit();
public slots:
    void onPropertiesLoaded();

private slots:
    void on_filterColumnsLineEdit_textChanged(const QString &arg1);

    void on_locationCheckBox_clicked(bool checked);

    void on_cameraCheckBox_clicked(bool checked);

    void on_artistCheckBox_clicked(bool checked);

    void on_updateButton_clicked();

    void on_closeButton_clicked();

    void onGeoCodingFinished(QGeoCoordinate geoCoordinate);
    void on_renameButton_clicked();

    void checkAndMatchPicasa();

    void on_refreshButton_clicked();

signals:
    void releaseMedia(QString folderName, QString fileName);
    void loadClips(QStandardItem *parentItem);
    void loadProperties(QStandardItem *parentItem);

};

#endif // PROPERTYEDITORDIALOG_H
