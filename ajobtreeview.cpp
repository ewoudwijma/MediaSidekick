#include "aglobal.h"
#include "ajobitemdelegate.h"
#include "ajobtreeview.h"

#include <QDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QKeyEvent>
#include <QSettings>
#include <QTextBrowser>
#include <QTime>
#include <QVBoxLayout>
#include <QtDebug>
#include <QApplication>

//static const int jobIndex = 0;
//static const int timestampIndex = 1;
//static const int folderIndex = 2;
//static const int fileIndex = 3;
//static const int logIndex = 5;

AJobTreeView::AJobTreeView(QWidget *parent) : QTreeView(parent)
{
    jobItemModel = new QStandardItemModel(this);
    headerlabels <<"Job"<<"CreateTime"<<"StartTime"<<"EndTime"<<"Duration"<<"Progress"<<"Command"<<"Log"<<"All";
    jobItemModel->setHorizontalHeaderLabels(headerlabels);

    AJobItemDelegate *logItemDelegate = new AJobItemDelegate(this);
    setItemDelegate(logItemDelegate);

    setModel(jobItemModel);

//    qDebug()<<"AJobTreeView::AJobTreeView"<<parent;

//    header()->setSectionResizeMode(QHeaderView::Interactive);
    setItemsExpandable(true);

    setColumnWidth(headerlabels.indexOf("Job"), columnWidth(headerlabels.indexOf("Job")) * 4);

    setColumnHidden(headerlabels.indexOf("CreateTime"), true);
    setColumnHidden(headerlabels.indexOf("All"), true);

    setSortingEnabled(true);

    setSelectionMode(QAbstractItemView::NoSelection); //no blue bars

//    header()->setSectionResizeMode (QHeaderView::Fixed);

    expandAll();

    process = new QProcess(this);
    process->setProcessChannelMode(QProcess::MergedChannels);
    connect (process, SIGNAL(readyReadStandardOutput()), this, SLOT(processOutput()));  // connect process signals with your code
    connect (process, SIGNAL(readyReadStandardError()), this, SLOT(processOutput()));  // same here

//    connect(process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(onProcessFinished(int, QProcess::ExitStatus))); //not working with 'clean notation'
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &AJobTreeView::onProcessFinished);
    connect(process, &QProcess::errorOccurred, this, &AJobTreeView::onProcessErrorOccurred);

    jobThread = new AJobThread(this);
    connect(jobThread, &AJobThread::threadResultReady, this, &AJobTreeView::onThreadResultReady);
//    connect(jobThread, &AJobThread::finished, jobThread, &QObject::deleteLater); //NO! as job needs to stay

    connect(this, &AJobTreeView::jobItemModelSetData, this, &AJobTreeView::onJobItemModelSetData); //run in the main thread
}

AJobTreeView::~AJobTreeView()
{
//     qDebug()<<"AJobTreeView::~AJobTreeView()";
//    jobThread->quit();
//    jobThread->wait();
}

void AJobTreeView::onThreadResultReady(QString errorString)
{
//    qDebug()<<"AJobTreeView::handleThreadResults (QTread)"<<jobQueue.first().jobParams.action<<jobQueue.count()<<errorString;

    jobThread->functionIsCalled = false; //enforce sequential execution

    if (process->state() == QProcess::NotRunning) //job contains both thread and process, wait until the process finishes to do processFinished (ignoring the thread result).
    {
        if (errorString == "Job cancelled")
            processFinished(-300, errorString);
        else if (errorString != "")
            processFinished(-1, errorString);
        else
            processFinished(0, "");
    }
}

