#ifndef AGProcessAndThread_H
#define AGProcessAndThread_H

#include <QProcess>
#include <QThread>
#include <QObject>
#include <QTime>
#ifdef Q_OS_WIN
#include <QWinTaskbarButton>
#endif

class AGProcessAndThread: public QObject
{
    Q_OBJECT

    QString commandString;

#ifdef Q_OS_WIN
    QWinTaskbarButton *taskbarButton;
#endif

public:
    AGProcessAndThread(QObject *parent = nullptr);
    ~AGProcessAndThread();

    QProcess *process = nullptr;
    QThread *jobThread = nullptr;

    QStringList log;

    void command(QString name, QString commandString);
    void command(QString name, std::function<void ()> commandFunction);

    QString name;

    void start();
    void kill();

    bool processStopped = false;

    QTime totalTime;

public slots:
    void addProcessLog(QString event, QString outputString);

signals:
    void processOutput(QTime time, QTime totalTime, QString event, QString outputString);
    void stopThreadProcess();

};

#endif // AGProcessAndThread_H
