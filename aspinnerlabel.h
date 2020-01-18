#ifndef ASPINNERLABEL_H
#define ASPINNERLABEL_H

#include <QLabel>

class ASpinnerLabel : public QLabel
{
public:
    ASpinnerLabel(QWidget *parent=nullptr);
    void stop();
    void start();

private:
    int startCounter;
};

#endif // ASPINNERLABEL_H
