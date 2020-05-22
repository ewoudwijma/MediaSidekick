#include "agprocessthread.h"

#include <QDebug>

AGProcessAndThread::AGProcessAndThread(QObject *parent):
    QObject(parent)
{
}

AGProcessAndThread::~AGProcessAndThread()
{
//    qDebug()<<"AGProcessAndThread::~AGProcessAndThread"<<name;
//    if (process != nullptr)
//    {
//        delete process;
//        process = nullptr;
//    }
//    if (jobThread != nullptr)
//    {
//        delete jobThread;
//        jobThread = nullptr;
//    }
}

void AGProcessAndThread::kill()
{
    if (process != nullptr && process->state() != QProcess::NotRunning)
    {
        qDebug()<<"AGProcessAndThread::kill"<<name<<" kill";
        processStopped = true;
        process->kill();
        qDebug()<<"    "<<name<<" killed";
    }

    if (jobThread != nullptr && jobThread->isRunning())
    {
        qDebug()<<"AGProcessAndThread::kill"<<name<<" stopThreadProcess";
        processStopped = true;
        emit stopThreadProcess();
    }
}

void AGProcessAndThread::command(QString name, const QString &commandString)
{
    process = new QProcess(this);
    process->setProcessChannelMode(QProcess::MergedChannels);

    connect (process, &QProcess::readyReadStandardOutput, [=]()
    {
        QString output = process->readAllStandardOutput();

        QStringList outputStringList = output.split(QRegExp("[\r\n]"),QString::SkipEmptyParts);

        foreach (QString outputString, outputStringList)
        {
            onProcessOutput("output", outputString);
        }
    });

    connect (process, &QProcess::readyReadStandardError, [=]()
    {
        QString outputString = process->readAllStandardError();
        onProcessOutput("output", outputString);
    });

    connect(process, &QProcess::errorOccurred, [=](QProcess::ProcessError error)
    {
        QString outputString = "Error " + QString::number(error) + ": " + process->errorString();
        onProcessOutput("error", outputString);
    });

    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), [=](int exitCode, QProcess::ExitStatus exitStatus)
    {
        QString outputString = "Finished " + QString::number(exitCode) + ": " + exitStatus;
        onProcessOutput("finished", outputString);
    });


    this->name = name;
    log.clear();

    log << commandString;

    QString execPath = "";
#ifdef Q_OS_MAC
    execPath = qApp->applicationDirPath() + "/";
#endif

    process->setProgram(execPath + commandString);
//    process->start(execPath + commandString);
}

void AGProcessAndThread::command(QString name, std::function<void ()> commandFunction)
{
    jobThread = QThread::create(commandFunction);
    //https://www.kdab.com/new-qt-5-10-qthread-create/
    jobThread->setParent(this);
    jobThread->setObjectName(name);

    connect(jobThread, &QThread::started, [=]()
    {
        QString outputString = "started";
        onProcessOutput("started", outputString);
    });

    connect(jobThread, &QThread::finished, [=]()
    {
        QString outputString = "finished";
        onProcessOutput("finished", outputString);
    });

    this->name = name;
    log.clear();
}

void AGProcessAndThread::start()
{
    if (process != nullptr)
        process->start();
    if (jobThread != nullptr)
        jobThread->start();
}

void AGProcessAndThread::onProcessOutput(QString event, QString outputString)
{
    QTime time = QTime();

    int timeIndex = outputString.indexOf("time=");

    if (timeIndex >= 0)
    {
        QString timeString = outputString.mid(timeIndex + 5, 11) + "0";
        time = QTime::fromString(timeString,"HH:mm:ss.zzz");
    }
    log << outputString;
    emit processOutput(time, event, outputString);
}

//void AGProcessAndThread::start(QString name, const QString &commandString)
//{
//    this->name = name;
//    log.clear();

//    log << commandString;

//    QString execPath = "";
//#ifdef Q_OS_MAC
//    execPath = qApp->applicationDirPath() + "/";
//#endif

//    process->start(execPath + commandString);
//}

//void AGProcessAndThread::start(QString name, std::function<void ()> commandFunction)
//{
//    this->name = name;
//    log.clear();

//    QThread *ano = QThread::create(commandFunction);
//    ano->setParent(this);
//    ano->setObjectName(name);

//    jobThread->commandFunction = commandFunction;

//    jobThread->start();
//}
