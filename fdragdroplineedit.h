#ifndef FDRAGDROPLINEEDIT_H
#define FDRAGDROPLINEEDIT_H

#include <QLineEdit>

class ADragDropLineEdit: public QLineEdit
{
    Q_OBJECT
public:
    ADragDropLineEdit(QWidget *parent = nullptr);

    void dragEnterEvent(QDragEnterEvent *e);
    void dropEvent(QDropEvent *e);

};

#endif // FDRAGDROPLINEEDIT_H
