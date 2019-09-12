#ifndef FGLOBAL_H
#define FGLOBAL_H

#include <QString>

static const int orderBeforeLoadIndex = 0;
static const int orderAtLoadIndex = 1;
static const int orderAfterMovingIndex = 2;
static const int folderIndex = 3;
static const int fileIndex = 4;
static const int fpsIndex = 5;
static const int inIndex = 6;
static const int outIndex = 7;
static const int durationIndex = 8;
static const int ratingIndex = 9;
static const int repeatIndex = 10;
static const int tagIndex = 12;

static const int frameRate = 25;

class FGlobal
{
public:
    FGlobal();
    int time_to_frames(const QString time);
    QString frames_to_time(int frames);
    int msec_to_frames(int msec);
    int msec_rounded_to_fps(int msec);
    QString msec_to_time(int msec);
    int frames_to_msec(int frames);
    static const int hintIndex = 11;
};

#endif // FGLOBAL_H
