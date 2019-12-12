#ifndef SSCRUBBAR_H
#define SSCRUBBAR_H

#include <QMap>
#include <QWidget>

typedef struct
{
    QString AV;
    int row;
    int in;
    int out;
} ClipInOutStruct;

class SScrubBar : public QWidget
{
    Q_OBJECT

    enum controls {
        CONTROL_NONE,
        CONTROL_HEAD,
        CONTROL_IN,
        CONTROL_OUT,
        CONTROL_BOTH
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
    QString audioOrVideo;
    bool setInOutPoint(QString AV, int row, int in, int out);
    void progressToRow(QString AV, int position, int *prevRow, int *nextRow, int *relativePosition);
    void rowToPosition(QString AV, int row, int* relativePosition);
    ClipInOutStruct getInOutPoint(QString AV, int row);
    void setAudioOrVideo(QString AV);
signals:
    void seeked(int);
    void scrubberInChanged(QString AV, int row, int in);
    void scrubberOutChanged(QString AV, int row, int out);

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
//    QMap<int, ClipInOutStruct> m_in_out_map;
    QList<ClipInOutStruct> video_in_out_list;
    QList<ClipInOutStruct> audio_in_out_list;
    enum controls m_activeControl;
    QPixmap m_pixmap;
    int m_timecodeWidth;
    int m_secondsPerTick;
    QList<int> m_markers;

    void updatePixmap();

    int currentClip;
};

#endif // SSCRUBBAR_H
