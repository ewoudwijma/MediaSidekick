#include "ajobitemdelegate.h"
#include <QDebug>
#include <QTextBrowser>
#include <QPainter>
#include <QScrollBar>
#include <QProgressBar>

AJobItemDelegate::AJobItemDelegate(QObject *parent)
 : QStyledItemDelegate(parent)
{

}

void AJobItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                         const QModelIndex &index) const
{
    if (index.column() == 333)
    {
        QTextBrowser *textBrowser = new QTextBrowser();
//        textBrowser->setText(index.data().toString());
//        textBrowser->moveCursor (QTextCursor::End);
//        textBrowser->moveCursor (QTextCursor::End);
        textBrowser->insertPlainText (index.data().toString());
//        textBrowser->moveCursor (QTextCursor::End);


        textBrowser->setGeometry(option.rect);

        if(option.state & QStyle::State_Selected)
        {
            painter->fillRect(option.rect, option.palette.highlight());
        }

        QPixmap map = textBrowser->grab();
        painter->drawPixmap(option.rect.x(), option.rect.y(), map);
//        QScrollBar *sb = textBrowser->verticalScrollBar();
//        sb->setValue(sb->maximum());
    }
    else if (index.column() == 5) //progress
    {
        QProgressBar *progressBar = new QProgressBar();

        if (index.data().toInt() == -200) //parent job failed
        {
            progressBar->setValue(100);
            progressBar->setStyleSheet("QProgressBar::chunk {background: orange}");
        }
        else if (index.data().toInt() == -300) //job cancelled
        {
            progressBar->setValue(100);
            progressBar->setStyleSheet("QProgressBar::chunk {background: magenta}");
        }
        else if (index.data().toInt() < 0)
        {
            progressBar->setValue(100);
            progressBar->setStyleSheet("QProgressBar::chunk {background: red}");
        }
        else if (index.data().toInt() == 100)
        {
            progressBar->setValue(100);
            progressBar->setStyleSheet("QProgressBar::chunk {background: green}");
        }
        else
            progressBar->setValue(index.data().toInt());

        progressBar->setGeometry(option.rect); //fit into cell
        QPixmap map = progressBar->grab();
        painter->drawPixmap(option.rect.x(), option.rect.y(), map);
    }
    else
    {
        QStyledItemDelegate::paint(painter, option, index);
    }
}

QWidget *AJobItemDelegate::createEditor(QWidget *parent,
                                    const QStyleOptionViewItem &option,
                                    const QModelIndex &index) const

{
//    qDebug()<<"createEditor" <<index.data()<<index.model()->index(index.row(), 1,index.parent()).data().toString();

    if (index.column() == 3333)
    {
        QTextBrowser *textBrowser = new QTextBrowser(parent);
//        textBrowser->moveCursor (QTextCursor::End);
        return textBrowser;
    }
    else
    {
        return QStyledItemDelegate::createEditor(parent, option, index);
    }
}

void AJobItemDelegate::setEditorData(QWidget *editor,
                                 const QModelIndex &index) const
{
//    qDebug()<<"setEditorData" <<index.data()<<index.model()->index(index.row(), 1,index.parent()).data().toString();
    if (index.column() == 3333)
    {
        QTextBrowser* textBrowser = qobject_cast<QTextBrowser*>(editor);
        Q_ASSERT(textBrowser);
//        textBrowser->setText(index.data().toString());
//        textBrowser->moveCursor (QTextCursor::End);
//        textBrowser->moveCursor (QTextCursor::End);
        textBrowser->insertPlainText (index.data().toString());
//        textBrowser->moveCursor (QTextCursor::End);
//        QScrollBar *sb = textBrowser->verticalScrollBar();
//        sb->setValue(sb->maximum());

    }
    else
    {
        QStyledItemDelegate::setEditorData(editor, index);
    }
}

void AJobItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                const QModelIndex &index) const
{
//    qDebug()<<"setModelData" <<index.data()<<index.model()->index(index.row(), 1,index.parent()).data().toString();
    if (index.column() == 3333)
    {
        QTextBrowser* textBrowser = qobject_cast<QTextBrowser*>(editor);
//        textBrowser->moveCursor (QTextCursor::End);
        model->setData(index,textBrowser->toPlainText(), Qt::EditRole);
//        QScrollBar *sb = textBrowser->verticalScrollBar();
//        sb->setValue(sb->maximum());

    }
    else {
        QStyledItemDelegate::setModelData(editor, model, index);
    }
}

QSize AJobItemDelegate::sizeHint(const QStyleOptionViewItem &option,
                             const QModelIndex &index) const
{
//    qDebug()<<"sizeHint"<<index.data();
    if (index.column() == 333)
        return QSize(2000, 1000);
    else
        return QStyledItemDelegate::sizeHint(option, index);
}
