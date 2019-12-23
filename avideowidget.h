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
#include <QVBoxLayout>


class AVideoWidget : public QVideoWidget
{
    Q_OBJECT

public:
    AVideoWidget(QWidget *parent = nullptr);
//    int fpsRounded;
    int m_position;
    void setSourceVideoVolume(int volume);
    int playerDuration;
    QAction *actionSetIn;

    void setPlaybackRate(qreal rate);
    QString selectedFileName;

private:
    QMediaPlayer *m_player;

    QVBoxLayout *parentLayout;
    SScrubBar* m_scrubber;
    int m_previousIn;
    int m_previousOut;
    int m_duration;
    bool m_isSeekable;

    QString selectedFolderName;

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
    void onStop();

private slots:
    void onDurationChanged(int duration);
    void onPlayerPositionChanged(int progress);
    void onScrubberSeeked(int mseconds);
//    void onSpinnerChanged(int mseconds);
    void onPlayerStateChanged(QMediaPlayer::State state);
    void onScrubberInChanged(QString AV, int row, int in);
    void onScrubberOutChanged(QString AV, int row, int out);
    void onMutedChanged(bool muted);
    void onPlaybackRateChanged(qreal rate);
    void onMediaStatusChanged(QMediaPlayer::MediaStatus status);
signals:
    void videoPositionChanged(int position, int clipRow, int relativeProgress);
    void durationChanged(int duration);
    void scrubberInChanged(QString AV, int row, int in);
    void scrubberOutChanged(QString AV, int row, int out);
    void getPropertyValue(QString fileName, QString key, QString *value);
//    void fpsChanged(int fps);
    void createNewEdit(int frames);

    void playerStateChanged(QMediaPlayer::State state);
    void mutedChanged(bool muted);
    void playbackRateChanged(qreal rate);

};

#endif // AVIDEOWIDGET_H