QStandardItem *AJobTreeView::createJob(AJobParams jobParams, QString (*functionCall)(AJobParams jobParams), void (*processResult)(AJobParams jobParams, QStringList result))
{
//    qDebug()<<"AJobTreeView::createJob"<<jobParams.parentItem<<jobParams.folderName<<jobParams.fileName<<jobParams.action;

    QString selectedFolderName = QSettings().value("selectedFolderName").toString();
    QString jobName = jobParams.action;

    if (jobParams.fileName != "")
        jobName = jobParams.fileName + " → " + jobName;

    if (jobParams.folderName != "")
    {
        jobName = jobParams.folderName + jobName;
//        if (jobParams.folderName != lastFolder)
            jobName = jobName.replace(selectedFolderName,"");
    }

    QList<QStandardItem *> items;
    QStandardItem *childItem = nullptr;
    foreach (QString headerLabel, headerlabels) //if crash here then jobTreeView not assigned yet in class calling addjob
    {
        QStandardItem *item;

        if (headerLabel == "Job")
            item = new QStandardItem(jobName);
        else if (headerLabel == "CreateTime")
            item = new QStandardItem(QTime().currentTime().toString("hh:mm:ss.zzz"));
        else if (headerLabel == "Command")
        {
            if (jobParams.command != "")
                item = new QStandardItem(jobParams.command);
            else if (functionCall != nullptr)
                item = new QStandardItem(jobParams.action);
            else
                item = new QStandardItem("--");
        }
        else if (headerLabel == "Progress")
            item = new QStandardItem("0");
        else if (headerLabel == "Duration")
        {
            int durationMSec = jobParams.parameters["totalDuration"].toInt();
            if (jobParams.parameters["durationMultiplier"].toDouble() != 0)
                durationMSec /= jobParams.parameters["durationMultiplier"].toDouble();
            QTime duration = QTime::fromMSecsSinceStartOfDay(durationMSec);
//            qDebug()<<"DURATION"<<jobParams.parameters["totalDuration"]<<duration;
            item = new QStandardItem(duration.toString("hh:mm:ss.zzz"));
        }
        else if (headerLabel == "All" || headerLabel == "Log")
            item = new QStandardItem("");
        else
            item  = new QStandardItem(headerLabel);

        if (childItem == nullptr)
            childItem = item;

        items.append(item);
    }

    if (jobParams.parentItem != nullptr)
    {
        jobParams.parentItem->appendRow(items);
    }
    else
        jobItemModel->appendRow(items);

    expandAll();

//    QMap<QString, QString> parameters;

    AJobQueueParams jobQueueParams;
    jobQueueParams.jobParams = jobParams;

    jobQueueParams.jobParams.currentItem = childItem;
    jobQueueParams.jobParams.currentIndex = jobItemModel->indexFromItem(childItem);

    jobQueueParams.functionCall = functionCall;
    jobQueueParams.processOutput = [] (AJobParams jobParams, QString result)
    {
//        AJobTreeView *jobTreeView = qobject_cast<AJobTreeView *>(jobParams.thisObject);
    };
    jobQueueParams.processResult = processResult;

    scrollTo(jobQueueParams.jobParams.currentIndex);

    if (jobQueue.count() == 0)
        emit initProgress();

    jobQueue.enqueue(jobQueueParams);

    ExecuteNextProcess();

    return childItem;
}

void AJobTreeView::onJobAddLog(AJobParams jobParams, QString logMessage)
{
    QStringList resultList = logMessage.split("\n");

    QModelIndex currentIndex =  jobItemModel->indexFromItem(jobParams.currentItem);

//    qDebug()<<"AJobTreeView::onJobAddLog"<<logMessage<<resultList<<resultList.count()<<currentIndex<<currentIndex.data().toString();

    emit jobItemModelSetData(jobItemModel->index(currentIndex.row(), headerlabels.indexOf("Log"), currentIndex.parent()), resultList.last()); //last entry

    QString currentAll = jobItemModel->index(currentIndex.row(), headerlabels.indexOf("All"), currentIndex.parent()).data().toString();
    QString allPlusLogMessage;
    if (currentAll == "")
        allPlusLogMessage = logMessage;
    else
        allPlusLogMessage = currentAll + "\n" + logMessage;

//    if (currentIndex.data().toString().contains("Wideview"))
//        qDebug()<<"AJobTreeView::onJobAddLog"<<resultList.last()<<headerlabels.indexOf("All")<< jobItemModel->index(currentIndex.row(), headerlabels.indexOf("All"), currentIndex.parent()).data().toString() <<logMessage<<allPlusLog;

   emit jobItemModelSetData(jobItemModel->index(currentIndex.row(), headerlabels.indexOf("All"), currentIndex.parent()), allPlusLogMessage); //all entries

    int timeIndex = logMessage.indexOf("time=");
    if (timeIndex >= 0)
    {
        QString timeString = logMessage.mid(timeIndex + 5, 11) + "0";
        QTime time = QTime::fromString(timeString,"HH:mm:ss.zzz");

        int progress;
        if (jobParams.parameters["totalDuration"].toInt() == 0)
            progress = jobItemModel->index(currentIndex.row(), headerlabels.indexOf("Progress"), currentIndex.parent()).data().toInt() + 1;
        else
            progress = 100.0 * time.msecsSinceStartOfDay() / jobParams.parameters["totalDuration"].toInt();

        //emit to get it done in the main thread
        emit jobItemModelSetData(jobItemModel->index(currentIndex.row(), headerlabels.indexOf("Progress"), currentIndex.parent()), QString::number(progress));

        emit updateProgress(jobItemModel->index(currentIndex.row(), headerlabels.indexOf("Progress"), currentIndex.parent()).data().toInt());

        emit jobItemModelSetData(jobItemModel->index(currentIndex.row(), headerlabels.indexOf("EndTime"), currentIndex.parent()), time.toString("hh:mm:ss.zzz"));

    }
    else
        emit jobItemModelSetData(jobItemModel->index(currentIndex.row(), headerlabels.indexOf("EndTime"), currentIndex.parent()), QTime().currentTime().toString("hh:mm:ss.zzz"));
}

