#include "aprocessmanager.h"

#include <QDebug>

AProcessManager::AProcessManager(QWidget *parent) : QWidget(parent)
{
    process = new QProcess(this);
    process->setProcessChannelMode(QProcess::MergedChannels);
    connect (process, SIGNAL(readyReadStandardOutput()), this, SLOT(processOutput()));  // connect process signals with your code
    connect (process, SIGNAL(readyReadStandardError()), this, SLOT(processOutput()));  // same here
    connect (process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(processFinished(int, QProcess::ExitStatus)));  // same here
    processQueue = new QStringList();
    parameterQueue = new QList<QMap<QString, QString>>();
    processOutputQueue = new QList<void (*)(QWidget *, QMap<QString, QString>, QString)>;
    processResultsQueue = new QList<void (*)(QWidget *, QString, QMap<QString, QString>, QStringList)>;

}

void AProcessManager::startProcess(QString code, QMap<QString, QString> parameters, void (*processOutput)(QWidget *, QMap<QString, QString>, QString), void (*processResult)(QWidget *, QString, QMap<QString, QString>, QStringList))
{
//    qDebug()<<"startProcess"<<code<<processOutput<<processResult;
    processQueue->append(code);
    parameterQueue->append(parameters);
    processOutputQueue->append(processOutput);
    processResultsQueue->append(processResult);
    ExecuteProcess();
}

void AProcessManager::startProcess(QMap<QString, QString> parameters, void (*processResult)(QWidget *, QString, QMap<QString, QString>, QStringList))
{
    processQueue->append("");
//    allDoneProcess = pallDoneProcess;
    parameterQueue->append(parameters);
    processOutputQueue->append(nullptr);
    processResultsQueue->append(processResult);
}

void AProcessManager::stopAll()
{
    process->kill();
}

void AProcessManager::ExecuteProcess()
{
    //https://stackoverflow.com/questions/4713140/how-can-i-use-a-queue-with-qprocess
   if (!processQueue->isEmpty() && process->state() == QProcess::NotRunning)
    {
        processOutputString = "";
        QString tf = processQueue->takeFirst();
//        qDebug()<<"AProcessManager::ExecuteProcess()"<<tf;
        if (tf != "") //in case no command (execute processfinished after other commands)
            process->start(tf);
        else
            processFinished(0, QProcess::NormalExit);
    }
//   else if (allDoneProcess != nullptr)
//   {
//       qDebug()<<"alldone process"<<allDoneProcess<<parameterQueue->count();
//       QMap<QString, QString> parameters = parameterQueue->first();
//       allDoneProcess(parentWidget(), parameters);
//   }
}

void AProcessManager::processOutput()
{
    QString text = process->readAllStandardOutput();
    processOutputString += text;
    void (*processOutput)(QWidget *, QMap<QString, QString>, QString) = processOutputQueue->first();
    QMap<QString, QString> parameters = parameterQueue->first();

    if (processOutput != nullptr)
        processOutput(parentWidget(), parameters, text);
//    qDebug() << "processOutput" << text;  // read normal output
//    qDebug() << process->readAllStandardError();  // read error channel
}

void AProcessManager::processFinished(int exitCode , QProcess::ExitStatus exitStatus)
{
    QStringList processOutputStringList = processOutputString.split(QRegExp("[\r\n]"),QString::SkipEmptyParts);

    processOutputQueue->takeFirst(); //remove it
    void (*processResult)(QWidget *, QString, QMap<QString, QString>, QStringList) = processResultsQueue->takeFirst();
    QMap<QString, QString> parameters = parameterQueue->takeFirst();

    if (processResult != nullptr)
    {
//        qDebug()<<"processFinished"<<exitCode<<exitStatus<<process->program()<<process->arguments()<<processResult;
        QString command = process->program();
        for (int i=0; i<process->arguments().count();i++)
        {
            command += " " + process->arguments()[i];
        }
        processResult(parentWidget(), command, parameters, processOutputStringList);
    }
    ExecuteProcess();
}

