#include "sscrubbar.h"

#include <QtWidgets>
#include "aglobal.h"

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

    currentClip = -1;
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

void SScrubBar::setInOutPoint(int row, int in, int out)
{
//    qDebug()<<"SScrubBar::setInOutPoint"<<row<<in<<out;

    int foundRow = -1;
    for (int i=0; i< m_in_out_list.count();i++)
    {
        if (m_in_out_list[i].row == row)
            foundRow = i;
    }

    if (foundRow != -1)
    {
        if (m_in_out_list[foundRow].in != in && in<m_in_out_list[foundRow].out)
        {
            m_in_out_list[foundRow].in = in;
            emit scrubberInChanged(row, in);
        }
        if (m_in_out_list[foundRow].out != out && out>m_in_out_list[foundRow].in )
        {
            m_in_out_list[foundRow].out = out;
            emit scrubberOutChanged(row, out);
        }
    }
    else
        m_in_out_list.append({row,in,out});

    updatePixmap();
}

ClipInOutStruct SScrubBar::getInOutPoint(int row)
{
    int foundRow = -1;
    for (int i=0; i< m_in_out_list.count();i++)
    {
        if (m_in_out_list[i].row == row)
            foundRow = i;
    }

    if (foundRow != -1)
    {
        return m_in_out_list[foundRow];
    }
    else
        return ClipInOutStruct();
}

void SScrubBar::clearInOuts()
{
    m_in_out_list.clear();
//    qDebug()<<"SScrubBar::clearInOuts"<<m_in_out_list.count();
    updatePixmap();
}

void SScrubBar::setMarkers(const QList<int> &list)
{
    m_markers = list;
    updatePixmap();
}

void SScrubBar::mousePressEvent(QMouseEvent * event)
{
    int x = event->x() - margin;
    int head = int(m_head * m_scale);
    int pos = int(CLAMP(x / m_scale, 0, m_max));

    currentClip = -1;
    if (!readOnly)
//    foreach (ClipInOutStruct inOut, m_in_out_list)
    for (int i=0; i< m_in_out_list.count();i++)
    {
        ClipInOutStruct inOut = m_in_out_list[i];

        int in = int(inOut.in * m_scale);
        int out = int(inOut.out * m_scale);

        if (x >= in - 12 && x <= in + 6)
        {
            m_activeControl = CONTROL_IN;
//            setInOutPoint(inOut.row, pos, inOut.out); //out the same
            currentClip = i;
        }
        else if (x >= out - 6 && x <= out + 12)
        {
            m_activeControl = CONTROL_OUT;
//            setInOutPoint(inOut.row, inOut.in, pos); //in the same
            currentClip = i;
        }
        else if (x > in + 6 && x < out -12)
        {
            m_activeControl = CONTROL_BOTH;
            currentClip = i;
//            qDebug()<<"mousePressEvent"<<event->x()<<currentClip<<in<<x<<out;
        }
    }

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
//    qDebug()<<"mouseReleaseEvent";
    Q_UNUSED(event)
    m_activeControl = CONTROL_NONE;
    currentClip = -1;
}