void AJobTreeView::onJobItemModelSetData(QModelIndex index, QString value) //run this in the main thread
{
    jobItemModel->setData(index, value);
}

void AJobTreeView::mousePressEvent(QMouseEvent *event)
{
    QModelIndex index = indexAt(event->pos());
//    qDebug()<<"AJobTreeView::mousePressEvent"<<index.data().toString()<<jobItemModel->index(index.row(), headerlabels.indexOf("All"), index.parent()).data().toString()<<event->pos();

    if ((index.column() > 0 || event->pos().x() > 50) && index.data().toString() != "") //only show if an item is clicked on
    {
        QDialog *dialog = new QDialog(this);
        dialog->setWindowTitle("Log details of " + jobItemModel->index(index.row(), 0, index.parent()).data().toString());

        QRect savedGeometry = QSettings().value("Geometry").toRect();
        savedGeometry.setX(savedGeometry.x() + savedGeometry.width()/4);
        savedGeometry.setY(savedGeometry.y() + savedGeometry.height()/4);
        savedGeometry.setWidth(savedGeometry.width()/2);
        savedGeometry.setHeight(savedGeometry.height()/2);
        dialog->setGeometry(savedGeometry);

        QTextBrowser *textBrowser = new QTextBrowser(dialog);
        textBrowser->setWordWrapMode(QTextOption::NoWrap);
        textBrowser->setText(jobItemModel->index(index.row(), headerlabels.indexOf("All"), index.parent()).data().toString());

        QVBoxLayout *m_pDialogLayout = new QVBoxLayout(this);

        m_pDialogLayout->addWidget(textBrowser);

        dialog->setLayout(m_pDialogLayout);

        dialog->show();
    }
    else
        QTreeView::mousePressEvent(event); //expand and collapse
}

void AJobTreeView::testPopulate()
{
    return;
    qDebug()<<"AJobTreeView::testPopulate";
    QStandardItemModel model( 5, 2 );
      for( int r=0; r<5; r++ )
        for( int c=0; c<2; c++)
        {
          QStandardItem *item = new QStandardItem( QString("Row:%0, Column:%1").arg(r).arg(c) );

          if( c == 0 )
            for( int i=0; i<3; i++ )
            {
              QStandardItem *child = new QStandardItem( QString("Item %0").arg(i) );
              child->setEditable( false );
              item->appendRow( child );
            }

          model.setItem(r, c, item);
          qDebug()<<"AJobTreeView::testPopulate"<<r<<c<<item->index().data().toString();

        }

      model.setHorizontalHeaderItem( 0, new QStandardItem( "Foo" ) );
        model.setHorizontalHeaderItem( 1, new QStandardItem( "Bar-Baz" ) );

        setModel( &model );
//        expandAll();
//        show();
        qDebug()<<"AJobTreeView::testPopulate"<<this<<this->model()<<model.rowCount();
}

void AJobTreeView::stopAll()
{
    qDebug()<<"stopAll";

    foreach (AJobQueueParams jobQueueParams, jobQueue)
    {
        QModelIndex currentIndex = jobQueueParams.jobParams.currentIndex;
//        qDebug()<<"AJobTreeView::stopAll"<<currentIndex.row()<<currentIndex.column()<<currentIndex.data().toString();
        emit jobItemModelSetData(jobItemModel->index(currentIndex.row(), headerlabels.indexOf("Progress"), currentIndex.parent()), QString::number(-300)); //cancelled
    }

    process->kill();
    emit stopThreadProcess();
}

