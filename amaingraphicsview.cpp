#include "amaingraphicsitem.h"
#include "amaingraphicsview.h"

#include <QGraphicsWidget>
#include <QGraphicsPixmapItem>
#include <QPushButton>
#include <QGraphicsProxyWidget>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QFormLayout>
#include <QTabWidget>
#include <QMessageBox>
#include <QMediaPlayer>
#include <QVideoWidget>

AMainGraphicsView::AMainGraphicsView(QWidget *parent)
    : QGraphicsView(parent)
{
    QGraphicsScene *scene = new QGraphicsScene(this);
    setScene(scene);

    AMainGraphicsItem *w = new AMainGraphicsItem();

//    w->setFlag(QGraphicsItem::ItemIsMovable);
//    w->setPos(20, 20);
//    w->setMinimumSize(100, 100);
//    w->setPreferredSize(320, 240);
//    w->setLayout(layout);
//    w->setWindowTitle(tr("simpleanchorlayout", "QGraphicsAnchorLayout in use"));
    scene->addItem(w);

    QGraphicsPixmapItem* item = scene->addPixmap(QPixmap(":/acvc.ico"));
    item->setFlag(QGraphicsItem::ItemIsMovable);
    item->setPos(200,200);
//    QGraphicsPixmapItem *item = new QGraphicsPixmapItem(QPixmap("yourpixmapfilehere.png"), &scene);

    QGraphicsPixmapItem* item2 = scene->addPixmap(QPixmap("D:\\Video\\2020-01 Picasa star migration\\2018-08-14 19-25-40 [56.000000] one x ewoud.jpg"));
    item2->setScale(0.1);
//    item2->setOffset(-100,-100);
    item2->setPos(-200,-200);
    item2->setFlag(QGraphicsItem::ItemIsMovable);

    if (false)
    {
        QVideoWidget *videoWidget = new QVideoWidget();
        videoWidget->setMinimumSize(200,200);
        QMediaPlayer *m_player = new QMediaPlayer(this);

        m_player->setVideoOutput(videoWidget);

        m_player->setMedia(QUrl::fromLocalFile("D://Video//2019-09-02 Fipre//2020-01-01 16-28-03 [52.109900,4.282550]@60m One X John Doe +53632ms.MP4"));
        m_player->play();
#ifndef Q_OS_MAC
        QSize s1 = videoWidget->size();
        QSize s2 = s1 + QSize(1, 1);
        videoWidget->resize(s2);// enlarge by one pixel
        videoWidget->resize(s1);// return to original size
#endif
        m_player->setMuted(true);

        //    QGraphicsWidget *gw = new QGraphicsWidget();

            QPushButton *button = new QPushButton(this);

            QGroupBox *groupBox = new QGroupBox("Contact Details");
            QLabel *numberLabel = new QLabel("Telephone number");
            QLineEdit *numberEdit = new QLineEdit;

        //    QFormLayout *layout = new QFormLayout;
        //    layout->addRow(numberLabel, numberEdit);
        //    groupBox->setLayout(layout);

            QTabWidget *tabWidget = new QTabWidget;
        //    tabWidget->addTab(videoWidget,"tab1");
            tabWidget->addTab(numberEdit,"tab2");


            QVBoxLayout *layout = new QVBoxLayout;
            layout->addWidget(numberLabel);
            layout->addWidget(videoWidget);
        //    layout->addWidget(numberEdit);
            layout->addWidget(button);
            layout->addWidget(tabWidget);

            groupBox->setLayout(layout);

            QGraphicsProxyWidget *gw = scene->addWidget(groupBox);
        //    gw->setScale(0.01);

            gw->setFlag(QGraphicsItem::ItemIsMovable);
            gw->setPos(300,300);
    }
}
