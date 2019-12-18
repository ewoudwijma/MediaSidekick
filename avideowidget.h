#ifndef AVIDEOWIDGET_H
#define AVIDEOWIDGET_H

#include "aclipssortfilterproxymodel.h"
#include "sscrubbar.h"
#include "stimespinbox.h"

#include <QtMultimediaWidgets/QVideoWidget>

#include <QComboBox>
#include <QLabel>
#include <QMediaPlayer>
#include <QModelIndex>
#include <QToolBar>
#include <QVBoxLayout>


class AVideoWidget : public QVideoWidget
{
    Q_OBJECT

public:
    AVideoWidget(QWidget *parent = nullptr);
//    int fpsRounded;
    int m_position;
    STimeSpinBox* m_positionSpinner;
    void setSourceVideoVolume(int volume);
    int playerDuration;

private:
    QMediaPlayer *m_player;

    QAction *actionPlay;
    QAction *actionPause;
    QAction *actionSkipNext;
    QAction *actionSkipPrevious;
    QAction *actionRewind;
    QAction *actionFastForward;
    QAction *actionVolume;
    QAction *actionSetIn;
    QAction *actionSetOut;
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

    int sourceVideoVolume;

public slots:
    void onFolderIndexClicked(QModelIndex index);
    void onFileIndexClicked(QModelIndex index);
    void onClipIndexClicked(QModelIndex index);
    void togglePlayPaused();
    void fastForward();
    void rewind();
    void onClipsChangedToVideo(QAbstractItemModel *clipsProxyModel);
    void onTimelinePositionChanged(int progress, int row, int relativeProgress);
    void onReleaseMedia(QString fileName);
    void skipNext();
    void skipPrevious();
    void onSpinnerPositionChanged(int frames);
    void onSetIn(int frames = -1);
    void onSetOut(int frames = -1);
    void onFileRename();
    void onMute();
private slots:
    void onDurationChanged(int duration);
    void onPlayerPositionChanged(int progress);
    void onScrubberSeeked(int mseconds);
//    void onSpinnerChanged(int mseconds);
    void onPlayerStateChanged(QMediaPlayer::State state);
    void onScrubberInChanged(QString AV, int row, int in);
    void onScrubberOutChanged(QString AV, int row, int out);
    void onMutedChanged(bool muted);
    void onStop();
    void onSpeedChanged(QString speed);
    void onPlaybackRateChanged(qreal rate);
    void onMediaStatusChanged(QMediaPlayer::MediaStatus status);
signals:
    void videoPositionChanged(int position, int clipRow, int relativeProgress);
    void scrubberInChanged(QString AV, int row, int in);
    void scrubberOutChanged(QString AV, int row, int out);
    void getPropertyValue(QString fileName, QString key, QString *value);
//    void fpsChanged(int fps);
    void createNewEdit(int frames);

};

#endif // AVIDEOWIDGET_H
