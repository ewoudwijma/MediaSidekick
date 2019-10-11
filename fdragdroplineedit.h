#ifndef FDRAGDROPLINEEDIT_H
#define FDRAGDROPLINEEDIT_H

#include <QLineEdit>

class FDragDropLineEdit: public QLineEdit
{
    Q_OBJECT
public:
    FDragDropLineEdit(QWidget *parent = nullptr);

    void dragEnterEvent(QDragEnterEvent *e);
    void dropEvent(QDropEvent *e);

};

#endif // FDRAGDROPLINEEDIT_H
