#include "fglobal.h"
#include "stimespinbox.h"

#include <QDebug>

/*
 * Copyright (c) 2012-2019 Meltytech, LLC
 * Author: Dan Dennedy <dan@dennedy.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QRegExpValidator>
#include <QKeyEvent>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QTime>

STimeSpinBox::STimeSpinBox(QWidget *parent)
    : QSpinBox(parent)
{
    setLineEdit(new STimeSpinBoxLineEdit);
    setRange(0, INT_MAX);
    setFixedWidth(this->fontMetrics().boundingRect("_HHH:MM:SS;MMM_").width());
    setAlignment(Qt::AlignRight);
    m_validator = new QRegExpValidator(QRegExp("^\\s*(\\d*:){0,2}(\\d*[.;:])?\\d*\\s*$"), this);
    setValue(0);
#ifdef Q_OS_MAC
    QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    font.setPointSize(QGuiApplication::font().pointSize());
    setFont(font);
#endif
}

QValidator::State STimeSpinBox::validate(QString &input, int &pos) const
{
    return m_validator->validate(input, pos);
}


int STimeSpinBox::valueFromText(const QString &text) const
{
    return FGlobal().time_to_frames(25, text.toLatin1().constData());
//    if (MLT.producer() && MLT.producer()->is_valid()) {
//        return MLT.producer()->time_to_frames(text.toLatin1().constData());
//    } else {
//        return Mlt::Producer(MLT.profile(), "colour").time_to_frames(text.toLatin1().constData());
//    }
//    return 0;
}

QString STimeSpinBox::textFromValue(int val) const
{
    return FGlobal().frames_to_time(25, val);
//    if (MLT.producer() && MLT.producer()->is_valid()) {
//        return MLT.producer()->frames_to_time(val);
//    } else {
//        return Mlt::Producer(MLT.profile(), "colour").frames_to_time(val);
//    }
//    return QString();
}

void STimeSpinBox::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return)
        clearFocus();
    else
        QSpinBox::keyPressEvent(event);
}


STimeSpinBoxLineEdit::STimeSpinBoxLineEdit(QWidget *parent)
    : QLineEdit(parent)
    , m_selectOnMousePress(false)
{
}

void STimeSpinBoxLineEdit::focusInEvent(QFocusEvent *event)
{
    QLineEdit::focusInEvent(event);
    selectAll();
    m_selectOnMousePress = true;
}

void STimeSpinBoxLineEdit::focusOutEvent(QFocusEvent* event)
{
    // QLineEdit::focusOutEvent() calls deselect() on OtherFocusReason,
    // which prevents using the clipboard actions with the text.
    if (event->reason() != Qt::OtherFocusReason)
        QLineEdit::focusOutEvent(event);
}

void STimeSpinBoxLineEdit::mousePressEvent(QMouseEvent *event)
{
    QLineEdit::mousePressEvent(event);
    if (m_selectOnMousePress) {
        selectAll();
        m_selectOnMousePress = false;
    }
}

