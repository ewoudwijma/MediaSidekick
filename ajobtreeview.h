#ifndef AJOBTREEVIEW_H
#define AJOBTREEVIEW_H

#include "ajobthread.h"

#include <QTreeView>
#include <QStandardItemModel>
#include <QProcess>
#include <QQueue>

typedef struct {
    AJobParams jobParams;
    QString (*functionCall)(AJobParams jobParams);
    void (*processOutput)(AJobParams jobParams, QString result) ;
    void (*processResult)(AJobParams jobParams, QStringList result);
} AJobQueueParams;

class AJobTreeView: public QTreeView
{
    Q_OBJECT
//from jobtableView
public:
    AJobTreeView(QWidget *parent = nullptr);
    ~AJobTreeView();

    QStandardItemModel *jobItemModel;

    void testPopulate();

    QStandardItem *createJob(AJobParams jobParams, QString (*functionCall)(AJobParams jobParams), void (*processResult)(AJobParams jobParams, QStringList result)); //no signal/slot as functioncalls not working there

private:
    QStringList headerlabels;

public:
    void mousePressEvent(QMouseEvent *event);
//end jobTableView

//from processManager
public:
    void stopAll();

private slots:
    void processOutput();
    void processFinished(int, QString errorString);

    void onProcessErrorOccurred(QProcess::ProcessError processError);
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onThreadResultReady(QString s);
    void onJobItemModelSetData(QModelIndex index, QString value);
private:
    QProcess *process;
    AJobThread *jobThread;
    QString processOutputString;
    void ExecuteNextProcess();

    QQueue<AJobQueueParams> jobQueue;
//end processManager

signals:
    void jobItemModelSetData(QModelIndex index, QString value);

    void initProgress();
    void updateProgress(int value);
    void readyProgress(int result, QString errorString);
    void stopThreadProcess();

public slots:
    void onJobAddLog(AJobParams jobParams, QString logMessage);  //signal/slot to allow cross thread communication
};

#endif // AJOBTREEVIEW_H
