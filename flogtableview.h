#ifndef FLOGTABLEVIEW_H
#define FLOGTABLEVIEW_H

#include <QStandardItemModel>
#include <QTableView>

class ALogTableView: public QTableView
{
    Q_OBJECT
public:
    ALogTableView(QWidget *parent = nullptr);
public slots:
    void onAddEntry(QString folder, QString file, QString action, QString* id);
    void onAddLogToEntry(QString id, QString log);
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
protected slots:
private:
    QStandardItemModel *logItemModel;

};

#endif // FLOGTABLEVIEW_H
