#ifndef AClipsItemDelegate_H
#define AClipsItemDelegate_H

#include "stimespinbox.h"

#include <QStyledItemDelegate>

class AClipsItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override;
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const override;
    bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index) override;
signals:
//    void onStartEditing(QModelIndex);
//    void spinnerChanged(STimeSpinBox *timeSpinBox);
private slots:
    void commitAndCloseEditor();

    void onSpinnerPositionChanged(int frames);
};

#endif // AClipsItemDelegate_H

