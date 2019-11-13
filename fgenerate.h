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

    void generate(QAbstractItemModel *timelineModel, QString target, QString size, QString pframeRate, int transitionTimeMSecs, QProgressBar *progressBar, bool includingSRT, bool includeAudio);
private:
//    MainWindow *mainWindow;
    FProcessManager *processManager;
    QProgressBar *progressBar;
    QTextStream stream;

    void shotcutTransitionTractor(QXmlStreamWriter *stream, int transitionTime);
    void s(QString y, QString arg1 = "", QString arg2 ="", QString arg3="");

//    void propertyUpdate(QString folderName, QString fileNameSource, QString fileNameTarget);
signals:
    void addLogEntry(QString folder, QString file, QString action, QString *id);
    void addLogToEntry(QString function, QString log);
    void getPropertyValue(QString fileName, QString key, QString *value);

    void reloadEdits();
    void reloadProperties();

public slots:
    void onPropertyUpdate(QString folderName, QString fileNameSource, QString fileNameTarget);

    void onTrim(QString folderName, QString fileNameSource, QString fileNameTarget, QTime inTime, QTime outTime, int progressPercentage);
    void onReloadAll(bool includingSRT);
};

#endif // FGENERATE_H
