#include "fpropertyitemdelegate.h"

#include <QDateTimeEdit>
#include <QDebug>
//#include <QHBoxLayout>
//#include <QPainter>
//#include <QStandardItemModel>
//#include <QQuickWidget>
//#include <QLineEdit>


APropertyItemDelegate::APropertyItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void APropertyItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                         const QModelIndex &index) const
{
    if (index.model()->index(index.row(),0, index.parent()).data().toString() == "SuggestedName" and index.column()> 2) //suggestedName
    {
        QString fileName = index.model()->headerData(index.column(), Qt::Horizontal).toString();
//        qDebug()<<"APropertyItemDelegate::paint" <<index.data()<<index.row()<<index.column()<<fileName;

        QStyleOptionViewItem optCopy = option;

        initStyleOption(&optCopy, index);

//        optCopy.text = "hee";

        optCopy.palette.setColor(QPalette::WindowText, Qt::red);
//        optCopy.palette.setBrush(QPalette::Window, QBrush(Qt::blue));
//        optCopy.palette.text();

        if (!fileName.contains(index.data().toString() + "."))
            optCopy.font.setBold(true);
        else
            optCopy.font.setBold(false);

        QStyledItemDelegate::paint(painter, optCopy, index);
    }
    else
        QStyledItemDelegate::paint(painter, option, index);
}

QWidget *APropertyItemDelegate::createEditor(QWidget *parent,
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

void APropertyItemDelegate::setEditorData(QWidget *editor,
                                 const QModelIndex &index) const
{
    qDebug()<<"setEditorData" <<index.data()<<index.model()->index(index.row(), 1,index.parent()).data().toString();
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

void APropertyItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                const QModelIndex &index) const
{
    qDebug()<<"setModelData" <<index.data()<<index.model()->index(index.row(), 1,index.parent()).data().toString();
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

QSize APropertyItemDelegate::sizeHint(const QStyleOptionViewItem &option,
                             const QModelIndex &index) const
{
//    qDebug()<<"sizeHint"<<index.data();
//    if (index.column() > 1 && index.model()->index(index.row(), 1,index.parent()).data().toString() == "QPushButton2")
//        return QSize(200, 100);
//    else
        return QStyledItemDelegate::sizeHint(option, index);

}

