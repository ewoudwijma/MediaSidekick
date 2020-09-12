#ifndef PROPERTYEDITORDIALOG_H
#define PROPERTYEDITORDIALOG_H

#include "aspinnerlabel.h"

#include <QDialog>
#include <QGeoCoordinate>
#include <QStandardItemModel>

#include "aglobal.h"

namespace Ui {
class propertyEditorDialog;
}

class APropertyEditorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit APropertyEditorDialog(QWidget *parent = nullptr);
    ~APropertyEditorDialog();

    void setProperties(QStandardItemModel *itemModel, QStringList filesList);

    void closeEvent(QCloseEvent *event);

    QMap<QString, QMap<QString, QMap<QString, MMetaDataStruct>>> exifToolMap;

private:
    Ui::propertyEditorDialog *ui;
    ASpinnerLabel *spinnerLabel;

    void onRedrawMap();
    bool checkExit();
public slots:
    void onPropertiesLoaded();

    void onLoadProperties();

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
    void loadProperties();

};

#endif // PROPERTYEDITORDIALOG_H
