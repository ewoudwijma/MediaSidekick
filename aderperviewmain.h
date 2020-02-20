#ifndef ADerperview_H
#define ADerperview_H

#include <QStandardItem>
#include <QString>
#include <QThread>
#include <string>

using namespace std;

class ADerperView: public QObject
{
    Q_OBJECT
public:
    QString Go(const string inputFilename, const string outputFilename, const int totalThreads);
    bool processStopped = false;

signals:
    void processOutput(QString output);

public slots:
    void onStopThreadProcess();
};


#endif // ADerperview_H
