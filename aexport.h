#ifndef AEXPORT_h
#define AEXPORT_h

#include "aclipssortfilterproxymodel.h"
#include "mainwindow.h"

#include <QProgressBar>
#include <QTextStream>
#include <QWidget>
#include <QXmlStreamWriter>

#include <QLabel>

class AExport: public QWidget
{
    Q_OBJECT
public:
    explicit AExport(QWidget *parent = nullptr);

    void exportClips(QAbstractItemModel *timelineModel, QString target, QString size, QString pframeRate, int transitionTimeMSecs, QProgressBar *progressBar, bool includingSRT, bool includeAudio, QLabel *spinnerLabel, QString watermarkFileName);
private:
//    MainWindow *mainWindow;
    FProcessManager *processManager;
    QProgressBar *progressBar;
    QTextStream stream;

    void shotcutTransitionTractor(QXmlStreamWriter *stream, int transitionTime);
    void s(QString y, QString arg1 = "", QString arg2 ="", QString arg3="");

    QString processError = "";

    QLabel *spinnerLabel;

signals:
    void addLogEntry(QString folder, QString file, QString action, QString *id);
    void addLogToEntry(QString function, QString log);
    void getPropertyValue(QString fileName, QString key, QString *value);

    void reloadClips();
    void reloadProperties();

public slots:
    void onPropertyUpdate(QString folderName, QString fileNameSource, QString fileNameTarget);

    void onTrim(QString folderName, QString fileNameSource, QString fileNameTarget, QTime inTime, QTime outTime, int progressPercentage);
    void onReloadAll(bool includingSRT);
};

#endif // AEXPORT_h
