#ifndef FTIMELINE_H
#define FTIMELINE_H

#include "fedititemmodel.h"
#include "feditsortfilterproxymodel.h"
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
    int m_duration;
    bool m_isSeekable;

    void setupActions(QWidget *widget);

public slots:
    void onDurationChanged(int duration);
    void onFolderIndexClicked(FEditSortFilterProxyModel *editProxyModel);
    void onEditsChanged(FEditSortFilterProxyModel *editProxyModel);
    void onFileIndexClicked(QModelIndex index);

    void onPositionChanged(int progress);
};

#endif // FTIMELINE_H
