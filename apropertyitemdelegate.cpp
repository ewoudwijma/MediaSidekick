#include "apropertyitemdelegate.h"

#include <QDateTimeEdit>
#include <QDebug>
#include <QComboBox>
#include <QGeoCoordinate>
#include <QLineEdit>
#include <QStandardItem>
#include <QLabel>

#include "aglobal.h"
#include "astareditor.h"
#include "astarrating.h"
#include "atagslistview.h"

#include <QPainter>

APropertyItemDelegate::APropertyItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void APropertyItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                         const QModelIndex &index) const
{
    QColor backgroundColor = index.data(Qt::BackgroundRole).value<QColor>();
    QPalette optionPalette = option.palette;
    if(option.state & QStyle::State_Selected)
        optionPalette.setColor(QPalette::Base, optionPalette.color(QPalette::Highlight));
    else if (backgroundColor != QColor())
        optionPalette.setColor(QPalette::Base, backgroundColor);

    bool isEditable = index.flags() & Qt::ItemIsEditable;

    QString value = index.data().toString();

    QWidget *widget = nullptr;

    if (isEditable && index.model()->index(index.row(), propertyIndex, index.parent()).data().toString() == "SuggestedName" && index.column() >= firstFileColumnIndex) //suggestedName
    {
        QString folderFileName = index.model()->headerData(index.column(), Qt::Horizontal).toString();
//        qDebug()<<"APropertyItemDelegate::paint" <<index.data()<<index.row()<<index.column()<<folderFileName;

        QStyleOptionViewItem optCopy = option;

        initStyleOption(&optCopy, index);

        optCopy.palette.setColor(QPalette::WindowText, Qt::red);
//        optCopy.palette.setBrush(QPalette::Window, QBrush(Qt::blue));
//        optCopy.palette.text();

        if (!folderFileName.contains(value + "."))
            optCopy.font.setBold(true);
        else
            optCopy.font.setBold(false);

        QStyledItemDelegate::paint(painter, optCopy, index);
    }
    else if (isEditable && index.model()->index(index.row(), typeIndex, index.parent()).data().toString() == "QDateTime" && index.column() >= minimumIndex && index.column() != typeIndex)
    {
        if (index.column() == deltaIndex)
        {
            QLineEdit *editor = new QLineEdit();

            editor->setText(AGlobal().csvToString(value));

            widget = editor;
        }
        else
        {
            if (index.data().toString() != "")
            {
                QDateTimeEdit *editor = new QDateTimeEdit();
                editor->setCalendarPopup(true);
                editor->setDisplayFormat("yyyy:MM:dd HH:mm:ss");
                editor->setDateTime(index.data(Qt::EditRole).toDateTime());

                widget = editor;
            }
        }
    }
    else if (index.model()->index(index.row(), typeIndex, index.parent()).data().toString() == "QGeoCoordinate" && index.column() >= minimumIndex && index.column() != typeIndex)
    {
        if (index.column() == deltaIndex) //always editable
        {
            QLineEdit *editor = new QLineEdit();

            QString text = "";
            QStringList valueList = value.split(";");

            if (valueList.count() > 0 && valueList[0].toDouble() != 0)
                text += "⇔" + valueList[0] + "m";
            if (valueList.count() > 1 && valueList[1].toDouble() != 0)
                text += " " + valueList[1] + "°";
            if (valueList.count() > 2 && valueList[2].toDouble() != 0)
                text += " Δ" + valueList[2] + "m";

            editor->setText(text);

            widget = editor;
        }
        else
        {
            QString text = "";
            QStringList valueList = value.split(";");

            if (valueList.count() > 0 && valueList[0].toDouble() != 0)
                text += "↕" + QString::number(valueList[0].toDouble(), 'f', 3) + "°";
            if (valueList.count() > 1 && valueList[1].toDouble() != 0)
                text += " ↔" + QString::number(valueList[1].toDouble(), 'f', 3) + "°";
            if (valueList.count() > 2 && valueList[2].toInt() != 0)
                text += " Δ" + valueList[2] + "m";

            if (!isEditable)
            {
                QLabel *editor = new QLabel();
                editor->setText(text);
                widget = editor;
            }
            else
            {
                QLineEdit *editor = new QLineEdit();

                editor->setText(text);

                widget = editor;

            }
        }
    }
    else if (isEditable && index.model()->index(index.row(), typeIndex, index.parent()).data().toString() == "QComboBox" && (index.column() == minimumIndex))
    {
        QComboBox *editor = new QComboBox();

        QStringList values = value.split(";");

        for (int i=0; i < values.count();i++)
        {
            if (values[i].contains("***"))
            {
                editor->addItem(values[i].replace("***",""));
                editor->setCurrentIndex(i);
            }
            else
                editor->addItem(values[i]);
        }

        widget = editor;
    }
    else if (index.data().canConvert<AStarRating>())
    {
        AStarRating starRating = qvariant_cast<AStarRating>(index.data());

        if (option.state & QStyle::State_Selected)
            painter->fillRect(option.rect, option.palette.highlight());
        else if (backgroundColor != QColor())
            painter->fillRect(option.rect, QBrush(backgroundColor));

        starRating.paint(painter, option.rect, optionPalette,
                         AStarRating::EditMode::ReadOnly);
    }
    else if (index.model()->index(index.row(), typeIndex, index.parent()).data().toString() == "ATags" && index.column() >= minimumIndex && index.column() != typeIndex)
    {
        ATagsListView* listView = new ATagsListView();

        QVariant variant = index.data(Qt::EditRole);

        listView->stringToModel(variant.toString());

        widget = listView;

    }
    else
        QStyledItemDelegate::paint(painter, option, index);

    if (widget != nullptr)
    {
        widget->setGeometry(option.rect);

        if (option.state & QStyle::State_Selected)
        {
            painter->fillRect(option.rect, option.palette.highlight());
            widget->setStyleSheet("background-color: " + option.palette.highlight().color().name());
        }
        else if (backgroundColor != QColor())
        {
            widget->setStyleSheet("background-color: " + backgroundColor.name());
        }
        else
            widget->setStyleSheet("background-color: " + option.palette.base().color().name());

        widget->setAutoFillBackground(true);
        widget->setPalette(optionPalette);

        QPixmap map = widget->grab();
        painter->drawPixmap(option.rect.x(), option.rect.y(), map);
    }
} //paint

