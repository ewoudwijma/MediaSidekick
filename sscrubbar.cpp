#include "sscrubbar.h"

#include <QtWidgets>

static const int margin = 14;        /// left and right margins
static const int selectionSize = 18; /// the height of the top bar
#ifndef CLAMP
#define CLAMP(x, min, max) (((x) < (min))? (min) : ((x) > (max))? (max) : (x))
#endif

SScrubBar::SScrubBar(QWidget *parent)
    : QWidget(parent)
    , m_head(-1)
    , m_scale(-1)
    , m_fps(25)
    , m_max(1)
//    , m_in(-1)
//    , m_out(-1)
    , m_activeControl(CONTROL_NONE)
    , m_timecodeWidth(0)
{
    readOnly = true;
    setMouseTracking(true);
    setMinimumHeight(fontMetrics().height() + 2 * selectionSize);
}

void SScrubBar::setScale(int maximum)
{
//    qDebug()<<"setScale"<<maximum;
    if (!m_timecodeWidth) {
        const int fontSize = font().pointSize() - (font().pointSize() > 10? 2 : (font().pointSize() > 8? 1 : 0));
        setFont(QFont(font().family(), fontSize * devicePixelRatio()));
        m_timecodeWidth = fontMetrics().width("00:00:00:00") / devicePixelRatio();
    }
    m_max = maximum;
    /// m_scale is the pixels per frame ratio
    m_scale = (double) (width() - 2 * margin) / (double) maximum;
    if (m_scale == 0) m_scale = -1;
    m_secondsPerTick = qRound(double(m_timecodeWidth * 1.8) / m_scale / m_fps);
    if (m_secondsPerTick > 3600)
        // force to a multiple of one hour
        m_secondsPerTick += 3600 - m_secondsPerTick % 3600;
    else if (m_secondsPerTick > 300)
        // force to a multiple of 5 minutes
        m_secondsPerTick += 300 - m_secondsPerTick % 300;
    else if (m_secondsPerTick > 60)
        // force to a multiple of one minute
        m_secondsPerTick += 60 - m_secondsPerTick % 60;
    else if (m_secondsPerTick > 5)
        // force to a multiple of 10 seconds
        m_secondsPerTick += 10 - m_secondsPerTick % 10;
    else if (m_secondsPerTick > 2)
        // force to a multiple of 5 seconds
        m_secondsPerTick += 5 - m_secondsPerTick % 5;
    /// m_interval is the number of pixels per major tick to be labeled with time
    m_interval = qRound(double(m_secondsPerTick) * m_fps * m_scale);
    m_head = -1;
    updatePixmap();
}

void SScrubBar::setFramerate(double fps)
{
    m_fps = fps;
}

int SScrubBar::position() const
{
    return m_head;
}

//void SScrubBar::setInPoint(int index, int in)
//{
////    qDebug()<<"setInPoint"<<index<<in;
//    int old;
//    if (index == -1)
//    {
//        old = -1;
//        m_in_list.append(qMax(in, -1));
//    }
//    else {
//        old = m_in_list[index];
//        m_in_list[index] = qMax(in, -1);
//    }
//    updatePixmap();
//    emit inChanged(old, in);
//}

//void SScrubBar::setOutPoint(int index, int out)
//{
////    qDebug()<<"setOutPoint"<<index<<out;
//    int old;
//    if (index == -1)
//    {
//        old = -1;
//        m_out_list.append(out);//qMin(out, m_max));
//    }
//    else {
//        old = m_out_list[index];
//        m_out_list[index] = out;//qMin(out, m_max);
//    }
//    updatePixmap();
//    emit outChanged(old, out);

//}

