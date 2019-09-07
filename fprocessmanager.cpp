#include "fprocessmanager.h"

#include <QDebug>

FProcessManager::FProcessManager(QWidget *parent) : QWidget(parent)
{
    process = new QProcess(this);
    process->setProcessChannelMode(QProcess::MergedChannels);
    connect (process, SIGNAL(readyReadStandardOutput()), this, SLOT(processOutput()));  // connect process signals with your code
    connect (process, SIGNAL(readyReadStandardError()), this, SLOT(processOutput()));  // same here
    connect (process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(processFinished(int, QProcess::ExitStatus)));  // same here
    processQueue = new QStringList();
    processOutputQueue = new QList<void (*)(QWidget *, QString)>;
    processResultsQueue = new QList<void (*)(QWidget *, QString, QStringList)>;

}

void FProcessManager::startProcess(QString code, void (*processOutput)(QWidget *, QString), void (*processResult)(QWidget *, QString, QStringList))
{
//    qDebug()<<"startProcess"<<code<<processOutput<<processResult;
    processQueue->append(code);
    processOutputQueue->append(processOutput);
    processResultsQueue->append(processResult);
//    processResult("hoi");
    ExecuteProcess();
}

void FProcessManager::stopAll()
{
    process->kill();
}

void FProcessManager::ExecuteProcess()
{
    //https://stackoverflow.com/questions/4713140/how-can-i-use-a-queue-with-qprocess
   if (!processQueue->isEmpty() && process->state() == QProcess::NotRunning)
    {
        processOutputString = "";
        process->start(processQueue->takeFirst());
    }
}

void FProcessManager::processOutput()
{
    QString text = process->readAllStandardOutput();
    processOutputString += text;
    void (*processOutput)(QWidget *, QString) = processOutputQueue->first();
    if (processOutput != nullptr)
        processOutput(parentWidget(), text);
//    qDebug() << "processOutput" << text;  // read normal output
//    qDebug() << process->readAllStandardError();  // read error channel
}

void FProcessManager::processFinished(int exitCode , QProcess::ExitStatus exitStatus)
{
    QStringList processOutputStringList = processOutputString.split(QRegExp("[\r\n]"),QString::SkipEmptyParts);

    processOutputQueue->takeFirst(); //remove it
    void (*processResult)(QWidget *, QString, QStringList) = processResultsQueue->takeFirst();
    if (processResult != nullptr)
    {
//        qDebug()<<"processFinished"<<exitCode<<exitStatus<<process->program()<<process->arguments()<<processResult;
        QString command = process->program();
        for (int i=0; i<process->arguments().count();i++)
        {
            command += " " + process->arguments()[i];
        }
        processResult(parentWidget(), command, processOutputStringList);
    }
    ExecuteProcess();
}

