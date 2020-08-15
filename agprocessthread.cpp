#include "agprocessthread.h"

#include <QDebug>
#include <QApplication>
#include <QWidget>
#ifdef Q_OS_WINxxx
#include <QWinTaskbarProgress>
#endif
#include <QTimer>

#ifdef Q_OS_MAC
#include <QApplication>
#endif

AGProcessAndThread::AGProcessAndThread(QObject *parent):
    QObject(parent)
{
#ifdef Q_OS_WINxxx
    taskbarButton = new QWinTaskbarButton(this);
    taskbarButton->setWindow(((QWidget *)parent)->windowHandle());
//       taskbarButton->setOverlayIcon(QIcon(":/loading.png"));
#endif
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

void AGProcessAndThread::command(QString name, QString commandString)
{
    process = new QProcess(this);
    process->setProcessChannelMode(QProcess::MergedChannels);

    connect (process, &QProcess::readyReadStandardOutput, [=]()
    {
        QString output = process->readAllStandardOutput();

        QStringList outputStringList = output.split(QRegExp("[\r\n]"),QString::SkipEmptyParts);

        foreach (QString outputString, outputStringList)
        {
            addProcessLog("output", outputString);
        }
    });

    connect (process, &QProcess::readyReadStandardError, [=]()
    {
        QString outputString = process->readAllStandardError();
        addProcessLog("output", outputString);
    });

    connect(process, &QProcess::errorOccurred, [=](QProcess::ProcessError error)
    {
        QString outputString = "Error " + QString::number(error) + ": " + process->errorString();
        addProcessLog("error", outputString);
    });

    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), [=](int exitCode, QProcess::ExitStatus exitStatus)
    {
        QString outputString = "Finished " + QString::number(exitCode) + ": " + exitStatus;
        addProcessLog("finished", outputString);
    });


    this->name = name;
    log.clear();

//    log << commandString;

    QString execPath = "";
#ifdef Q_OS_MAC
    execPath = qApp->applicationDirPath() + "/";
#endif

    this->commandString = execPath + commandString;
//    process->setProgram(execPath + commandString); //does not work in MacOS
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
        addProcessLog("started", outputString);
    });

    connect(jobThread, &QThread::finished, [=]()
    {
        QString outputString = "finished";
        addProcessLog("finished", outputString);
    });

    this->name = name;
    log.clear();
}

void AGProcessAndThread::start()
{
    if (process != nullptr)
    {
        process->start(commandString);
        addProcessLog("started", commandString);
    }
    if (jobThread != nullptr)
        jobThread->start();
}

void AGProcessAndThread::addProcessLog(QString event, QString outputString)
{
    QTime time = QTime();

    int timeIndex = outputString.indexOf("time="); //ffmpeg and derperview logging

    if (timeIndex >= 0)
    {
        QString timeString = outputString.mid(timeIndex + 5, 11) + "0";
        time = QTime::fromString(timeString,"HH:mm:ss.zzz");
    }

    timeIndex = outputString.indexOf("% of"); //youtube-dl logging
    //[download]   0.0% of 17.66MiB at 503.03KiB/s ETA 00:35
    if (timeIndex >= 0)
    {
        QString timeString = outputString.mid(timeIndex - 5, 5);
        time = QTime::fromMSecsSinceStartOfDay(timeString.toDouble() * 1000);
        totalTime = QTime::fromMSecsSinceStartOfDay(100 * 1000);
//        qDebug()<<"timeString"<<timeString<<timeString.toDouble()<<time<<totalTime;
    }

    log << outputString;
//    qDebug()<<"outputString"<<outputString;

#ifdef Q_OS_WINxxx
    QWinTaskbarProgress *progress = taskbarButton->progress();
    progress->setVisible(true);
    progress->setValue(time.msecsSinceStartOfDay() / totalTime.msecsSinceStartOfDay());

    if (event == "error")
    {
        progress->setVisible(false);
    }
    if (event == "finished")
    {
        progress->stop();

        QTimer::singleShot(5000, this, [progress]()->void //timer needed to show first message
        {
                               progress->resume();
                               progress->setVisible(false);
        });
    }
#endif

    emit processOutput(time, totalTime, event, outputString);
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
