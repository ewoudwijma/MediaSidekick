#ifndef AJobTableView_H
#define AJobTableView_H

#include <QStandardItemModel>
#include <QTableView>

class AJobTableView: public QTableView
{
    Q_OBJECT
public:
    AJobTableView(QWidget *parent = nullptr);
    QStandardItemModel *jobItemModel;

public slots:
    void onAddJob(QString folder, QString file, QString action, QString* id);
    void onAddToJob(QString id, QString log);
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);

};

#endif // AJobTableView_H