void AJobTreeView::ExecuteNextProcess()
{
    //https://stackoverflow.com/questions/4713140/how-can-i-use-a-queue-with-qprocess
//    qDebug()<<"AJobTreeView::ExecuteNextProcess"<<jobQueue.count()<<process->state();

    if (!jobQueue.isEmpty() && process->state() == QProcess::NotRunning && !jobThread->functionIsCalled) //NotRunning => sequentual execution
    {
//        qDebug()<<"AJobTreeView::ExecuteNextProcess"<<jobQueue.count()<<process->state()<<jobQueue.first().jobParams.action;
//        qDebug()<<"AJobTreeView::ExecuteNextProcess"<<jobQueue.first().command;

        //check if parent item failed
        QStandardItem *parentItem = jobQueue.first().jobParams.parentItem;
        QModelIndex parentIndex = jobItemModel->indexFromItem(parentItem);
        int parentProgress = jobItemModel->index(parentIndex.row(), headerlabels.indexOf("Progress"), parentIndex.parent()).data().toInt();

        QModelIndex currentIndex = jobQueue.first().jobParams.currentIndex;

        int progress = jobItemModel->index(currentIndex.row(), headerlabels.indexOf("Progress"), currentIndex.parent()).data().toInt();

//         qDebug()<<"AJobTreeView::ExecuteNextProcess"<<parentIndex.data().toString()<<currentIndex.data().toString()<<parentProgress<<progress;
        if ((parentItem == nullptr || parentProgress == 100) && progress == 0) //if parent then it must be successful && not started earlier
        {
            processOutputString = "";
    //        qDebug()<<"AProcessManager::ExecuteProcess()"<<command;

    //        qDebug()<<"AJobTreeView::ExecuteNextProcess currentindex"<<currentIndex<<currentIndex.data().toString()<<currentIndex.row()<<currentIndex.parent();
            jobItemModel->setData(jobItemModel->index(currentIndex.row(), headerlabels.indexOf("StartTime"), currentIndex.parent()), QTime().currentTime().toString("hh:mm:ss.zzz")); //last entry

            if (jobQueue.first().jobParams.command != "")
                onJobAddLog(jobQueue.first().jobParams, jobQueue.first().jobParams.command);
            else if (jobQueue.first().functionCall != nullptr)
                onJobAddLog(jobQueue.first().jobParams, jobQueue.first().jobParams.action);
            else
                onJobAddLog(jobQueue.first().jobParams, "-");

            //QProcess or QThreat or none (e.g. wrapper)

            bool somethingStarted = false;
            if (jobQueue.first().functionCall != nullptr)
            {
                somethingStarted = true;
                if (true)//jobQueue.first().jobParams.action.contains("ACVC"))
                {
                    jobThread->jobParams = jobQueue.first().jobParams;
                    jobThread->functionCall = jobQueue.first().functionCall;
                    jobThread->functionIsCalled = true;
                    jobThread->start();
                }
                else
                {
                    QString errorString = jobQueue.first().functionCall(jobQueue.first().jobParams);

                    if (errorString != "")
                        processFinished(-1, errorString);
                    else
                        processFinished(0, "");
                }
            }
            if (jobQueue.first().jobParams.command != "") //in case no command (execute processfinished after other commands)
            {
                somethingStarted = true;
                QString execPath = "";
#ifdef Q_OS_MAC
    execPath = qApp->applicationDirPath() + "/";
#endif

                process->start(execPath + jobQueue.first().jobParams.command);
            }

            if (!somethingStarted)//no thread or process
            {
//                qDebug()<<"AJobTreeView::ExecuteNextProcess no thread or process"<<jobQueue.first().jobParams.action;
                processFinished(0, "");
            }
        }
        else
        {
            if (parentProgress != 100)
                processFinished(-200, "Parent job failed");
            else if (progress == -300)
                processFinished(progress, "Job cancelled");
            else
                processFinished(progress, "Error");
        }
    }
}

void AJobTreeView::processOutput()
{
    QString text = process->readAllStandardOutput();
    processOutputString += text;

    if (!jobQueue.isEmpty())
    {

//        qDebug()<<"processOutput"<<text.count(), processOutput;

        if (jobQueue.first().processOutput != nullptr)
            jobQueue.first().processOutput(jobQueue.first().jobParams, text);

        onJobAddLog(jobQueue.first().jobParams, text);

    //    qDebug() << "processOutput" << text;  // read normal output
    //    qDebug() << process->readAllStandardError();  // read error channel
    }
}

void AJobTreeView::onProcessFinished(int exitCode , QProcess::ExitStatus exitStatus)
{
    qDebug()<<"AJobTreeView::onProcessFinished"<<exitCode << exitStatus;
    if (exitStatus == QProcess::NormalExit)
    {
        if (exitCode == 0)
            processFinished(0, ""); //make error code negative value
        else
            processFinished(0 - qAbs(exitCode), "Normal exit with error"); //make error code negative value
    }
    else
        processFinished(0 - qAbs(exitCode), "Crash exit");
}

