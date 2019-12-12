#include "aclipsitemdelegate.h"

#include "aglobal.h"
#include "astareditor.h"
#include "astarrating.h"

#include <QCheckBox>
#include <QDebug>
#include <QEvent>
#include <QListView>
#include <QPainter>
#include <QStandardItemModel>
#include <QTime>

void AClipsItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                         const QModelIndex &index) const
{
    QColor backgroundColor = index.data(Qt::BackgroundRole).value<QColor>();

//    if (index.column() == inIndex)
//        qDebug()<<"AClipsItemDelegate::paint"<<index.data().toString()<<option.palette.color(QPalette::Base)<<index.data(Qt::BackgroundRole).value<QColor>();

//    qDebug()<<"AClipsItemDelegate::paint"<<painter<<option.state<<index.data()<<option.palette.background()<<backgroundColor;
//    if (index.column() == ratingIndex)
//        qDebug()<<"backgroundColor"<<index.data().toString()<<backgroundColor<<QColor();
    QPalette optionPalette = option.palette;
    if(option.state & QStyle::State_Selected)
        optionPalette.setColor(QPalette::Base, optionPalette.color(QPalette::Highlight));
    else if (backgroundColor != QColor())
        optionPalette.setColor(QPalette::Base, backgroundColor);

    if (!index.isValid())
        return;

    if (index.column() == fileDurationIndex || index.column() == inIndex || index.column() == outIndex || index.column() == durationIndex)
    {
        STimeSpinBox *spinBox = new STimeSpinBox();
        QTime time = QTime::fromString(index.data().toString(),"HH:mm:ss.zzz");
        spinBox->setValue(AGlobal().msec_to_frames(time.msecsSinceStartOfDay()));

        spinBox->setGeometry(option.rect);

        if(option.state & QStyle::State_Selected)
        {
            painter->fillRect(option.rect, option.palette.highlight());
        }
        spinBox->setAutoFillBackground(true);
        spinBox->setPalette(optionPalette);

        QPixmap map = spinBox->grab();
        painter->drawPixmap(option.rect.x(), option.rect.y(), map);
//        QStyledItemDelegate::paint(painter, option, index);
    }
    else if (index.column() == alikeIndex)
    {
        QCheckBox *checkBox = new QCheckBox();
        checkBox->setChecked(index.data().toBool());
//        checkBox->setText("xx");
        QPalette checkBoxPalette = checkBox->palette( );

        checkBox->setGeometry(option.rect);
        if(option.state & QStyle::State_Selected)
        {
            painter->fillRect(option.rect, option.palette.highlight());
//            checkBoxPalette.setColor( QPalette::Active, QPalette::Background, option.palette.highlight().color() );
            checkBox->setStyleSheet("background-color: " + option.palette.highlight().color().name());
        }
        else if (backgroundColor != QColor())
        {
//            checkBoxPalette.setColor( QPalette::Active, QPalette::Background, backgroundColor );
            checkBox->setStyleSheet("background-color: " + backgroundColor.name());
        }
        else
            checkBox->setStyleSheet("background-color: " + option.palette.base().color().name());
        checkBox->setAutoFillBackground(true);
        checkBox->setPalette(checkBoxPalette);

        QPixmap map = checkBox->grab();
        painter->drawPixmap(option.rect.x(), option.rect.y(), map);
    }
    else if (index.column() == tagIndex)
    {
        QListView* listView = new QListView();
        listView->setFlow(QListView::LeftToRight);
        listView->setWrapping(true);
        listView->setDragDropMode(QListView::DragDrop);
        listView->setDefaultDropAction(Qt::MoveAction);
        listView->setSpacing(4);
        listView->setEditTriggers(QAbstractItemView::SelectedClicked | QAbstractItemView::AnyKeyPressed| QAbstractItemView::EditKeyPressed | QAbstractItemView::DoubleClicked);
        QStandardItemModel *listModel = new QStandardItemModel();
//        listModel->supportedDropActions();
        listView->setModel(listModel);

        QVariant variant = index.data(Qt::EditRole);
        QStringList tagList = variant.toString().split(";", QString::SkipEmptyParts);

//        if (tagList.count() != 1 || tagList.first() != "")
        for (int i=0; i < tagList.count();i++)
        {
//            qDebug()<<"paint"<<stringList[i];
            QList<QStandardItem *> items;
            QStandardItem *item = new QStandardItem(tagList[i].toLower());
            item->setBackground(QBrush(Qt::red));
            items.append(item);
            listModel->appendRow(items);
        }

        listView->setGeometry(option.rect);
        if(option.state & QStyle::State_Selected)
        {
            painter->fillRect(option.rect, option.palette.highlight());
        }
        listView->setAutoFillBackground(true);
        listView->setPalette(optionPalette);
        QPixmap map = listView->grab();
        painter->drawPixmap(option.rect.x(), option.rect.y(), map);
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
}

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
        QListView* listView = new QListView(parent);
        listView->setFlow(QListView::LeftToRight);
        listView->setWrapping(true);
        listView->setDragDropMode(QListView::DragDrop);
        listView->setDefaultDropAction(Qt::MoveAction);
        listView->setSpacing(4);
        listView->setEditTriggers(QAbstractItemView::SelectedClicked | QAbstractItemView::AnyKeyPressed| QAbstractItemView::EditKeyPressed | QAbstractItemView::DoubleClicked);
        QStandardItemModel *listModel = new QStandardItemModel(parent);
        listView->setModel(listModel);

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
        QListView* listView = qobject_cast<QListView*>(editor);
        Q_ASSERT(listView);
        QStandardItemModel *listModel = qobject_cast<QStandardItemModel*>(listView->model());

        QVariant variant = index.data(Qt::EditRole);
        QStringList tagList = variant.toString().split(";", QString::SkipEmptyParts);
//        if (tagList.count() != 1 || tagList.first() != "")
        for (int i=0; i < tagList.count();i++)
        {
//            qDebug()<<"setEditorData"<<stringList[i];
            QList<QStandardItem *> items;
            QStandardItem *item = new QStandardItem(tagList[i].toLower());
            item->setBackground(QBrush(Qt::red));
            items.append(item);
            listModel->appendRow(items);
        }

//        qDebug()<<"setEditorData"<<listModel->rowCount();
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
    model->setData(model->index(index.row(), changedIndex), "yes");
    if (index.column() == fileDurationIndex || index.column() == inIndex || index.column() == outIndex || index.column() == durationIndex)
    {
        STimeSpinBox* spinBox = qobject_cast<STimeSpinBox*>(editor);
//        qDebug()<<"AClipsItemDelegate::setModelData"<<index.row()<<index.column()<<index.data().toString()<<spinBox->value();
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
        QListView* listView = qobject_cast<QListView*>(editor);
        QStandardItemModel *listModel = qobject_cast<QStandardItemModel*>(listView->model());

        QString string = "";
        QString sep = "";
        for (int i=0; i < listModel->rowCount();i++)
        {
            string += sep + listModel->index(i,0).data().toString();
            sep = ";";
        }

        model->setData(index, string);
    }
    else if (index.data().canConvert<AStarRating>())
    {
        AStarEditor *starEditor = qobject_cast<AStarEditor *>(editor);
//        qDebug()<<"setModelData"<<index.row()<<index.column()<<index.data()<<starEditor->starRating().starCount();
        model->setData(index, QVariant::fromValue(starEditor->starRating()));
    }
    else
    {
//        qDebug()<<"setModelData"<<editor<<model<<index.data();
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
        emit onStartEditing(index);
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