void SScrubBar::setInOutPoint(int row, int in, int out)
{
//    int size = m_in_out_map.size();
//    m_in_out_map[size].row = row;
//    m_in_out_map[size].in = in;
//    m_in_out_map[size].out = out;

    int foundRow = -1;
    for (int i=0; i< m_in_out_list.count();i++)
    {
        if (m_in_out_list[i].row == row)
            foundRow = i;
    }

    if (foundRow != -1)
    {
        m_in_out_list[foundRow].in = in;
        m_in_out_list[foundRow].out = out;
    }
    else
        m_in_out_list.append({row,in,out});

    updatePixmap();
    emit inChanged(row, in);
    emit outChanged(row, out);
}

//void SScrubBar::removeInOutPoint(int in, int out)
//{
//    for (int i=0;i<m_in_list.count();i++)
//    {
//        if (m_in_list[i] == in && m_out_list[i] == out)
//        {
//            m_in_list.removeAt(i);
//            m_out_list.removeAt(i);
//            updatePixmap();
//        }
//    }
//}

void SScrubBar::clearInOuts()
{
//    m_in_list.clear();
//    m_out_list.clear();
//    m_in_out_map.clear();
    m_in_out_list.clear();
//    qDebug()<<"clearInOuts"<<m_in_list.count()<<m_out_list.count();
    updatePixmap();
}

//int SScrubBar::getInPoint(int index)
//{
//    return m_in_list[index];
//}

//int SScrubBar::getOutPoint(int index)
//{
//    return m_out_list[index];
//}

void SScrubBar::setMarkers(const QList<int> &list)
{
    m_markers = list;
    updatePixmap();
}

void SScrubBar::mousePressEvent(QMouseEvent * event)
{
    if (readOnly)
        return;
//    qDebug()<<"mousePressEvent";
    int x = event->x() - margin;
    int head = m_head * m_scale;
    int pos = CLAMP(x / m_scale, 0, m_max);

    foreach (EditInOutStruct inOut, m_in_out_list)
    {

//    QMapIterator<int, EditInOutStruct> editIterator(m_in_out_map);
//    while (editIterator.hasNext()) //all files
//    {
//        editIterator.next();
//        EditInOutStruct inOut = editIterator.value();
        int in = inOut.in * m_scale;
        int out = inOut.out * m_scale;

        if (x >= in - 12 && x <= in + 6)
        {
            m_activeControl = CONTROL_IN;
            setInOutPoint(inOut.row, pos, inOut.out); //out the same
        }
        else if (x >= out - 6 && x <= out + 12)
        {
            m_activeControl = CONTROL_OUT;
            setInOutPoint(inOut.row, inOut.in, pos); //in the same
        }
    }

//    for (int i=0;i<m_out_list.count();i++)
//    {
//        int in = m_in_list[i] * m_scale;
//        int out = m_out_list[i] * m_scale;
//        if (x >= in - 12 && x <= in + 6) {
//            m_activeControl = CONTROL_IN;
//            setInPoint(i, pos);
//        }
//        else if (x >= out - 6 && x <= out + 12) {
//            m_activeControl = CONTROL_OUT;
//            setOutPoint(i, pos);
//        }

//    }
    if (m_head > -1) {
        if (m_activeControl == CONTROL_NONE) {
            m_activeControl = CONTROL_HEAD;
            m_head = pos;
            const int offset = height() / 2;
            const int x = head;
            const int w = qAbs(x - head);
            update(margin + x - offset, 0, w + 2 * offset, height());
        }
    }
    emit seeked(pos);
}

void SScrubBar::mouseReleaseEvent(QMouseEvent * event)
{
    if (readOnly)
        return;
//    qDebug()<<"mouseReleaseEvent";
    Q_UNUSED(event)
    m_activeControl = CONTROL_NONE;
}

