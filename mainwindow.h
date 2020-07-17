#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "aclipssortfilterproxymodel.h"
#include "aclipstableview.h"
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

    void createContextSensitiveHelp(QString eventName, QString arg1 = "");
    void showContextSensitiveHelp(int index);

    QList<AContextSensitiveHelpRequest> requestList;
    int currentRequestNumber;

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
    void onClipsFilterChanged(bool fromFilters);
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
    void on_actionIn_triggered();
    void on_actionOut_triggered();
    void on_actionPrevious_frame_triggered();
    void on_actionNext_frame_triggered();
    void on_actionPrevious_in_out_triggered();
    void on_actionNext_in_out_triggered();
    void onFolderSelected(QAbstractItemModel *itemModel);
    void on_newTagLineEdit_returnPressed();
    void on_actionExport_triggered();
    void onFileIndexClicked(QModelIndex index, QStringList filePathList);
    void on_alikeCheckBox_clicked(bool checked);
    void on_fileOnlyCheckBox_clicked(bool checked);
    void on_actionDebug_mode_triggered(bool checked);
    void on_resetSortButton_clicked();
    void showUpgradePrompt();
    void onUpgradeCheckFinished(QNetworkReply *reply);
    void onVideoPositionChanged(int progress, int row, int relativeProgress);
    void onDurationChanged(int duration);
    void on_clipsTabWidget_currentChanged(int index);
    void on_filesTabWidget_currentChanged(int index);
    void on_actionDonate_triggered();
    void on_actionCheck_for_updates_triggered();
    void on_actionWhatIsNew_triggered();

    void on_actionGithub_MSK_Issues_triggered();

    void onCreateNewEdit();
    void on_actionMute_triggered();

    void on_ratingFilterComboBox_currentIndexChanged(int index);

    void on_skipBackwardButton_clicked();

    void on_seekBackwardButton_clicked();

    void on_playButton_clicked();

    void on_seekForwardButton_clicked();

    void on_skipForwardButton_clicked();

    void on_stopButton_clicked();

    void on_muteButton_clicked();

    void on_setInButton_clicked();

    void on_setOutButton_clicked();

    void on_speedComboBox_currentTextChanged(const QString &arg1);

    void on_actionTooltips_changed();

    void onPlayerStateChanged(QMediaPlayer::State state);
    void onMutedChanged(bool muted);
    void onPlaybackRateChanged(qreal rate);

    void onTagFilter1ListViewChanged();
    void onTagFilter2ListViewChanged();

    void on_actionOpen_Folder_triggered();

    void on_tabUIWidget_currentChanged(int index);

    void on_spotviewDownButton_clicked();
    void on_spotviewRightButton_clicked();
    void on_spotviewReturnButton_clicked();

    void on_timelineViewButton_clicked();

    void on_searchLineEdit_textChanged(const QString &arg1);

    void on_reloadViewButton_clicked();

    void on_mediaFileScaleSlider_valueChanged(int value);

    void on_playerInDialogcheckBox_clicked(bool checked);

    void on_clipScaleSlider_valueChanged(int value);

    void on_orderByDateButton_clicked();

    void on_orderByNameButton_clicked();

//    void onExportClips(QStandardItem *parentItem, QStandardItem *&currentItem, QString folderName, QString fileName, bool moveToBin);

    void on_refreshViewButton_clicked();

signals:
    void clipsFilterChanged(QComboBox *ratingFilterComboBox, QCheckBox *alikeCheckBox, QListView *tagFilter1ListView, QListView *tagFilter2ListView, QCheckBox *allCheckBox);
    void giveStars(int starCount);
    void timelineWidgetsChanged(int transitionTime, QString transitionType, AClipsTableView *clipsTableView);

protected:
    bool eventFilter(QObject *obj, QEvent *event);
};

#endif // MAINWINDOW_H
