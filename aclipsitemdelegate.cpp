#include "aclipsitemdelegate.h"

#include "aglobal.h"
#include "astareditor.h"
#include "astarrating.h"

#include <QCheckBox>
#include <QDebug>
#include <QEvent>
#include <atagslistview.h>
#include <QPainter>
#include <QStandardItemModel>
#include <QTime>

void AClipsItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                         const QModelIndex &index) const
{
    QColor backgroundColor = index.data(Qt::BackgroundRole).value<QColor>();

    QPalette optionPalette = option.palette;
    if(option.state & QStyle::State_Selected)
        optionPalette.setColor(QPalette::Base, optionPalette.color(QPalette::Highlight));
    else if (backgroundColor != QColor())
        optionPalette.setColor(QPalette::Base, backgroundColor);

    if (!index.isValid())
        return;

    QWidget *widget = nullptr;

    if (index.column() == fileDurationIndex || index.column() == inIndex || index.column() == outIndex || index.column() == durationIndex)
    {
        STimeSpinBox *spinBox = new STimeSpinBox();
        QTime time = QTime::fromString(index.data().toString(),"HH:mm:ss.zzz");
        spinBox->setValue(AGlobal().msec_to_frames(time.msecsSinceStartOfDay()));

        widget = spinBox;

    }
    else if (index.column() == alikeIndex)
    {
        QCheckBox *checkBox = new QCheckBox();
        checkBox->setChecked(index.data().toBool());
//        checkBox->setText("xx");
        QPalette checkBoxPalette = checkBox->palette( );

        widget = checkBox;

    }
    else if (index.column() == tagIndex)
    {
        ATagsListView* listView = new ATagsListView();

        QVariant variant = index.data(Qt::EditRole);

        listView->stringToModel(variant.toString());

        widget = listView;

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
    else
    {
        QStyledItemDelegate::paint(painter, option, index);
    }

    if (widget != nullptr)
    {
        QColor backgroundColor = index.data(Qt::BackgroundRole).value<QColor>();
        QPalette optionPalette = option.palette;
        if(option.state & QStyle::State_Selected)
            optionPalette.setColor(QPalette::Base, optionPalette.color(QPalette::Highlight));
        else if (backgroundColor != QColor())
            optionPalette.setColor(QPalette::Base, backgroundColor);

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

QWidget *AClipsItemDelegate::createEditor(QWidget *parent,
                                    const QStyleOptionViewItem &option,
                                    const QModelIndex &index) const
{
    if (index.column() < inIndex) //readonly
    {
        return nullptr;
    }
    if (index.column() == fileDurationIndex || index.column() == inIndex || index.column() == outIndex || index.column() == durationIndex)
    {
        STimeSpinBox *editor = new STimeSpinBox(parent);

        connect(editor, SIGNAL(valueChanged(int)), this, SLOT(onSpinnerPositionChanged(int))); //using other syntax not working...

        return editor;
    }
    else if (index.column() == alikeIndex)
    {
        QCheckBox *checkBox = new QCheckBox(parent);
        return checkBox;
    }
    else if (index.column() == tagIndex)
    {
        ATagsListView* listView = new ATagsListView(parent);
        return listView;
    }
    else if (index.data().canConvert<AStarRating>())
    {
//        qDebug()<<"createEditor"<<index.row()<<index.column()<<index.data();
        AStarEditor *editor = new AStarEditor(parent);
        connect(editor, &AStarEditor::editingFinished,
                this, &AClipsItemDelegate::commitAndCloseEditor);
        return editor;
    }
    else
        return QStyledItemDelegate::createEditor(parent, option, index);
}

void AClipsItemDelegate::setEditorData(QWidget *editor,
                                 const QModelIndex &index) const
{
    if (index.column() == fileDurationIndex || index.column() == inIndex || index.column() == outIndex || index.column() == durationIndex)
    {
        qDebug()<<"AClipsItemDelegate::setEditorData"<<index.row()<<index.column()<<index.data();
        STimeSpinBox* spinBox = qobject_cast<STimeSpinBox*>(editor);
        QTime time = QTime::fromString(index.data().toString(),"HH:mm:ss.zzz");
        spinBox->setValue(AGlobal().msec_to_frames(time.msecsSinceStartOfDay()));
    }
    else if (index.column() == alikeIndex)
    {
        QCheckBox *checkBox = qobject_cast<QCheckBox*>(editor);
        checkBox->setChecked(index.data().toBool());
    }
    else if (index.column() == tagIndex)
    {
        ATagsListView* listView = qobject_cast<ATagsListView*>(editor);
        Q_ASSERT(listView);

        QVariant variant = index.data(Qt::EditRole);
        listView->stringToModel(variant.toString());
    }
    else if (index.data().canConvert<AStarRating>())
    {
        qDebug()<<"setEditorData"<<index.row()<<index.column()<<index.data();
        AStarRating starRating = qvariant_cast<AStarRating>(index.data());
        AStarEditor *starEditor = qobject_cast<AStarEditor *>(editor);
        starEditor->setStarRating(starRating);
    }
    else
        QStyledItemDelegate::setEditorData(editor, index);
}

void AClipsItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                const QModelIndex &index) const
{
    qDebug()<<"setModelData"<<editor<<model<<index.data();
    model->setData(model->index(index.row(), changedIndex), "yes");
    if (index.column() == fileDurationIndex || index.column() == inIndex || index.column() == outIndex || index.column() == durationIndex)
    {
        STimeSpinBox* spinBox = qobject_cast<STimeSpinBox*>(editor);
        qDebug()<<"AClipsItemDelegate::setModelData"<<index.row()<<index.column()<<index.data().toString()<<spinBox->value();
        QTime time = QTime::fromMSecsSinceStartOfDay(AGlobal().frames_to_msec(spinBox->value()));
        model->setData(index, time.toString("HH:mm:ss.zzz"));
    }
    else if (index.column() == alikeIndex)
    {
        QCheckBox *checkBox = qobject_cast<QCheckBox*>(editor);
        model->setData(index, checkBox->isChecked());
    }
    else if (index.column() == tagIndex)
    {
        ATagsListView* listView = qobject_cast<ATagsListView*>(editor);

        model->setData(index, listView->modelToString());
    }
    else if (index.data().canConvert<AStarRating>())
    {
        AStarEditor *starEditor = qobject_cast<AStarEditor *>(editor);
//        qDebug()<<"setModelData"<<index.row()<<index.column()<<index.data()<<starEditor->starRating().starCount();
        model->setData(index, QVariant::fromValue(starEditor->starRating()));
    }
    else
    {
        QStyledItemDelegate::setModelData(editor, model, index);
    }
}

QSize AClipsItemDelegate::sizeHint(const QStyleOptionViewItem &option,
                             const QModelIndex &index) const
{
    qDebug()<<"sizeHint"<<index.row()<<index.column()<<index.data();
    if (index.column() == inIndex)
    {
        return QSize(200,200);
    }
    else if (index.column() == tagIndex)
    {
        return QSize(200,200);
    }
    else if (index.data().canConvert<AStarRating>())
    {
        AStarRating starRating = qvariant_cast<AStarRating>(index.data());
        return starRating.sizeHint();
    }
    else
        return QStyledItemDelegate::sizeHint(option, index);
}

bool AClipsItemDelegate::editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index)
{
    if (index.column() == alikeIndex && event->type() == QEvent::MouseButtonRelease && false)
    {
        bool value = index.data( ).toBool();
        qDebug()<<"editorEvent"<<index.row()<<index.column()<<index.data()<<event->type()<<value;
        model->setData(index, !value);
        return true;
    }
    else if (event->type() == QEvent::KeyPress || event->type() == QEvent::MouseButtonDblClick)
    {
//        emit onStartEditing(index);
//        QStandardItemModel *modelS = qobject_cast<QStandardItemModel*>(model);
//        QStandardItem *item = modelS->itemFromIndex(index);
//        qDebug()<<"Item"<<item->data()<<index.data();

//        QStandardItemModel *listModel = qobject_cast<QStandardItemModel*>(index.editor);
//        QList<QStandardItem *> items;
//        QStandardItem *item = new QStandardItem("New");
//        item->setBackground(QBrush(Qt::blue));
//        items.append(item);
//        listModel->appendRow(items);

    }

    return QStyledItemDelegate::editorEvent(event, model, option, index);
}

//! [5]
void AClipsItemDelegate::commitAndCloseEditor()
{
    AStarEditor *editor = qobject_cast<AStarEditor *>(sender());
    emit commitData(editor);
    emit closeEditor(editor);
}
//! [5]

void AClipsItemDelegate::onSpinnerPositionChanged(int )//frames
{
    STimeSpinBox *editor = qobject_cast<STimeSpinBox *>(sender());
//    qDebug()<<"AClipsItemDelegate::onSpinnerPositionChanged"<<frames<<editor->value();
    emit commitData(editor);
//    emit spinnerChanged(editor);
}