void SScrubBar::mouseMoveEvent(QMouseEvent * event)
{
    if (readOnly)
        return;
//    qDebug()<<"mouseMoveEvent"<<m_in<<m_out;
    int x = event->x() - margin;
    int pos = CLAMP(x / m_scale, 0, m_max);

    if (event->buttons() & Qt::LeftButton)
    {
        foreach (EditInOutStruct inOut, m_in_out_list)
        {

//        QMapIterator<int, EditInOutStruct> editIterator(m_in_out_map);
//        while (editIterator.hasNext()) //all files
//        {
//            editIterator.next();
//            EditInOutStruct inOut = editIterator.value();
            int in = inOut.in * m_scale;
            int out = inOut.out * m_scale;

            if (m_activeControl == CONTROL_IN)
            {
                if (x >= in - 12 && x <= in + 6)
                {
                    setInOutPoint(inOut.row,pos, inOut.out); //out the same
                }
            }
            if (m_activeControl == CONTROL_OUT)
            {
                if (x >= in - 12 && x <= in + 6)
                {
                    setInOutPoint(inOut.row, inOut.in, pos); //in the same
                }
            }
        }

//        if (m_activeControl == CONTROL_IN)
//        {
//            for (int i=0;i<m_in_list.count();i++)
//            {
//                int in = m_in_list[i] * m_scale;

//                if (x >= in - 12 && x <= in + 6)
//                {
//                    setInPoint(i,pos);
//                }
//            }

//        }
//        else if (m_activeControl == CONTROL_OUT)
//        {
//            for (int i=0;i<m_out_list.count();i++)
//            {
//                int out = m_out_list[i] * m_scale;

//                if (x >= out - 12 && x <= out + 6)
//                {
//                    setOutPoint(i, pos);
//                }
//            }
//        }
//        else
        if (m_activeControl == CONTROL_HEAD) {
            const int head = m_head * m_scale;
            const int offset = height() / 2;
            const int x = head;
            const int w = qAbs(x - head);
            update(margin + x - offset, 0, w + 2 * offset, height());
            m_head = pos;
        }
        emit seeked(pos);
    }
}

bool SScrubBar::onSeek(int value)
{
    if (m_activeControl != CONTROL_HEAD)
        m_head = value;
    int oldPos = m_cursorPosition;
    m_cursorPosition = value * m_scale;
    const int offset = height() / 2;
    const int x = qMin(oldPos, m_cursorPosition);
    const int w = qAbs(oldPos - m_cursorPosition);
    update(margin + x - offset, 0, w + 2 * offset, height());
    return true;
}

void SScrubBar::paintEvent(QPaintEvent *e)
{
    QPen pen(QBrush(palette().text().color()), 2);
    QPainter p(this);
    QRect r = e->rect();
    p.setClipRect(r);
    p.drawPixmap(0, 0, width(), height(), m_pixmap);

    if (!isEnabled()) return;

    // draw pointer
    QPolygon pa(3);
    const int x = selectionSize / 2 - 1;
    int head = margin + m_cursorPosition;
    pa.setPoints(3, head - x - 1, 0, head + x, 0, head, x);
    p.setBrush(palette().text().color());
    p.setPen(Qt::NoPen);
    p.drawPolygon(pa);
    p.setPen(pen);
    if (m_head >= 0) {
        head = margin + m_head * m_scale;
        p.drawLine(head, 0, head, height() - 1);
    }

    int altY = 0;
    if (!readOnly)
    {
        foreach (EditInOutStruct inOut, m_in_out_list)
        {

//        QMapIterator<int, EditInOutStruct> editIterator(m_in_out_map);
//        while (editIterator.hasNext()) //all files
//        {
//            editIterator.next();
//            EditInOutStruct inOut = editIterator.value();
            int in = margin + inOut.in * m_scale;
            int out = margin + inOut.out * m_scale;

            // draw in point
//            const int in = margin + m_in_list[i] * m_scale;
            pa.setPoints(3, in - selectionSize / 2, altY, in - selectionSize / 2, selectionSize - 1 + altY, in - 1, selectionSize / 2 + altY);
            p.setBrush(palette().text().color());
            p.setPen(Qt::NoPen);
            p.drawPolygon(pa);
            p.setPen(pen);
            p.drawLine(in, altY, in, selectionSize - 1 + altY);

            // draw out point
//            const int out = margin + m_out_list[i] * m_scale;
            pa.setPoints(3, out + selectionSize / 2, altY, out + selectionSize / 2, selectionSize - 1 + altY, out, selectionSize / 2 + altY);
            p.setBrush(palette().text().color());
            p.setPen(Qt::NoPen);
            p.drawPolygon(pa);
            p.setPen(pen);
            p.drawLine(out, altY, out, selectionSize - 1 + altY);

            if (altY == 0)
                altY = selectionSize;
            else
                altY = 0;
        }


    }
//    for (int i=0;i<m_in_list.count();i++)
//    {
//        // draw in point
//        const int in = margin + m_in_list[i] * m_scale;
//        pa.setPoints(3, in - selectionSize / 2, altY, in - selectionSize / 2, selectionSize - 1 + altY, in - 1, selectionSize / 2 + altY);
//        p.setBrush(palette().text().color());
//        p.setPen(Qt::NoPen);
//        p.drawPolygon(pa);
//        p.setPen(pen);
//        p.drawLine(in, altY, in, selectionSize - 1 + altY);

//        // draw out point
//        const int out = margin + m_out_list[i] * m_scale;
//        pa.setPoints(3, out + selectionSize / 2, altY, out + selectionSize / 2, selectionSize - 1 + altY, out, selectionSize / 2 + altY);
//        p.setBrush(palette().text().color());
//        p.setPen(Qt::NoPen);
//        p.drawPolygon(pa);
//        p.setPen(pen);
//        p.drawLine(out, altY, out, selectionSize - 1 + altY);

//        if (altY == 0)
//            altY = selectionSize;
//        else
//            altY = 0;
//    }
}

