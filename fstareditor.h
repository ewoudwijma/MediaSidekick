#ifndef FSTAREDITOR_H
#define FSTAREDITOR_H

#include <QWidget>

#include "fstarrating.h"

//! [0]
class FStarEditor : public QWidget
{
    Q_OBJECT

public:
    FStarEditor(QWidget *parent = nullptr);

    QSize sizeHint() const override;
    void setStarRating(const FStarRating &starRating) {
        myStarRating = starRating;
        oldStarRating = starRating;
    }
    FStarRating starRating() { return myStarRating; }

signals:
    void editingFinished();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    int starAtPosition(int x);

    FStarRating myStarRating;
    FStarRating oldStarRating;
};
//! [0]

#endif // FSTAREDITOR_H
