#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "feditsortfilterproxymodel.h"
#include "fedittableview.h"
#include "fstareditor.h"

#include <QCheckBox>
#include <QLineEdit>
#include <QListView>
#include <QMainWindow>
#include <QSettings>
#include <QStandardItemModel>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_actionBlack_theme_triggered();

    void on_actionQuit_triggered();

    void on_actionAbout_triggered();

    void on_actionAbout_Qt_triggered();

    void on_newEditButton_clicked();

    void on_propertyFilterLineEdit_textChanged(const QString &arg1);

    void on_propertyDiffCheckBox_stateChanged(int arg1);

    void onEditFilterChanged();
    void on_actionWhite_theme_triggered();

    void on_action5_stars_triggered();
    void on_action4_stars_triggered();
    void on_action3_stars_triggered();
    void on_action2_stars_triggered();
    void on_action1_star_triggered();
    void on_action0_stars_triggered();
    void on_actionAlike_triggered();

    void on_actionSave_triggered();

    void on_actionPlay_Pause_triggered();

    void on_actionNew_triggered();

    void onEditsChangedToVideo(QAbstractItemModel *itemModel);

    void on_transitionTimeSpinBox_valueChanged(int arg1);

    void on_stretchedDurationSpinBox_valueChanged(int arg1);

    void on_actionIn_triggered();

    void on_actionOut_triggered();

    void on_actionPrevious_frame_triggered();

    void on_actionNext_frame_triggered();

    void on_actionPrevious_in_out_triggered();

    void on_actionNext_in_out_triggered();

    void on_defaultMinSpinBox_valueChanged(int arg1);

    void on_defaultPlusSpinBox_valueChanged(int arg1);

    void on_actionAdd_tag_triggered();

    void onFolderIndexClicked(QAbstractItemModel *itemModel);
    void on_newTagLineEdit_returnPressed();

    void on_generateButton_clicked();

    void on_generateTargetComboBox_currentTextChanged(const QString &arg1);

    void on_generateSizeComboBox_currentTextChanged(const QString &arg1);

    void on_frameRateSpinBox_valueChanged(int arg1);

    void on_actionGenerate_triggered();

    void onFileIndexClicked(QModelIndex index);
    void on_alikeCheckBox_clicked(bool checked);

    void on_fileOnlyCheckBox_clicked(bool checked);

    void on_actionDebug_mode_triggered(bool checked);

    void on_resetSortButton_clicked();

    void on_stretchedCheckBox_clicked(bool checked);

    void onFrameRateChanged(int frameRate);

    void on_locationCheckBox_clicked(bool checked);

    void on_cameraCheckBox_clicked(bool checked);

    void on_transitionComboBox_currentTextChanged(const QString &arg1);

    void on_stretch100Button_clicked();

    void on_stretchDial_valueChanged(int value);

    void onEditsChangedToTimeline(QAbstractItemModel *itemModel);
    void on_stretchDial_sliderMoved(int position);

private:
    Ui::MainWindow *ui;
    QStandardItemModel *tagFilter1Model;
    QStandardItemModel *tagFilter2Model;

    QMetaObject::Connection myConnect(const QObject *sender, const QMetaMethod &signal, const QObject *receiver, const QMetaMethod &method, Qt::ConnectionType type);
    void onTagFiltersChanged();
    QWidget *graphWidget;
    QString stretchValueChangedBy;
signals:
    void propertyFilterChanged(QLineEdit *propertyFilterLineEdit, QCheckBox *propertyDiffCheckBox, QCheckBox *locationCheckBox, QCheckBox *cameraCheckBox);
    void editFilterChanged(FStarEditor *starEditorFilterWidget, QCheckBox *alikeCheckBox, QListView *tagFilter1ListView, QListView *tagFilter2ListView, QCheckBox *allCheckBox);
    void giveStars(int starCount);
    void timelineWidgetsChanged(int transitionTime, QString transitionType, int stretchTime, bool stretchChecked, FEditTableView *editTableView);
};

#endif // MAINWINDOW_H
