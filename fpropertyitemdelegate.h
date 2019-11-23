#ifndef FPROPERTYITEMDELEGATE_H
#define FPROPERTYITEMDELEGATE_H


#include <QStyledItemDelegate>

class APropertyItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    APropertyItemDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override;
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const override;

private slots:
//    void buttonPressed();
//    void cellEntered(const QModelIndex &index);

private:
//    QTreeView *myView;
//    QPushButton *btn;
//    QGroupBox *groupBox;
//    bool isOneCellInEditMode;
//    QPersistentModelIndex currentEditedCellIndex;
};

#endif // FPROPERTYITEMDELEGATE_H
