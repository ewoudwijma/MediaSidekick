#ifndef FVIDEOWIDGET_H
#define FVIDEOWIDGET_H

#include "feditsortfilterproxymodel.h"
#include "sscrubbar.h"
#include "stimespinbox.h"

#include <QtMultimediaWidgets/QVideoWidget>

#include <QComboBox>
#include <QLabel>
#include <QMediaPlayer>
#include <QModelIndex>
#include <QToolBar>
#include <QVBoxLayout>


class FVideoWidget : public QVideoWidget
{
    Q_OBJECT

public:
    FVideoWidget(QWidget *parent = nullptr);
//    int fpsRounded;
    int m_position;
    STimeSpinBox* m_positionSpinner;
private:
    QMediaPlayer *m_player;

    QAction *actionPlay;
    QAction *actionPause;
    QAction *actionSkipNext;
    QAction *actionSkipPrevious;
    QAction *actionRewind;
    QAction *actionFastForward;
    QAction *actionVolume;
    QAction *actionUpdateIn;
    QAction *actionUpdateOut;
    QAction *actionStop;
    QAction *actionMute;
    QComboBox *speedComboBox;

    QVBoxLayout *parentLayout;
    SScrubBar* m_scrubber;
    QLabel *m_durationLabel;
    QToolBar* toolbar;
    int m_previousIn;
    int m_previousOut;
    int m_duration;
    bool m_isSeekable;

    void setupActions(QWidget *widget);

    QString selectedFolderName;
    QString selectedFileName;

    bool isTimelinePlaymode;
    int lastHighlightedRow;

    bool isLoading;

public slots:
    void onFolderIndexClicked(QModelIndex index);
    void onFileIndexClicked(QModelIndex index);
    void onEditIndexClicked(QModelIndex index);
    void togglePlayPaused();
    void fastForward();
    void rewind();
    void onEditsChangedToVideo(QAbstractItemModel *editProxyModel);
    void onTimelinePositionChanged(int progress, int row, int relativeProgress);
    void onReleaseMedia(QString fileName);
    void skipNext();
    void skipPrevious();
    void onSpinnerPositionChanged(int frames);
    void onUpdateIn(int frames = -1);
    void onUpdateOut(int frames = -1);
    void onFileRename();
private slots:
    void onDurationChanged(int duration);
    void onPlayerPositionChanged(int progress);
    void onScrubberSeeked(int mseconds);
//    void onSpinnerChanged(int mseconds);
    void onPlayerStateChanged(QMediaPlayer::State state);
    void onScrubberInChanged(int row, int in);
    void onScrubberOutChanged(int row, int out);
    void onMutedChanged(bool muted);
    void onMute();
    void onStop();
    void onSpeedChanged(QString speed);
    void onPlaybackRateChanged(qreal rate);
    void onMediaStatusChanged(QMediaPlayer::MediaStatus status);
signals:
    void videoPositionChanged(int position, int editRow, int relativeProgress);
    void scrubberInChanged(int row, int in);
    void scrubberOutChanged(int row, int out);
    void getPropertyValue(QString fileName, QString key, QString *value);
//    void fpsChanged(int fps);

};

#endif // FVIDEOWIDGET_H
