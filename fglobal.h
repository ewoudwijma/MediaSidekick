#ifndef FGLOBAL_H
#define FGLOBAL_H

#include <QString>

class FGlobal
{
public:
    FGlobal();
    int time_to_frames(int fps, const QString time);
    QString frames_to_time(int fps, int frames);
    int msec_to_frames(int fps, int msec);
    int msec_rounded_to_fps(int fps, int msec);
    QString msec_to_time(int fps, int msec);
    int frames_to_msec(int fps, int frames);
};

#endif // FGLOBAL_H
