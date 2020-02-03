#ifndef AMAINGRAPHICSVIEW_H
#define AMAINGRAPHICSVIEW_H

#include <QGraphicsView>

class AMainGraphicsView: public QGraphicsView

{
    Q_OBJECT

public:
    AMainGraphicsView(QWidget *parent = nullptr);
};

#endif // AMAINGRAPHICSVIEW_H
