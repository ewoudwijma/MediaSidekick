#ifndef FPROCESSMANAGER_H
#define FPROCESSMANAGER_H


#include <QProcess>
#include <QWidget>

class FProcessManager : public QWidget
{
    Q_OBJECT
public:
    explicit FProcessManager(QWidget *parent = nullptr);

    void startProcess(QString code, void (*)(QWidget *, QString), void (*)(QWidget *, QString, QStringList));
    void stopAll();

private slots:
    void processOutput();
    void processFinished(int, QProcess::ExitStatus);

private:
    QProcess *process;
    QStringList *processQueue;
    QList<void (*)(QWidget *, QString)> *processOutputQueue;
    QList<void (*)(QWidget *, QString, QStringList)> *processResultsQueue;
    QString processOutputString;
    void ExecuteProcess();
};

#endif // FPROCESSMANAGER_H
