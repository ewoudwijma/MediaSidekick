#ifndef FTIMELINE_H
#define FTIMELINE_H

#include "aclipssortfilterproxymodel.h"
#include "aclipstableview.h"
#include "sscrubbar.h"
#include "stimespinbox.h"

#include <QLabel>
#include <QObject>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWidget>
#include <QModelIndex>


class FTimeline : public QWidget
{
    Q_OBJECT
public:
    explicit FTimeline(QWidget *parent = nullptr);

    int originalDuration;
    int transitiontimeDuration;
//    void calculateTransitiontimeDuration(QAbstractItemModel *itemModel);
private:
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
    bool m_isSeekable;

    void setupActions(QWidget *widget);

    int transitiontime;

    int transitiontimeLastGood;
//    Qt::CheckState transitionChecked;

public slots:
    void onDurationChanged(int duration);
    void onClipsChangedToTimeline(QAbstractItemModel *itemModel);
    void onFileIndexClicked(QModelIndex index);
    void onVideoPositionChanged(int progress, int row, int relativeProgress);
    void onTimelineWidgetsChanged(int transitiontime, QString transitionType, AClipsTableView *clipsTableView);

private slots:
    void onScrubberSeeked(int mseconds);

signals:
    void timelinePositionChanged(int position, int clipRow, int relativeProgress);
    void clipsChangedToVideo(QAbstractItemModel *itemModel);
    void clipsChangedToTimeline(QAbstractItemModel *itemModel);
    void adjustTransitionTime(int transitionTime);

};

#endif // FTIMELINE_H
