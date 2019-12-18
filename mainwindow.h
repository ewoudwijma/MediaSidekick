#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "aclipssortfilterproxymodel.h"
#include "aclipstableview.h"
#include "astareditor.h"

#include <QCheckBox>
#include <QLineEdit>
#include <QListView>
#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QSettings>
#include <QStandardItemModel>
#include <QComboBox>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void closeEvent(QCloseEvent *event);
private slots:
    void on_actionBlack_theme_triggered();
    void on_actionQuit_triggered();
    void on_actionAbout_triggered();
    void on_actionAbout_Qt_triggered();
    void on_propertyFilterLineEdit_textChanged(const QString &arg1);
    void on_propertyDiffCheckBox_stateChanged(int arg1);
    void onClipsFilterChanged();
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
    void onClipsChangedToVideo(QAbstractItemModel *itemModel);
    void on_transitionTimeSpinBox_valueChanged(int arg1);
    void on_actionIn_triggered();
    void on_actionOut_triggered();
    void on_actionPrevious_frame_triggered();
    void on_actionNext_frame_triggered();
    void on_actionPrevious_in_out_triggered();
    void on_actionNext_in_out_triggered();
    void onFolderIndexClicked(QAbstractItemModel *itemModel);
    void on_newTagLineEdit_returnPressed();
    void on_exportButton_clicked();
    void on_exportTargetComboBox_currentTextChanged(const QString &arg1);
    void on_exportSizeComboBox_currentTextChanged(const QString &arg1);
    void on_actionExport_triggered();
    void onFileIndexClicked(QModelIndex index);
    void on_alikeCheckBox_clicked(bool checked);
    void on_fileOnlyCheckBox_clicked(bool checked);
    void on_actionDebug_mode_triggered(bool checked);
    void on_resetSortButton_clicked();
    void on_locationCheckBox_clicked(bool checked);
    void on_cameraCheckBox_clicked(bool checked);
    void on_transitionComboBox_currentTextChanged(const QString &arg1);
    void onClipsChangedToTimeline(QAbstractItemModel *itemModel);
    void on_transitionDial_valueChanged(int value);
    void on_positionDial_valueChanged(int value);
    void showUpgradePrompt();
    void onUpgradeCheckFinished(QNetworkReply *reply);
    void onAdjustTransitionTime(int transitionTime);
    void onPropertiesLoaded();
    void onVideoPositionChanged(int progress, int row, int relativeProgress);
    void on_exportFramerateComboBox_currentTextChanged(const QString &arg1);
    void on_authorCheckBox_clicked(bool checked);
    void on_clipsTabWidget_currentChanged(int index);
    void on_filesTabWidget_currentChanged(int index);
    void on_actionDonate_triggered();
    void on_actionCheck_for_updates_triggered();
    void on_actionHelp_triggered();
    void on_watermarkButton_clicked();

    void on_actionGithub_ACVC_Issues_triggered();

    void onCreateNewEdit();
    void on_actionMute_triggered();

    void watermarkFileNameChanged(QString newFileName);
    void on_clipsFramerateComboBox_currentTextChanged(const QString &arg1);

    void on_exportVideoAudioSlider_valueChanged(int value);

    void on_clearJobsButton_clicked();

    void on_positionDial_sliderMoved(int position);

    void on_transitionDial_sliderMoved(int position);

    void on_ratingFilterComboBox_currentIndexChanged(int index);

private:
    Ui::MainWindow *ui;
    QStandardItemModel *tagFilter1Model;
    QStandardItemModel *tagFilter2Model;

    QMetaObject::Connection myConnect(const QObject *sender, const QMetaMethod &signal, const QObject *receiver, const QMetaMethod &method, Qt::ConnectionType type);
    void onTagFiltersChanged();
    QWidget *graphWidget1, *graphWidget2;
    QString transitionValueChangedBy;
    QString positionValueChangedBy;
    QNetworkAccessManager m_network;
    int positiondialOldValue;

    QString watermarkFileName = "";

    void allConnects();
    void allTooltips();
    void loadSettings();
    void changeUIProperties();
    bool checkExit();
signals:
    void propertyFilterChanged(QLineEdit *propertyFilterLineEdit, QCheckBox *propertyDiffCheckBox, QCheckBox *locationCheckBox, QCheckBox *cameraCheckBox, QCheckBox *authorCheckBox);
    void clipsFilterChanged(QComboBox *ratingFilterComboBox, QCheckBox *alikeCheckBox, QListView *tagFilter1ListView, QListView *tagFilter2ListView, QCheckBox *allCheckBox);
    void giveStars(int starCount);
    void timelineWidgetsChanged(int transitionTime, QString transitionType, AClipsTableView *clipsTableView);
};

#endif // MAINWINDOW_H
