#ifndef Aderperviewvideo_H
#define Aderperviewvideo_H

extern "C"
{
    #include "libavformat/avformat.h"
    #include "libavcodec/avcodec.h"
    #include "libavutil/common.h"
    #include "libavutil/imgutils.h"
}

#include <string>
#include <iostream>

namespace DerperView
{
struct VideoInfo
{
    int width;
    int height;
    int bitRate;
    AVPixelFormat pixelFormat;
    int64_t totalFrames;
    AVRational frameRate;
    AVRational avg_frame_rate; //ewi 20200318
    AVRational videoTimeBase;
    AVRational streamTimeBase;
    AVRational audioTimeBase;

    int64_t audioBitRate;
    int audioSampleRate;
    uint64_t audioChannelLayout;

    int64_t duration; //ewi 20200326
    int64_t videoDuration; //ewi 20200326
    int64_t audioDuration; //ewi 20200326
};

class InputVideoFile
{
public:
    InputVideoFile(std::string filename);
    virtual ~InputVideoFile();

    void Dump();
    AVFrame *GetNextFrame(bool ignoreErrors = false);
    AVFrame *GetNextDrainFrame();
    int GetWidth() { return videoCodecContext_->width; }
    int GetHeight() { return videoCodecContext_->height; }
    AVRational GetVideoTimebase() { return videoCodecContext_->time_base; }
    AVRational GetAudioTimebase() { return audioCodecContext_->time_base; }

    VideoInfo GetVideoInfo();
    int GetLastError() { return lastError_; }

protected:
    std::string filename_;
    AVFormatContext *formatContext_;
    AVCodecContext *videoCodecContext_;
    AVCodecContext *audioCodecContext_;
    int videoStreamIndex_;
    int audioStreamIndex_;
    AVPacket packet_;
    AVFrame *frame_;
    bool draining_;
    int videoFrameCount_;
    int lastError_;
};

class OutputVideoFile
{
public:
    OutputVideoFile(std::string filename, VideoInfo sourceInfo);
    virtual ~OutputVideoFile();

    int WriteNextFrame(AVFrame *frame);
    void Flush();
    int GetLastError() { return lastError_; }

protected:
    std::string filename_;
    AVFormatContext *formatContext_;
    AVCodecContext *videoCodecContext_;
    AVCodecContext *audioCodecContext_;
    AVStream *videoStream_;
    AVStream *audioStream_;
    int videoFrameCount_;
    int audioFrameCount_;
    int lastError_;
};

std::string GetErrorString(int errorCode);
} //namespace DerperView

#endif