QWidget *APropertyItemDelegate::createEditor(QWidget *parent,
                                    const QStyleOptionViewItem &option,
                                    const QModelIndex &index) const

{
//    qDebug()<<"createEditor" << index.data()<<index.model()->index(index.row(), typeIndex, index.parent()).data().toString()<< index.row() << index.column() << index.model();
    if (index.model()->index(index.row(), typeIndex, index.parent()).data().toString() == "QDateTime" && index.column() >= minimumIndex && index.column() != deltaIndex && index.column() != typeIndex)
    {
        QDateTimeEdit* editor = new QDateTimeEdit(parent);
        editor->setCalendarPopup(true);
        editor->setDisplayFormat("yyyy:MM:dd HH:mm:ss");

        return editor;
    }
    else if (index.model()->index(index.row(), typeIndex, index.parent()).data().toString() == "QComboBox" && (index.column() == minimumIndex))
    {
        QComboBox *editor = new QComboBox(parent);
        editor->setEditable(true);

        return editor;
    }
    else if (index.data().canConvert<AStarRating>())
    {
        AStarEditor *editor = new AStarEditor(parent);
        connect(editor, &AStarEditor::editingFinished,
                this, &APropertyItemDelegate::commitAndCloseEditor);
        return editor;
    }
    else if (index.model()->index(index.row(), typeIndex, index.parent()).data().toString() == "ATagsxxx" && index.column() >= minimumIndex && index.column() != typeIndex)
    {
        ATagsListView* listView = new ATagsListView(parent);
        return listView;
    }
    else
    {
        return QStyledItemDelegate::createEditor(parent, option, index);
    }

}

