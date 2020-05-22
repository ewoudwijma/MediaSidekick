#ifndef AGProcessAndThread_H
#define AGProcessAndThread_H

#include <QProcess>
#include <QThread>
#include <QObject>
#include <QTime>

class AGProcessAndThread: public QObject
{
    Q_OBJECT

public:
    AGProcessAndThread(QObject *parent = nullptr);
    ~AGProcessAndThread();

    QProcess *process = nullptr;
    QThread *jobThread = nullptr;

    QStringList log;

    void command(QString name, const QString &commandString);
    void command(QString name, std::function<void ()> commandFunction);

    QString name;

    void start();
    void kill();

    bool processStopped = false;

public slots:
    void onProcessOutput(QString event, QString outputString);

signals:
    void processOutput(QTime time, QString event, QString outputString);
    void stopThreadProcess();

};

#endif // AGProcessAndThread_H
