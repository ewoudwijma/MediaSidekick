#ifndef ADRAGDROPLINEEDIT_H
#define ADRAGDROPLINEEDIT_H

#include <QLineEdit>

class ADragDropLineEdit: public QLineEdit
{
    Q_OBJECT
public:
    ADragDropLineEdit(QWidget *parent = nullptr);

    void dragEnterEvent(QDragEnterEvent *e);
    void dropEvent(QDropEvent *e);

};

#endif // ADRAGDROPLINEEDIT_H
