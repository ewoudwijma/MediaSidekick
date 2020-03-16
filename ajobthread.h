#ifndef AJOBTHREAD_H
#define AJOBTHREAD_H

#include <QMap>
#include <QStandardItem>
#include <QThread>

typedef struct {
    QObject *thisWidget;
    QString folderName = "";
    QString fileName = "";
    QString action;
    QString command = "";
    QMap<QString, QString> parameters;
    QStandardItem *parentItem = nullptr;
    QStandardItem *currentItem = nullptr;
    QModelIndex currentIndex = QModelIndex();
} AJobParams;

Q_DECLARE_METATYPE(AJobParams); //so signal/slots can use it (jobAddlog)

class AJobThread: public QThread
{
    Q_OBJECT
public:
    void run();
    AJobThread(QObject *parent = nullptr);

    AJobParams jobParams;
    QString (*functionCall)(AJobParams jobParams);

    bool functionIsCalled;

signals:
      void threadResultReady(QString s);
};

#endif // AJOBTHREAD_H
