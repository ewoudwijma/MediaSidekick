#ifndef FTIMELINE_H
#define FTIMELINE_H

#include "feditsortfilterproxymodel.h"
#include "fedittableview.h"
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
    void onFolderIndexClicked(QAbstractItemModel *itemModel);
    void onEditsChangedToTimeline(QAbstractItemModel *itemModel);
    void onFileIndexClicked(QModelIndex index);

    void onVideoPositionChanged(int progress, int row, int relativeProgress);
    void onTimelineWidgetsChanged(int transitiontime, QString transitionType, FEditTableView *editTableView);

private slots:
    void onScrubberSeeked(int mseconds);
signals:
    void timelinePositionChanged(int position, int editRow, int relativeProgress);
//    void getPropertyValue(QString fileName, QString key, QString *value);
    void editsChangedToVideo(QAbstractItemModel *itemModel);
    void editsChangedToTimeline(QAbstractItemModel *itemModel);
    void adjustTransitionTime(int transitionTime);

};

#endif // FTIMELINE_H
