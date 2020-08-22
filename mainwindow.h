#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "agfilesystem.h"
#include "astareditor.h"

#include <QCheckBox>
#include <QLineEdit>
#include <QListView>
#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QSettings>
#include <QStandardItemModel>
#include <QComboBox>

#include <QMediaPlayer>

#include "apropertyeditordialog.h"

#include <QGraphicsItem>

namespace Ui {
class MainWindow;
}

typedef struct {
    QString context;
    QWidget *widget;
    QString widgetName;
    QString helpText;
    QTabWidget *tabWidget;
    int tabIndex;
} AContextSensitiveHelpRequest;

class MainWindow : public QMainWindow
{
    Q_OBJECT

    Ui::MainWindow *ui;

    QMetaObject::Connection myConnect(const QObject *sender, const QMetaMethod &signal, const QObject *receiver, const QMetaMethod &method, Qt::ConnectionType type);
    QWidget *graphWidget1, *graphWidget2, *graphicsWidget;
    QNetworkAccessManager m_network;

    void allConnects();
    void allTooltips();
    void loadSettings();
    void changeUIProperties();
    bool checkExit();

    APropertyEditorDialog *propertyEditorDialog;

    AGFileSystem *agFileSystem;

    int countFolders(QString folderName, int depth = 0);
    void checkAndOpenFolder(QString selectedFolderName);

    QString latestVersionURL = "";

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void closeEvent(QCloseEvent *event);

    QList<AGProcessAndThread *> processes;

public slots:
    void onShowInStatusBar(QString message, int timeout = 0);

private slots:
    void on_actionBlack_theme_triggered();
    void on_actionQuit_triggered();
    void on_actionAbout_triggered();
    void on_actionAbout_Qt_triggered();
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
    void on_actionIn_triggered();
    void on_actionOut_triggered();
    void on_actionPrevious_frame_triggered();
    void on_actionNext_frame_triggered();
    void on_actionPrevious_in_out_triggered();
    void on_actionNext_in_out_triggered();
    void on_actionExport_triggered();
    void showUpgradePrompt();
    void onUpgradeCheckFinished(QNetworkReply *reply);
    void on_actionDonate_triggered();
    void on_actionCheck_for_updates_triggered();
    void on_actionWhatIsNew_triggered();

    void on_actionGithub_MSK_Issues_triggered();

    void on_actionMute_triggered();

    void on_actionTooltips_changed();

    void on_actionOpen_Folder_triggered();

    void on_spotviewDownButton_clicked();
    void on_spotviewRightButton_clicked();
    void on_spotviewReturnButton_clicked();

    void on_timelineViewButton_clicked();

    void on_searchLineEdit_textChanged(const QString &arg1);

    void on_actionReload_triggered();

    void on_mediaFileScaleSlider_valueChanged(int value);

    void on_playerInDialogcheckBox_clicked(bool checked);

    void on_clipScaleSlider_valueChanged(int value);

    void on_orderByDateButton_clicked();

    void on_orderByNameButton_clicked();

//    void onExportClips(QStandardItem *parentItem, QStandardItem *&currentItem, QString folderName, QString fileName, bool moveToBin);

    void on_actionRefresh_triggered();

    void on_actionUndo_triggered();

    void on_actionRedo_triggered();

    void on_actionSpeed_Up_triggered();

    void on_actionSpeed_Down_triggered();

    void on_actionVolume_Up_triggered();

    void on_actionVolume_Down_triggered();

    void on_actionZoom_In_triggered();

    void on_actionZoom_Out_triggered();

    void on_actionItem_Up_triggered();

    void on_actionItem_Down_triggered();

    void on_actionItem_Left_triggered();

    void on_actionItem_Right_triggered();

    void on_actionTop_Folder_triggered();

    void on_actionHelp_triggered();

signals:
    void clipsFilterChanged(QComboBox *ratingFilterComboBox, QListView *tagFilter1ListView, QListView *tagFilter2ListView, QCheckBox *allCheckBox);
      void giveStars(int starCount);
//    void timelineWidgetsChanged(int transitionTime, QString transitionType, AClipsTableView *clipsTableView);

protected:
    bool eventFilter(QObject *obj, QEvent *event);
    void resizeEvent(QResizeEvent *event);
};

#endif // MAINWINDOW_H
