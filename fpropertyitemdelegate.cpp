#include "fpropertyitemdelegate.h"

#include <QDateTimeEdit>
#include <QDebug>
//#include <QHBoxLayout>
//#include <QPainter>
//#include <QStandardItemModel>
//#include <QQuickWidget>
//#include <QLineEdit>


FPropertyItemDelegate::FPropertyItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

QWidget *FPropertyItemDelegate::createEditor(QWidget *parent,
                                    const QStyleOptionViewItem &option,
                                    const QModelIndex &index) const

{
    qDebug()<<"createEditor" <<index.data()<<index.model()->index(index.row(), 1,index.parent()).data().toString();
    if (index.model()->index(index.row(), 1,index.parent()).data().toString() == "QDateTime") {
        QDateTimeEdit* result = new QDateTimeEdit(parent);
        result->setCalendarPopup(true);
        result->setDisplayFormat("yyyy:MM:dd HH:mm:ss");

        return result;
    }
    else
    {
        return QStyledItemDelegate::createEditor(parent, option, index);
    }

}

void FPropertyItemDelegate::setEditorData(QWidget *editor,
                                 const QModelIndex &index) const
{
//    qDebug()<<"setEditorData" <<index.data()<<index.model()->index(index.row(), 1,index.parent()).data().toString();
    if (index.model()->index(index.row(), 1,index.parent()).data().toString() == "QDateTime")
    {
        QDateTimeEdit* dtEditor = qobject_cast<QDateTimeEdit*>(editor);
        Q_ASSERT(dtEditor);
        if (index.data(Qt::EditRole).isNull())
            dtEditor->clear();
        else
            dtEditor->setDateTime(index.data(Qt::EditRole).toDateTime());
    }
    else
    {
        QStyledItemDelegate::setEditorData(editor, index);
    }
}

void FPropertyItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                const QModelIndex &index) const
{
//    qDebug()<<"setModelData" <<index.data()<<index.model()->index(index.row(), 1,index.parent()).data().toString();
    if (index.model()->index(index.row(), 1,index.parent()).data().toString() == "QDateTime")
    {
        QDateTimeEdit* dtEditor = qobject_cast<QDateTimeEdit*>(editor);
        if(dtEditor->text().isEmpty())
            model->setData(index, QVariant(), Qt::EditRole);
        else
            model->setData(index, dtEditor->dateTime().toString("yyyy:MM:dd HH:mm:ss"), Qt::EditRole);
    }
    else {
        QStyledItemDelegate::setModelData(editor, model, index);
    }
}

QSize FPropertyItemDelegate::sizeHint(const QStyleOptionViewItem &option,
                             const QModelIndex &index) const
{
//    qDebug()<<"sizeHint"<<index.data();
//    if (index.column() > 1 && index.model()->index(index.row(), 1,index.parent()).data().toString() == "QPushButton2")
//        return QSize(200, 100);
//    else
        return QStyledItemDelegate::sizeHint(option, index);

}