void SScrubBar::resizeEvent(QResizeEvent *)
{
    setScale(m_max);
}

bool SScrubBar::event(QEvent *event)
{
    QWidget::event(event);
    if (event->type() == QEvent::PaletteChange || event->type() == QEvent::StyleChange)
        updatePixmap();
    return false;
}

void SScrubBar::updatePixmap()
{
    const int ratio = devicePixelRatio();
    const int l_width = width() * ratio;
    const int l_height = height() * ratio;
    const int l_margin = margin * ratio;
    const int l_selectionSize = selectionSize * ratio;
    const int l_interval = m_interval * ratio;
    const int l_timecodeWidth = m_timecodeWidth * ratio;
    m_pixmap = QPixmap(l_width, l_height);
    m_pixmap.fill(palette().window().color());
    QPainter p(&m_pixmap);
    p.setFont(font());
    const int markerHeight = fontMetrics().ascent() + 2 * ratio;
    QPen pen;

    if (!isEnabled()) {
        p.fillRect(0, 0, l_width, l_height, palette().background().color());
        p.end();
        update();
        return;
    }

    // background color
    p.fillRect(l_margin, 0, l_width - 2 * l_margin, l_height, palette().base().color());

    pen.setColor(palette().text().color());
    pen.setWidth(ratio);
    p.setPen(pen);

    int altY = 0;
    // selected region
    foreach (EditInOutStruct inOut, m_in_out_list)
    {

//    QMapIterator<int, EditInOutStruct> editIterator(m_in_out_map);
//    while (editIterator.hasNext()) //all files
//    {
//        editIterator.next();
//        EditInOutStruct inOut = editIterator.value();
        int in = inOut.in * m_scale * ratio;
        int out = inOut.out * m_scale * ratio;

        //        qDebug()<<"updatepixmap"<<m_in_list[i]<<m_out_list[i]<<m_scale<<ratio;
                p.fillRect(l_margin + in, altY, out - in, l_selectionSize, Qt::red);
        //        p.fillRect(l_margin + in + (2 + ratio), ratio, // 2 for the in point line
        //                   out - in - 2 * (2 + ratio) - qFloor(0.5 * ratio),
        //                   l_selectionSize - ratio * 2,
        //                   palette().highlight().color());
                p.fillRect(l_margin + in + (ratio), altY + ratio, // 2 for the in point line
                           out - in - 2 * (ratio) - qFloor(0.5 * ratio),
                           l_selectionSize - ratio * 2,
                           palette().highlight().color());
                 int pixelsWide = fontMetrics().width(QString::number(inOut.row));
                p.drawText(l_margin + in + (out-in)/2 - pixelsWide / 2, altY + l_selectionSize - ratio * 3, QString::number(inOut.row));
                if (altY == 0)
                    altY = l_selectionSize;
                else
                    altY = 0;
    }

//    for (int i=0;i<m_out_list.count();i++)
//    {
//        const int in = m_in_list[i] * m_scale * ratio;
//        const int out = m_out_list[i] * m_scale * ratio;
////        qDebug()<<"updatepixmap"<<m_in_list[i]<<m_out_list[i]<<m_scale<<ratio;
//        p.fillRect(l_margin + in, altY, out - in, l_selectionSize, Qt::red);
////        p.fillRect(l_margin + in + (2 + ratio), ratio, // 2 for the in point line
////                   out - in - 2 * (2 + ratio) - qFloor(0.5 * ratio),
////                   l_selectionSize - ratio * 2,
////                   palette().highlight().color());
//        p.fillRect(l_margin + in + (ratio), altY + ratio, // 2 for the in point line
//                   out - in - 2 * (ratio) - qFloor(0.5 * ratio),
//                   l_selectionSize - ratio * 2,
//                   palette().highlight().color());
//         int pixelsWide = fontMetrics().width(QString::number(i+1));
//        p.drawText(l_margin + in + (out-in)/2 - pixelsWide / 2, altY + l_selectionSize - ratio * 3, QString::number(i+1));
//        if (altY == 0)
//            altY = l_selectionSize;
//        else
//            altY = 0;
//    }

    // draw time ticks
//    qDebug()<<"interval:"<<l_interval<<"ratio:"<<ratio<<"tcw"<<l_timecodeWidth;
    if (l_interval > 2) {
        for (int x = l_margin; x < l_width - l_margin; x += l_interval) {
//            qDebug()<<"Drawline"<<x;
            p.drawLine(x, l_selectionSize * 2, x, l_height - 1);
            if (x + l_interval / 4 < l_width - l_margin)
                p.drawLine(x + l_interval / 4,     l_height - 3 * ratio, x + l_interval / 4,     l_height - 1);
            if (x + l_interval / 2 < l_width - l_margin)
                p.drawLine(x + l_interval / 2,     l_height - 7 * ratio, x + l_interval / 2,     l_height - 1);
            if (x + l_interval * 3 / 4 < l_width - l_margin)
                p.drawLine(x + l_interval * 3 / 4, l_height - 3 * ratio, x + l_interval * 3 / 4, l_height - 1);
        }
    }

    // draw timecode
    if (l_interval > l_timecodeWidth )
    {
        int x = l_margin;
        for (int i = 0; x < l_width - l_margin - l_timecodeWidth; i++, x += l_interval) {
            int y = l_selectionSize*2 + fontMetrics().ascent() - 2 * ratio;
            int frames = qRound(i * m_fps * m_secondsPerTick);
            QString frames_to_time = "5";
            QTime time = QTime::fromMSecsSinceStartOfDay(frames);
            QString text = time.toString("hh:mm:ss");
            p.drawText(x + 2 * ratio, y, text);
//            qDebug()<<"Drawtext"<<i<<x;
        }
    }

    // draw markers
    if (m_in_out_list.count() == 0)
//        if (m_in_list.count()==0 && m_out_list.count() == 0)
    {
        int i = 1;
        foreach (int pos, m_markers) {
            int x = l_margin + pos * m_scale * ratio;
            QString s = QString::number(i++);
            int markerWidth = fontMetrics().width(s) * 1.5;
            p.fillRect(x, 0, 1, l_height, palette().highlight().color());
            p.fillRect(x - markerWidth/2, 0, markerWidth, markerHeight, palette().highlight().color());
            p.drawText(x - markerWidth/3, markerHeight - 2 * ratio, s);
        }
    }

    p.end();
    update();
}
