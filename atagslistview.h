#ifndef ATAGSLISTVIEW_H
#define ATAGSLISTVIEW_H

#include <QListView>
#include <QStandardItemModel>

class ATagsListView: public QListView
{
    Q_OBJECT
public:
    ATagsListView(QWidget *parent = nullptr);
    QStandardItemModel *tagsItemModel;
    bool addTag(QString tagString);
    void stringToModel(QString string);
    QString modelToString();
private:
//    void loadModel(QAbstractItemModel *editItemModel);

public slots:
//    void onFolderSelected(QAbstractItemModel *model);
private slots:
    void onDoubleClicked(const QModelIndex &index);
    void onTagChanged();

signals:
    void tagChanged(QString string);
};

#endif // ATAGSLISTVIEW_H
