#ifndef ATIMELINE_H
#define ATIMELINE_H

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


class ATimeline : public QWidget
{
    Q_OBJECT
public:
    explicit ATimeline(QWidget *parent = nullptr);
    int transitiontimeLastGood;

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

//    Qt::CheckState transitionChecked;

public slots:
    void onDurationChanged(int duration);
    void onClipsChangedToTimeline(QAbstractItemModel *itemModel);
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

#endif // ATIMELINE_H