void SScrubBar::mouseMoveEvent(QMouseEvent * event)
{
    int x = event->x() - margin;
    int pos = int(CLAMP(x / m_scale, 0, m_max));

    if (event->buttons() & Qt::LeftButton)
    {
        if (!readOnly && currentClip != -1)
        {
            //no currentclip or odd currentclip and lower then currentclip or even currentclip and higher or lower then currentclip
            if (currentClip == -1 || (currentClip%2 == 0 && event->y() < height() * 0.33)  || (currentClip%2 == 1 && (event->y() > height() * 0.33 && event->y() < height() * 0.67)))
            {
                //            qDebug()<<"SScrubBar::mouseMoveEvent"<<m_activeControl<<currentClip<<x;
                            int i = currentClip;
                            ClipInOutStruct inOut = m_in_out_list[i];

                //            int in = inOut.in * m_scale;
                //            int out = inOut.out * m_scale;

                            if (m_activeControl == CONTROL_IN)
                            {
                                if ( i == 0 || (i > 0 && pos > m_in_out_list[i-1].out + 12)) //not overlapping previous
                //                if (x >= in - 24 && x <= in + 12)
                                {
                //                    qDebug()<<"SScrubBar::mouseMoveEvent"<<m_activeControl<<inOut.row<<pos<<inOut.out;
                                    if (pos < inOut.out)
                                        setInOutPoint(inOut.row, pos, inOut.out); //out the same
                                }
                            }
                            else if (m_activeControl == CONTROL_OUT)
                            {
                                if ( i == m_in_out_list.count()-1 || (i<m_in_out_list.count()-1 && pos < m_in_out_list[i+1].in - 12)) //not overlapping previous
                //                if (x >= out - 24 && x <= out + 100)
                                {
                //                    qDebug()<<"SScrubBar::mouseMoveEvent"<<m_activeControl<<inOut.row<<inOut.in<<pos;
                                    if (inOut.in < pos)
                                        setInOutPoint(inOut.row, inOut.in, pos); //in the same
                                }
                            }
                            else if (m_activeControl == CONTROL_BOTH)//move whole clip
                            {
                                int newIn = pos - (inOut.out - inOut.in)/2;
                                int newOut = pos + (inOut.out - inOut.in)/2;
                                if ( ((i == m_in_out_list.count()-1 && newOut <= m_max ) || (i<m_in_out_list.count()-1 && newOut < m_in_out_list[i+1].in - 12)) &&
                                     ((i== 0 && newIn >= 0)|| (i>0 && newIn > m_in_out_list[i-1].out + 12))                     ) //not overlapping previous
                //                if (x > in + 12 && x < out -24)
                                {
                //                    qDebug()<<"SScrubBar::mouseMoveEvent - moveinandout"<<m_activeControl<<inOut.row<<inOut.in<<inOut.out<<pos;
                                    setInOutPoint(inOut.row, newIn, newOut);
                                }
                            }
            }
        }

        if (m_activeControl == CONTROL_HEAD) {
            const int head = int(m_head * m_scale);
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
    m_cursorPosition = int(value * m_scale);
    const int offset = height() / 2;
    const int x = qMin(oldPos, m_cursorPosition);
    const int w = qAbs(oldPos - m_cursorPosition);
    update(margin + x - offset, 0, w + 2 * offset, height());
    return true;
}

void SScrubBar::progressToRow(int position, int *prevRow, int *nextRow, int* relativePosition)
{
//    qDebug()<<"SScrubBar::progressToRow"<<position<<*row<<*relativePosition;
    *prevRow = -1;
    *nextRow = 99999;
    *relativePosition = -1;
    foreach (ClipInOutStruct inOut, m_in_out_list)
    {
        if (inOut.in <= position)
            *prevRow = qMax(inOut.row, *prevRow);
        if (inOut.out >= position)
            *nextRow = qMin(inOut.row, *nextRow);

        if (inOut.in <= position && inOut.out >= position)
        {
            *relativePosition = position - inOut.in;
        }
    }
}

void SScrubBar::rowToPosition(int row, int* relativePosition)
{
    *relativePosition = -1;
    foreach (ClipInOutStruct inOut, m_in_out_list)
    {
        if (inOut.row == row)
        {
            *relativePosition = inOut.in;
        }
    }
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
        head = int(margin + m_head * m_scale);
        p.drawLine(head, 0, head, height() - 1);
    }

    int altY = 0;
    if (!readOnly)
    {
        foreach (ClipInOutStruct inOut, m_in_out_list)
        {

            int in = int(margin + inOut.in * m_scale);
            int out = int(margin + inOut.out * m_scale);

            // draw in point
//            const int in = margin + m_in_list[i] * m_scale;
            int selsiz = int(selectionSize * 0.8);
            int plusY = int(selectionSize * 0.1);
            pa.setPoints(3, in - selsiz / 2, altY + plusY, in - selsiz / 2, selsiz - 1 + altY + plusY, in - 1, selsiz / 2 + altY + plusY);
            p.setBrush(palette().text().color());
            p.setPen(Qt::NoPen);
            p.drawPolygon(pa);
//            p.setPen(pen);
//            p.drawLine(in, altY, in, selsiz - 1 + altY);

            // draw out point
//            const int out = margin + m_out_list[i] * m_scale;
            pa.setPoints(3, out + selsiz / 2, altY + plusY, out + selsiz / 2, selsiz - 1 + altY + plusY, out, selsiz / 2 + altY + plusY);
            p.setBrush(palette().text().color());
            p.setPen(Qt::NoPen);
            p.drawPolygon(pa);
//            p.setPen(pen);
//            p.drawLine(out, altY, out, selsiz - 1 + altY);

            if (altY == 0)
                altY = selectionSize;
            else
                altY = 0;
        }


    }
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
    foreach (ClipInOutStruct inOut, m_in_out_list)
    {

        int in = int(inOut.in * m_scale * ratio);
        int out = int(inOut.out * m_scale * ratio);

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
                 if (pixelsWide <= (out-in))
                    p.drawText(l_margin + in + (out-in)/2 - pixelsWide / 2, altY + l_selectionSize - ratio * 3, QString::number(inOut.row));
                if (altY == 0)
                    altY = l_selectionSize;
                else
                    altY = 0;
    }

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
//            qDebug()<<"Drawtext"<<i<<x<<time<<frames;
        }
    }

    // draw markers
    if (m_in_out_list.count() == 0)
//        if (m_in_list.count()==0 && m_out_list.count() == 0)
    {
        int i = 1;
        foreach (int pos, m_markers) {
            int x = int(l_margin + pos * m_scale * ratio);
            QString s = QString::number(i++);
            int markerWidth = int(fontMetrics().width(s) * 1.5);
            p.fillRect(x, 0, 1, l_height, palette().highlight().color());
            p.fillRect(x - markerWidth/2, 0, markerWidth, markerHeight, palette().highlight().color());
            p.drawText(x - markerWidth/3, markerHeight - 2 * ratio, s);
        }
    }

    p.end();
    update();
}
