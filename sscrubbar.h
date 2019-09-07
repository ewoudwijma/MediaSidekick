#ifndef SSCRUBBAR_H
#define SSCRUBBAR_H

#include <QMap>
#include <QWidget>

typedef struct {
    int row;
    int in;
    int out;
} EditInOutStruct;

class SScrubBar : public QWidget
{
    Q_OBJECT

    enum controls {
        CONTROL_NONE,
        CONTROL_HEAD,
        CONTROL_IN,
        CONTROL_OUT
    };

public:
    explicit SScrubBar(QWidget *parent = nullptr);

    virtual void mousePressEvent(QMouseEvent * event);
    virtual void mouseReleaseEvent(QMouseEvent * event);
    virtual void mouseMoveEvent(QMouseEvent * event);
    void setScale(int maximum);
    void setFramerate(double fps);
    int position() const;
//    void setInPoint(int index, int in);
//    void setOutPoint(int index, int out);
    void setMarkers(const QList<int>&);
    QList<int> markers() const {
        return m_markers;
    }

//    int getInPoint(int index);
//    int getOutPoint(int index);

//    void addInOutPoint(int in, int out);
    void clearInOuts();
//    void removeInOutPoint(int in, int out);
    bool readOnly;
    void setInOutPoint(int row, int in, int out);
signals:
    void seeked(int);
    void inChanged(int row, int in);
    void outChanged(int row, int out);

public slots:
    bool onSeek(int value);

protected:
    virtual void paintEvent(QPaintEvent *e);
    virtual void resizeEvent(QResizeEvent *);
    virtual bool event(QEvent *event);

private:
    int m_cursorPosition;
    int m_head;
    double m_scale;
    double m_fps;
    int m_interval;
    int m_max;
//    int m_in;
//    int m_out;
//    QList<int> m_in_list;
//    QList<int> m_out_list;
//    QMap<int, EditInOutStruct> m_in_out_map;
    QList<EditInOutStruct> m_in_out_list;
    enum controls m_activeControl;
    QPixmap m_pixmap;
    int m_timecodeWidth;
    int m_secondsPerTick;
    QList<int> m_markers;

    void updatePixmap();
};

#endif // SSCRUBBAR_H