void AJobTreeView::onProcessErrorOccurred(QProcess::ProcessError error)
{
//    qDebug()<<"AJobTreeView::onProcessErrorOccurred"<<error << process->errorString();
    processFinished(1, process->errorString());
}

void AJobTreeView::processFinished(int exitCode , QString errorString)
{
    if (!jobQueue.isEmpty())
    {
        QStringList processOutputStringList = processOutputString.split(QRegExp("[\r\n]"),QString::SkipEmptyParts);

        AJobQueueParams jobQueueParams = jobQueue.dequeue();

//        qDebug()<<"AJobTreeView::processFinished"<<exitCode<<errorString<<jobQueueParams.jobParams.command<<jobQueueParams.jobParams.action<<jobQueue.count();

        QModelIndex currentIndex = jobQueueParams.jobParams.currentIndex;

        jobItemModel->setData(jobItemModel->index(currentIndex.row(), headerlabels.indexOf("EndTime"), currentIndex.parent()), QTime().currentTime().toString("hh:mm:ss.zzz"));
        QTime startTime = QTime::fromString(jobItemModel->index(currentIndex.row(), headerlabels.indexOf("StartTime"), currentIndex.parent()).data().toString());
        QTime endTime = QTime::fromString(jobItemModel->index(currentIndex.row(), headerlabels.indexOf("EndTime"), currentIndex.parent()).data().toString());
        jobItemModel->setData(jobItemModel->index(currentIndex.row(), headerlabels.indexOf("Duration"), currentIndex.parent()), AGlobal().msec_to_time(startTime.msecsTo(endTime)));

        QString errorMessage = "";
        if (exitCode == 0)
        {
            if (jobQueueParams.jobParams.parameters["exportFolderFileName"] != "")
            {
                QFile file(jobQueueParams.jobParams.parameters["exportFolderFileName"]);//jobQueueStruct.folderName +
                if (file.exists())
                {
                    QDateTime changeTime = QFileInfo(jobQueueParams.jobParams.parameters["exportFolderFileName"]).metadataChangeTime();//jobQueueStruct.folderName +
                    QDateTime currentTime = QDateTime::fromString(jobQueueParams.jobParams.parameters["startTime"]);

//                    qDebug()<<"  AExport::processFinished changetime"<<changeTime<<currentTime;

                    if (changeTime.secsTo(currentTime) > 0) // changetime always later then starttime
                        errorMessage = "File " + jobQueueParams.jobParams.parameters["exportFolderFileName"] + " created before this job";
                    else if (file.size() < 100)
                        errorMessage = "File possibly corrupted as size is smaller then 100 bytes: " + jobQueueParams.jobParams.parameters["exportFolderFileName"] + " " + QString::number(file.size()) + " bytes";
                }
                else
                    errorMessage = "File " + jobQueueParams.jobParams.parameters["exportFolderFileName"] + " not created";

                if (errorMessage != "")
                {
                    onJobAddLog(jobQueueParams.jobParams, errorMessage);
                }
                else
                {
                    onJobAddLog(jobQueueParams.jobParams, "Success");
                }
                jobQueueParams.jobParams.parameters["errorMessage"] = errorMessage;
            }
            else
                onJobAddLog(jobQueueParams.jobParams, "Success");
        }
        else //QProcess or QThread error
        {
            errorMessage = errorString + " (" + QString::number(exitCode) + ")";
            onJobAddLog(jobQueueParams.jobParams, errorMessage);
            jobQueueParams.jobParams.parameters["errorMessage"] = errorMessage;
        }

//        qDebug()<<"processFinished"<<exitCode<<errorMessage<<process->program()<<process->arguments();
        if (exitCode < 0)
            jobItemModel->setData(jobItemModel->index(currentIndex.row(), headerlabels.indexOf("Progress"), currentIndex.parent()), QString::number(exitCode));
        else if (errorMessage == "")
            jobItemModel->setData(jobItemModel->index(currentIndex.row(), headerlabels.indexOf("Progress"), currentIndex.parent()), QString::number(100));

        if (jobQueueParams.processResult != nullptr)
        {
            jobQueueParams.processResult(jobQueueParams.jobParams, processOutputStringList);
        }

        if (jobQueue.isEmpty())
            emit readyProgress(exitCode,errorMessage);
        else
            ExecuteNextProcess(); //take the next process

    } //!jobQueue.isEmpty()
    else
        qDebug()<<"AJobTreeView::processFinished programming error? Jobqueue is empty"<<exitCode<<errorString;
}
