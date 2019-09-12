#ifndef FVIDEOWIDGET_H
#define FVIDEOWIDGET_H

#include "feditsortfilterproxymodel.h"
#include "sscrubbar.h"
#include "stimespinbox.h"

#include <QtMultimediaWidgets/QVideoWidget>

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
    int fpsRounded;
private:
    QMediaPlayer *m_player;

    QAction *actionPlay;
    QAction *actionPause;
    QAction *actionSkipNext;
    QAction *actionSkipPrevious;
    QAction *actionRewind;
    QAction *actionFastForward;
    QAction *actionVolume;

    QVBoxLayout *parentLayout;
    SScrubBar* m_scrubber;
    STimeSpinBox* m_positionSpinner;
    QLabel *m_durationLabel;
    QToolBar* toolbar;
    int m_position;
    int m_previousIn;
    int m_previousOut;
    int m_duration;
    bool m_isSeekable;

    void setupActions(QWidget *widget);

    QString selectedFolderName;
    QString selectedFileName;

    bool isTimelinePlaymode;

public slots:
    void onFolderIndexClicked(QModelIndex index);
    void onFileIndexClicked(QModelIndex index);
    void onEditIndexClicked(QModelIndex index);
    void togglePlayPaused();
    void fastForward();
    void rewind();
    void onEditsChanged(FEditSortFilterProxyModel *editProxyModel);
    void onTimelinePositionChanged(int progress, int row, int relativeProgress);
    void onFileDelete(QString fileName);
    void skipNext();
    void skipPrevious();
    void onSpinnerPositionChanged(int progress);
private slots:
    void onDurationChanged(int duration);
    void onPlayerPositionChanged(int progress);
    void onScrubberSeeked(int mseconds);
//    void onSpinnerChanged(int mseconds);
    void onPlayerStateChanged(QMediaPlayer::State state);
    void onScrubberInChanged(int row, int in);
    void onScrubberOutChanged(int row, int out);
signals:
    void videoPositionChanged(int position, int editRow, int relativeProgress);
    void inChanged(int row, int in);
    void outChanged(int row, int out);
    void getPropertyValue(QString fileName, QString key, QString *value);
    void fpsChanged(int fps);

};

#endif // FVIDEOWIDGET_H
