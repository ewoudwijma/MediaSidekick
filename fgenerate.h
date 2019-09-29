#ifndef FGENERATE_H
#define FGENERATE_H

#include "feditsortfilterproxymodel.h"
#include "mainwindow.h"

#include <QProgressBar>
#include <QTextStream>
#include <QWidget>
#include <QXmlStreamWriter>



class FGenerate: public QWidget
{
    Q_OBJECT
public:
    explicit FGenerate(QWidget *parent = nullptr);

    void generate(QStandardItemModel *timelineModel, QString target, QString size, double frameRate, int transitionTimeMSecs, QProgressBar *progressBar);
private:
//    MainWindow *mainWindow;
    FProcessManager *processManager;
    QProgressBar *progressBar;
    QTextStream stream;

    void shotcutTransitionTractor(QXmlStreamWriter *stream, int transitionTime);
    void s(QString y, QString arg1 = "", QString arg2 ="", QString arg3="");
signals:
    void addLogEntry(QString function);
    void addLogToEntry(QString function, QString log);
    void getPropertyValue(QString fileName, QString key, QString *value);

};

#endif // FGENERATE_H
