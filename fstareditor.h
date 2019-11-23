#ifndef FSTAREDITOR_H
#define FSTAREDITOR_H

#include <QWidget>

#include "fstarrating.h"

//! [0]
class AStarEditor : public QWidget
{
    Q_OBJECT

public:
    AStarEditor(QWidget *parent = nullptr);

    QSize sizeHint() const override;
    void setStarRating(const AStarRating &starRating) {
        myStarRating = starRating;
        oldStarRating = starRating;
    }
    AStarRating starRating() { return myStarRating; }

signals:
    void editingFinished();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    int starAtPosition(int x);

    AStarRating myStarRating;
    AStarRating oldStarRating;
};
//! [0]

#endif // FSTAREDITOR_H
