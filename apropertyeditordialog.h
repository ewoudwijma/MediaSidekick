#ifndef PROPERTYEDITORDIALOG_H
#define PROPERTYEDITORDIALOG_H

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
private:
    Ui::propertyEditorDialog *ui;
    ASpinnerLabel *spinnerLabel;

    void redrawMap();
    bool checkExit();
public slots:
    void onPropertiesLoaded();

private slots:
    void onAddJob(QString folder, QString file, QString action, QString *id);
    void onAddToJob(QString id, QString log);
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
    void addJob(QString folder, QString file, QString action, QString* id);
    void addToJob(QString id, QString log);
    void releaseMedia(QString fileName);
    void reloadClips();
    void reloadProperties();

};

#endif // PROPERTYEDITORDIALOG_H