void APropertyItemDelegate::setEditorData(QWidget *editor,
                                 const QModelIndex &index) const
{
//    qDebug()<<"setEditorData" <<index.row()<<index.column()<< index.data().toString()<<index.model()->index(index.row(), typeIndex, index.parent()).data().toString();
    if (index.model()->index(index.row(), typeIndex, index.parent()).data().toString() == "QDateTime" && index.column() >= minimumIndex && index.column() != deltaIndex && index.column() != typeIndex)
    {
        QDateTimeEdit* dtEditor = qobject_cast<QDateTimeEdit*>(editor);
        Q_ASSERT(dtEditor);
        if (index.data(Qt::EditRole).isNull())
            dtEditor->setDateTime(QDateTime::currentDateTime());
//            dtEditor->clear();
        else
            dtEditor->setDateTime(index.data(Qt::EditRole).toDateTime());
    }
    else if (index.model()->index(index.row(), typeIndex, index.parent()).data().toString() == "QComboBox" && (index.column() == minimumIndex))
    {
        QComboBox* dtEditor = qobject_cast<QComboBox*>(editor);
        Q_ASSERT(dtEditor);
        if (index.data(Qt::EditRole).isNull())
            dtEditor->clear();
        else
        {
            QStringList values = index.data().toString().split(";");

            for (int i=0; i < values.count();i++)
                if (values[i].contains("***"))
                {
                    dtEditor->addItem(values[i].replace("***",""));
                    dtEditor->setCurrentIndex(i);
                }
                else
                    dtEditor->addItem(values[i]);

            dtEditor->setCurrentIndex(0);
        }
    }
    else if (index.data().canConvert<AStarRating>())
    {
        AStarRating starRating = qvariant_cast<AStarRating>(index.data());
        AStarEditor *starEditor = qobject_cast<AStarEditor *>(editor);
//        qDebug()<<"setEditorData"<<index.row()<<index.column()<<index.data()<<starRating.starCount();
        starEditor->setStarRating(starRating);
    }
    else if (index.model()->index(index.row(), typeIndex, index.parent()).data().toString() == "ATagsxxx" && index.column() >= minimumIndex && index.column() != typeIndex)
    {
        ATagsListView* listView = qobject_cast<ATagsListView*>(editor);
        Q_ASSERT(listView);

        QVariant variant = index.data(Qt::EditRole);
        listView->stringToModel(variant.toString());
    }
    else
    {
        QStyledItemDelegate::setEditorData(editor, index);
    }
}

void APropertyItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                const QModelIndex &index) const
{
//    qDebug()<<"setModelData" << index.row()<<index.column()<< index.data().toString() << index.model()->index(index.row(), typeIndex,index.parent()).data().toString();
    if (index.model()->index(index.row(), typeIndex, index.parent()).data().toString() == "QDateTime" && index.column() >= minimumIndex && index.column() != deltaIndex && index.column() != typeIndex)
    {
        QDateTimeEdit* dtEditor = qobject_cast<QDateTimeEdit*>(editor);
        if(dtEditor->text().isEmpty())
            model->setData(index, QVariant(), Qt::EditRole);
        else
            model->setData(index, dtEditor->dateTime().toString("yyyy:MM:dd HH:mm:ss"), Qt::EditRole);
    }
    else if (index.model()->index(index.row(), typeIndex, index.parent()).data().toString() == "QComboBox" && (index.column() == minimumIndex))
    {
        QComboBox* dtEditor = qobject_cast<QComboBox*>(editor);
        if(dtEditor->currentText().isEmpty())
            model->setData(index, QVariant(), Qt::EditRole);
        else
        {
            QStringList values;
            for (int i=0; i < dtEditor->count(); i++)
            {
                if (dtEditor->currentIndex() == i)
                    values = values << dtEditor->itemText(i) + "***";
                else
                    values = values << dtEditor->itemText(i);
            }
//            qDebug()<<"values.join"<<values.join(";")<<dtEditor->count();
            model->setData(index, values.join(";"), Qt::EditRole);//dtEditor->currentText()
        }
    }
    else if (index.data().canConvert<AStarRating>())
    {

        AStarEditor *starEditor = qobject_cast<AStarEditor *>(editor);
//        qDebug()<<"setModelData"<<index.row()<<index.column()<<index.data()<<starEditor->starRating().starCount();
        model->setData(index, QVariant::fromValue(starEditor->starRating()));
    }
    else if (index.model()->index(index.row(), typeIndex, index.parent()).data().toString() == "ATagsxxx" && index.column() >= minimumIndex && index.column() != typeIndex)
    {
        ATagsListView* listView = qobject_cast<ATagsListView*>(editor);

        model->setData(index, listView->modelToString());
    }
    else
        QStyledItemDelegate::setModelData(editor, model, index);
}

QSize APropertyItemDelegate::sizeHint(const QStyleOptionViewItem &option,
                             const QModelIndex &index) const
{
    if (index.model()->index(index.row(), typeIndex, index.parent()).data().toString() == "ATags" && index.column() >= minimumIndex && index.column() != typeIndex)
        return QSize(option.decorationSize.width(), option.decorationSize.height() * 1.3);
    else
        return QSize(option.decorationSize.width(), option.decorationSize.height() * 1.2);
//        return QStyledItemDelegate::sizeHint(option, index);

}

void APropertyItemDelegate::commitAndCloseEditor()
{
    AStarEditor *editor = qobject_cast<AStarEditor *>(sender());
    emit commitData(editor);
    emit closeEditor(editor);
}
