#include "fedititemdelegate.h"

#include "fglobal.h"
#include "fstareditor.h"
#include "fstarrating.h"
#include "stimespinbox.h"

#include <QCheckBox>
#include <QDebug>
#include <QEvent>
#include <QListView>
#include <QPainter>
#include <QStandardItemModel>
#include <QTime>

static const int orderBeforeLoadIndex = 0;
static const int orderAtLoadIndex = 1;
static const int orderAfterMovingIndex = 2;
static const int folderIndex = 3;
static const int fileIndex = 4;
static const int inIndex = 5;
static const int outIndex = 6;
static const int durationIndex = 7;
static const int ratingIndex = 8;
static const int repeatIndex = 9;
static const int hintIndex = 10;
static const int tagIndex = 11;

void FEditItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                         const QModelIndex &index) const
{
    QColor backgroundColor = index.data(Qt::BackgroundRole).value<QColor>();
    QPalette optionPalette = option.palette;
    if(option.state & QStyle::State_Selected)
        optionPalette.setColor(QPalette::Base, optionPalette.color(QPalette::Highlight));
    else
        optionPalette.setColor(QPalette::Base, backgroundColor);

    if (!index.isValid())
        return;
    if (index.column() == inIndex || index.column() == outIndex || index.column() == durationIndex)
    {
        STimeSpinBox *spinBox = new STimeSpinBox();
        QTime inTime = QTime::fromString(index.data().toString(),"HH:mm:ss.zzz");
        spinBox->setValue(FGlobal().msec_to_frames(25, inTime.msecsSinceStartOfDay()));

        spinBox->setGeometry(option.rect);

//        qDebug()<<"FEditItemDelegate::paint"<<painter<<option.state<<index.data()<<option.palette.background()<<backgroundColor;

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
    else if (index.column() == repeatIndex)
    {
        QCheckBox *checkBox = new QCheckBox();
        checkBox->setChecked(index.data().toBool());
//        checkBox->setText("xx");
        QPalette checkBoxPalette = checkBox->palette( );

        checkBox->setGeometry(option.rect);
        if(option.state & QStyle::State_Selected)
        {
            painter->fillRect(option.rect, option.palette.highlight());
            checkBoxPalette.setColor( QPalette::Active, QPalette::Background, option.palette.highlight().color() );
        }
        else {
            checkBoxPalette.setColor( QPalette::Active, QPalette::Background, backgroundColor );
        }
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
        QStringList stringList = variant.toString().split(";");
        if (stringList.count() != 1 || stringList.first() != "")
        for (int i=0; i < stringList.count();i++)
        {
//            qDebug()<<"paint"<<stringList[i];
            QList<QStandardItem *> items;
            QStandardItem *item = new QStandardItem(stringList[i].toLower());
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
    else if (index.data().canConvert<FStarRating>())
    {
            FStarRating starRating = qvariant_cast<FStarRating>(index.data());

            if (option.state & QStyle::State_Selected)
                painter->fillRect(option.rect, option.palette.highlight());
            else
                painter->fillRect(option.rect, QBrush(backgroundColor));

            starRating.paint(painter, option.rect, optionPalette,
                             FStarRating::EditMode::ReadOnly);
    }
    else
    {
        QStyledItemDelegate::paint(painter, option, index);
    }
}

QWidget *FEditItemDelegate::createEditor(QWidget *parent,
                                    const QStyleOptionViewItem &option,
                                    const QModelIndex &index) const
{
    if (index.column() == inIndex || index.column() == outIndex || index.column() == durationIndex)
    {
        STimeSpinBox *editor = new STimeSpinBox(parent);
        return editor;
    }
    else if (index.column() == repeatIndex)
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
    else if (index.data().canConvert<FStarRating>())
    {
//        qDebug()<<"createEditor"<<index.row()<<index.column()<<index.data();
        FStarEditor *editor = new FStarEditor(parent);
        connect(editor, &FStarEditor::editingFinished,
                this, &FEditItemDelegate::commitAndCloseEditor);
        return editor;
    }
    else
        return QStyledItemDelegate::createEditor(parent, option, index);
}

void FEditItemDelegate::setEditorData(QWidget *editor,
                                 const QModelIndex &index) const
{
    if (index.column() == inIndex || index.column() == outIndex || index.column() == durationIndex)
    {
        STimeSpinBox* spinBox = qobject_cast<STimeSpinBox*>(editor);
        QTime inTime = QTime::fromString(index.data().toString(),"HH:mm:ss.zzz");
        spinBox->setValue(FGlobal().msec_to_frames(25, inTime.msecsSinceStartOfDay()));
    }
    else if (index.column() == repeatIndex)
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
        QStringList stringList = variant.toString().split(";");
        for (int i=0; i < stringList.count();i++)
        {
//            qDebug()<<"setEditorData"<<stringList[i];
            QList<QStandardItem *> items;
            QStandardItem *item = new QStandardItem(stringList[i].toLower());
            item->setBackground(QBrush(Qt::red));
            items.append(item);
            listModel->appendRow(items);
        }

//        qDebug()<<"setEditorData"<<listModel->rowCount();
    }
    else if (index.data().canConvert<FStarRating>())
    {
        qDebug()<<"setEditorData"<<index.row()<<index.column()<<index.data();
        FStarRating starRating = qvariant_cast<FStarRating>(index.data());
        FStarEditor *starEditor = qobject_cast<FStarEditor *>(editor);
        starEditor->setStarRating(starRating);
    }
    else
        QStyledItemDelegate::setEditorData(editor, index);
}

void FEditItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                const QModelIndex &index) const
{
    if (index.column() == inIndex || index.column() == outIndex || index.column() == durationIndex)
    {
        STimeSpinBox* spinBox = qobject_cast<STimeSpinBox*>(editor);
        QTime time = QTime::fromMSecsSinceStartOfDay(FGlobal().frames_to_msec(25, spinBox->value()));
        model->setData(index, time.toString("HH:mm:ss.zzz"));
    }
    else if (index.column() == repeatIndex)
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
    else if (index.data().canConvert<FStarRating>())
    {
        FStarEditor *starEditor = qobject_cast<FStarEditor *>(editor);
        qDebug()<<"setModelData"<<index.row()<<index.column()<<index.data()<<starEditor->starRating().starCount();
        model->setData(index, QVariant::fromValue(starEditor->starRating()));
    }
    else
    {
        qDebug()<<"setModelData"<<editor<<model<<index.data();
        QStyledItemDelegate::setModelData(editor, model, index);
    }
}

QSize FEditItemDelegate::sizeHint(const QStyleOptionViewItem &option,
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
    else if (index.data().canConvert<FStarRating>())
    {
        FStarRating starRating = qvariant_cast<FStarRating>(index.data());
        return starRating.sizeHint();
    }
    else
        return QStyledItemDelegate::sizeHint(option, index);
}

bool FEditItemDelegate::editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index)
{
    if (index.column() == repeatIndex && event->type() == QEvent::MouseButtonRelease && false)
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
void FEditItemDelegate::commitAndCloseEditor()
{
    FStarEditor *editor = qobject_cast<FStarEditor *>(sender());
    emit commitData(editor);
    emit closeEditor(editor);
}
//! [5]
