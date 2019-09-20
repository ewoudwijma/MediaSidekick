#ifndef FLOGTABLEVIEW_H
#define FLOGTABLEVIEW_H

#include <QStandardItemModel>
#include <QTableView>

class FLogTableView: public QTableView
{
    Q_OBJECT
public:
    FLogTableView(QWidget *parent = nullptr);
public slots:
    void onAddEntry(QString function);
    void onAddLogToEntry(QString function, QString log);
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
protected slots:
private:
    QStandardItemModel *logItemModel;

};

#endif // FLOGTABLEVIEW_H
