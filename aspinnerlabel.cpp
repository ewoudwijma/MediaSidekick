#include "aspinnerlabel.h"

#include <QMovie>
#include <QDebug>

ASpinnerLabel::ASpinnerLabel(QWidget *parent): QLabel(parent)
{
//    setMinimumWidth(height() * 2.0);

    setAlignment(Qt::AlignCenter);

    int x = parent->geometry().width () / 2.0 ;//- width () / 2.0
    int y = parent->geometry().height () / 2.0 ;//- height () / 2.0

    setGeometry (x, y, 66, 66);

//    qDebug()<<"geometry"<<this<<parent<<parent->geometry() << x << y << geometry();
    startCounter = 0;
}

void ASpinnerLabel::start()
{
    if (startCounter == 0)
    {
        show();
        if (movie() == nullptr)
        {
            QMovie *movie = new QMovie(":/Spinner.gif");
            movie->setScaledSize(QSize(height()*2,height()*2));

            setMovie(movie);
        }

        movie()->start();
    }
    startCounter++;
//    qDebug()<<"ASpinnerLabel::start"<<parent()<<startCounter;
}

void ASpinnerLabel::stop()
{
    startCounter--;
    if (startCounter == 0)
    {
        if (movie() != nullptr)
            movie()->stop();
        clear();
    }
//    qDebug()<<"ASpinnerLabel::stop"<<parent()<<startCounter;
}

////    spinnerLabel->setGeometry(QRect(geometry().x() + geometry().width() / 2.0, geometry().y() + geometry().height() / 2.0, geometry().width() / 4.0, geometry().height() / 4.0));

