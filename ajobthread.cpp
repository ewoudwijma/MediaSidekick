#include "ajobthread.h"

#include <QDebug>

AJobThread::AJobThread(QObject *parent): QThread(parent)
{
    functionIsCalled = false;
}

void AJobThread::run()
{
//    qDebug()<<"AJobThread::run"<<jobParams.action;

    QString result = functionCall(jobParams);

    emit threadResultReady(result);
}
