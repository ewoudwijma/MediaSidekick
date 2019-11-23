#ifndef APROCESSMANAGER_H
#define APROCESSMANAGER_H


#include <QProcess>
#include <QWidget>

class AProcessManager : public QWidget
{
    Q_OBJECT
public:
    explicit AProcessManager(QWidget *parent = nullptr);

    void startProcess(QString code, QMap<QString, QString> parameters, void (*)(QWidget *, QMap<QString, QString>, QString), void (*)(QWidget *, QString, QMap<QString, QString>, QStringList));
    void startProcess(QMap<QString, QString> parameters, void (*)(QWidget *, QString, QMap<QString, QString>, QStringList));
    void stopAll();

private slots:
    void processOutput();
    void processFinished(int, QProcess::ExitStatus);

private:
    QProcess *process;
    QStringList *processQueue;
    QList<QMap<QString, QString>> *parameterQueue;
    QList<void (*)(QWidget *, QMap<QString, QString>, QString)> *processOutputQueue;
    QList<void (*)(QWidget *, QString, QMap<QString, QString>, QStringList)> *processResultsQueue;
    QString processOutputString;
    void ExecuteProcess();
//    void (*allDoneProcess)(QWidget *, QMap<QString, QString>);
};

#endif // APROCESSMANAGER_H
